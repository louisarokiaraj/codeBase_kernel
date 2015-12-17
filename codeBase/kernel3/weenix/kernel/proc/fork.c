/******************************************************************************/
/* Important Fall 2015 CSCI 402 usage information:                            */
/*                                                                            */
/* This fils is part of CSCI 402 kernel programming assignments at USC.       */
/* Please understand that you are NOT permitted to distribute or publically   */
/*         display a copy of this file (or ANY PART of it) for any reason.    */
/* If anyone (including your prospective employer) asks you to post the code, */
/*         you must inform them that you do NOT have permissions to do so.    */
/* You are also NOT permitted to remove or alter this comment block.          */
/* If this comment block is removed or altered in a submitted file, 20 points */
/*         will be deducted.                                                  */
/******************************************************************************/

#include "types.h"
#include "globals.h"
#include "errno.h"

#include "util/debug.h"
#include "util/string.h"

#include "proc/proc.h"
#include "proc/kthread.h"

#include "mm/mm.h"
#include "mm/mman.h"
#include "mm/page.h"
#include "mm/pframe.h"
#include "mm/mmobj.h"
#include "mm/pagetable.h"
#include "mm/tlb.h"

#include "fs/file.h"
#include "fs/vnode.h"

#include "vm/shadow.h"
#include "vm/vmmap.h"

#include "api/exec.h"

#include "main/interrupt.h"

/* Pushes the appropriate things onto the kernel stack of a newly forked thread
 * so that it can begin execution in userland_entry.
 * regs: registers the new thread should have on execution
 * kstack: location of the new thread's kernel stack
 * Returns the new stack pointer on success. */
static uint32_t
fork_setup_stack(const regs_t *regs, void *kstack)
{
        /* Pointer argument and dummy return address, and userland dummy return
         * address */
        uint32_t esp = ((uint32_t) kstack) + DEFAULT_STACK_SIZE - (sizeof(regs_t) + 12);
        *(void **)(esp + 4) = (void *)(esp + 8); /* Set the argument to point to location of struct on stack */
        memcpy((void *)(esp + 8), regs, sizeof(regs_t)); /* Copy over struct */
        return esp;
}


/*
 * The implementation of fork(2). Once this works,
 * you're practically home free. This is what the
 * entirety of Weenix has been leading up to.
 * Go forth and conquer.
 */
int
do_fork(struct regs *regs)
{
        /*NOT_YET_IMPLEMENTED("VM: do_fork");
        return 0;*/
		int i=0;
		vmarea_t *p_vmarea=NULL;
		vmarea_t *c_vmarea=NULL;
		mmobj_t *p_ShadowObj = NULL;
		mmobj_t *c_ShadowObj = NULL;
		mmobj_t *temp_Obj = NULL;
		mmobj_t *temp_Bottom_Obj = NULL;
		KASSERT(regs != NULL);
		dbg(DBG_PRINT,"(GRADING3A 7.a) Input registers passed to fork are valid\n");
		KASSERT(curproc != NULL);
		dbg(DBG_PRINT,"(GRADING3A 7.a) The current process is not NULL\n");
		KASSERT(curproc->p_state == PROC_RUNNING);
		dbg(DBG_PRINT,"(GRADING3A 7.a) The current process is the running process\n");
		proc_t *child_proc = proc_create("CHILD_PROCESS");
		vmmap_destroy(child_proc->p_vmmap);
		KASSERT(child_proc->p_state == PROC_RUNNING);
		dbg(DBG_PRINT,"(GRADING3A 7.a) The child process out of fork is the running process\n");
		vmmap_t *child_proc_vmmap = vmmap_clone(curproc->p_vmmap);
		list_iterate_begin(&(curproc->p_vmmap->vmm_list),p_vmarea,vmarea_t,vma_plink){
       dbg(DBG_PRINT,"(GRADING3D), iterating through the vmm_list\n");
			c_vmarea = vmmap_lookup(child_proc_vmmap,p_vmarea->vma_start);
			if(p_vmarea->vma_flags & MAP_PRIVATE){
         dbg(DBG_PRINT,"(GRADING3D), creating a shadow object for mapping\n");
				c_ShadowObj = shadow_create();
				temp_Obj = p_vmarea->vma_obj;
				temp_Bottom_Obj = mmobj_bottom_obj(temp_Obj);
				c_ShadowObj->mmo_shadowed = temp_Obj;
				c_ShadowObj->mmo_un.mmo_bottom_obj = temp_Bottom_Obj;
				list_insert_head(&(temp_Bottom_Obj->mmo_un.mmo_vmas),&(c_vmarea->vma_olink));
				c_vmarea->vma_obj=c_ShadowObj;
				p_ShadowObj = shadow_create();
				p_ShadowObj->mmo_un.mmo_bottom_obj = temp_Bottom_Obj;
				p_ShadowObj->mmo_shadowed=temp_Obj;
				p_vmarea->vma_obj = p_ShadowObj;
			}else{
        dbg(DBG_PRINT,"(GRADING3D), continue with the iteration\n");
				continue;
			}
		}list_iterate_end();
		child_proc->p_vmmap = child_proc_vmmap;
		child_proc_vmmap->vmm_proc = child_proc;
		pt_unmap_range(curproc->p_pagedir,USER_MEM_LOW,USER_MEM_HIGH);
		tlb_flush_all();
		KASSERT(child_proc->p_pagedir != NULL);
		dbg(DBG_PRINT,"(GRADING3A 7.a) The child process page directory is not pointing to NULL\n");
		dbg(DBG_PRINT,"(GRADING3D) Setting up the thread context\n");
		kthread_t *child_thread = kthread_clone(curthr);
		child_thread->kt_ctx.c_pdptr = child_proc->p_pagedir;
	    child_thread->kt_ctx.c_eip = (uintptr_t)userland_entry;
	    child_thread->kt_ctx.c_ebp = curthr->kt_ctx.c_ebp;
	    regs->r_eax = 0;
	    KASSERT(child_thread->kt_kstack != NULL);
	    dbg(DBG_PRINT,"(GRADING3A 7.a) Child's kernel stack is not NULL\n");
	    child_thread->kt_ctx.c_esp = fork_setup_stack(regs,child_thread->kt_kstack);
	    child_thread->kt_ctx.c_kstack = (uintptr_t) child_thread->kt_kstack;
	    child_thread->kt_ctx.c_kstacksz = DEFAULT_STACK_SIZE;
	    child_thread->kt_proc = child_proc;
		list_insert_tail(&(child_proc->p_threads),&(child_thread->kt_plink));
		for(i=0;i<NFILES;i++){
       dbg(DBG_PRINT,"(GRADING3D), looping through all files\n");
			child_proc->p_files[i] = curproc->p_files[i];
			if(child_proc->p_files[i] !=NULL){
				fref(child_proc->p_files[i]);
         dbg(DBG_PRINT,"(GRADING3D), incrementing the ref count on files\n");
			}
		}
		child_proc->p_brk = curproc->p_brk;
		child_proc->p_start_brk = curproc->p_start_brk;
		sched_make_runnable(child_thread);
		dbg(DBG_PRINT,"(GRADING3D) Fork has returned ! \n");
        return child_proc->p_pid;
}
