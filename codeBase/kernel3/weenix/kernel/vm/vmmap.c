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

#include "kernel.h"
#include "errno.h"
#include "globals.h"

#include "vm/vmmap.h"
#include "vm/shadow.h"
#include "vm/anon.h"

#include "proc/proc.h"

#include "util/debug.h"
#include "util/list.h"
#include "util/string.h"
#include "util/printf.h"

#include "fs/vnode.h"
#include "fs/file.h"
#include "fs/fcntl.h"
#include "fs/vfs_syscall.h"

#include "mm/slab.h"
#include "mm/page.h"
#include "mm/mm.h"
#include "mm/mman.h"
#include "mm/mmobj.h"

static slab_allocator_t *vmmap_allocator;
static slab_allocator_t *vmarea_allocator;

void
vmmap_init(void)
{
        vmmap_allocator = slab_allocator_create("vmmap", sizeof(vmmap_t));
        KASSERT(NULL != vmmap_allocator && "failed to create vmmap allocator!");
        vmarea_allocator = slab_allocator_create("vmarea", sizeof(vmarea_t));
        KASSERT(NULL != vmarea_allocator && "failed to create vmarea allocator!");
}

vmarea_t *
vmarea_alloc(void)
{
        vmarea_t *newvma = (vmarea_t *) slab_obj_alloc(vmarea_allocator);
        if (newvma) {
                newvma->vma_vmmap = NULL;
        }
        return newvma;
}

void
vmarea_free(vmarea_t *vma)
{
        KASSERT(NULL != vma);
        slab_obj_free(vmarea_allocator, vma);
}

/* a debugging routine: dumps the mappings of the given address space. */
size_t
vmmap_mapping_info(const void *vmmap, char *buf, size_t osize)
{
        KASSERT(0 < osize);
        KASSERT(NULL != buf);
        KASSERT(NULL != vmmap);

        vmmap_t *map = (vmmap_t *)vmmap;
        vmarea_t *vma;
        ssize_t size = (ssize_t)osize;

        int len = snprintf(buf, size, "%21s %5s %7s %8s %10s %12s\n",
                           "VADDR RANGE", "PROT", "FLAGS", "MMOBJ", "OFFSET",
                           "VFN RANGE");

        list_iterate_begin(&map->vmm_list, vma, vmarea_t, vma_plink) {
                size -= len;
                buf += len;
                if (0 >= size) {
                        goto end;
                }

                len = snprintf(buf, size,
                               "%#.8x-%#.8x  %c%c%c  %7s 0x%p %#.5x %#.5x-%#.5x\n",
                               vma->vma_start << PAGE_SHIFT,
                               vma->vma_end << PAGE_SHIFT,
                               (vma->vma_prot & PROT_READ ? 'r' : '-'),
                               (vma->vma_prot & PROT_WRITE ? 'w' : '-'),
                               (vma->vma_prot & PROT_EXEC ? 'x' : '-'),
                               (vma->vma_flags & MAP_SHARED ? " SHARED" : "PRIVATE"),
                               vma->vma_obj, vma->vma_off, vma->vma_start, vma->vma_end);
        } list_iterate_end();

end:
        if (size <= 0) {
                size = osize;
                buf[osize - 1] = '\0';
        }

        return osize - size;
}

/* Create a new vmmap, which has no vmareas and does
 * not refer to a process. */
vmmap_t *
vmmap_create(void)
{
    dbg(DBG_PRINT,"(GRADING3D) inside vmmap_create\n");
    vmmap_t *new_vmmap = (vmmap_t*)slab_obj_alloc(vmmap_allocator);
    KASSERT(new_vmmap != NULL);
    new_vmmap->vmm_proc = NULL;
    list_init(&(new_vmmap->vmm_list));
    dbg(DBG_PRINT," (GRADING3D) Vmmap obj was created successfully\n");
    return new_vmmap;
}

/* Removes all vmareas from the address space and frees the
 * vmmap struct. */
