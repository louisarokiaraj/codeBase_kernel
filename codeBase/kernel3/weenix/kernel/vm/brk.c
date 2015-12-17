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

#include "globals.h"
#include "errno.h"
#include "util/debug.h"

#include "mm/mm.h"
#include "mm/page.h"
#include "mm/mman.h"

#include "vm/mmap.h"
#include "vm/vmmap.h"

#include "proc/proc.h"

/*
 * This function implements the brk(2) system call.
 *
 * This routine manages the calling process's "break" -- the ending address
 * of the process's "dynamic" region (often also referred to as the "heap").
 * The current value of a process's break is maintained in the 'p_brk' member
 * of the proc_t structure that represents the process in question.
 *
 * The 'p_brk' and 'p_start_brk' members of a proc_t struct are initialized
 * by the loader. 'p_start_brk' is subsequently never modified; it always
 * holds the initial value of the break. Note that the starting break is
 * not necessarily page aligned!
 *
 * 'p_start_brk' is the lower limit of 'p_brk' (that is, setting the break
 * to any value less than 'p_start_brk' should be disallowed).
 *
 * The upper limit of 'p_brk' is defined by the minimum of (1) the
 * starting address of the next occuring mapping or (2) USER_MEM_HIGH.
 * That is, growth of the process break is limited only in that it cannot
 * overlap with/expand into an existing mapping or beyond the region of
 * the address space allocated for use by userland. (note the presence of
 * the 'vmmap_is_range_empty' function).
 *
 * The dynamic region should always be represented by at most ONE vmarea.
 * Note that vmareas only have page granularity, you will need to take this
 * into account when deciding how to set the mappings if p_brk or p_start_brk
 * is not page aligned.
 *
 * You are guaranteed that the process data/bss region is non-empty.
 * That is, if the starting brk is not page-aligned, its page has
 * read/write permissions.
 *
 * If addr is NULL, you should NOT fail as the man page says. Instead,
 * "return" the current break. We use this to implement sbrk(0) without writing
 * a separate syscall. Look in user/libc/syscall.c if you're curious.
 *
 * Also, despite the statement on the manpage, you MUST support combined use
 * of brk and mmap in the same process.
 *
 * Note that this function "returns" the new break through the "ret" argument.
 * Return 0 on success, -errno on failure.
 */
int
do_brk(void *addr, void **ret)
{
        /*NOT_YET_IMPLEMENTED("VM: do_brk");*/
		uint32_t page_no = ADDR_TO_PN(PAGE_ALIGN_DOWN((uintptr_t)curproc->p_start_brk));
		void  *u_limit_pbrk;
		vmarea_t *new_vmarea = NULL;
		vmmap_t *vmmapObj = curproc->p_vmmap;
		if(addr == NULL){
			dbg(DBG_PRINT,"(GRADING3D) Given addr is NULL, hence setting it to current process break value\n");
			*ret = curproc->p_brk;
			return 0;
		}
		KASSERT(((uint32_t)addr > USER_MEM_LOW) || ((uint32_t)addr < USER_MEM_HIGH));
		if((uintptr_t)addr < (uintptr_t)curproc->p_start_brk){
			dbg(DBG_PRINT,"(GRADING3D) Try to set brk value below p_start_brk. Not allowed.! \n");
			return -ENOMEM;
		}
		new_vmarea = vmmap_lookup(vmmapObj,page_no);
		if(new_vmarea->vma_end <= ADDR_TO_PN((uintptr_t)addr - 1)){
			dbg(DBG_PRINT,"(GRADING3D) vma_end is less than or equal to given address \n");
			if(!vmmap_is_range_empty(vmmapObj, new_vmarea->vma_end,ADDR_TO_PN((uintptr_t)addr - 1)- new_vmarea->vma_end + 1)){
				dbg(DBG_PRINT,"(GRADING3D) the range between the given break value and the previous vmarea has mappings. Returning ENOMEM\n");
				return -ENOMEM;
			}
		}
		dbg(DBG_PRINT,"(GRADING3D) Changing the break value of the curproc->p_brk\n");
		*ret = addr;
		curproc->p_brk = addr;
		new_vmarea->vma_end = ADDR_TO_PN((uintptr_t)addr-1) + 1;

    	return 0;
}
