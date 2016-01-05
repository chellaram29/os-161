#include <types.h>
#include <kern/errno.h>
#include <kern/unistd.h>
#include <lib.h>
#include <uio.h>
#include <syscall.h>
#include <vnode.h>
#include <vfs.h>
#include <current.h>
#include <proc.h>
#include<synch.h>
#include<kern/fcntl.h>
#include<kern/seek.h>

int sys_open(userptr_t filename, int flags, int *error) {

	int ret = 0;
	int fd = curproc->iter_ftable;
	char *fname = kstrdup((char *) filename);

	*error = -1;

	if (curproc->iter_ftable == __OPEN_MAX)
		return EMFILE;

	curproc->ftable[fd] = (struct ftentry *) kmalloc(sizeof(struct ftentry));
	if (curproc->ftable[fd] == NULL )
		return ENOMEM;

	ret = vfs_open(fname, flags, (mode_t) NULL, &(curproc->ftable[fd]->vnode));
	if (ret == 0) {
		*error = 0;
		curproc->ftable[fd]->offset = 0;
		curproc->ftable[fd]->permissions = flags;
		curproc->ftable[fd]->refcount = 1;
		curproc->ftable[fd]->ft_lock = lock_create("ft_lock");
		curproc->ftable[fd]->eof = 0;
		curproc->iter_ftable++;

		return fd;

	}

	else {

		kfree(curproc->ftable[fd]);
		return ret;
	}

}

int sys_write(int fd, userptr_t ubuf, unsigned int nbytes, int *error) {

	int retval;
	*error = -1;

	if (curproc->ftable[fd] == NULL )
		return EBADF;

	if (fd >= curproc->iter_ftable)
		return EBADF;

	if (curproc->ftable[fd]->permissions == O_RDONLY)
		return EBADF;

	else {
		struct uio u;
		struct iovec iv;

		lock_acquire(curproc->ftable[fd]->ft_lock);

		iv.iov_ubase = ubuf;
		iv.iov_len = nbytes;
		u.uio_iov = &iv;
		u.uio_iovcnt = 1;
		u.uio_offset = curproc->ftable[fd]->offset;
		u.uio_resid = nbytes;
		u.uio_rw = UIO_WRITE;
		u.uio_segflg = UIO_USERSPACE;
		u.uio_space = curproc->p_addrspace;
		retval = VOP_WRITE(curproc->ftable[fd]->vnode, &u);
		if (retval == 0) {
			*error = 0;
			curproc->ftable[fd]->offset = u.uio_offset;
			curproc->ftable[fd]->eof = nbytes - u.uio_resid;
			lock_release(curproc->ftable[fd]->ft_lock);
			return nbytes - u.uio_resid;
		}
		lock_release(curproc->ftable[fd]->ft_lock);
		return retval;

	}

}

int sys_read(int fd, userptr_t ubuf, unsigned int nbytes, int *error) {

	int retval;
	*error = -1;
	if (curproc->ftable[fd] == NULL )
		return EBADF;

	if (fd >= curproc->iter_ftable)
		return EBADF;

	if (curproc->ftable[fd]->permissions == O_WRONLY)
		return EBADF;
	else {
		struct uio u;
		struct iovec iv;

		lock_acquire(curproc->ftable[fd]->ft_lock);
		iv.iov_ubase = ubuf;
		iv.iov_len = nbytes;

		u.uio_iov = &iv;
		u.uio_iovcnt = 1;
		u.uio_offset = curproc->ftable[fd]->offset;
		u.uio_resid = nbytes;
		u.uio_rw = UIO_READ;
		u.uio_segflg = UIO_USERSPACE;
		u.uio_space = curproc->p_addrspace;

		retval = VOP_READ(curproc->ftable[fd]->vnode,&u);
		if (retval == 0) {
			*error = 0;
			curproc->ftable[fd]->offset = u.uio_offset;
			lock_release(curproc->ftable[fd]->ft_lock);
			return nbytes - u.uio_resid;
		}

		lock_release(curproc->ftable[fd]->ft_lock);
		return retval;

	}

}