void
vmmap_destroy(vmmap_t *map)
{
    KASSERT(NULL != map);
    dbg(DBG_PRINT,"(GRADIN3A 3.a) map is not NULL\n");
    vmarea_t *temp_vmarea = NULL;
    if(list_empty(&(map->vmm_list))){
        dbg(DBG_PRINT, "(GRADING3D) The vmmap object list is empty. Nothing to remove\n");
        return;
    }
    list_iterate_begin(&(map->vmm_list),temp_vmarea,vmarea_t,vma_plink){
        dbg(DBG_PRINT, "(GRADING3D) Removing vma_plink associated with the vmarea\n");
        list_remove(&(temp_vmarea->vma_plink));
        if(list_link_is_linked(&(temp_vmarea->vma_olink))){
            dbg(DBG_PRINT, "(GRADING3D) Removing vma_olink associated with the vmarea\n");
            list_remove(&temp_vmarea->vma_olink);
        }
    dbg(DBG_PRINT, "(GRADING3D) Decrementing the reference of the file object\n");
    mmobj_t *shadow = temp_vmarea->vma_obj;
    shadow->mmo_ops->put(shadow);
    dbg(DBG_PRINT, "(GRADING3D)Freeing the memory of the vmarea\n");
    vmarea_free(temp_vmarea);
    }list_iterate_end();
    dbg(DBG_PRINT, "(GRADING3D) Freeing the memory of the vmmap obj\n");
    slab_obj_free(vmmap_allocator, map);
}

/* Add a vmarea to an address space. Assumes (i.e. asserts to some extent)
 * the vmarea is valid.  This involves finding where to put it in the list
 * of VM areas, and adding it. Don't forget to set the vma_vmmap for the
 * area. */
void
vmmap_insert(vmmap_t *map, vmarea_t *newvma)
{
        KASSERT(NULL != map && NULL != newvma);
        dbg(DBG_PRINT, "(GRADING3A 3.b) map and area not NULL\n");
        KASSERT(NULL == newvma->vma_vmmap);
        dbg(DBG_PRINT, "(GRADING3A 3.b) address space that it belongs to is not NULL\n");
        KASSERT(newvma->vma_start < newvma->vma_end);
        dbg(DBG_PRINT, "(GRADING3A 3.b) vm area's start is less than end\n");
        KASSERT(ADDR_TO_PN(USER_MEM_LOW) <= newvma->vma_start && ADDR_TO_PN(USER_MEM_HIGH) >= newvma->vma_end);
        dbg(DBG_PRINT, "(GRADING3A 3.b) vm area address is in the valid address range\n");
        newvma->vma_vmmap = map;
        int inserted = 0;
        if(!list_empty(&(map->vmm_list))){
            dbg(DBG_PRINT,"(GRADING3D)vmmap is non-empty, Comparing blocks to find the correct place\n");
            vmarea_t *vma_iterator;
            list_iterate_begin(&(map->vmm_list), vma_iterator, vmarea_t, vma_plink) {
                dbg(DBG_PRINT,"(GRADING3D) Iterating through the vmareas to find the place\n");
                if(vma_iterator->vma_start > newvma->vma_start){
                    dbg(DBG_PRINT,"(GRADING3D) inserting the vmarea in the correct place\n");
                    list_insert_before(&vma_iterator->vma_plink, &newvma->vma_plink);
                    inserted = 1;
                    dbg(DBG_PRINT,"(GRADING3D)returning from vmmap_insert with success\n");
                    return;
                }
            }list_iterate_end();
            if(!inserted){
                dbg(DBG_PRINT,"(GRADING3D) Appending to the list of vmarea\n");
                list_insert_tail(&(map->vmm_list), &newvma->vma_plink);
            }
        }else{
            dbg(DBG_PRINT,"(GRADING3D) List is empty, appending to the list of vmarea\n");
            list_insert_tail(&(map->vmm_list), &newvma->vma_plink);
        }
}

/* Find a contiguous range of free virtual pages of length npages in
 * the given address space. Returns starting vfn for the range,
 * without altering the map. Returns -1 if no such range exists.
 *
 * Your algorithm should be first fit. If dir is VMMAP_DIR_HILO, you
 * should find a gap as high in the address space as possible; if dir
 * is VMMAP_DIR_LOHI, the gap should be as low as possible. */
