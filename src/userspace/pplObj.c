// pplObj.c
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

#define _PPLOBJ_C 1

#include <stdlib.h>
#include <string.h>

#include "coreUtils/errorReport.h"
#include "coreUtils/memAlloc.h"
#include "coreUtils/list.h"
#include "coreUtils/dict.h"

#include "userspace/context.h"
#include "userspace/pplObj_fns.h"
#include "userspace/pplObjFunc.h"
#include "userspace/pplObjMethods.h"
#include "userspace/pplObjUnits.h"

// Items with ! should never be displayed because they are internal markers
const char *pplObjTypeNames[] = {"number","string","boolean","date","color","dictionary","module","list","vector","matrix","file handle","function","type","null","exception","!global","!zombie","instance",NULL};
const int   pplObjTypeOrder[] = { 2      , 4      , 1       , 3    , 5     ,  9          , 11    , 7    , 6      , 8      , 13          , 12       , 14    , 0   , 15        , 0       , 0       , 10       };
pplObj     *pplObjPrototypes;

void pplObjInit(ppl_context *c)
 {
  static int initialised=0; // Only ever run this once
  int i;
  const int n = PPLOBJ_USER+1;
  if (initialised) return;
  pplType *typeObjs = (pplType *)malloc(n * sizeof(pplType));
  pplObjPrototypes  = (pplObj  *)malloc(n * sizeof(pplObj));
  if ((typeObjs==NULL)||(pplObjPrototypes==NULL)) ppl_fatal(&c->errcontext,__FILE__,__LINE__,"Out of memory.");
  pplObjMethodsInit(c);
  for (i=0; i<n; i++)
   {
    typeObjs[i].refCount = 1;
    typeObjs[i].id       = i;
    typeObjs[i].methods  = pplObjMethods[i];
    pplObjType(&pplObjPrototypes[i],1,1,&typeObjs[i]);
   }
  initialised=1;
  return;
 }

pplObj *pplObjNum(pplObj *in, unsigned char amMalloced, double real, double imag)
 {
  int i;
  in->objType = PPLOBJ_NUM;
  in->real = real;
  in->imag = imag;
  in->dimensionless = 1;
  in->flagComplex = in->tempType = 0;
  in->objPrototype = &pplObjPrototypes[PPLOBJ_NUM];
  in->self_lval = NULL; in->self_dval = NULL;
  in->self_this = NULL;
  in->amMalloced = amMalloced;
  in->immutable = 0;
  for (i=0; i<UNITS_MAX_BASEUNITS; i++) in->exponent[i]=0;
  return in;
 }

pplObj *pplObjStr(pplObj *in, unsigned char amMalloced, unsigned char auxilMalloced, char *str)
 {
  in->objType = PPLOBJ_ZOM; // In case we get interrupted
  in->auxil = (void *)str;
  in->auxilMalloced = auxilMalloced;
  in->auxilLen = strlen(str)+1;
  in->objPrototype = &pplObjPrototypes[PPLOBJ_STR];
  in->self_lval = NULL; in->self_dval = NULL;
  in->self_this = NULL;
  in->amMalloced = amMalloced;
  in->immutable = 0;
  in->objType = PPLOBJ_STR;
  return in;
 }

pplObj *pplObjBool(pplObj *in, unsigned char amMalloced, int stat)
 {
  in->objType = PPLOBJ_BOOL;
  in->real = stat;
  in->auxil = NULL;
  in->objPrototype = &pplObjPrototypes[PPLOBJ_BOOL];
  in->self_lval = NULL; in->self_dval = NULL;
  in->self_this = NULL;
  in->auxilMalloced = 0;
  in->auxilLen = 0;
  in->amMalloced = amMalloced;
  return in;
 }

pplObj *pplObjDate(pplObj *in, unsigned char amMalloced, double unixTime)
 {
  in->objType = PPLOBJ_DATE;
  in->real = unixTime;
  in->auxil = NULL;
  in->objPrototype = &pplObjPrototypes[PPLOBJ_DATE];
  in->self_lval = NULL; in->self_dval = NULL;
  in->self_this = NULL;
  in->auxilMalloced = 0;
  in->auxilLen = 0;
  in->amMalloced = amMalloced;
  return in;
 }

