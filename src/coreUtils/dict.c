// lt_dict.c
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

// Functions for manupulating linked lists

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include "coreUtils/dict.h"
#include "coreUtils/memAlloc.h"

#include "stringTools/asciidouble.h"

void _ppl_dictRemoveEngine(dict *in, dictItem *ptr);

dict *ppl_dictInit(int HashSize, int useMalloc)
 {
  dict *out;
  if (useMalloc) out = (dict *)malloc(sizeof(dict));
  else           out = (dict *)ppl_memAlloc(sizeof(dict));
  if (out==NULL) return NULL;
  out->first     = NULL;
  out->last      = NULL;
  out->length    = 0;
  out->iNodeCount= 0;
  out->HashSize  = HashSize;
  if (useMalloc) out->HashTable = (dictItem **)malloc(HashSize * sizeof(dictItem *));
  else           out->HashTable = (dictItem **)ppl_memAlloc(HashSize * sizeof(dictItem *));
  if (out->HashTable==NULL) { if (useMalloc) free(out); return NULL; }
  memset(out->HashTable, 0, HashSize * sizeof(dictItem *));
  out->useMalloc = useMalloc;
  out->memory_context = ppl_memAlloc_GetMemContext();
  return out;
 }

int ppl_dictFree(dict *in)
 {
  dictItem *ptr, *ptrnext;
  if ((in==NULL)||(!in->useMalloc)) return 0;
  ptr = in->first;
  while (ptr != NULL)
   {
    ptrnext = ptr->next;
    free(ptr->key);
    free(ptr->data);
    free(ptr);
    ptr = ptrnext;
   }
  free(in);
  return 0;
 }

int ppl_dictHash(const char *str, int HashSize)
 {
  unsigned int hash = 5381;
  int c;
  while ((c = *str++)) hash = ((hash << 5) + hash) + c;
  return hash % HashSize;
 }

int ppl_dictLen(dict *in)
 {
  if (in==NULL) return 0;
  return in->length;
 }

#define alloc(X) ( in->useMalloc ? malloc(X) : ppl_memAlloc_incontext(X, in->memory_context) )

int ppl_dictAppend(dict *in, char *key, void *item)
 {
  dictItem *ptr=NULL, *ptrnew=NULL, *prev=NULL;
  int       cmp = -1, hash;

  ptr = in->first;
  while (ptr != NULL)
   {
    if ( ((cmp = ppl_strCmpNoCase(ptr->key, key)) > 0) || ((cmp = strcmp(ptr->key, key)) == 0) ) break;
    prev = ptr;
    ptr  = ptr->next;
   }
  if (cmp == 0) // Overwrite an existing entry in dictionary
   {
    if (in->useMalloc) free(ptr->data);
    ptr->data = item;
   }
  else
   {
    ptrnew           = (dictItem *)alloc(sizeof(dictItem));
    if (ptrnew==NULL) return 1;
    ptrnew->prev     = prev;
    ptrnew->next     = ptr;
    ptrnew->key      = (char *)alloc((strlen(key)+1));
    if (ptrnew->key==NULL) { if (in->useMalloc) free(ptrnew); return 1; }
    strcpy(ptrnew->key, key);
    ptrnew->data     = item;
    if (prev == NULL) in->first = ptrnew; else prev->next = ptrnew;
    if (ptr  == NULL) in->last  = ptrnew; else ptr ->prev = ptrnew;
    in->length++;

    hash = ppl_dictHash(key, in->HashSize);
    in->HashTable[hash] = ptrnew;
   }
  return 0;
 }