int
vmmap_find_range(vmmap_t *map, uint32_t npages, int dir)
{
        dbg(DBG_PRINT,"(GRADING3D), check if address range is available\n");
        vmarea_t *temp_vmarea;
        int ret_pages=-1;
        int flag=0;
        if(dir == VMMAP_DIR_HILO){
            dbg(DBG_PRINT,"(GRADING3D) Input dir is VMMAP_DIR_HILO\n");
            list_iterate_reverse(&(map->vmm_list),temp_vmarea,vmarea_t,vma_plink){
                dbg(DBG_PRINT,"(GRADING3D)Iterating through the list to find the free virtual page\n");
                if(temp_vmarea->vma_plink.l_next==&(map->vmm_list)){
                    if((temp_vmarea->vma_end+npages) <= ADDR_TO_PN(USER_MEM_HIGH)){
                        dbg(DBG_PRINT,"(GRADING3D) Checking the last chunk of memory after last block\n");
                        ret_pages = (ADDR_TO_PN(USER_MEM_HIGH)-npages);
                        goto END;
                    }
                }
                if((temp_vmarea->vma_start-npages) >= ADDR_TO_PN(USER_MEM_LOW)){
                    dbg(DBG_PRINT,"(GRADING3D),Checking the validity of given address range\n");
                    flag = vmmap_is_range_empty(map,(temp_vmarea->vma_start-npages),npages);
                    if(flag){
                        dbg(DBG_PRINT,"(GRADING3D), Found free pages in the middle relatively higher\n");
                        ret_pages = (temp_vmarea->vma_start-npages);
                        goto END;
                    }else{
                        dbg(DBG_PRINT,"(GRADING3D) going to the next chunk of memory\n");
                        continue;
                    }
                }
            }list_iterate_end();
        }
    END:
    dbg(DBG_PRINT,"(GRADING3D) returning the range of pages found. if not found -1\n");
    return ret_pages;
}

/* Find the vm_area that vfn lies in. Simply scan the address space
 * looking for a vma whose range covers vfn. If the page is unmapped,
 * return NULL. */
vmarea_t *
vmmap_lookup(vmmap_t *map, uint32_t vfn)
{
        KASSERT(NULL != map);
        dbg(DBG_PRINT,"(GRADING3A 3.c)given map is not NULL\n");
        vmarea_t *vma_iterator;
        list_iterate_begin(&(map->vmm_list), vma_iterator, vmarea_t, vma_plink) {
            if((vfn >= vma_iterator->vma_start) && (vfn < vma_iterator->vma_end)){
                dbg(DBG_PRINT,"(GRADING3D) vmarea with the given vfn is found\n");
                return vma_iterator;
            }
        }list_iterate_end();
        dbg(DBG_PRINT,"(GRADING3D) vmarea with the given vfn is not found\n");
        return NULL;
}

/* Allocates a new vmmap containing a new vmarea for each area in the
 * given map. The areas should have no mmobjs set yet. Returns pointer
 * to the new vmmap on success, NULL on failure. This function is
 * called when implementing fork(2). */
vmmap_t *
vmmap_clone(vmmap_t *map)
{
        dbg(DBG_PRINT,"(GRADING3D) inside vmmap_clone\n");
        vmmap_t *new_map = vmmap_create();
        vmarea_t *vma_iterator;
        dbg(DBG_PRINT,"(GRADING3D) Cloning of given vmmap started\n");
        list_iterate_begin(&(map->vmm_list), vma_iterator, vmarea_t, vma_plink) {
            vmarea_t *new_area = vmarea_alloc();
            new_area->vma_start = vma_iterator->vma_start;
            new_area->vma_end = vma_iterator->vma_end;
            new_area->vma_off = vma_iterator->vma_off;
            new_area->vma_prot = vma_iterator->vma_prot;
            new_area->vma_flags = vma_iterator->vma_flags;
            new_area->vma_vmmap = NULL;
            new_area->vma_obj = vma_iterator->vma_obj;
            new_area->vma_obj->mmo_ops->ref(new_area->vma_obj);
            list_link_init(&(new_area->vma_olink));
            list_link_init(&(new_area->vma_plink));
            vmmap_insert(new_map, new_area);
        }list_iterate_end();
        dbg(DBG_PRINT,"(GRADING3D) Cloning of given vmmap completed successfully\n");
        return new_map;
}

