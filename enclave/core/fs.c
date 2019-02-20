// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <openenclave/bits/fs.h>
#include <openenclave/internal/fs.h>
#include <openenclave/internal/print.h>

static oe_device_t* _get_fs_device(oe_devid_t devid)
{
    oe_device_t* ret = NULL;
    oe_device_t* device = oe_get_devid_device(devid);

    if (!device || device->type != OE_DEVICETYPE_FILESYSTEM)
        goto done;

    ret = device;

done:
    return ret;
}

static int _open(const char* pathname, int flags, mode_t mode)
{
    int ret = -1;
    int fd;
    oe_device_t* fs;
    oe_device_t* file;
    char filepath[OE_PATH_MAX] = {0};

    if (!(fs = oe_mount_resolve(pathname, filepath)))
        goto done;

    if (!(file = (*fs->ops.fs->open)(fs, filepath, flags, mode)))
        goto done;

    if ((fd = oe_assign_fd_device(file)) == -1)
    {
        (*fs->ops.fs->base.close)(file);
        goto done;
    }

    ret = fd;

done:
    return ret;
}

int oe_open(oe_devid_t devid, const char* pathname, int flags, mode_t mode)
{
    int ret = -1;

    if (devid == OE_DEVID_NULL)
    {
        ret = _open(pathname, flags, mode);
    }
    else
    {
        oe_device_t* dev;
        oe_device_t* file;

        if (!(dev = _get_fs_device(devid)))
        {
            oe_errno = OE_EINVAL;
            goto done;
        }

        if (!(file = (*dev->ops.fs->open)(dev, pathname, flags, mode)))
            goto done;

        if ((ret = oe_assign_fd_device(file)) == -1)
        {
            (*dev->ops.fs->base.close)(file);
            goto done;
        }
    }

done:
    return ret;
}

static OE_DIR* _opendir(const char* pathname)
{
    OE_DIR* ret = NULL;
    oe_device_t* fs;
    char filepath[OE_PATH_MAX] = {0};

    if (!(fs = oe_mount_resolve(pathname, filepath)))
        goto done;

    if (fs->type != OE_DEVICETYPE_FILESYSTEM)
    {
        oe_errno = OE_EINVAL;
        goto done;
    }

    if (fs->ops.fs->opendir == NULL)
    {
        oe_errno = OE_EINVAL;
        goto done;
    }

    ret = (OE_DIR*)(*fs->ops.fs->opendir)(fs, filepath);

done:
    return ret;
}

struct oe_dirent* oe_readdir(OE_DIR* dir)
{
    struct oe_dirent* ret = NULL;
    oe_device_t* dev = (oe_device_t*)dir;

    if (dev->type != OE_DEVICETYPE_DIRECTORY)
    {
        oe_errno = OE_EINVAL;
        goto done;
    }

    if (dev->ops.fs->readdir == NULL)
    {
        oe_errno = OE_EINVAL;
        goto done;
    }

    ret = (*dev->ops.fs->readdir)(dev);

done:
    return ret;
}

int oe_closedir(OE_DIR* dir)
{
    int ret = -1;
    oe_device_t* dev = (oe_device_t*)dir;

    if (dev->type != OE_DEVICETYPE_DIRECTORY)
    {
        oe_errno = OE_EINVAL;
        goto done;
    }

    if (dev->ops.fs->closedir == NULL)
    {
        oe_errno = OE_EINVAL;
        goto done;
    }

    ret = (*dev->ops.fs->closedir)(dev);

done:
    return ret;
}

static int _rmdir(const char* pathname)
{
    int ret = -1;
    oe_device_t* fs;
    char filepath[OE_PATH_MAX] = {0};

    if (!(fs = oe_mount_resolve(pathname, filepath)))
        goto done;

    if (fs->ops.fs->rmdir == NULL)
    {
        oe_errno = OE_EINVAL;
        goto done;
    }

    ret = (*fs->ops.fs->rmdir)(fs, filepath);

done:
    return ret;
}

static int _stat(const char* pathname, struct oe_stat* buf)
{
    int ret = -1;
    oe_device_t* fs;
    char filepath[OE_PATH_MAX] = {0};

    if (!(fs = oe_mount_resolve(pathname, filepath)))
        goto done;

    if (fs->ops.fs->stat == NULL)
    {
        oe_errno = OE_EINVAL;
        goto done;
    }

    ret = (*fs->ops.fs->stat)(fs, filepath, buf);

done:
    return ret;
}