int ppl_dictAppendCpy(dict *in, char *key, void *item, int size)
 {
  dictItem *ptr=NULL, *ptrnew=NULL, *prev=NULL;
  void     *cpy;
  int       cmp = -1, hash;

  cpy = alloc(size);
  if (cpy==NULL) return 1;
  memcpy(cpy, item, size);

  ptr = in->first;
  while (ptr != NULL)
   {
    if ( ((cmp = ppl_strCmpNoCase(ptr->key, key)) > 0) || ((cmp = strcmp(ptr->key, key)) == 0) ) break;
    prev = ptr;
    ptr  = ptr->next;
   }
  if (cmp == 0) // Overwrite an existing entry in dictionary
   {
    if (in->useMalloc) free(ptr->data);
    ptr->data = cpy;
    ptrnew = ptr;
   }
  else
   {
    ptrnew           = (dictItem *)alloc(sizeof(dictItem));
    if (ptrnew==NULL) { if (in->useMalloc) free(cpy); return 1; }
    ptrnew->prev     = prev;
    ptrnew->next     = ptr;
    ptrnew->key      = (char *)alloc((strlen(key)+1));
    if (ptrnew->key==NULL) { if (in->useMalloc) { free(cpy); free(ptrnew); } return 1; }
    strcpy(ptrnew->key, key);
    ptrnew->data     = cpy;
    memcpy(ptrnew->data, item, size);
    if (prev == NULL) in->first = ptrnew; else prev->next = ptrnew;
    if (ptr  == NULL) in->last  = ptrnew; else ptr ->prev = ptrnew;
    in->length++;

    hash = ppl_dictHash(key, in->HashSize);
    in->HashTable[hash] = ptrnew;
   }
  return 0;
 }

void *ppl_dictLookup(dict *in, char *key)
 {
  int hash = ppl_dictHash(key, in->HashSize);
  return ppl_dictLookupHash(in, key, hash);
 }

void *ppl_dictLookupHash(dict *in, char *key, int hash)
 {
  dictItem *ptr;

  if (in==NULL) { return NULL; }

#define DICTLOOKUP_TEST \
   if (strcmp(ptr->key, key) == 0) return ptr->data; \

  // Check hash table
  ptr  = in->HashTable[hash];
  if (ptr==NULL) { return NULL; }
  DICTLOOKUP_TEST;

  // Hash table clash; need to exhaustively search dictionary
  ptr = in->first;
  while (ptr != NULL)
   {
    DICTLOOKUP_TEST
    else if (ppl_strCmpNoCase(ptr->key, key) > 0) break;
    ptr = ptr->next;
   }
  return NULL;
 }

void ppl_dictLookupWithWildcard(dict *in, dict *in_w, char *key, char *SubsString, int SubsMaxLen, dictItem **ptrout)
 {
  int hash, k, l;
  char tmp;
  dictItem *ptr;

  SubsString[0]='\0';
  if (in==NULL) { *ptrout=NULL; return; }

  // Check hash table
  for (k=0; (isalnum(key[k]) || (key[k]=='_')); k++);
  tmp=key[k];
  key[k]='\0';
  hash = ppl_dictHash(key, in->HashSize);
  key[k]=tmp;
  ptr  = in->HashTable[hash];
  if (ptr!=NULL)
   {
    for (k=0; ((ptr->key[k]>' ')&&(ptr->key[k]!='?')&&(ptr->key[k]==key[k])); k++);
    if (!((ptr->key[k]>' ') || (isalnum(key[k])) || (key[k]=='_')) ) { *ptrout = ptr;  return; }

    // Hash table clash; need to exhaustively search dictionary
    ptr = in->first;
    while (ptr != NULL)
     {
      for (k=0; ((ptr->key[k]>' ')&&(ptr->key[k]!='?')&&(ptr->key[k]==key[k])); k++);
      if (!((ptr->key[k]>' ') || (isalnum(key[k])) || (key[k]=='_')) ) { *ptrout = ptr;  return; }
      ptr = ptr->next;
     }
   }

  // Need to start exhaustive search of dictionary of "int_d?"-like wildcards
  for (ptr=in_w->first; ptr!=NULL; ptr=ptr->next)
   {
    for (k=0; ((ptr->key[k]>' ')&&(ptr->key[k]!='?')&&(ptr->key[k]==key[k])); k++);
    if (ptr->key[k]=='?') // dictionary key ends with a ?, e.g. "int_d?"
     {
      for (l=0; ((isalnum(key[k+l]) || (key[k+l]=='_')) && (l<SubsMaxLen)); l++) SubsString[l]=key[k+l];
      if (l==SubsMaxLen) continue; // Dummy variable name was too long
      if (l==0) continue; // Dummy variable was too short
      SubsString[l]='\0';
      *ptrout = ptr;
      return;
     } else { // Otherwise, we have to match function name exactly
      if ((ptr->key[k]>' ') || (isalnum(key[k])) || (key[k]=='_')) continue; // Nope...
      *ptrout = ptr;
      SubsString[0]='\0';
      return;
     }
   }
  SubsString[0]='\0';
  *ptrout = NULL;
  return;
 }