pplObj *pplObjColor(pplObj *in, unsigned char amMalloced, int scheme, double c1, double c2, double c3, double c4)
 {
  if (c1<0) c1=0; if (c1>1) c1=1;
  if (c2<0) c2=0; if (c2>1) c2=1;
  if (c3<0) c3=0; if (c3>1) c3=1;
  if (c4<0) c4=0; if (c4>1) c4=1;
  in->objType = PPLOBJ_COL;
  in->auxil = NULL;
  in->objPrototype = &pplObjPrototypes[PPLOBJ_COL];
  in->self_lval = NULL; in->self_dval = NULL;
  in->self_this = NULL;
  in->auxilMalloced = 0;
  in->auxilLen = 0; 
  in->amMalloced = amMalloced;
  in->exponent[ 0] = scheme;
  in->exponent[ 8] = c1;
  in->exponent[ 9] = c2;
  in->exponent[10] = c3;
  in->exponent[11] = c4;
  return in;
 }

pplObj *pplObjDict(pplObj *in, unsigned char amMalloced, unsigned char auxilMalloced, dict *d)
 {
  in->objType = PPLOBJ_ZOM;
  if (d==NULL) in->auxil = (void *)ppl_dictInit(HASHSIZE_LARGE,auxilMalloced);
  else         in->auxil = (void *)d;
  if (in->auxil==NULL) return NULL;
  in->auxilMalloced = auxilMalloced;
  in->auxilLen = 0;
  in->objPrototype = &pplObjPrototypes[PPLOBJ_DICT];
  in->self_lval = NULL; in->self_dval = NULL;
  in->self_this = NULL;
  in->amMalloced = amMalloced;
  in->immutable = 0;
  in->objType = PPLOBJ_DICT;
  return in;
 }

pplObj *pplObjModule(pplObj *in, unsigned char amMalloced, unsigned char auxilMalloced, unsigned char frozen)
 {
  in->objType = PPLOBJ_ZOM;
  in->auxil = (void *)ppl_dictInit(HASHSIZE_LARGE,auxilMalloced);
  ((dict *)in->auxil)->immutable = frozen;
  if (in->auxil==NULL) return NULL;
  in->auxilMalloced = auxilMalloced;
  in->auxilLen = 0;
  in->objPrototype = &pplObjPrototypes[PPLOBJ_MOD];
  in->self_lval = NULL; in->self_dval = NULL;
  in->self_this = NULL;
  in->amMalloced = amMalloced;
  in->immutable = frozen;
  in->objType = PPLOBJ_MOD;
  return in;
 }

pplObj *pplObjList(pplObj *in, unsigned char amMalloced, unsigned char auxilMalloced, list *l)
 {
  in->objType = PPLOBJ_ZOM;
  if (l==NULL) in->auxil = (void *)ppl_listInit(auxilMalloced);
  else         in->auxil = (void *)l;
  if (in->auxil==NULL) return NULL;
  in->auxilMalloced = auxilMalloced;
  in->auxilLen = 0;
  in->objPrototype = &pplObjPrototypes[PPLOBJ_LIST];
  in->self_lval = NULL; in->self_dval = NULL;
  in->self_this = NULL;
  in->amMalloced = amMalloced;
  in->immutable = 0;
  in->objType = PPLOBJ_LIST;
  return in;
 }

