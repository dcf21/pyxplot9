// lt_dict.c
//
// The code in this file is part of Pyxplot
// <http://www.pyxplot.org.uk>
//
// Copyright (C) 2006-2012 Dominic Ford <coders@pyxplot.org.uk>
//               2008-2012 Ross Church
//
// $Id$
//
// Pyxplot is free software; you can redistribute it and/or modify it under the
// terms of the GNU General Public License as published by the Free Software
// Foundation; either version 2 of the License, or (at your option) any later
// version.
//
// You should have received a copy of the GNU General Public License along with
// Pyxplot; if not, write to the Free Software Foundation, Inc., 51 Franklin
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

dict *ppl_dictInit(int useMalloc)
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
  out->hashTree  = NULL;
  out->useMalloc = useMalloc;
  out->memory_context = ppl_memAlloc_GetMemContext();
  return out;
 }

void ppl_dictFreeTree(dictHash *i)
 {
  if (i==NULL) return;
  ppl_dictFreeTree(i->next);
  ppl_dictFreeTree(i->prev);
  free(i);
  return;
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
  ppl_dictFreeTree(in->hashTree);
  free(in);
  return 0;
 }

int ppl_dictHash(const char *str, const int strLen)
 {
  unsigned int hash = 5381;
  int c, i=0;
  if (strLen<0) while (               (c = *str++) ) hash = ((hash << 5) + hash) + c;
  else          while ((i++<strLen)&&((c = *str++))) hash = ((hash << 5) + hash) + c;
  return hash;
 }

#define alloc(X) ( in->useMalloc ? malloc(X) : ppl_memAlloc_incontext(X, in->memory_context) )

static void ppl_dictHashAdd(dict *in, const char *str, const int hash, dictItem *item)
 {
  dictHash **ptr = &in->hashTree;

  while ((*ptr)!=NULL)
   {
    if      ((*ptr)->hash < hash) ptr = &((*ptr)->prev);
    else if ((*ptr)->hash > hash) ptr = &((*ptr)->next);
    else
     {
      if (strcmp((*ptr)->item->key , str)==0) { (*ptr)->item=item; return; } // Overwriting a pre-existing item
      else return; // Genuine clash. Nothing we can do.
     }
   }

  *ptr = alloc(sizeof(dictHash));
  if (*ptr==NULL) return;
  (*ptr)->hash = hash;
  (*ptr)->item = item;
  (*ptr)->prev = NULL;
  (*ptr)->next = NULL;
 }

static dictItem *ppl_dictHashLookup(dict *in, const char *str, int slen, const int hash, int *clash)
 {
  dictHash **ptr = &in->hashTree;
 
  if (slen<=0) slen=strlen(str);
  *clash=0;
  while ((*ptr)!=NULL)
   {
    if      ((*ptr)->hash < hash) ptr = &((*ptr)->prev);
    else if ((*ptr)->hash > hash) ptr = &((*ptr)->next);
    else 
     {
      if (strncmp((*ptr)->item->key , str , slen)==0) return (*ptr)->item; // Found it
      *clash=1;
      return NULL;
     }
   }
  return NULL;
 }

static dictItem *ppl_dictHashRemove(dict *in, const char *str, const int hash, int *clash)
 {
  dictHash **ptr = &in->hashTree;

  *clash=0;
  while ((*ptr)!=NULL)
   {
    if      ((*ptr)->hash < hash) ptr = &((*ptr)->prev);
    else if ((*ptr)->hash > hash) ptr = &((*ptr)->next);
    else
     {
      if (strcmp((*ptr)->item->key , str)==0) // Found it
       {
        dictHash  *old = *ptr;
        dictHash **near=  ptr, *nearEntry;
        dictItem  *out = (*ptr)->item;
        if (old->prev==NULL) { *ptr=old->next; if (in->useMalloc) free(old); return out; }
        if (old->next==NULL) { *ptr=old->prev; if (in->useMalloc) free(old); return out; }
        if (hash & 1)        { while ((*near)->prev!=NULL) near=&((*near)->prev); nearEntry=*near; *near=(*near)->next; }
        else                 { while ((*near)->next!=NULL) near=&((*near)->next); nearEntry=*near; *near=(*near)->prev; }
        *ptr=nearEntry;
        if (in->useMalloc) free(old);
        return out;
       }
      else
       {
        *clash=1;
        return NULL; // Genuine clash. Nothing we can do.
       }
     }
   }
  return NULL;
 }

int ppl_dictLen(dict *in)
 {
  if (in==NULL) return 0;
  return in->length;
 }

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

    hash = ppl_dictHash(key, -1);
    ppl_dictHashAdd(in, key, hash, ptrnew);
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

    hash = ppl_dictHash(key, -1);
    ppl_dictHashAdd(in, key, hash, ptrnew);
   }
  return 0;
 }

void *ppl_dictLookup(dict *in, const char *key)
 {
  int hash = ppl_dictHash(key, -1);
  return ppl_dictLookupHash(in, key, hash);
 }

void *ppl_dictLookupHash(dict *in, const char *key, int hash)
 {
  int       clash;
  dictItem *ptr;

  if (in==NULL) { return NULL; }

  // Check hash table
  ptr  = ppl_dictHashLookup(in, key, -1, hash, &clash);
  if (!clash)
   {
    if (ptr==NULL) return NULL;
    return ptr->data;
   }

  // Hash table clash; need to exhaustively search dictionary
  ptr = in->first;
  while (ptr != NULL)
   {
    if (strcmp(ptr->key, key) == 0) return ptr->data;
    else if (ppl_strCmpNoCase(ptr->key, key) > 0) break;
    ptr = ptr->next;
   }
  return NULL;
 }

void ppl_dictLookupWithWildcard(dict *in, char *key, char *SubsString, int SubsMaxLen, dictItem **ptrout)
 {
  int       hash, clash, i, k, keylen;
  char     *magicFns[] = { "diff_d", "int_d" , NULL };
  dictItem *ptr;

  SubsString[0]='\0';
  if (in==NULL) { *ptrout=NULL; return; }

  // Check hash table
  for (k=0; (isalnum(key[k]) || (key[k]=='_')); k++);
  keylen=k;
  hash = ppl_dictHash(key, keylen);
  ptr  = ppl_dictHashLookup(in, key, keylen, hash, &clash);
  if (ptr!=NULL) { *ptrout=ptr; return; }
  if (clash)
   {
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
  int hash, clash;
  dictItem *ptr;
  if (in==NULL) return 0;

  // Check hash table
  hash = ppl_dictHash(key, -1);
  ptr  = ppl_dictHashLookup(in, key, -1, hash, &clash);
  if (!clash) return (ptr!=NULL);

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
  int hash, clash;
  dictItem *ptr;

  if (in==NULL) return 1;

  // Check hash table
  hash = ppl_dictHash(key, -1);
  ptr  = ppl_dictHashRemove(in, key, hash, &clash);
  if (ptr==NULL) return 1;
  if (!clash) { _ppl_dictRemoveEngine(in, ptr); return 0; }

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
    if (ptr->data == item) { return ppl_dictRemoveKey(in,ptr->key); }
    ptr = ptr->next;
   }
  return 1;
 }

void _ppl_dictRemoveEngine(dict *in, dictItem *ptr)
 {
  dictItem *ptrnext;

  if (in ==NULL) return;
  if (ptr==NULL) return;

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