/* Insert a mapping into the map starting at lopage for npages pages.
 * If lopage is zero, we will find a range of virtual addresses in the
 * process that is big enough, by using vmmap_find_range with the same
 * dir argument.  If lopage is non-zero and the specified region
 * contains another mapping that mapping should be unmapped.
 *
 * If file is NULL an anon mmobj will be used to create a mapping
 * of 0's.  If file is non-null that vnode's file will be mapped in
 * for the given range.  Use the vnode's mmap operation to get the
 * mmobj for the file; do not assume it is file->vn_obj. Make sure all
 * of the area's fields except for vma_obj have been set before
 * calling mmap.
 *
 * If MAP_PRIVATE is specified set up a shadow object for the mmobj.
 *
 * All of the input to this function should be valid (KASSERT!).
 * See mmap(2) for for description of legal input.
 * Note that off should be page aligned.
 *
 * Be very careful about the order operations are performed in here. Some
 * operation are impossible to undo and should be saved until there
 * is no chance of failure.
 *
 * If 'new' is non-NULL a pointer to the new vmarea_t should be stored in it.
 */
int
vmmap_map(vmmap_t *map, vnode_t *file, uint32_t lopage, uint32_t npages,
          int prot, int flags, off_t off, int dir, vmarea_t **new)
{

        KASSERT(NULL != map);
        dbg(DBG_PRINT,"GRADING3A 3.d map is valid\n");
        KASSERT(0 < npages);
        dbg(DBG_PRINT,"GRADING3A 3.d npages > 0\n");
        KASSERT((MAP_SHARED & flags) || (MAP_PRIVATE & flags));
        dbg(DBG_PRINT,"GRADING3A 3.d mapping is either shared or private\n");
        KASSERT((0 == lopage) || (ADDR_TO_PN(USER_MEM_LOW) <= lopage));
        dbg(DBG_PRINT,"GRADING3A 3.d lopage is either 0 or greater than min. permissible address\n");
        KASSERT((0 == lopage) || (ADDR_TO_PN(USER_MEM_HIGH) >= (lopage + npages)));
        dbg(DBG_PRINT,"GRADING3A 3.d lopage is either 0 or less than max. permissible address\n");
        KASSERT(PAGE_ALIGNED(off));
        dbg(DBG_PRINT,"GRADING3A 3.d off is page aligned\n");

        int ret_range=0;
        int is_empty;
        vmarea_t *new_vmarea;
        if(lopage==0){
             dbg(DBG_PRINT,"(GRADING3D) lopage is zero\n");
            ret_range = vmmap_find_range(map,npages,dir);
            if(ret_range<0){
                dbg(DBG_PRINT,"(GRADING3D) No memory for npages\n");
                return -ENOMEM;
            }
        }else{
            dbg(DBG_PRINT,"(GRADING3D), Entering lopage is non 0\n");
            is_empty = vmmap_is_range_empty(map,lopage,npages);
            if(is_empty!=0){
                dbg(DBG_PRINT, "(GRADING3D) Mapping is not existing !!\n");
            }else{
                dbg(DBG_PRINT, "(GRADING3D) Mapping exists! Hence Removing\n");
                vmmap_remove(map,lopage,npages);
            }
        ret_range = lopage;
        }
        dbg(DBG_PRINT, "(GRADING3D) The returned range of virtual address is not NULL\n");
        new_vmarea = vmarea_alloc();
        new_vmarea->vma_start = ret_range;
        new_vmarea->vma_end = ret_range + npages;
        new_vmarea->vma_off = ADDR_TO_PN(off);
        new_vmarea->vma_prot = prot;
        new_vmarea->vma_flags = flags;
        list_link_init(&(new_vmarea->vma_plink));
        list_link_init(&(new_vmarea->vma_olink));
    if(file == NULL){
        dbg(DBG_PRINT,"(GRADING3D) File is NULL. So creating anon object and mapping it\n");
        mmobj_t *new_anon_obj = anon_create();
        new_vmarea->vma_obj = new_anon_obj;
    }else{
        dbg(DBG_PRINT,"(GRADING3D)File is not NULL. Mapping a file object\n");
        mmobj_t *new_file_obj;
        file->vn_ops->mmap(file,new_vmarea, &new_file_obj);
        new_vmarea->vma_obj = new_file_obj;
    }
    if(flags & MAP_PRIVATE){
        dbg(DBG_PRINT,"(GRADING3D) Pvt mapping , file is NULL. So creating shadow object and mapping it\n");
        mmobj_t *new_shadow_obj = shadow_create();
        new_shadow_obj->mmo_shadowed = new_vmarea->vma_obj;
        new_shadow_obj->mmo_un.mmo_bottom_obj = mmobj_bottom_obj(new_vmarea->vma_obj);
        list_insert_head(&new_vmarea->vma_obj->mmo_un.mmo_vmas, &new_vmarea->vma_olink);
        new_vmarea->vma_obj = new_shadow_obj;
    }
    if(new != NULL){
        dbg(DBG_PRINT,"(GRADING3D) the input new is not NULL. Hence creating a pointer to new on the newly created vmarea\n");
        *new = new_vmarea;
        vmmap_insert(map,new_vmarea);
    }else{
        dbg(DBG_PRINT,"(GRADING3D),new is NULL. Hence inserting the newly created vmarea\n");
        vmmap_insert(map,new_vmarea);
    }
    dbg(DBG_PRINT,"(GRADING3D) returning from vmmap_map with success\n");
    return 0;
}

