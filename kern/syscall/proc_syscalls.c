#include <types.h>

#include <kern/errno.h>
#include <kern/unistd.h>
#include <kern/wait.h>
#include <lib.h>
#include <syscall.h>
#include <current.h>
#include <proc.h>
#include <thread.h>
#include <addrspace.h>
#include<synch.h>
#include<wchan.h>
#include<kern/list.h>
#include<thread.h>
#include<kern/fcntl.h>
#include<vfs.h>

#include <copyinout.h>
int findpaddedlen(char *str);
/* this implementation of sys__exit does not do anything with the exit code */
/* this needs to be fixed to get exit() and waitpid() working properly */

void sys__exit(int exitcode) {
	struct proc *p = curproc;

	struct ptentry *parentptentry = findptentry(curproc->ppid);
	struct ptentry *childptentry = findptentry(curproc->pid);

	if (strcmp(curthread->t_name, "childproc") == 0) {

		if (parentptentry == NULL ) {
			delete(&proctable, childptentry);

		} else {

			(void)parentptentry;
			childptentry->exitcode = exitcode;
			childptentry->exited = 1;
			wchan_wakeall(childptentry->synch);
		}
	}

	proc_remthread(curthread);
	proc_destroy(p);

	thread_exit();

}

/* stub handler for getpid() system call                */
pid_t sys_getpid(int *error) {

	*error = 0;
	return curproc->pid;

}

/* stub handler for waitpid() system call                */

int sys_waitpid(pid_t pid, userptr_t status, int options, int *error) {

	struct ptentry *pte;
	struct node *p;
	int rv = 0;
	*error = -1;

	if (options != 0) {
		return (EINVAL);
	}

	p = curproc->childpid;
	while (p != NULL ) {
		if (pid == *((int *) p->data))
			break;
		p = p->next;
	}

	if (p == NULL )
		return ECHILD;

	pte = findptentry(pid);
	if (pte == NULL )
		return ECHILD;

	while (1) {

		if (pte->exited == 1) {

			rv = copyout(&(pte->exitcode), status, sizeof(status));
			if (rv == 0) {
				*error = 0;
				delete(&proctable, pte);
				return 0;
			} else
				return rv;

		}

		else {
			wchan_lock( pte->synch);
			wchan_sleep(pte->synch);

		}

	}

}

pid_t sys_fork(struct trapframe *tf, int *error) {

	struct proc *childproc;
	struct ptentry *childptentry;
	pid_t childpid;
	pid_t *childpid_p = (pid_t *) kmalloc(sizeof(pid_t));

	struct trapframe *utf = (struct trapframe *) kmalloc(
			sizeof(struct trapframe));

	int result;
	unsigned long nargs = 1;

	*error = -1;
	childproc = proc_create_runprogram("childproc");
	if (childproc == NULL )
		return ENOMEM;

	result = as_copy(curproc->p_addrspace, &childproc->p_addrspace);
	if (result != 0)
		return ENOMEM;

	utf->tf_vaddr = tf->tf_vaddr;
	utf->tf_status = tf->tf_status;
	utf->tf_cause = tf->tf_cause;
	utf->tf_lo = tf->tf_lo;
	utf->tf_hi = tf->tf_hi;
	utf->tf_ra = tf->tf_ra;
	utf->tf_at = tf->tf_at;
	utf->tf_v0 = tf->tf_v0;
	utf->tf_v1 = tf->tf_v1;
	utf->tf_a0 = tf->tf_a0;
	utf->tf_a1 = tf->tf_a1;
	utf->tf_a2 = tf->tf_a2;
	utf->tf_a3 = tf->tf_a3;
	utf->tf_t0 = tf->tf_t0;
	utf->tf_t1 = tf->tf_t1;
	utf->tf_t2 = tf->tf_t2;
	utf->tf_t3 = tf->tf_t3;
	utf->tf_t4 = tf->tf_t4;
	utf->tf_t5 = tf->tf_t5;
	utf->tf_t6 = tf->tf_t6;
	utf->tf_t7 = tf->tf_t7;
	utf->tf_s0 = tf->tf_s0;
	utf->tf_s1 = tf->tf_s1;
	utf->tf_s2 = tf->tf_s2;
	utf->tf_s3 = tf->tf_s3;
	utf->tf_s4 = tf->tf_s4;
	utf->tf_s5 = tf->tf_s5;
	utf->tf_s6 = tf->tf_s6;
	utf->tf_s7 = tf->tf_s7;
	utf->tf_t8 = tf->tf_t8;
	utf->tf_t9 = tf->tf_t9;
	utf->tf_k0 = tf->tf_k0;
	utf->tf_k1 = tf->tf_k1;
	utf->tf_gp = tf->tf_gp;
	utf->tf_sp = tf->tf_sp;
	utf->tf_s8 = tf->tf_s8;
	utf->tf_epc = tf->tf_epc;




//maximum process count

	lock_acquire(proctable_lk);
	childpid = findunusedpid();
	if (childpid == ENPROC) {
		lock_release(proctable_lk);
		return childpid;
	} else {
		childptentry = (struct ptentry *) kmalloc(sizeof(struct ptentry));

		if (childptentry == NULL )
			return ENOMEM;

		childptentry->pid = childpid;
		childptentry->ppid = curproc->pid;
		childptentry->exited = 0;
		childptentry->synch = wchan_create("proctable");
		childptentry->exitcode = -1;
		childptentry->isparentexited = -1;

		result = insert(&proctable, childptentry);
		if (result != 0)
			return result;

		*childpid_p = childpid;

		insert(&curproc->childpid, childpid_p);

		childproc->pid = childpid;
		childproc->ppid = curproc->pid;

		lock_release(proctable_lk);

	}

//copy the filetable to child process
	result = thread_fork("childproc" /* thread name */,
			childproc /* new process */,
			enter_forked_process /* thread function */, utf /* thread arg */,
			nargs /* thread arg */);
	if (result) {
		kprintf("thread_fork failed: %s\n", strerror(result));
		proc_destroy(childproc);
		kfree(childptentry);
		return result;
	}

	*error = 0;

	return childpid;

}