pplObj *pplObjVector(pplObj *in, unsigned char amMalloced, unsigned char auxilMalloced, int size)
 {
  int i;
  pplVector *pv;
  pplVectorRaw *pvr;
  in->objType = PPLOBJ_ZOM;
  if (auxilMalloced) pv = (pplVector *)(in->auxil = malloc      (sizeof(pplVector)));
  else               pv = (pplVector *)(in->auxil = ppl_memAlloc(sizeof(pplVector)));
  if (pv==NULL) return NULL;
  if (auxilMalloced) pvr = pv->raw = (pplVectorRaw *)malloc      (sizeof(pplVectorRaw));
  else               pvr = pv->raw = (pplVectorRaw *)ppl_memAlloc(sizeof(pplVectorRaw));
  if (pvr==NULL) { if (auxilMalloced) free(pv); return NULL; }
  if ((pvr->v = gsl_vector_calloc(size))==NULL) { if (auxilMalloced) { free(pv); free(pvr); } return NULL; }
  pvr->refCount  = 1;
  pv ->refCount  = 1;
  pv ->view      = NULL;
  pv ->v         = pvr->v;
  in->auxilMalloced = auxilMalloced;
  in->auxilLen = sizeof(pplMatrix);
  in->objPrototype = &pplObjPrototypes[PPLOBJ_VEC];
  in->self_lval = NULL; in->self_dval = NULL;
  in->self_this = NULL;
  in->amMalloced = amMalloced;
  in->immutable = 0;
  in->dimensionless = 1;
  for (i=0; i<UNITS_MAX_BASEUNITS; i++) in->exponent[i]=0;
  in->objType = PPLOBJ_VEC;
  return in;
 }

pplObj *pplObjMatrix(pplObj *in, unsigned char amMalloced, unsigned char auxilMalloced, int size1, int size2)
 {
  int i;
  pplMatrix *pm;
  pplMatrixRaw *pmr;
  in->objType = PPLOBJ_ZOM;
  if (auxilMalloced) pm = (pplMatrix *)(in->auxil = malloc      (sizeof(pplMatrix)));
  else               pm = (pplMatrix *)(in->auxil = ppl_memAlloc(sizeof(pplMatrix)));
  if (pm==NULL) return NULL;
  if (auxilMalloced) pmr = pm->raw = (pplMatrixRaw *)malloc      (sizeof(pplMatrixRaw));
  else               pmr = pm->raw = (pplMatrixRaw *)ppl_memAlloc(sizeof(pplMatrixRaw));
  if (pmr==NULL) { if (auxilMalloced) free(pm); return NULL; }
  if ((pmr->m = gsl_matrix_calloc(size1 , size2))==NULL) { if (auxilMalloced) { free(pm); free(pmr); } return NULL; }
  pmr->refCount  = 1;
  pm ->refCount  = 1;
  pm ->sliceNext = 0;
  pm ->view      = NULL;                     
  pm ->m         = pmr->m;
  in->auxilMalloced = auxilMalloced;
  in->auxilLen = sizeof(pplMatrix);
  in->objPrototype = &pplObjPrototypes[PPLOBJ_MAT];
  in->self_lval = NULL; in->self_dval = NULL;
  in->self_this = NULL;
  in->amMalloced = amMalloced;
  in->immutable = 0;
  in->dimensionless = 1;
  for (i=0; i<UNITS_MAX_BASEUNITS; i++) in->exponent[i]=0;
  in->objType = PPLOBJ_MAT;
  return in;
 }

pplObj *pplObjFile(pplObj *in, unsigned char amMalloced, unsigned char auxilMalloced, FILE *f, int pipe)
 {
  in->objType = PPLOBJ_ZOM;
  if (auxilMalloced) in->auxil = (void *)malloc      (sizeof(pplFile));
  else               in->auxil = (void *)ppl_memAlloc(sizeof(pplFile));
  if (in->auxil==NULL) return NULL;
  ((pplFile *)in->auxil)->refCount = 1;
  ((pplFile *)in->auxil)->file     = f;
  ((pplFile *)in->auxil)->open     = 1;
  ((pplFile *)in->auxil)->pipe     = pipe;
  in->auxilMalloced = auxilMalloced;
  in->auxilLen = sizeof(pplFile);
  in->objPrototype = &pplObjPrototypes[PPLOBJ_FILE];
  in->self_lval = NULL; in->self_dval = NULL;
  in->self_this = NULL;
  in->amMalloced = amMalloced;
  in->immutable = 0;
  in->objType = PPLOBJ_FILE;
  return in;
 }

pplObj *pplObjFunc(pplObj *in, unsigned char amMalloced, unsigned char auxilMalloced, pplFunc *f)
 {
  in->objType       = PPLOBJ_ZOM;
  in->auxilLen      = sizeof(pplFunc);
  in->auxil         = (void *)f;
  in->auxilMalloced = auxilMalloced;
  in->objPrototype = &pplObjPrototypes[PPLOBJ_FUNC];
  in->self_lval = NULL; in->self_dval = NULL;
  in->self_this = NULL;
  in->amMalloced = amMalloced;
  in->immutable = 0;
  in->objType       = PPLOBJ_FUNC;
  return in;
 }