int sys_close(int fd, int *error) {

	*error = -1;
	if (curproc->ftable[fd] == NULL )
		return EBADF;
	if (fd >= curproc->iter_ftable)
		return EBADF;
	lock_acquire(curproc->ftable[fd]->ft_lock);
	*error = 0;
	curproc->ftable[fd]->refcount = curproc->ftable[fd]->refcount - 1;

	if (curproc->ftable[fd]->refcount == 0) {
		vfs_close(curproc->ftable[fd]->vnode);

	}
	lock_release(curproc->ftable[fd]->ft_lock);
	curproc->ftable[fd] = NULL;
	return 0;
}

int sys_dup2(int oldfd, int newfd, int *error) {
	*error = -1;
	if ((oldfd < 0) | (newfd < 0) | (oldfd >= curproc->iter_ftable)
			| (newfd >= curproc->iter_ftable))
		return EBADF;
	else {
		*error = 0;
		lock_acquire(curproc->ftable[oldfd]->ft_lock);
		curproc->ftable[newfd] = curproc->ftable[oldfd];
		lock_release(curproc->ftable[oldfd]->ft_lock);

		return 0;

	}

}

off_t sys_lseek(int fd, off_t pos, userptr_t whence, int *error) {
	int rv;
	*error = -1;
	if (curproc->ftable[fd] == NULL )
		return EBADF;
	if (fd >= curproc->iter_ftable)
		return EBADF;

	switch (*(int *)whence) {

	case SEEK_SET:
		if (pos < 0)
			return EINVAL;
		rv = VOP_TRYSEEK(curproc->ftable[fd]->vnode,pos);
		if (rv != 0)
			return rv;
		*error = 0;
		lock_acquire(curproc->ftable[fd]->ft_lock);
		curproc->ftable[fd]->offset = pos;
		lock_release(curproc->ftable[fd]->ft_lock);

		break;

	case SEEK_CUR:
		if (curproc->ftable[fd]->offset + pos < 0)
			return EINVAL;
		rv = VOP_TRYSEEK(curproc->ftable[fd]->vnode,curproc->ftable[fd]->offset + pos);
		if (rv != 0)
			return rv;

		*error = 0;
		lock_acquire(curproc->ftable[fd]->ft_lock);
		curproc->ftable[fd]->offset = curproc->ftable[fd]->offset + pos;
		lock_release(curproc->ftable[fd]->ft_lock);
		break;

	case SEEK_END:
		if (curproc->ftable[fd]->eof + pos < 0)
			return EINVAL;
		rv = VOP_TRYSEEK(curproc->ftable[fd]->vnode,curproc->ftable[fd]->eof + pos);
		if (rv != 0)
			return rv;

		*error = 0;
		lock_acquire(curproc->ftable[fd]->ft_lock);
		curproc->ftable[fd]->offset = curproc->ftable[fd]->eof + pos;
		lock_release(curproc->ftable[fd]->ft_lock);
		break;

	default:
		return EINVAL;
		break;

	}

	return curproc->ftable[fd]->offset;
}
int sys_chdir(userptr_t pathname, int *error) {
	int retval = 0;
	*error = -1;

	retval = vfs_chdir((char *) pathname);

	if (retval == 0)

		*error = 0;

	return retval;

}

int sys__getcwd(userptr_t buf, size_t buflen, int *error) {
	struct uio u;
	struct iovec iv;
	int rv;
	*error = -1;

	iv.iov_ubase = buf;
	iv.iov_len = buflen;

	u.uio_iov = &iv;
	u.uio_iovcnt = 1;
	u.uio_offset = 0;
	u.uio_resid = buflen;
	u.uio_rw = UIO_READ;
	u.uio_segflg = UIO_USERSPACE;
	u.uio_space = curproc->p_addrspace;

	rv = vfs_getcwd(&u);
	if (rv == 0)
		*error = 0;

	return rv;

}