static int _link(const char* oldpath, const char* newpath)
{
    int ret = -1;
    oe_device_t* fs;
    oe_device_t* newfs;
    char filepath[OE_PATH_MAX] = {0};
    char newfilepath[OE_PATH_MAX] = {0};

    if (!(fs = oe_mount_resolve(oldpath, filepath)))
        goto done;

    if (!(newfs = oe_mount_resolve(newpath, newfilepath)))
        goto done;

    if (fs != newfs)
    {
        oe_errno = OE_EXDEV;
        goto done;
    }

    if (fs->ops.fs->link == NULL)
    {
        oe_errno = OE_EPERM;
        goto done;
    }

    ret = (*fs->ops.fs->link)(fs, filepath, newfilepath);

done:
    return ret;
}

static int _unlink(const char* pathname)
{
    int ret = -1;
    oe_device_t* fs;
    char filepath[OE_PATH_MAX] = {0};

    if (!(fs = oe_mount_resolve(pathname, filepath)))
        goto done;

    if (fs->ops.fs->unlink == NULL)
    {
        oe_errno = OE_EINVAL;
        goto done;
    }

    ret = (*fs->ops.fs->unlink)(fs, filepath);

done:
    return ret;
}

static int _rename(const char* oldpath, const char* newpath)
{
    int ret = -1;
    oe_device_t* fs;
    oe_device_t* newfs;
    char filepath[OE_PATH_MAX];
    char newfilepath[OE_PATH_MAX];

    if (!(fs = oe_mount_resolve(oldpath, filepath)))
        goto done;

    if (!(newfs = oe_mount_resolve(newpath, newfilepath)))
        goto done;

    if (fs != newfs)
    {
        oe_errno = OE_EXDEV;
        goto done;
    }

    if (fs->ops.fs->rename == NULL)
    {
        oe_errno = OE_EPERM;
        goto done;
    }

    ret = (*fs->ops.fs->rename)(fs, filepath, newfilepath);

done:
    return ret;
}

static int _truncate(const char* pathname, off_t length)
{
    int ret = -1;
    oe_device_t* fs = NULL;
    char filepath[OE_PATH_MAX] = {0};

    if (!(fs = oe_mount_resolve(pathname, filepath)))
        goto done;

    if (fs->ops.fs->truncate == NULL)
    {
        oe_errno = OE_EINVAL;
        goto done;
    }

    ret = (*fs->ops.fs->truncate)(fs, filepath, length);

done:
    return ret;
}

static int _mkdir(const char* pathname, mode_t mode)
{
    int ret = -1;
    oe_device_t* fs;
    char filepath[OE_PATH_MAX] = {0};

    if (!(fs = oe_mount_resolve(pathname, filepath)))
        goto done;

    if (fs->ops.fs->mkdir == NULL)
    {
        oe_errno = OE_EINVAL;
        goto done;
    }

    ret = (*fs->ops.fs->mkdir)(fs, filepath, mode);

done:
    return ret;
}

off_t oe_lseek(int fd, off_t offset, int whence)
{
    off_t ret = -1;
    oe_device_t* file;

    if (!(file = oe_get_fd_device(fd)))
    {
        oe_errno = OE_EBADF;
        goto done;
    }

    if (file->ops.fs->lseek == NULL)
    {
        oe_errno = OE_EINVAL;
        goto done;
    }

    ret = (*file->ops.fs->lseek)(file, offset, whence);

done:
    return ret;
}

ssize_t oe_readv(int fd, const oe_iovec_t* iov, int iovcnt)
{
    ssize_t ret = -1;
    ssize_t nread = 0;

    if (fd < 0 || !iov)
    {
        oe_errno = OE_EINVAL;
        goto done;
    }

    for (int i = 0; i < iovcnt; i++)
    {
        const oe_iovec_t* p = &iov[i];
        ssize_t n;

        n = oe_read(fd, p->iov_base, p->iov_len);

        if (n < 0)
            goto done;

        nread += n;

        if ((size_t)n < p->iov_len)
            break;
    }

    ret = nread;

done:
    return ret;
}

ssize_t oe_writev(int fd, const oe_iovec_t* iov, int iovcnt)
{
    ssize_t ret = -1;
    ssize_t nwritten = 0;

    if (fd < 0 || !iov)
    {
        oe_errno = OE_EINVAL;
        goto done;
    }

    for (int i = 0; i < iovcnt; i++)
    {
        const oe_iovec_t* p = &iov[i];
        ssize_t n;

        n = oe_write(fd, p->iov_base, p->iov_len);

        if ((size_t)n != p->iov_len)
        {
            oe_errno = OE_EIO;
            goto done;
        }

        nwritten += n;
    }

    ret = nwritten;

done:
    return ret;
}

static int _access(const char* pathname, int mode)
{
    int ret = -1;
    oe_device_t* fs = NULL;
    char suffix[OE_PATH_MAX];

    if (!(fs = oe_mount_resolve(pathname, suffix)))
        goto done;

    if (fs->ops.fs->access == NULL)
    {
        oe_errno = OE_EINVAL;
        goto done;
    }

    ret = (*fs->ops.fs->access)(fs, suffix, mode);

done:
    return ret;
}

