// expEvalSlice.c
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

#include <stdlib.h>
#include <string.h>
#include <limits.h>

#include <gsl/gsl_math.h>

#include "expressions/expEvalSlice.h"
#include "userspace/context.h"
#include "userspace/garbageCollector.h"
#include "userspace/pplObj_fns.h"
#include "userspace/unitsArithmetic.h"
#include "userspace/unitsDisp.h"

#include "pplConstants.h"

#define STACK_POP \
   { \
    context->stackPtr--; \
    ppl_garbageObject(&context->stack[context->stackPtr]); \
    if (context->stack[context->stackPtr].refCount != 0) { *status=1; *errType=ERR_INTERNAL; strcpy(errText,"Stack forward reference detected."); goto fail; } \
   } \

void ppl_sliceItem (ppl_context *context, int getPtr, int *status, int *errType, char *errText)
 {
  const int nArgs=1;
  pplObj  *out  = &context->stack[context->stackPtr-1-nArgs];
  pplObj  *args = context->stack+context->stackPtr-nArgs;
  pplObj   called;
  int      i;
  int      t = out->objType;
  if (context->stackPtr<nArgs+1) { *status=1; *errType=ERR_INTERNAL; sprintf(errText,"Attempt to slice object with few items on the stack."); return; }
  memcpy(&called, out, sizeof(pplObj));
  pplObjNum(out, 0, 0, 0); // Overwrite sliced object on stack, but don't garbage collect it yet
  out->refCount = 1;

  if ((t==PPLOBJ_DICT) || (t==PPLOBJ_MOD) || (t==PPLOBJ_USER))
   {
    dict   *din = (dict *)called.auxil;
    char   *key;
    pplObj *obj;
    if (args->objType!=PPLOBJ_STR) { *errType=ERR_TYPE; sprintf(errText,"Dictionary keys must be strings; supplied key has type <%s>.",pplObjTypeNames[args->objType]); goto fail; }
    key = (char *)args[0].auxil;
    if (!ppl_dictContains(din,key))
     {
      if (getPtr && (!din->immutable))
       {
        pplObj v;
        v.refCount=1;
        pplObjNull(&v,1);
        ppl_dictAppendCpy(din, key, &v, sizeof(pplObj));
       }
      else
       {
        *status=1; *errType=ERR_DICTKEY; sprintf(errText,"Undefined dictionary key '%s'.", key); goto fail;
       }
     }
    obj = (pplObj *)ppl_dictLookup(din,key);
    pplObjCpy(out, obj, 1, 0, 1);
    out->immutable = out->immutable || din->immutable;
    goto cleanup;
   }

  if (args->objType!=PPLOBJ_NUM) { *errType=ERR_TYPE; sprintf(errText,"Item numbers when slicing must be numerical values; supplied index has type <%s>.",pplObjTypeNames[args->objType]); goto fail; }
  if (!args->dimensionless) { *errType=ERR_NUMERICAL; sprintf(errText,"Item numbers when slicing must be dimensionless numbers; supplied index has units of <%s>.", ppl_printUnit(context, args, NULL, NULL, 0, 1, 0) ); goto fail; }
  if (args->flagComplex) { *errType=ERR_NUMERICAL; sprintf(errText,"Item numbers when slicing must be real numbers; supplied index is complex."); goto fail; }
  if ( (!gsl_finite(args->real)) || (args->real<INT_MIN) || (args->real>INT_MAX) ) { *errType=ERR_NUMERICAL; sprintf(errText,"Item numbers when slicing must be in the range %d to %d.", INT_MIN, INT_MAX); goto fail; }

  switch (t)
   {
    case PPLOBJ_STR:
     {
      const char *in  = (char *)called.auxil;
      const int   inl = strlen(in);
      char *outstr;
      int p = args[0].real;
      if (p<0) p+=inl;
      if ((p<0)||(p>=inl)) { *status=1; *errType=ERR_RANGE; sprintf(errText,"String index out of range."); goto fail; }
      if (getPtr) { *status=1; *errType=ERR_TYPE; sprintf(errText,"Cannot assign to a character in a string."); goto fail; }
      if ((outstr = (char *)malloc(2))==NULL) { *status=1; *errType=ERR_MEMORY; sprintf(errText,"Out of memory."); goto fail; }
      outstr[0] = in[p];
      outstr[1] = '\0';
      pplObjStr(out, 0, 1, outstr);
      break;
     }
    case PPLOBJ_LIST:
     {
      list *lin = (list *)called.auxil;
      const int   linl = lin->length;
      int         p    = args[0].real;
      pplObj     *obj;
      if (p<0) p+=linl;
      if (getPtr && (p==linl) && (!lin->immutable))
       {
        pplObj v;
        v.refCount=1;
        pplObjNull(&v,1);
        ppl_listAppendCpy(lin, &v, sizeof(pplObj));
       }
      else
       {
        if ((p<0)||(p>=linl)) { *status=1; *errType=ERR_RANGE; sprintf(errText,"List index out of range."); goto fail; }
       }
      obj = (pplObj *)ppl_listGetItem(lin,p);
      pplObjCpy(out, obj, 1, 0, 1);
      out->immutable = out->immutable || lin->immutable;
      break;
     }
    case PPLOBJ_VEC:
     {
      gsl_vector *vin = ((pplVector *)called.auxil)->v;
      const int   vinl = vin->size;
      int         p    = args[0].real;
      double     *obj;
      if (p<0) p+=vinl;
      if ((p<0)||(p>=vinl)) { *status=1; *errType=ERR_RANGE; sprintf(errText,"Vector index out of range."); goto fail; }
      obj = gsl_vector_ptr(vin, p);
      pplObjNum(out, 0, *obj, 0);
      ppl_unitsDimCpy(out,&called);
      if ((out->self_lval = called.self_lval)!=NULL) { __sync_add_and_fetch(&out->self_lval->refCount,1); }
      out->self_dval = obj;
      out->immutable = out->immutable || called.immutable;
      break;
     }
    case PPLOBJ_MAT:
     {
      pplMatrix  *mob  = (pplMatrix *)called.auxil;
      gsl_matrix *min  = mob->m;
      const int   minl = mob->sliceNext ? min->size2 : min->size1;
      int         p    = args[0].real;
      pplVector  *vo;
      if (p<0) p+=minl;
      if ((p<0)||(p>=minl)) { *status=1; *errType=ERR_RANGE; sprintf(errText,"Matrix index out of range."); goto fail; }
      vo = (pplVector *)malloc(sizeof(pplVector));
      if (vo==NULL) { *status=1; *errType=ERR_MEMORY; sprintf(errText,"Out of memory."); goto fail; }
      vo->refCount = 1;
      vo->raw  = NULL;
      vo->rawm = mob->raw; __sync_add_and_fetch(&mob->raw->refCount,1);
      if (!mob->sliceNext) vo->view = gsl_matrix_column(min, p);
      else                 vo->view = gsl_matrix_row   (min, p);
      vo->v = &vo->view.vector;
      out->objType = PPLOBJ_ZOM;
      out->refCount=1;
      out->auxil = (void*)vo;
      out->auxilMalloced = 1;
      out->auxilLen = sizeof(pplVector);
      out->objPrototype = &pplObjPrototypes[PPLOBJ_VEC];
      if ((out->self_lval = called.self_lval)!=NULL) { __sync_add_and_fetch(&out->self_lval->refCount,1); }
      out->self_dval = NULL;
      out->self_this = NULL;
      out->immutable = 0;
      ppl_unitsDimCpy(out,&called);
      out->objType   = PPLOBJ_VEC;
      out->immutable = called.immutable;
      break;
     }
    default:
      *status=1; *errType=ERR_TYPE;
      sprintf(errText,"Attempt to slice an object of type <%s>, which cannot be sliced.",pplObjTypeNames[t]);
      goto fail;
   }

cleanup:
  for (i=0; i<nArgs; i++) { STACK_POP; }
  ppl_garbageObject(&called);
  return;
fail:
  *status=1;
  goto cleanup;
 }