pplObj *pplObjType(pplObj *in, unsigned char amMalloced, unsigned char auxilMalloced, pplType *t)
 {
  in->objType       = PPLOBJ_ZOM;
  in->auxilLen      = sizeof(pplType);
  in->auxil         = (void *)t;
  in->auxilMalloced = auxilMalloced;
  in->objPrototype = &pplObjPrototypes[PPLOBJ_TYPE];
  in->self_lval = NULL; in->self_dval = NULL;
  in->self_this = NULL;
  in->amMalloced = amMalloced;
  in->immutable = 1; // Types are always immutable
  in->objType       = PPLOBJ_TYPE;
  return in;
 }

pplObj *pplObjNull(pplObj *in, unsigned char amMalloced)
 {
  in->objType      = PPLOBJ_NULL;
  in->objPrototype = &pplObjPrototypes[PPLOBJ_NULL];
  in->amMalloced   = amMalloced;
  in->immutable    = 0;
  return in;
 }

pplObj *pplObjException(pplObj *in, unsigned char amMalloced, unsigned char auxilMalloced, char *str)
 {
  in->objType      = PPLOBJ_ZOM;
  in->auxil = (void *)str;
  in->auxilMalloced = auxilMalloced;
  in->auxilLen = strlen(str+1);
  in->objPrototype = &pplObjPrototypes[PPLOBJ_EXC];
  in->self_lval = NULL; in->self_dval = NULL;
  in->self_this = NULL;
  in->amMalloced = amMalloced;
  in->immutable = 0;
  in->objType = PPLOBJ_EXC;
  return in;
 }

pplObj *pplObjGlobal(pplObj *in, unsigned char amMalloced)
 {
  in->objType      = PPLOBJ_GLOB;
  in->objPrototype = &pplObjPrototypes[PPLOBJ_GLOB];
  in->amMalloced   = amMalloced;
  in->immutable    = 0;
  return in;
 }

pplObj *pplObjZom(pplObj *in, unsigned char amMalloced)
 {
  in->objType      = PPLOBJ_ZOM;
  in->objPrototype = &pplObjPrototypes[PPLOBJ_ZOM];
  in->amMalloced   = amMalloced;
  in->immutable    = 0;
  return in;
 }

pplObj *pplObjUser(pplObj *in, unsigned char amMalloced, unsigned char auxilMalloced, pplObj *prototype)
 {
  in->objType = PPLOBJ_ZOM;
  in->auxil = (void *)ppl_dictInit(HASHSIZE_LARGE,auxilMalloced);
  if (in->auxil==NULL) return NULL;
  in->auxilMalloced = auxilMalloced;
  in->auxilLen = 0;
  in->objPrototype = (pplObj *)malloc(sizeof(pplObj));
  if (in->objPrototype==NULL) return NULL;
  pplObjCpy(in->objPrototype, prototype, 1, 1);
  in->self_lval = NULL; in->self_dval = NULL;
  in->self_this = NULL;
  in->amMalloced = amMalloced;
  in->immutable = 0;
  in->objType = PPLOBJ_USER;
  return in;
 }

