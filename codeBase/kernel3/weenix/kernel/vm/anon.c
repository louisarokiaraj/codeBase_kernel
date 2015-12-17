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

int anon_count = 0; /* for debugging/verification purposes */

static slab_allocator_t *anon_allocator;

static void anon_ref(mmobj_t *o);
static void anon_put(mmobj_t *o);
static int  anon_lookuppage(mmobj_t *o, uint32_t pagenum, int forwrite, pframe_t **pf);
static int  anon_fillpage(mmobj_t *o, pframe_t *pf);
static int  anon_dirtypage(mmobj_t *o, pframe_t *pf);
static int  anon_cleanpage(mmobj_t *o, pframe_t *pf);

static mmobj_ops_t anon_mmobj_ops = {
        .ref = anon_ref,
        .put = anon_put,
        .lookuppage = anon_lookuppage,
        .fillpage  = anon_fillpage,
        .dirtypage = anon_dirtypage,
        .cleanpage = anon_cleanpage
};

/*
 * This function is called at boot time to initialize the
 * anonymous page sub system. Currently it only initializes the
 * anon_allocator object.
 */
void
anon_init()
{

        anon_allocator=slab_allocator_create("anon", sizeof(mmobj_t));
        KASSERT(anon_allocator != NULL);
        dbg(DBG_PRINT, "(GRADING3A 4.a) anon object is not null\n");
}

/*
 * You'll want to use the anon_allocator to allocate the mmobj to
 * return, then then initialize it. Take a look in mm/mmobj.h for
 * macros which can be of use here. Make sure your initial
 * reference count is correct.
 */
mmobj_t *
anon_create()
{

        mmobj_t *newobj=(mmobj_t*)slab_obj_alloc(anon_allocator);
        KASSERT(newobj != NULL);
        dbg(DBG_PRINT, "(GRADING3D) new anon object created \n");
        mmobj_init(newobj,&anon_mmobj_ops);
        newobj->mmo_refcount = 1;
        dbg(DBG_PRINT, "(GRADING3D) new anon object created and ref count set to 0\n");
        return newobj;
}

/* Implementation of mmobj entry points: */

/*
 * Increment the reference count on the object.
 */
static void
anon_ref(mmobj_t *o)
{
        KASSERT(o && (0 < o->mmo_refcount) && (&anon_mmobj_ops == o->mmo_ops));
        dbg(DBG_PRINT, "(GRADING3A 4.b) ref count of valid anon object is greater than zero\n");


        o->mmo_refcount++;

}

/*
 * Decrement the reference count on the object. If, however, the
 * reference count on the object reaches the number of resident
 * pages of the object, we can conclude that the object is no
 * longer in use and, since it is an anonymous object, it will
 * never be used again. You should unpin and uncache all of the
 * object's pages and then free the object itself.
 */
static void
anon_put(mmobj_t *o)
{
      KASSERT(o && (0 < o->mmo_refcount) && (&anon_mmobj_ops == o->mmo_ops));
      dbg(DBG_PRINT, "(GRADING3A 4.c) ref count of valid anon object is greater than zero\n");


      if (o->mmo_refcount-1 == o->mmo_nrespages){
          dbg(DBG_PRINT, "(GRADING3D) refcount of anon obj equals nrespages\n");
          pframe_t *pf;
          list_iterate_begin(&(o->mmo_respages),pf, pframe_t, pf_olink){
              KASSERT((!pframe_is_free(pf)));
              dbg(DBG_PRINT, "(GRADING3D) page is not busy and not free, it can be unpinned and freed\n");
              if(pframe_is_pinned(pf)){
                  dbg(DBG_PRINT, "(GRADING3D) pframe is pinned\n");
                  pframe_unpin(pf);
              }if (pframe_is_dirty(pf)){
                  dbg(DBG_PRINT, "(GRADING3D) pframe is dirty\n");
                  pframe_clean(pf);
              }
              dbg(DBG_PRINT, "(GRADING3D) freeing the pframe\n");
              pframe_free(pf);
          }list_iterate_end();
       dbg(DBG_PRINT,"(GRADING3D), Freeing the anon object\n");
      slab_obj_free(anon_allocator,o);
      }
    dbg(DBG_PRINT,"(GRADING3D), decrementing the ref count\n");
    o->mmo_refcount--;
}

/* Get the corresponding page from the mmobj. No special handling is
 * required. */
static int
anon_lookuppage(mmobj_t *o, uint32_t pagenum, int forwrite, pframe_t **pf)
{

      KASSERT(NULL != pf);
      dbg(DBG_PRINT, "(GRADING3D) page frame is not null\n");
      KASSERT(NULL != o);
      dbg(DBG_PRINT, "(GRADING3D) mmobj is not null\n");
      int pageframe_ret;
      pageframe_ret = pframe_get(o,pagenum,pf);
      return pageframe_ret;
}

/* The following three functions should not be difficult. */

static int
anon_fillpage(mmobj_t *o, pframe_t *pf)
{
        KASSERT(pframe_is_busy(pf));
        dbg(DBG_PRINT, "(GRADING3A 4.d) pframe is busy \n");
        KASSERT(!pframe_is_pinned(pf));
        dbg(DBG_PRINT, "(GRADING3A 4.d) pframe is not pinned\n");

        KASSERT(NULL != pf);
        dbg(DBG_PRINT, "(GRADING3D) pframe is not null \n");
        KASSERT(NULL != o);
        dbg(DBG_PRINT, "(GRADING3D) mmobj is not null \n");
        memset(pf->pf_addr,0,PAGE_SIZE);
        if(!pframe_is_pinned(pf))
        {
             dbg(DBG_PRINT, "(GRADING3D)pinning the pframe\n");
			       pframe_pin(pf);
		    }
        return 0;
}

static int
anon_dirtypage(mmobj_t *o, pframe_t *pf)
{
       /*NOT_YET_IMPLEMENTED("VM: anon_dirtypage");*/
       dbg(DBG_PRINT,"(GRADING3D), returning from anon dirtypage\n");
        return 0;
}

static int
anon_cleanpage(mmobj_t *o, pframe_t *pf)
{

        /*NOT_YET_IMPLEMENTED("VM: anon_cleanpage");*/
        dbg(DBG_PRINT,"(GRADING3D), returning from anon cleanpage\n");
        return 0;
}