void ppl_sliceRange(ppl_context *context, int minset, int min, int maxset, int max, int *status, int *errType, char *errText)
 {
  const int nArgs = minset + maxset;
  pplObj  *out  = &context->stack[context->stackPtr-1-nArgs];
  pplObj   called;
  int      i;
  int      t = out->objType;
  if (context->stackPtr<nArgs+1) { *status=1; *errType=ERR_INTERNAL; sprintf(errText,"Attempt to slice object with few items on the stack."); return; }
  memcpy(&called, out, sizeof(pplObj));
  pplObjNum(out, 0, 0, 0); // Overwrite function object on stack, but don't garbage collect it yet
  out->refCount = 1;

  switch (t)
   {
    case PPLOBJ_STR:
     {
      const char *in  = (char *)called.auxil;
      const int   inl = strlen(in);
      int   outlen;
      char *outstr;
      if (!minset)    min =0;
      else if (min<0) min+=inl;
      if (!maxset)    max =inl;
      else if (max<0) max+=inl;
      if ((min<0)||(min>inl)||(max<0)||(max>inl)) { *status=1; *errType=ERR_RANGE; sprintf(errText,"String index out of range."); goto fail; }
      outlen = max-min;
      if (outlen<0) outlen=0;
      if ((outstr = (char *)malloc(outlen+1))==NULL) { *status=1; *errType=ERR_MEMORY; sprintf(errText,"Out of memory."); goto fail; }
      strncpy(outstr,in+min,outlen);
      outstr[outlen]='\0';
      pplObjStr(out, 0, 1, outstr);
      break;
     }
    case PPLOBJ_LIST:
     {
      list         *lin = (list *)called.auxil;
      list         *lout;
      const int     inl = lin->length;
      int           i;
      listIterator *li;
      pplObj        obj, *objptr;
      if (!minset)    min =0;
      else if (min<0) min+=inl;
      if (!maxset)    max =inl;
      else if (max<0) max+=inl;
      if ((min<0)||(min>inl)||(max<0)||(max>inl)) { *status=1; *errType=ERR_RANGE; sprintf(errText,"List index out of range."); goto fail; }
      if (pplObjList(out,0,1,NULL)==NULL) { *status=1; *errType=ERR_MEMORY; sprintf(errText,"Out of memory."); goto fail; }
      lout = (list *)out->auxil;
      obj.refCount=1;
      li = ppl_listIterateInit(lin);
      for (i=0; ((objptr=(pplObj*)ppl_listIterate(&li))!=NULL); i++)
       if ((i>=min)&&(i<max))
        {
         pplObjCpy(&obj,objptr,0,1,1);
         ppl_listAppendCpy(lout, &obj, sizeof(pplObj));
        }
      break;
     }
    case PPLOBJ_VEC:
     {
      pplVector  *vob   = (pplVector *)called.auxil;
      gsl_vector *vecin = vob->v;
      const int   vinl  = vecin->size;
      pplVector  *vo;
      if (!minset)    min =0;
      else if (min<0) min+=vinl;
      if (!maxset)    max =vinl;
      else if (max<0) max+=vinl;
      if ((min<0)||(min>vinl)||(max<0)||(max>vinl)) { *status=1; *errType=ERR_RANGE; sprintf(errText,"Vector index out of range."); goto fail; }
      if (max<min+1) { *status=1; *errType=ERR_RANGE; sprintf(errText,"Cannot create a vector of zero size."); goto fail; }
      vo = (pplVector *)malloc(sizeof(pplVector));
      if (vo==NULL) { *status=1; *errType=ERR_MEMORY; sprintf(errText,"Out of memory."); goto fail; }
      vo->refCount = 1;
      if ((vo->raw  = vob->raw )!=NULL) { __sync_add_and_fetch(&vob->raw ->refCount,1); }
      if ((vo->rawm = vob->rawm)!=NULL) { __sync_add_and_fetch(&vob->rawm->refCount,1); }
      vo->view = gsl_vector_subvector(vecin, min, max-min);
      vo->v = &vo->view.vector;
      out->objType = PPLOBJ_ZOM;
      out->refCount=1;
      out->auxil = (void*)vo;
      out->auxilMalloced = 1;
      out->auxilLen = sizeof(pplVector);
      out->objPrototype = &pplObjPrototypes[PPLOBJ_VEC];
      out->self_lval = NULL;
      out->self_dval = NULL;
      out->self_this = NULL;
      out->immutable = 0;
      ppl_unitsDimCpy(out,&called);
      out->objType   = PPLOBJ_VEC;
      out->immutable = called.immutable;
      break;
     }
    case PPLOBJ_MAT:
     {
      pplMatrix  *mob   = (pplMatrix *)called.auxil;
      gsl_matrix *matin = mob->m;
      const int   minl  = mob->sliceNext ? matin->size2 : matin->size1;
      pplMatrix  *mo;
      if (!minset)    min =0;
      else if (min<0) min+=minl;
      if (!maxset)    max =minl;
      else if (max<0) max+=minl;
      if ((min<0)||(min>minl)||(max<0)||(max>minl)) { *status=1; *errType=ERR_RANGE; sprintf(errText,"Matrix index out of range."); goto fail; }
      if (max<min+1) { *status=1; *errType=ERR_RANGE; sprintf(errText,"Cannot create a matrix of zero size."); goto fail; }
      mo = (pplMatrix *)malloc(sizeof(pplMatrix));
      if (mo==NULL) { *status=1; *errType=ERR_MEMORY; sprintf(errText,"Out of memory."); goto fail; }
      mo->refCount = 1;
      mo->raw = mob->raw; __sync_add_and_fetch(&mob->raw->refCount,1);
      if (mob->sliceNext) mo->view = gsl_matrix_submatrix(matin, min,  0 , max-min      , matin->size2);
      else                mo->view = gsl_matrix_submatrix(matin,  0 , min, matin->size1 , max-min     );
      mo->sliceNext = !mob->sliceNext;
      mo->m = &mo->view.matrix;
      out->objType = PPLOBJ_ZOM;
      out->refCount=1;
      out->auxil = (void*)mo;
      out->auxilMalloced = 1;
      out->auxilLen = sizeof(pplMatrix);
      out->objPrototype = &pplObjPrototypes[PPLOBJ_MAT];
      out->self_lval = NULL;
      out->self_dval = NULL;
      out->self_this = NULL;
      out->immutable = 0;
      ppl_unitsDimCpy(out,&called);
      out->objType   = PPLOBJ_MAT;
      out->immutable = called.immutable;
      break;
     }
    default:
      *status=1; *errType=ERR_TYPE;
      sprintf(errText,"Attempt to slice an object of type <%s>, which cannot be sliced.",pplObjTypeNames[t]);
      goto fail;
   }

cleanup:
  for (i=0; i<nArgs; i++) { STACK_POP; }
  ppl_garbageObject(&called);
  return;
fail:
  *status=1;
  goto cleanup;
 }

