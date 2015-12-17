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
#include "types.h"

#include "mm/mm.h"
#include "mm/tlb.h"
#include "mm/mman.h"
#include "mm/page.h"

#include "proc/proc.h"

#include "util/string.h"
#include "util/debug.h"

#include "fs/vnode.h"
#include "fs/vfs.h"
#include "fs/file.h"

#include "vm/vmmap.h"
#include "vm/mmap.h"

/*
 * This function implements the mmap(2) syscall, but only
 * supports the MAP_SHARED, MAP_PRIVATE, MAP_FIXED, and
 * MAP_ANON flags.
 *
 * Add a mapping to the current process's address space.
 * You need to do some error checking; see the ERRORS section
 * of the manpage for the problems you should anticipate.
 * After error checking most of the work of this function is
 * done by vmmap_map(), but remember to clear the TLB.
 */
int
do_mmap(void *addr, size_t len, int prot, int flags,
        int fd, off_t off, void **ret)
{
        uint32_t page_count =0;
        uintptr_t addr_Return=0;
        dbg(DBG_PRINT, "(GRADING3D)Inside do_mmap\n");
        if((addr != NULL) && ((uint32_t)addr < USER_MEM_LOW || (uint32_t)addr > USER_MEM_HIGH)){
                dbg(DBG_PRINT, "(GRADING3D) Address out of range.\n");
                return -EINVAL;
        }
        if(addr == NULL)
        {
         if((flags & MAP_FIXED) || (flags==0) || (flags == MAP_TYPE)){
                dbg(DBG_PRINT, "(GRADING3D) flags condition.\n");
                return -EINVAL;
              }
        }
        if(!PAGE_ALIGNED(off)){
                dbg(DBG_PRINT, "(GRADING3D) Offset not aligned.\n");
                return -EINVAL;
        }
        if(len == 0 || (sizeof(len)==NULL) || (len == (size_t)-1)){
                dbg(DBG_PRINT, "(GRADING3D) length is zero.\n");
                return -EINVAL;
        }
        if((!(flags & MAP_ANON)) && (fd < 0 || fd >= NFILES || curproc->p_files[fd] == NULL)){
                dbg(DBG_PRINT, "(GRADING3D) files descriptor out of range.\n");
                return -EBADF;
        }
        if((flags == MAP_SHARED) && (prot & PROT_WRITE) && (!(curproc->p_files[fd]->f_mode & FMODE_READ) || !(curproc->p_files[fd]->f_mode & FMODE_WRITE))){
                dbg(DBG_PRINT, "(GRADING3D) files descriptor not open in read/write mode.\n");
                return -EACCES;
        }
        while(len>PAGE_SIZE){


                len= len - PAGE_SIZE;
                page_count++;
        }
        if(len>0){
             dbg(DBG_PRINT, "(GRADING3D) still 1 more page is needed.\n");
                page_count++;
        }
        vnode_t *vn;
        file_t *temp_file= NULL;
        if(flags & MAP_ANON){
            dbg(DBG_PRINT, "(GRADING3D) mapping is anon type set file vnode =NULL.\n");
            vn = NULL;
        }else{
             dbg(DBG_PRINT, "(GRADING3D) mapping is not anon type get a file.\n");
            temp_file = fget(fd);
            if(temp_file != NULL){
                dbg(DBG_PRINT, "(GRADING3D) returned file is not NULL set file vnode.\n");
                vn = temp_file->f_vnode;
                fput(temp_file);
            }
        }
        vmarea_t *new_vmarea;
        int vmmap_ret = vmmap_map(curproc->p_vmmap, vn, ADDR_TO_PN(addr),page_count, prot, flags, off, VMMAP_DIR_HILO, &new_vmarea);
        if (((uintptr_t)PN_TO_ADDR(new_vmarea->vma_end) > USER_MEM_HIGH) || ((uintptr_t)PN_TO_ADDR(new_vmarea->vma_start) < USER_MEM_LOW)) {
            dbg(DBG_PRINT, "(GRADING3D) returned vmarea is out of bounds.\n");
            return (int)MAP_FAILED;
        }
        if (addr == NULL) {
                dbg(DBG_PRINT, "(GRADING3D) if addr is null, return the area start page number\n");
                addr_Return = (uintptr_t)PN_TO_ADDR(new_vmarea->vma_start);
                if(ret) {
                    dbg(DBG_PRINT, "(GRADING3D) passed pointer to hold addr is valid\n");
                    *ret = (void*)addr_Return;
                }
                tlb_flush_range(addr_Return,page_count);
                pagedir_t *pd = pt_get();
                pt_unmap_range(pd, addr_Return, (uintptr_t)PN_TO_ADDR(new_vmarea->vma_start + page_count));
        } else {
                dbg(DBG_PRINT, "(GRADING3D) if addr is not null, return the page number of addr\n");
                addr_Return = (uintptr_t)addr;
                if(ret) {
                    dbg(DBG_PRINT, "(GRADING3D) if ret is not null, return the page number of addr\n");
                        *ret = (void*)addr_Return;
                }
                tlb_flush_range(addr_Return,page_count);
                pagedir_t *pd = pt_get();
                pt_unmap_range(pd, addr_Return, addr_Return + (uintptr_t)(page_count * PAGE_SIZE));
        }
        KASSERT(NULL != curproc->p_pagedir);
        dbg(DBG_PRINT, "(GRADING3A 2.a) vm area's id in the address range\n");
        return 0;
}


/*
 * This function implements the munmap(2) syscall.
 *
 * As with do_mmap() it should perform the required error checking,
 * before calling upon vmmap_remove() to do most of the work.
 * Remember to clear the TLB.
 */
int
do_munmap(void *addr, size_t len)
{

        uint32_t page_count =0;
        if(addr == NULL || (uint32_t)addr < USER_MEM_LOW || (uint32_t)addr > USER_MEM_HIGH){
                dbg(DBG_PRINT, "(GRADING3D) Address invalid.\n");
                return -EINVAL;
        }

        if((addr != NULL) && (len > USER_MEM_HIGH - (uint32_t)addr)){
                dbg(DBG_PRINT, "(GRADING3D), out of bounds address range .\n");
                return -EINVAL;
        }
        if(len == 0 || len == (size_t)-1){
                dbg(DBG_PRINT, "(GRADING3D)length is incorrect.\n");
                return -EINVAL;
        }
        while(len>PAGE_SIZE){

                len= len - PAGE_SIZE;
                page_count++;
        }
        if(len>0){
                dbg(DBG_PRINT, "(GRADING3D), finding the number of pages\n");
                page_count++;
        }
        int vmmap_ret = vmmap_remove(curproc->p_vmmap,ADDR_TO_PN(addr),(uint32_t)page_count);
        tlb_flush_range((uintptr_t)addr,page_count);
        if (((uintptr_t)addr >= USER_MEM_LOW) && ((uintptr_t)PN_TO_ADDR(ADDR_TO_PN(addr) + page_count) <= USER_MEM_HIGH)) {
                dbg(DBG_PRINT, "(GRADING3D) unmapping the address range\n");
                pt_unmap_range(pt_get(),(uintptr_t)addr, (uintptr_t)PN_TO_ADDR(ADDR_TO_PN(addr) + page_count));
        }
        return 0;
}