int findpaddedlen(char *str) {

	int len = strlen(str) + 1;
	while ((len % 4 != 0) & (len % 8 != 0))
		len++;
	return len;

}
void memset(char *ptr, char c, int len);
void memset(char *ptr, char c, int len) {

	for (int i = 0; i < len; i++)
		*(ptr + i) = c;
}

int sys_execv(struct trapframe *tf, userptr_t program, userptr_t *uargs) {

	int argc = 0,totallen=0;
	int pgmnamelen = 0;
	char **kargs;
	int i = 0, result = 0;
	struct vnode *v;
	struct addrspace *as;
	vaddr_t entrypoint, stackptr;
	//find argc
	while (uargs[argc] != NULL ) {
		argc++;
	}
	argc = argc + 1;
	(void) tf;
	kargs = (char **) kmalloc(sizeof(char *) * argc);

	pgmnamelen = findpaddedlen((char *) program);
	kargs[0] = (char *) kmalloc(pgmnamelen);
	memset(kargs[0], '\0', pgmnamelen);
	strcpy(kargs[0], (char *) program);
	totallen+=pgmnamelen;
	while (i < argc - 1) {
		int j = i + 1;
		int len = findpaddedlen((char *) uargs[i]);
		totallen+=len;
		kargs[j] = (char *) kmalloc(len);
		memset(kargs[j], '\0', len);
		strcpy(kargs[j], (char *) uargs[i]);
		i++;
	}

	kargs[argc] = NULL;

	result = vfs_open((char *) program, O_RDONLY, 0, &v);
	if (result) {
		return result;
	}

	as = as_create();
	if (as == NULL ) {
		vfs_close(v);
		return ENOMEM;
	}

	//		as_deactivate();
	//	as_destroy(curproc_getas());

	/* Switch to it and activate it. */
	curproc_setas(as);
	as_activate();

	/* Load the executable. */
	result = load_elf(v, &entrypoint);
	if (result) {
		/* p_addrspace will go away when curproc is destroyed */
		vfs_close(v);
		return result;
	}

	/* Done with the file now. */
	vfs_close(v);

	/* Define the user stack in the address space */
	result = as_define_stack(as, &stackptr);
	if (result) {
		/* p_addrspace will go away when curproc is destroyed */
		return result;
	}

	i = 0;

	stackptr=stackptr-totallen-((argc+1)*4);

	char * p = (char *) (stackptr + ((argc+1)*4) );
	char **ptr = (char **) stackptr;
	while (kargs[i] != NULL ) {

		int len = findpaddedlen(kargs[i]);
		memset(p, '\0', len);

		strcpy(p, kargs[i]);
		ptr[i] = p;
		i++;
		p=p+len;

	}

	ptr[i] = (char *) NULL;

	/* Warp to user mode. */
	enter_new_process(argc /*argc*/,
			(userptr_t)stackptr /*userspace addr of argv*/, stackptr,
			entrypoint);

	return 0;

}
