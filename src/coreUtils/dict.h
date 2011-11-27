// dict.h
//
// The code in this file is part of PyXPlot
// <http://www.pyxplot.org.uk>
//
// Copyright (C) 2006-2011 Dominic Ford <coders@pyxplot.org.uk>
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

// Data structures for linked lists

#ifndef _LT_DICT_H
#define _LT_DICT_H 1

#define HASHSIZE_SMALL   128
#define HASHSIZE_LARGE 16384

typedef struct dictItemS
 {
  char             *key;
  void             *data;
  struct dictItemS *next;
  struct dictItemS *prev;
 } dictItem;


typedef struct dictS
 {
  struct dictItemS  *first;
  struct dictItemS  *last;
  int                length, iNodeCount;
  int                HashSize;
  struct dictItemS **HashTable;
  int                useMalloc;
  int                memory_context;
 } dict;

typedef dictItem dictIterator;

dict         *ppl_dictInit         (int HashSize, int useMalloc);
int           ppl_dictHash         (const char *str, int HashSize);
int           ppl_dictFree         (dict *in);
int           ppl_dictLen          (dict *in);
int           ppl_dictAppend       (dict *in, const char *key, void *item);
int           ppl_dictAppendCpy    (dict *in, const char *key, void *item, int size);
void         *ppl_dictLookup       (dict *in, const char *key);
void         *ppl_dictLookupHash   (dict *in, const char *key, int hash);
void          ppl_dictLookupWithWildcard(dict *in, dict *in_w, char *key, char *SubsString, int SubsMaxLen, dictItem **ptrout);
int           ppl_dictContains     (dict *in, const char *key);
int           ppl_dictRemoveKey    (dict *in, const char *key);
int           ppl_dictRemove       (dict *in, void *item);
int           ppl_dictRemoveAll    (dict *in, void *item);
dictIterator *ppl_dictIterateInit  (dict *in);
void         *ppl_dictIterate      (dictIterator **in, char **key);

#endif

