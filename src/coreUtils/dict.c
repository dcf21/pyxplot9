// lt_dict.c
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
#include <ctype.h>

#include "coreUtils/dict.h"
#include "coreUtils/memAlloc.h"

#include "stringTools/asciidouble.h"

void _ppl_dictRemoveEngine(dict *in, dictItem *ptr);

dict *ppl_dictInit(int hashSize, int useMalloc)
 {
  dict *out;
  if (useMalloc) out = (dict *)malloc(sizeof(dict));
  else           out = (dict *)ppl_memAlloc(sizeof(dict));
  if (out==NULL) return NULL;
  out->first     = NULL;
  out->last      = NULL;
  out->length    = 0;
  out->refCount  = 1;
  out->immutable = 0;
  out->hashSize  = hashSize;
  if (useMalloc) out->hashTable = (dictItem **)malloc(hashSize * sizeof(dictItem *));
  else           out->hashTable = (dictItem **)ppl_memAlloc(hashSize * sizeof(dictItem *));
  if (out->hashTable==NULL) { if (useMalloc) free(out); return NULL; }
  memset(out->hashTable, 0, hashSize * sizeof(dictItem *));
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
  free(in->hashTable);
  free(in);
  return 0;
 }

int ppl_dictHash(const char *str, const int strLen, const int hashSize)
 {
  unsigned int hash = 5381;
  int c, i=0;
  if (strLen<0) while (               (c = *str++) ) hash = ((hash << 5) + hash) + c;
  else          while ((i++<strLen)&&((c = *str++))) hash = ((hash << 5) + hash) + c;
  return hash % hashSize;
 }

int ppl_dictLen(dict *in)
 {
  if (in==NULL) return 0;
  return in->length;
 }

#define alloc(X) ( in->useMalloc ? malloc(X) : ppl_memAlloc_incontext(X, in->memory_context) )

int ppl_dictAppend(dict *in, const char *key, void *item)
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

    hash = ppl_dictHash(key, -1, in->hashSize);
    in->hashTable[hash] = ptrnew;
   }
  return 0;
 }

int ppl_dictAppendCpy(dict *in, const char *key, void *item, int size)
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

    hash = ppl_dictHash(key, -1, in->hashSize);
    in->hashTable[hash] = ptrnew;
   }
  return 0;
 }

void *ppl_dictLookup(dict *in, const char *key)
 {
  int hash = ppl_dictHash(key, -1, in->hashSize);
  return ppl_dictLookupHash(in, key, hash);
 }

void *ppl_dictLookupHash(dict *in, const char *key, int hash)
 {
  dictItem *ptr;

  if (in==NULL) { return NULL; }

#define DICTLOOKUP_TEST \
   if (strcmp(ptr->key, key) == 0) return ptr->data; \

  // Check hash table
  ptr  = in->hashTable[hash];
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

void ppl_dictLookupWithWildcard(dict *in, char *key, char *SubsString, int SubsMaxLen, dictItem **ptrout)
 {
  int       hash, i, k, keylen;
  char     *magicFns[] = { "diff_d", "int_d" , NULL };
  dictItem *ptr;

  SubsString[0]='\0';
  if (in==NULL) { *ptrout=NULL; return; }

  // Check hash table
  for (k=0; (isalnum(key[k]) || (key[k]=='_')); k++);
  keylen=k;
  hash = ppl_dictHash(key, keylen, in->hashSize);
  ptr  = in->hashTable[hash];
  if (ptr!=NULL)
   {
    for (k=0; ((ptr->key[k]>' ')&&(ptr->key[k]==key[k])); k++);
    if ((ptr->key[k]=='\0')&&(!(isalnum(key[k])||(key[k]=='_')))) { *ptrout=ptr; return; }

    // Hash table clash; need to exhaustively search dictionary
    ptr = in->first;
    while (ptr != NULL)
     {
      for (k=0; ((ptr->key[k]>' ')&&(ptr->key[k]==key[k])); k++);
      if ((ptr->key[k]=='\0')&&(!(isalnum(key[k])||(key[k]=='_')))) { *ptrout=ptr; return; }
      ptr = ptr->next;
     }
   }

  // Need to search "int_d?"-like wildcards
  for (i=0; magicFns[i]!=NULL; i++)
   {
    for (k=0; ((magicFns[i][k]>' ')&&(magicFns[i][k]==key[k])); k++);
    if (magicFns[i][k]=='\0') // each magic function name can be followed by a variable name
     {
      int l=0;
      if (isalpha(key[k+l])) // first character of dummy variable name must be a letter
       {
        for (l=0; ((isalnum(key[k+l]) || (key[k+l]=='_')) && (l<SubsMaxLen)); l++) SubsString[l]=key[k+l];
       }
      if (l==SubsMaxLen) continue; // Dummy variable name was too long
      if (l==0) continue; // Dummy variable was too short
      SubsString[l]='\0';
      ptr = in->first;
      while (ptr != NULL)
       {
        for (k=0; ((ptr->key[k]>' ')&&(ptr->key[k]==magicFns[i][k])); k++);
        if ((magicFns[i][k]=='\0')&&(ptr->key[k]=='\0')) { *ptrout=ptr; return; }
        ptr = ptr->next;
       }
     }
   }
  SubsString[0]='\0';
  *ptrout = NULL;
  return;
 }

int ppl_dictContains(dict *in, const char *key)
 {
  int hash;
  dictItem *ptr;
  if (in==NULL) return 0;

  // Check hash table
  hash = ppl_dictHash(key, -1, in->hashSize);
  ptr  = in->hashTable[hash];
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

int ppl_dictRemoveKey(dict *in, const char *key)
 {
  int hash;
  dictItem *ptr;

  if (in==NULL) return 1;

  // Check hash table
  hash = ppl_dictHash(key, -1, in->hashSize);
  ptr  = in->hashTable[hash];
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
  hash = ppl_dictHash(ptr->key, -1, in->hashSize);
  if (in->hashTable[hash]==ptr) in->hashTable[hash]=NULL;

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