/*
 * We have no guarantee that the region of the address space being
 * unmapped will play nicely with our list of vmareas.
 *
 * You must iterate over each vmarea that is partially or wholly covered
 * by the address range [addr ... addr+len). The vm-area will fall into one
 * of four cases, as illustrated below:
 *
 * key:
 *          [             ]   Existing VM Area
 *        *******             Region to be unmapped
 *
 * Case 1:  [   ******    ]
 * The region to be unmapped lies completely inside the vmarea. We need to
 * split the old vmarea into two vmareas. be sure to increment the
 * reference count to the file associated with the vmarea.
 *
 * Case 2:  [      *******]**
 * The region overlaps the end of the vmarea. Just shorten the length of
 * the mapping.
 *
 * Case 3: *[*****        ]
 * The region overlaps the beginning of the vmarea. Move the beginning of
 * the mapping (remember to update vma_off), and shorten its length.
 *
 * Case 4: *[*************]**
 * The region completely contains the vmarea. Remove the vmarea from the
 * list.
 */
int
vmmap_remove(vmmap_t *map, uint32_t lopage, uint32_t npages)
{

        dbg(DBG_PRINT,"(GRADING3D) inside vmmap_remove\n");
        vmarea_t *temp_vmarea;
        vmarea_t *split_vmarea;
        int case_flag = 0;
        if(vmmap_is_range_empty(map,lopage,npages)){
            dbg(DBG_PRINT, "(GRADING3D) There is no mapping to remove\n");
            return 0;
        }
        list_iterate_begin(&(map->vmm_list),temp_vmarea,vmarea_t,vma_plink){
            if((temp_vmarea->vma_start < lopage) && (temp_vmarea->vma_end > lopage+npages)){
                dbg(DBG_PRINT, "(GRADING3D) Case 1 of [   ******    ]\n");
                case_flag = 1;
            }if((temp_vmarea->vma_start < lopage) && (temp_vmarea->vma_end <= lopage+npages)){
                dbg(DBG_PRINT, "(GRADING3D) Case 2 of [      *******]**\n");
                case_flag = 2;
            }if((temp_vmarea->vma_start >= lopage) && (temp_vmarea->vma_end > lopage+npages)){
                dbg(DBG_PRINT, "(GRADING3D) Case 3 of *[*****        ]\n");
                case_flag = 3;
            }if((temp_vmarea->vma_start >= lopage) && (temp_vmarea->vma_end <= lopage + npages)){
                dbg(DBG_PRINT, "(GRADING3D) Case 4 of *[*************]**\n");
                case_flag = 4;
            }if(temp_vmarea->vma_start >= (lopage + npages) || temp_vmarea->vma_end <= lopage){
                dbg(DBG_PRINT, "(GRADING3D), Case 5 of Normal Vmarea found\n");
                case_flag = 5;
            }
            switch(case_flag){
                case 1:
                    dbg(DBG_PRINT, "(GRADING3D), Splitting the old vmarea into two vmareas\n");
                    split_vmarea = vmarea_alloc();
                    split_vmarea->vma_start = temp_vmarea->vma_start;
                    split_vmarea->vma_end = lopage;
                    split_vmarea->vma_off = temp_vmarea->vma_off;
                    split_vmarea->vma_prot = temp_vmarea->vma_prot;
                    split_vmarea->vma_flags = temp_vmarea->vma_flags;
                    split_vmarea->vma_obj = temp_vmarea->vma_obj;
                    split_vmarea->vma_vmmap = temp_vmarea->vma_vmmap;
                    split_vmarea->vma_obj->mmo_ops->ref(split_vmarea->vma_obj);
                    list_link_init(&(split_vmarea->vma_plink));
                    list_link_init(&(split_vmarea->vma_olink));
                    list_insert_before(&(temp_vmarea->vma_plink),&(split_vmarea->vma_plink));
                    mmobj_t *new_bottom_obj = mmobj_bottom_obj(temp_vmarea->vma_obj);
                    if(new_bottom_obj != temp_vmarea->vma_obj){
                         dbg(DBG_PRINT, "(GRADING3D), Insert the object into the list\n");
                        list_insert_head(&(new_bottom_obj->mmo_un.mmo_vmas), &(split_vmarea->vma_olink));
                    }
                    temp_vmarea->vma_off = lopage-temp_vmarea->vma_start+npages+temp_vmarea->vma_off;
                    temp_vmarea->vma_start = npages+lopage;
                    return 0;
                case 2:
                    dbg(DBG_PRINT, "(GRADING3D) Overlap Condition exists. Adjusting the end virtual address of vmarea\n");
                    temp_vmarea->vma_end = lopage;
                    continue;
                case 3:
                    dbg(DBG_PRINT, "(GRADING3D) Overlap Condition exists. Adjusting the end virtual address of vmarea and its offset\n");
                    temp_vmarea->vma_off = (temp_vmarea->vma_off +lopage-temp_vmarea->vma_start+npages);
                    temp_vmarea->vma_start =(lopage + npages);
                    continue;
                case 4:
                    dbg(DBG_PRINT, "(GRADING3D) Overlap Condition exists. removing the vmarea\n");
                    mmobj_t *shadow = temp_vmarea->vma_obj;
                    shadow->mmo_ops->put(shadow);
                    if(list_link_is_linked(&(temp_vmarea->vma_olink))){
                        list_remove(&(temp_vmarea->vma_olink));
                    }
                    list_remove(&(temp_vmarea->vma_plink));
                    vmarea_free(temp_vmarea);
                    continue;
                case 5:
                    dbg(DBG_PRINT, "(GRADING3D) normal vmarea\n");
                    continue;
                }
        }list_iterate_end();
    dbg(DBG_PRINT, "(GRADING3D),returning from vmmap_remove with success\n");
    return 0;
}

