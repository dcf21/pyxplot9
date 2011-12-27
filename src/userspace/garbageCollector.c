// garbageCollector.c
//
// The code in this file is part of PyXPlot
// <http://www.pyxplot.org.uk>
//
// Copyright (C) 2006-2012 Dominic Ford <coders@pyxplot.org.uk>
//               2008-2012 Ross Church
//
// $Id$
//
// PyXPlot is free software; you can redistribute it and/or modify it under the
// terms of the GNU General Public License as published by the Free Software
// Foundation; either version 2 of the License, or (at your option) any later
// version.
//
// You should have received a copy of the GNU General Public License along with
// PyXPlot; if not, write to the Free Software Foundation, Inc., 51 Franklin
// Street, Fifth Floor, Boston, MA  02110-1301, USA

// ----------------------------------------------------------------------------

#include "stdlib.h"
#include "stdio.h"

#include "gsl/gsl_matrix.h"
#include "gsl/gsl_vector.h"

#include "coreUtils/dict.h"
#include "coreUtils/list.h"
#include "userspace/garbageCollector.h"
#include "userspace/pplObj.h"
#include "userspace/pplObjFunc_fns.h"

void ppl_garbageNamespace(dict *n)
 {
  dictItem *ptr, *ptrnext;
  if ((n==NULL)||(!n->useMalloc)) return;
  ptr = n->first;
  while (ptr != NULL)
   {
    ptrnext = ptr->next;
    ppl_garbageObject((pplObj *)ptr->data);
    free(ptr->key);
    // free(ptr->data); -- already done by garbage collector
    free(ptr);
    ptr = ptrnext;
   }
  free(n);
  return;
 }

void ppl_garbageList(list *l)
 {
  listItem *ptr, *ptrnext;
  if ((l==NULL)||(!l->useMalloc)) return;
  ptr = l->first;
  while (ptr != NULL)
   {
    ptrnext = ptr->next;
    ppl_garbageObject((pplObj *)ptr->data);
    // free(ptr->data); -- already done by garbage collector
    free(ptr);
    ptr = ptrnext;
   }
  free(l);
  return;
 }

void ppl_garbageObject(pplObj *o)
 {
  int objType = o->objType;
  o->objType = PPLOBJ_ZOM; // Object is now a zombie
  if ((o->objType==PPLOBJ_ZOM)&&(o->self_this!=NULL)) ppl_garbageObject(o->self_this);
  switch(objType)
   {
    case PPLOBJ_STR:
    case PPLOBJ_EXC:
    case PPLOBJ_EXP:
    case PPLOBJ_BYT:
      if (o->auxilMalloced) { void *old=o->auxil; o->auxil=NULL; if (old!=NULL) free(old); }
      break;
    case PPLOBJ_FILE:
     {
      pplFile *f = (pplFile *)(o->auxil);
      o->auxil = NULL;
      if ((f!=NULL)&&( __sync_sub_and_fetch(&f->refCount,1) <= 0))
       {
        if ((f->open) && (f->file!=NULL)) { FILE *old=f->file; f->file=NULL; if (f->pipe) pclose(old); else fclose(old); }
        free(f);
       }
      break;
     }
    case PPLOBJ_FUNC:
     {
      pplFunc *f = (pplFunc *)(o->auxil);
      o->auxil = NULL;
      if ((f!=NULL)&&( __sync_sub_and_fetch(&f->refCount,1) <= 0)) pplObjFuncDestroy(f);
      break;
     }
    case PPLOBJ_TYPE:
     {
      pplType *t = (pplType *)(o->auxil);
      o->auxil = NULL;
      if (t!=NULL) { __sync_sub_and_fetch(&t->refCount,1); }
      break; // Types don't ever get garbage collected
     }
    case PPLOBJ_LIST:
     {
      list *l = (list *)(o->auxil);
      o->auxil = NULL;
      if ((l!=NULL)&&( __sync_sub_and_fetch(&l->refCount,1) <= 0)) ppl_garbageList(l);
      break;
     }
    case PPLOBJ_VEC:
     {
      pplVector    *v  = (pplVector *)(o->auxil);
      pplVectorRaw *vr = v->raw;
      pplMatrixRaw *vrm= v->rawm;
      o->auxil = NULL;
      if ((vr!=NULL)&&( __sync_sub_and_fetch(&vr->refCount,1) <= 0))
       {
        gsl_vector_free(vr->v);
        if (o->auxilMalloced) free(vr);
       }
      if ((vrm!=NULL)&&( __sync_sub_and_fetch(&vrm->refCount,1) <= 0))
       {
        gsl_matrix_free(vrm->m);
        if (o->auxilMalloced) free(vrm);
       }
      if ((v!=NULL)&&( __sync_sub_and_fetch(&v->refCount,1) <= 0)&&(o->auxilMalloced)) free(v);
      break;
     }
    case PPLOBJ_MAT:
     {
      pplMatrix    *m  = (pplMatrix *)(o->auxil);
      pplMatrixRaw *mr = m->raw;
      o->auxil = NULL;
      if ((mr!=NULL)&&( __sync_sub_and_fetch(&mr->refCount,1) <= 0))
       {
        gsl_matrix_free(mr->m);
        if (o->auxilMalloced) free(mr);
       }
      if ((m!=NULL)&&( __sync_sub_and_fetch(&m->refCount,1) <= 0)&&(o->auxilMalloced)) free(m);
      break;
     }
    case PPLOBJ_USER:
     {
      ppl_garbageObject(o->objPrototype);
     }
    case PPLOBJ_DICT:
    case PPLOBJ_MOD:
     {
      dict *d = (dict *)(o->auxil);
      if ((d!=NULL)&&( __sync_sub_and_fetch(&d->refCount,1) <= 0)) ppl_garbageNamespace(d);
      break;
     }
   }

  if (o->amMalloced) free(o);
  return;
 }

