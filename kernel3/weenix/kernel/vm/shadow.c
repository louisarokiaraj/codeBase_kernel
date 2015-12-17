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

#include "util/string.h"
#include "util/debug.h"

#include "mm/mmobj.h"
#include "mm/pframe.h"
#include "mm/mm.h"
#include "mm/page.h"
#include "mm/slab.h"
#include "mm/tlb.h"

#include "vm/vmmap.h"
#include "vm/shadow.h"
#include "vm/shadowd.h"

#define SHADOW_SINGLETON_THRESHOLD 5

int shadow_count = 0; /* for debugging/verification purposes */
#ifdef __SHADOWD__
/*
 * number of shadow objects with a single parent, that is another shadow
 * object in the shadow objects tree(singletons)
 */
static int shadow_singleton_count = 0;
#endif

static slab_allocator_t *shadow_allocator;

static void shadow_ref(mmobj_t *o);
static void shadow_put(mmobj_t *o);
static int  shadow_lookuppage(mmobj_t *o, uint32_t pagenum, int forwrite, pframe_t **pf);
static int  shadow_fillpage(mmobj_t *o, pframe_t *pf);
static int  shadow_dirtypage(mmobj_t *o, pframe_t *pf);
static int  shadow_cleanpage(mmobj_t *o, pframe_t *pf);

static mmobj_ops_t shadow_mmobj_ops = {
        .ref = shadow_ref,
        .put = shadow_put,
        .lookuppage = shadow_lookuppage,
        .fillpage  = shadow_fillpage,
        .dirtypage = shadow_dirtypage,
        .cleanpage = shadow_cleanpage
};

/*
 * This function is called at boot time to initialize the
 * shadow page sub system. Currently it only initializes the
 * shadow_allocator object.
 */
void
shadow_init()
{
        shadow_allocator = slab_allocator_create("shadow_mmobj", sizeof(mmobj_t));
        KASSERT(shadow_allocator);
        dbg(DBG_PRINT, "(GRADING3A 6.a) shadow allocator is not NULL \n");
}

/*
 * You'll want to use the shadow_allocator to allocate the mmobj to
 * return, then then initialize it. Take a look in mm/mmobj.h for
 * macros which can be of use here. Make sure your initial
 * reference count is correct.
 */
mmobj_t *
shadow_create()
{
        mmobj_t *new_shadow = (mmobj_t *)slab_obj_alloc(shadow_allocator);
        if(new_shadow != NULL){
          dbg(DBG_PRINT, "(GRADING3D) shadow object is allocated memory, creating one\n");
          mmobj_init(new_shadow, &shadow_mmobj_ops);
          new_shadow->mmo_refcount=1;
          return new_shadow;
        }
        return NULL;

}

/* Implementation of mmobj entry points: */

/*
 * Increment the reference count on the object.
 */
static void
shadow_ref(mmobj_t *o)
{
        KASSERT(o && (0 < o->mmo_refcount) && (&shadow_mmobj_ops == o->mmo_ops));
        dbg(DBG_PRINT, "(GRADING3A 6.b) reference count is not less than 0\n");
        o->mmo_refcount++;
}

/*
 * Decrement the reference count on the object. If, however, the
 * reference count on the object reaches the number of resident
 * pages of the object, we can conclude that the object is no
 * longer in use and, since it is a shadow object, it will never
 * be used again. You should unpin and uncache all of the object's
 * pages and then free the object itself.
 */
static void
shadow_put(mmobj_t *o)
{
       KASSERT(o && (0 < o->mmo_refcount) && (&shadow_mmobj_ops == o->mmo_ops));
       dbg(DBG_PRINT, "(GRADING3A 6.c) refcount of shadow obj is  greater than 0\n");

        if (o->mmo_refcount-1 == o->mmo_nrespages){
                dbg(DBG_PRINT, "(GRADING3D) refcount of shadow_create obj equals nrespages\n");
                pframe_t *pf;
                list_iterate_begin(&(o->mmo_respages),pf, pframe_t, pf_olink){
                  KASSERT((!pframe_is_free(pf)));
                  dbg(DBG_PRINT, "(GRADING3D) page is not busy and not free, it can be unpinned and freed\n");
                    if(pframe_is_pinned(pf)){
                     dbg(DBG_PRINT, "(GRADING3D) page is pinned so unpin\n");
                      pframe_unpin(pf);
                      }
                    if (pframe_is_dirty(pf)){
                      dbg(DBG_PRINT, "(GRADING3D) page is dirty so clean it\n");
                      pframe_clean(pf);
                     }
                    dbg(DBG_PRINT, "(GRADING3D) free the page\n");
                    pframe_free(pf);
                }list_iterate_end();
                dbg(DBG_PRINT, "(GRADING3D) put the bottom object\n");
                o->mmo_shadowed->mmo_ops->put(o->mmo_shadowed);
                slab_obj_free(shadow_allocator,o);
        }
        o->mmo_refcount--;
        dbg(DBG_PRINT, "(GRADING3D) coming out of shadow put\n");

}