/*
 * Returns 1 if the given address space has no mappings for the
 * given range, 0 otherwise.
 */
int
vmmap_is_range_empty(vmmap_t *map, uint32_t startvfn, uint32_t npages)
{
        vmarea_t *vma_iterator;
        uint32_t endvfn = startvfn+npages;
        KASSERT((startvfn < endvfn) && (ADDR_TO_PN(USER_MEM_LOW) <= startvfn) && (ADDR_TO_PN(USER_MEM_HIGH) >= endvfn));
        dbg(DBG_PRINT, "(GRADING3A 3.e) address range it's asking is valid\n");
        if (list_empty(&(map->vmm_list))){
            dbg(DBG_PRINT," (GRADING3D) vmarea list is empty\n");
            return 1;
        }

        list_iterate_begin(&(map->vmm_list), vma_iterator, vmarea_t, vma_plink) {
            if ((vma_iterator->vma_start >= endvfn) || (vma_iterator->vma_end <= startvfn)){
                 dbg(DBG_PRINT, "(GRADING3D) Not found\n");
                continue;
            }else{
                dbg(DBG_PRINT, "(GRADING3D) Overlap Found\n");
                return 0;
            }
        }list_iterate_end();
        dbg(DBG_PRINT, "(GRADING3D) mapping not found\n");
        return 1;
}