pplObj *pplObjCpy(pplObj *out, pplObj *in, unsigned char outMalloced, unsigned char useMalloc)
 {
  if (in==NULL) return NULL;

  if (out==NULL)
   {
    if (outMalloced) out = (pplObj *)malloc      (sizeof(pplObj));
    else             out = (pplObj *)ppl_memAlloc(sizeof(pplObj));
    if (out==NULL) return out;
   }

  memcpy(out, in, sizeof(pplObj));
  out->self_lval  = in;
  out->self_dval  = NULL;
  out->self_this  = NULL;
  out->amMalloced = outMalloced;

  switch(in->objType)
   {
    case PPLOBJ_STR:
    case PPLOBJ_EXC: // copy string value
      if (useMalloc) out->auxil = (void *)malloc      (in->auxilLen);
      else           out->auxil = (void *)ppl_memAlloc(in->auxilLen);
      if (out->auxil==NULL) { out->objType=PPLOBJ_ZOM; return NULL; }
      memcpy(out->auxil, in->auxil, in->auxilLen);
      out->auxilMalloced = useMalloc;
      break;
    case PPLOBJ_USER:
     {
      pplObj *p = (pplObj *)malloc(sizeof(pplObj)); // Copy prototype pointer object
      if (p==NULL) { out->objType=PPLOBJ_ZOM; return NULL; }
      pplObjCpy(p,out->objPrototype,1,1);
      out->objPrototype=p;
     }
    case PPLOBJ_DICT: // dictionary -- pass by pointer
    case PPLOBJ_MOD:
      ((dict *)(out->auxil))->refCount++;
      break;
    case PPLOBJ_LIST: // list
      ((list *)(out->auxil))->refCount++;
      break;
    case PPLOBJ_VEC: // vector
     {
      pplVector *pv = (pplVector *)out->auxil;
      pv->refCount++;
      pv->raw->refCount++;
      break;
     }
    case PPLOBJ_MAT: // matrix
     {
      pplMatrix *pm = (pplMatrix *)out->auxil;
      pm->refCount++;
      pm->raw->refCount++;
      break;
     }
    case PPLOBJ_FILE: // file handle
      ((pplFile *)(out->auxil))->refCount++; 
      break;
    case PPLOBJ_FUNC: // function pointer
      ((pplFunc *)(out->auxil))->refCount++;
      break;
    case PPLOBJ_TYPE: // data type
      ((pplType *)(out->auxil))->refCount++;
      break;
   }
  return out;
 }

pplObj *pplObjDeepCpy(pplObj *out, pplObj *in, int deep, unsigned char outMalloced, unsigned char useMalloc)
 {
  int t=in->objType;

  if (in==NULL) return NULL;

  if (out==NULL)
   {
    if (outMalloced) out = (pplObj *)malloc      (sizeof(pplObj));
    else             out = (pplObj *)ppl_memAlloc(sizeof(pplObj));
    if (out==NULL) return out;
   }

  switch (t)
   {
    case PPLOBJ_DICT:
    case PPLOBJ_MOD:
    case PPLOBJ_USER:
     {
      pplObj       *item, v;
      char         *key;
      dict         *d;
      dictIterator *di = ppl_dictIterateInit((dict *)in->auxil);
      memcpy(out, in, sizeof(pplObj));
      out->objType    = PPLOBJ_ZOM;
      out->self_lval  = NULL;
      out->self_dval  = NULL;
      out->self_this  = NULL;
      out->amMalloced = outMalloced;
      out->auxil      = (void *)(d = ppl_dictInit(HASHSIZE_LARGE,useMalloc));
      if (out->auxil==NULL) return NULL;
      while ((item = (pplObj *)ppl_dictIterate(&di,&key))!=NULL)
       {
        if (!deep) pplObjCpy(&v,item,useMalloc,useMalloc);
        else       pplObjDeepCpy(&v,item,1,useMalloc,useMalloc);
        ppl_dictAppendCpy(d,key,&v,sizeof(pplObj));
       }
      out->objType = t;
      return out;
     }
    case PPLOBJ_LIST:
     {
      pplObj       *item, v;
      list         *l;
      listIterator *li = ppl_listIterateInit((list *)in->auxil);
      memcpy(out, in, sizeof(pplObj));
      out->objType    = PPLOBJ_ZOM;
      out->self_lval  = NULL;
      out->self_dval  = NULL;
      out->self_this  = NULL;
      out->amMalloced = outMalloced;
      out->auxil      = (void *)(l = ppl_listInit(useMalloc));
      if (out->auxil==NULL) return NULL; 
      while ((item = (pplObj *)ppl_listIterate(&li))!=NULL)
       {
        if (!deep) pplObjCpy(&v,item,useMalloc,useMalloc);
        else       pplObjDeepCpy(&v,item,1,useMalloc,useMalloc);
        ppl_listAppendCpy(l,&v,sizeof(pplObj));
       }
      out->objType = t;
      return out;
     }
    default:
     return pplObjCpy(out,in,outMalloced,useMalloc);
   }
 }