/* This function looks up the given page in this shadow object. The
 * forwrite argument is true if the page is being looked up for
 * writing, false if it is being looked up for reading. This function
 * must handle all do-not-copy-on-not-write magic (i.e. when forwrite
 * is false find the first shadow object in the chain which has the
 * given page resident). copy-on-write magic (necessary when forwrite
 * is true) is handled in shadow_fillpage, not here. It is important to
 * use iteration rather than recursion here as a recursive implementation
 * can overflow the kernel stack when looking down a long shadow chain */
static int
shadow_lookuppage(mmobj_t *o, uint32_t pagenum, int forwrite, pframe_t **pf)
{
  mmobj_t *shadow=o;
  int found=0;
  int ret_pget=0;
  if(!forwrite)
   {
      while(shadow->mmo_shadowed!=NULL){
           pframe_t *tempframe = pframe_get_resident(shadow,pagenum);
           if(tempframe){
              dbg(DBG_PRINT, "(GRADING3D) found a resident frame\n");
              while (pframe_is_busy(tempframe)){
                sched_sleep_on(&tempframe->pf_waitq);
              }
              *pf =tempframe;
               KASSERT(NULL != (*pf));
               dbg(DBG_PRINT, "(GRADING3A 6.d) returned resident pframe is not null\n");
               found++;
               pframe_clear_busy(*pf);
               KASSERT((pagenum == (*pf)->pf_pagenum) && (!pframe_is_busy(*pf)));
               dbg(DBG_PRINT, "(GRADING3A 6.d) returned frame is not busy and page num is correct\n");
               return 0;
            }
            dbg(DBG_PRINT, "(GRADING3D) check the next shadow object which has the pg\n");
            shadow=shadow->mmo_shadowed;
        }

              ret_pget = pframe_lookup(shadow,pagenum,0,pf);
              if (ret_pget==0)
              {
                KASSERT(NULL != (*pf));
                dbg(DBG_PRINT, "(GRADING3A 6.d) returned pframe is not null\n");
                pframe_clear_busy(*pf);
                KASSERT((pagenum == (*pf)->pf_pagenum) && (!pframe_is_busy(*pf)));
                dbg(DBG_PRINT, "(GRADING3A 6.d) returned frame is not busy and page num is correct\n");
                return 0;
              }
              dbg(DBG_PRINT, "(GRADING3D)  frame lookup returned in error\n");
                return ret_pget;
        }
 else
        {
          ret_pget= pframe_get(o,pagenum,pf);
          if (ret_pget==0)
           {
             KASSERT(NULL != (*pf));
             dbg(DBG_PRINT, "(GRADING3A 6.d) returned pframe is not null\n");
             pframe_clear_busy(*pf);
             KASSERT((pagenum == (*pf)->pf_pagenum) && (!pframe_is_busy(*pf)));
             dbg(DBG_PRINT, "(GRADING3A 6.d) returned frame is not busy and page num is correct\n");
             return 0;
           }
           dbg(DBG_PRINT, "(GRADING3D) pframe get returned in error\n");
           return ret_pget;
       }

}

/* As per the specification in mmobj.h, fill the page frame starting
 * at address pf->pf_addr with the contents of the page identified by
 * pf->pf_obj and pf->pf_pagenum. This function handles all
 * copy-on-write magic (i.e. if there is a shadow object which has
 * data for the pf->pf_pagenum-th page then we should take that data,
 * if no such shadow object exists we need to follow the chain of
 * shadow objects all the way to the bottom object and take the data
 * for the pf->pf_pagenum-th page from the last object in the chain).
 * It is important to use iteration rather than recursion here as a
 * recursive implementation can overflow the kernel stack when
 * looking down a long shadow chain */
static int
shadow_fillpage(mmobj_t *o, pframe_t *pf)
{
       pframe_t *pf1;
       KASSERT(pframe_is_busy(pf));
       dbg(DBG_PRINT, "(GRADING3A 6.e) pf is not busy.\n");
       KASSERT(!pframe_is_pinned(pf));
       dbg(DBG_PRINT, "(GRADING3A 6.e) pf is not pinned.\n");
       int ret_lookup = shadow_lookuppage(o->mmo_shadowed,pf->pf_pagenum,0,&pf1);
       if(ret_lookup==0)
       {  dbg(DBG_PRINT, "(GRADING3D) found a page frame of the given shadow object.\n");
              dbg(DBG_PRINT, "(GRADING3D) filling the given page and pinning it.\n");
              memcpy(pf->pf_addr,pf1->pf_addr,PAGE_SIZE);
              pframe_pin(pf);
              return 0;
        }else{
               dbg(DBG_PRINT, "(GRADING3D) lookup returning error null.\n");
               return ret_lookup;
        }
}

/* These next two functions are not difficult. */

static int
shadow_dirtypage(mmobj_t *o, pframe_t *pf)
{
         dbg(DBG_PRINT, "(GRADING3D) shadow dirty page.\n");
        /*NOT_YET_IMPLEMENTED("VM: shadow_dirtypage");*/
        return 0;
}

static int
shadow_cleanpage(mmobj_t *o, pframe_t *pf)
{
        dbg(DBG_PRINT, "(GRADING3D) shadow clean page.\n");
        /*NOT_YET_IMPLEMENTED("VM: shadow_cleanpage");*/
        return 0;
}