/* Read into 'buf' from the virtual address space of 'map' starting at
 * 'vaddr' for size 'count'. To do so, you will want to find the vmareas
 * to read from, then find the pframes within those vmareas corresponding
 * to the virtual addresses you want to read, and then read from the
 * physical memory that pframe points to. You should not check permissions
 * of the areas. Assume (KASSERT) that all the areas you are accessing exist.
 * Returns 0 on success, -errno on error.
 */
int
vmmap_read(vmmap_t *map, const void *vaddr, void *buf, size_t count)
{

        dbg(DBG_PRINT,"(GRADING3D) inside vmmap_read\n");
        vmarea_t *new_vmarea;
        uint32_t physical_pageNo=0;
        int pframe_lookup_ret=0;
        pframe_t *physical_page;
        int to_read=0;
        char *src = NULL;
        char *dest = (char *)buf;
        uint32_t temp_addr = (uint32_t)vaddr;
        uint32_t page_no = ADDR_TO_PN(vaddr);
        uint32_t page_off = PAGE_OFFSET(vaddr);
        while(count>0){
            dbg(DBG_PRINT, "(GRADING3D) Reading mapping of given count\n");
            new_vmarea = vmmap_lookup(map,page_no);
            physical_pageNo = (page_no - (new_vmarea->vma_start) + (new_vmarea->vma_off));
            pframe_lookup(new_vmarea->vma_obj,physical_pageNo,0,&physical_page);
            src = (char *)physical_page->pf_addr+page_off;

                dbg(DBG_PRINT, "(GRADING3D) Finding the minimum memory space required for reading\n");
                to_read = count;
            
            memcpy(dest,src,to_read);
            dest = dest+to_read;
            count = count - to_read;
            temp_addr = temp_addr + to_read;
            page_off = PAGE_OFFSET(temp_addr);
            page_no = ADDR_TO_PN(temp_addr);
              dbg(DBG_PRINT, "(GRADING3D) copying to destn for read\n");
        }
    dbg(DBG_PRINT, "(GRADING3D) Reading is completed successfully\n");
    return 0;

}

/* Write from 'buf' into the virtual address space of 'map' starting at
 * 'vaddr' for size 'count'. To do this, you will need to find the correct
 * vmareas to write into, then find the correct pframes within those vmareas,
 * and finally write into the physical addresses that those pframes correspond
 * to. You should not check permissions of the areas you use. Assume (KASSERT)
 * that all the areas you are accessing exist. Remember to dirty pages!
 * Returns 0 on success, -errno on error.
 */
int
vmmap_write(vmmap_t *map, void *vaddr, const void *buf, size_t count)
{

        dbg(DBG_PRINT,"(GRADING3D), inside vmmap_write\n");
        vmarea_t *new_vmarea;
        uint32_t physical_pageNo=0;
        int pframe_lookup_ret=0;
        pframe_t *physical_page;
        size_t to_write=0;
        char *dest = NULL;
        char *src = (char *)buf;
        uint32_t temp_addr = (uint32_t)vaddr;
        uint32_t page_no = ADDR_TO_PN(vaddr);
        uint32_t page_off = PAGE_OFFSET(vaddr);
        while(count>0){

            new_vmarea = vmmap_lookup(map,page_no);
            dbg(DBG_PRINT, "(GRADING3D) writing mapping of given count\n");
            physical_pageNo = (page_no - (new_vmarea->vma_start) + (new_vmarea->vma_off));
            pframe_lookup(new_vmarea->vma_obj,physical_pageNo,1,&physical_page);
            dest = (char *)physical_page->pf_addr+page_off;

                dbg(DBG_PRINT, "(GRADING3D) Finding the minimum memory space required for writing\n");
                to_write = count;
            
            memcpy(dest,src,to_write);
            pframe_dirty(physical_page);
            dbg(DBG_PRINT, "(GRADING3D) Writing is completed successfully\n");
            count = count - to_write;
            dest = dest+to_write;
            temp_addr = temp_addr + to_write;
            page_off = PAGE_OFFSET(temp_addr);
            page_no = ADDR_TO_PN(temp_addr);
        }
    return 0;
}
