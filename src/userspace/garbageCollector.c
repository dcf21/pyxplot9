// garbageCollector.c
//
// The code in this file is part of PyXPlot
// <http://www.pyxplot.org.uk>
//
// Copyright (C) 2006-2012 Dominic Ford <coders@pyxplot.org.uk>
//               2008-2011 Ross Church
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

#include "coreUtils/dict.h"
#include "coreUtils/list.h"
#include "userspace/garbageCollector.h"
#include "userspace/pplObj.h"
#include "userspace/pplObjFunc.h"

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
    free(ptr->data);
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
    free(ptr->data);
    free(ptr);
    ptr = ptrnext;
   }
  free(l);
  return;
 }

void ppl_garbageObject(pplObj *o)
 {
  switch(o->objType)
   {
    case PPLOBJ_STR:
    case PPLOBJ_EXC:
      if (o->auxilMalloced) free(o->auxil);
      break;
    case PPLOBJ_FILE:
     {
      pplFile *f = (pplFile *)(o->auxil);
      if (--f->iNodeCount <= 0)
       {
        if ((f->open) && (f->file!=NULL)) fclose(f->file);
        free(f);
       }
      break;
     }
    case PPLOBJ_FUNC:
     {
      pplFunc *f = (pplFunc *)(o->auxil);
      if (--f->iNodeCount <= 0)
       {
        free(f);
       }
      break;
     }
    case PPLOBJ_TYPE:
     {
      pplType *t = (pplType *)(o->auxil);
      if (--t->iNodeCount <= 0) 
       { 
        free(t);
       }
      break;
     }
    case PPLOBJ_LIST:
     {
      list *l = (list *)(o->auxil);
      if (--l->iNodeCount <= 0) ppl_garbageList(l);
      break;
     }
    case PPLOBJ_DICT:
    case PPLOBJ_MOD:
    case PPLOBJ_USER:
     {
      dict *d = (dict *)(o->auxil);
      if (--d->iNodeCount <= 0) ppl_garbageNamespace(d);
      break;
     }
   }

  if (o->amMalloced) free(o);
  return;
 }