int ppl_dictContains(dict *in, char *key)
 {
  int hash;
  dictItem *ptr;
  if (in==NULL) return 0;

  // Check hash table
  hash = ppl_dictHash(key, in->HashSize);
  ptr  = in->HashTable[hash];
  if (ptr==NULL) return 0;
  if (strcmp(ptr->key, key)==0) return 1;

  // Hash table clash; need to exhaustively search dictionary
  ptr = in->first;
  while (ptr != NULL)
   {
    if (strcmp(ptr->key, key)==0) return 1;
    ptr = ptr->next;
   }
  return 0;
 }

int ppl_dictRemoveKey(dict *in, char *key)
 {
  int hash;
  dictItem *ptr;

  if (in==NULL) return 1;

  // Check hash table
  hash = ppl_dictHash(key, in->HashSize);
  ptr  = in->HashTable[hash];
  if (ptr==NULL) return 1;
  if (strcmp(ptr->key, key)==0) { _ppl_dictRemoveEngine(in, ptr); return 0; }

  // Hash table clash; need to exhaustively search dictionary
  ptr = in->first;
  while (ptr != NULL)
   {
    if (strcmp(ptr->key, key)==0) { _ppl_dictRemoveEngine(in, ptr); return 0; }
    else if (ppl_strCmpNoCase(ptr->key, key) > 0) break;
    ptr = ptr->next;
   }
  return 1;
 }

int ppl_dictRemove(dict *in, void *item)
 {
  dictItem *ptr;
  if (in==NULL) return 1;
  ptr = in->first;
  while (ptr != NULL)
   {
    if (ptr->data == item) { _ppl_dictRemoveEngine(in, ptr); return 0; }
    ptr = ptr->next;
   }
  return 1;
 }

void _ppl_dictRemoveEngine(dict *in, dictItem *ptr)
 {
  int hash;
  dictItem *ptrnext;

  if (in ==NULL) return;
  if (ptr==NULL) return;

  // Remove hash table entry
  hash = ppl_dictHash(ptr->key,in->HashSize);
  if (in->HashTable[hash]==ptr) in->HashTable[hash]=NULL;

  if (in->useMalloc) { free(ptr->data); free(ptr->key); }

  if (ptr->next != NULL) // We are not the last item in the list
   {
    ptrnext       = ptr->next;
    ptr->key      = ptrnext->key;
    ptr->data     = ptrnext->data;
    ptr->next     = ptrnext->next;
    if (in->last == ptrnext) in->last = ptr;
    else ptrnext->next->prev = ptr;
    if (in->useMalloc) free(ptrnext);
   }
  else if (ptr->prev != NULL) // We are the last item in the list, but not the first item
   {
    ptrnext       = ptr->prev;
    ptr->key      = ptrnext->key;
    ptr->data     = ptrnext->data;
    ptr->prev     = ptrnext->prev;
    if (in->first == ptrnext) in->first = ptr;
    else ptrnext->prev->next = ptr;
    if (in->useMalloc) free(ptrnext);
   }
  else // We are the only item in the list
   {
    in->first = NULL;
    in->last  = NULL;
   }
  in->length--;

  return;
 }

int ppl_dictRemoveAll(dict *in, void *item)
 {
  while ( !ppl_dictRemove(in,item) );
  return 0;
 }

dictIterator *ppl_dictIterateInit(dict *in)
 {
  if (in==NULL) return NULL;
  return in->first;
 }

void *ppl_dictIterate(dictIterator **in, char **key)
 {
  void *out;
  if ((in==NULL) || (*in==NULL)) return NULL;
  *key = (*in)->key;
  out  = (*in)->data;
  *in  = (*in)->next;
  return out;
 }

