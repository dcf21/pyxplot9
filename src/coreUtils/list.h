// list.h
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

// Data structures for linked lists

#ifndef _LIST_H
#define _LIST_H 1

typedef struct listItemS
 {
  void             *data;
  struct listItemS *next;
  struct listItemS *prev;
 } listItem;


typedef struct listS
 {
  struct listItemS *first;
  struct listItemS *last;
  int               length, refCount, immutable;
  int               useMalloc;
  int               memory_context;
 } list;

typedef listItem listIterator;

list         *ppl_listInit       (int useMalloc);
int           ppl_listFree       (list *in);
int           ppl_listLen        (list *in);
list         *ppl_listCpy        (list *in, int useMalloc, int itemSize);
int           ppl_listAppend     (list *in, void *item);
int           ppl_listAppendCpy  (list *in, void *item, int size);
int           ppl_listInsertCpy  (list *in, int N, void *item, int size);
int           ppl_listRemove     (list *in, void *item);
int           ppl_listRemoveAll  (list *in, void *item);
void         *ppl_listGetItem    (list *in, int   N);
void         *ppl_listPop        (list *in);
void         *ppl_listPopItem    (list *in, int   N);
void         *ppl_listLast       (list *in);
listIterator *ppl_listIterateInit(list *in);
void         *ppl_listIterate    (listIterator **in);

#endif

