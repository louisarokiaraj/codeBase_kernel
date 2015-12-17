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
#include "kernel.h"
#include "errno.h"

#include "util/debug.h"

#include "proc/proc.h"

#include "mm/mm.h"
#include "mm/mman.h"
#include "mm/page.h"
#include "mm/mmobj.h"
#include "mm/pframe.h"
#include "mm/pagetable.h"

#include "vm/pagefault.h"
#include "vm/vmmap.h"

/*
 * This gets called by _pt_fault_handler in mm/pagetable.c The
 * calling function has already done a lot of error checking for
 * us. In particular it has checked that we are not page faulting
 * while in kernel mode. Make sure you understand why an
 * unexpected page fault in kernel mode is bad in Weenix. You
 * should probably read the _pt_fault_handler function to get a
 * sense of what it is doing.
 *
 * Before you can do anything you need to find the vmarea that
 * contains the address that was faulted on. Make sure to check
 * the permissions on the area to see if the process has
 * permission to do [cause]. If either of these checks does not
 * pass kill the offending process, setting its exit status to
 * EFAULT (normally we would send the SIGSEGV signal, however
 * Weenix does not support signals).
 *
 * Now it is time to find the correct page (don't forget
 * about shadow objects, especially copy-on-write magic!). Make
 * sure that if the user writes to the page it will be handled
 * correctly.
 *
 * Finally call pt_map to have the new mapping placed into the
 * appropriate page table.
 *
 * @param vaddr the address that was accessed to cause the fault
 *
 * @param cause this is the type of operation on the memory
 *              address which caused the fault, possible values
 *              can be found in pagefault.h
 */
void
handle_pagefault(uintptr_t vaddr, uint32_t cause)
{

        int forwrite = 0;
        if( vaddr<(USER_MEM_LOW) ||vaddr >= (USER_MEM_HIGH)){
                dbg(DBG_PRINT, "(GRADING3D) ADDRESS NOT VALID \n");
                do_exit(EFAULT);
                return;
        }
        vmarea_t *container = vmmap_lookup(curproc->p_vmmap, ADDR_TO_PN(vaddr));
        if(container == NULL){
          dbg(DBG_PRINT, "(GRADING3D) VMAREA NOT VALID \n");
                do_exit(EFAULT);
                return;
        }
        if(container->vma_prot == PROT_NONE){
          dbg(DBG_PRINT, "(GRADING3D) PROT NOT VALID \n");
                do_exit(EFAULT);
                return;
        }
        if((cause & FAULT_WRITE) && !(container->vma_prot & PROT_WRITE)){
          dbg(DBG_PRINT, "(GRADING3D) CONTAINER PROT NOT VALID \n");
                do_exit(EFAULT);
                return;
        }
        if(!(container->vma_prot & PROT_READ)){
          dbg(DBG_PRINT, "(GRADING3D)  PROT IS NOT PROT READ \n");
                do_exit(EFAULT);
                return;
        }
        int pagenum = ADDR_TO_PN(vaddr)-container->vma_start+container->vma_off;
        pframe_t *pf;
        if((container->vma_prot & PROT_WRITE) && (cause & FAULT_WRITE)){
          dbg(DBG_PRINT, "(GRADING3D) prot write fault write \n");
            int pf_res = pframe_lookup(container->vma_obj, pagenum, 1, &pf);
            if(pf_res<0){
              dbg(DBG_PRINT, "(GRADING3D) pframe lookup failed\n");
                do_exit(EFAULT);
            }
            pframe_dirty(pf);
        }else{
          dbg(DBG_PRINT, "(GRADING3D) prot write fault write else \n");
            int pf_res = pframe_lookup(container->vma_obj, pagenum, forwrite, &pf);
            if(pf_res<0){
              dbg(DBG_PRINT, "(GRADING3D) pframe lookup failed \n");
                do_exit(EFAULT);
            }
        }
        KASSERT(pf);
        dbg(DBG_PRINT, "(GRADING3A 5.a) pf is not NULL\n");
        KASSERT(pf->pf_addr);
        dbg(DBG_PRINT, "(GRADING3A 5.a) pf->addr is not NULL\n");
        uint32_t pdflags = PD_PRESENT | PD_USER;
        uint32_t ptflags = PT_PRESENT | PT_USER;
        if(cause & FAULT_WRITE){
          dbg(DBG_PRINT, "(GRADING3D) cause is fault write \n");
            pdflags = pdflags | PD_WRITE;
            ptflags = ptflags | PT_WRITE;
        }
        int ptmap_res = pt_map(curproc->p_pagedir, (uintptr_t)PAGE_ALIGN_DOWN(vaddr), pt_virt_to_phys((uintptr_t)pf->pf_addr), pdflags, ptflags);
}
