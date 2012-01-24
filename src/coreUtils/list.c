// list.c
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

// Functions for manupulating linked lists

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "coreUtils/list.h"
#include "coreUtils/memAlloc.h"

list *ppl_listInit(int useMalloc)
 {
  list *out;
  if (useMalloc) out = (list *)malloc(sizeof(list));
  else           out = (list *)ppl_memAlloc(sizeof(list));
  if (out==NULL) return NULL;
  out->first     = NULL;
  out->last      = NULL;
  out->length    = 0;
  out->refCount  = 1;
  out->immutable = 0;
  out->useMalloc = useMalloc;
  out->memory_context = ppl_memAlloc_GetMemContext();
  return out;
 }

int ppl_listFree(list *in)
 {
  listItem *ptr, *ptrnext;
  if ((in==NULL)||(!in->useMalloc)) return 0;
  ptr = in->first;
  while (ptr != NULL)
   {
    ptrnext = ptr->next;
    free(ptr->data);
    free(ptr);
    ptr = ptrnext;
   }
  free(in);
  return 0;
 }

int ppl_listLen(list *in)
 {
  if (in==NULL) return 0;
  return in->length;
 }

#define alloc(X) ( in->useMalloc ? malloc(X) : ppl_memAlloc_incontext(X, in->memory_context) )

int ppl_listAppend(list *in, void *item)
 {
  listItem *ptrnew;
  if (in==NULL) return 1;
  ptrnew           = (listItem *)alloc(sizeof(listItem));
  if (ptrnew==NULL) return 1;
  ptrnew->prev     = in->last;
  ptrnew->next     = NULL;
  ptrnew->data     = item;
  if (in->first == NULL) in->first = ptrnew;
  if (in->last  != NULL) in->last->next = ptrnew;
  in->last = ptrnew;
  in->length++;
  return 0;
 }

int ppl_listAppendCpy(list *in, void *item, int size)
 {
  listItem *ptrnew;
  if (in==NULL) return 1;
  ptrnew         = (listItem *)alloc(sizeof(listItem));
  if (ptrnew==NULL) return 1;
  ptrnew->prev   = in->last;
  ptrnew->next   = NULL;
  ptrnew->data   = alloc(size);
  if (ptrnew->data==NULL) { if (in->useMalloc) free(ptrnew); return 1; }
  memcpy(ptrnew->data, item, size);
  if (in->first == NULL) in->first = ptrnew;
  if (in->last  != NULL) in->last->next = ptrnew;
  in->last = ptrnew;
  in->length++;
  return 0;
 }

int ppl_listInsertCpy(list *in, int N, void *item, int size)
 {
  int i;
  listItem **ptr, *ptrnew;
  if (in==NULL) return 1;
  ptrnew = (listItem *)alloc(sizeof(listItem));
  if (ptrnew==NULL) return 1;
  ptrnew->data   = alloc(size);
  if (ptrnew->data==NULL) { if (in->useMalloc) free(ptrnew); return 1; }
  memcpy(ptrnew->data, item, size);
  ptr = &in->first;
  for (i=0; ((i<N) && (*ptr!=NULL)); i++, ptr=&(*ptr)->next);
  ptrnew->prev = (*ptr==NULL) ? in->last : (*ptr)->prev;
  ptrnew->next = *ptr;
  if (*ptr!=NULL) (*ptr)->prev = ptrnew;
  *ptr         = ptrnew;
  in->length++;
  return 0;
 }

static void ppl_listRemoveEngine(list *in, listItem *ptr)
 {
  listItem *ptrnext;
  if (ptr->next != NULL) // We are not the last item in the list
   {
    ptrnext       = ptr->next;
    ptr->data     = ptrnext->data;
    ptr->next     = ptrnext->next;
    if (in->last == ptrnext) in->last = ptr;
    else ptr->next->prev = ptr;
   }
  else if (ptr->prev != NULL) // We are the last item in the list, but not the first item
   {
    ptrnext       = ptr->prev;
    ptr->data     = ptrnext->data;
    ptr->prev     = ptrnext->prev;
    if (in->first == ptrnext) in->first = ptr;
    else ptr->prev->next = ptr;
   }
  else // We are the only item in the list
   {
    in->first = NULL;
    in->last  = NULL;
   }
  in->length--;
  return;
 }

int ppl_listRemove(list *in, void *item)
 {
  listItem *ptr;
  if (in==NULL) return 1;
  ptr = in->first;
  while (ptr != NULL)
   {
    if (ptr->data == item)
     {
      ppl_listRemoveEngine(in, ptr);
      return 0;
     }
    ptr = ptr->next;
   }
  return 1;
 }

int ppl_listRemoveAll(list *in, void *item)
 {
  while ( !ppl_listRemove(in,item) );
  return 0;
 }

void *ppl_listGetItem(list *in, int N)
 {
  listItem *ptr;
  int   i;
  if (in==NULL) return NULL;
  ptr = in->first;
  for (i=0; ((i<N) && (ptr!=NULL)); i++, ptr=ptr->next);
  if (ptr==NULL) return NULL;
  else           ppl_listRemoveEngine(in, ptr);
  return ptr->data;
 }

void *ppl_listPop(list *in)
 {
  void *out;
  if (in->last == NULL) return NULL;
  out = in->last->data;
  if (in->first == in->last)
   {
    in->first = in->last = NULL;
   } else {
    in->last = in->last->prev;
    in->last->next = NULL;
   }
  in->length--;
  return out;
 }

void *ppl_listPopItem(list *in, int N)
 {
  listItem *ptr;
  int   i;
  if (in==NULL) return NULL;
  ptr = in->first;
  for (i=0; ((i<N) && (ptr!=NULL)); i++, ptr=ptr->next);
  if (ptr==NULL) return NULL;
  return ptr->data;
 }

void *ppl_listLast(list *in)
 {
  if (in->last == NULL) return NULL;
  return in->last->data;
 }

listIterator *ppl_listIterateInit(list *in)
 {
  if (in==NULL) return NULL;
  return in->first;
 }

void *ppl_listIterate(listIterator **in)
 {
  void *out;
  if ((in==NULL) || (*in==NULL)) return NULL;
  out = (*in)->data;
  *in = (*in)->next;
  return out;
 }