OE_DIR* oe_opendir(oe_devid_t devid, const char* pathname)
{
    OE_DIR* ret = NULL;

    if (devid == OE_DEVID_NULL)
    {
        ret = _opendir(pathname);
    }
    else
    {
        oe_device_t* dev;

        if (!(dev = _get_fs_device(devid)))
        {
            oe_errno = OE_EINVAL;
            goto done;
        }

        ret = (OE_DIR*)dev->ops.fs->opendir(dev, pathname);
    }

done:
    return ret;
}

int oe_unlink(oe_devid_t devid, const char* pathname)
{
    int ret = -1;

    if (devid == OE_DEVID_NULL)
    {
        ret = _unlink(pathname);
    }
    else
    {
        oe_device_t* dev;

        if (!(dev = _get_fs_device(devid)))
        {
            oe_errno = OE_EINVAL;
            goto done;
        }

        ret = dev->ops.fs->unlink(dev, pathname);
    }

done:
    return ret;
}

int oe_link(oe_devid_t devid, const char* oldpath, const char* newpath)
{
    int ret = -1;

    if (devid == OE_DEVID_NULL)
    {
        ret = _link(oldpath, newpath);
    }
    else
    {
        oe_device_t* dev;

        if (!(dev = _get_fs_device(devid)))
        {
            oe_errno = OE_EINVAL;
            goto done;
        }

        ret = dev->ops.fs->link(dev, oldpath, newpath);
    }

done:
    return ret;
}

int oe_rename(oe_devid_t devid, const char* oldpath, const char* newpath)
{
    int ret = -1;

    if (devid == OE_DEVID_NULL)
    {
        ret = _rename(oldpath, newpath);
    }
    else
    {
        oe_device_t* dev;

        if (!(dev = _get_fs_device(devid)))
        {
            oe_errno = OE_EINVAL;
            goto done;
        }

        ret = dev->ops.fs->rename(dev, oldpath, newpath);
    }

done:
    return ret;
}

int oe_mkdir(oe_devid_t devid, const char* pathname, mode_t mode)
{
    int ret = -1;

    if (devid == OE_DEVID_NULL)
    {
        ret = _mkdir(pathname, mode);
    }
    else
    {
        oe_device_t* dev;

        if (!(dev = _get_fs_device(devid)))
        {
            oe_errno = OE_EINVAL;
            goto done;
        }

        ret = dev->ops.fs->mkdir(dev, pathname, mode);
    }

done:
    return ret;
}

int oe_rmdir(oe_devid_t devid, const char* pathname)
{
    int ret = -1;

    if (devid == OE_DEVID_NULL)
    {
        ret = _rmdir(pathname);
    }
    else
    {
        oe_device_t* dev;

        if (!(dev = _get_fs_device(devid)))
        {
            oe_errno = OE_EINVAL;
            goto done;
        }

        ret = dev->ops.fs->rmdir(dev, pathname);
    }

done:
    return ret;
}

int oe_stat(oe_devid_t devid, const char* pathname, struct oe_stat* buf)
{
    int ret = -1;

    if (devid == OE_DEVID_NULL)
    {
        ret = _stat(pathname, buf);
    }
    else
    {
        oe_device_t* dev;

        if (!(dev = _get_fs_device(devid)))
        {
            oe_errno = OE_EINVAL;
            goto done;
        }

        ret = dev->ops.fs->stat(dev, pathname, buf);
    }

done:
    return ret;
}

int oe_truncate(oe_devid_t devid, const char* path, off_t length)
{
    int ret = -1;

    if (devid == OE_DEVID_NULL)
    {
        ret = _truncate(path, length);
    }
    else
    {
        oe_device_t* dev;

        if (!(dev = _get_fs_device(devid)))
        {
            oe_errno = OE_EINVAL;
            goto done;
        }

        ret = dev->ops.fs->truncate(dev, path, length);
    }

done:
    return ret;
}

int oe_access(oe_devid_t devid, const char* pathname, int mode)
{
    int ret = -1;

    if (devid == OE_DEVID_NULL)
    {
        ret = _access(pathname, mode);
    }
    else
    {
        oe_device_t* dev;
        struct oe_stat buf;

        if (!(dev = _get_fs_device(devid)))
        {
            oe_errno = OE_EINVAL;
            goto done;
        }

        if (!pathname)
        {
            oe_errno = OE_EINVAL;
            goto done;
        }

        if (_stat(pathname, &buf) != 0)
        {
            oe_errno = OE_ENOENT;
            goto done;
        }

        ret = 0;
    }

done:
    return ret;
}
