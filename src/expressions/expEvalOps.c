// expEvalOps.c
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

#include <gsl/gsl_blas.h>
#include <gsl/gsl_errno.h>
#include <gsl/gsl_math.h>
#include <gsl/gsl_matrix.h>
#include <gsl/gsl_vector.h>
#define GSL_RANGE_CHECK_OFF 1

#include "expressions/expEval.h"
#include "expressions/expEvalOps.h"
#include "settings/settingTypes.h"
#include "stringTools/asciidouble.h"
#include "userspace/context.h"
#include "userspace/garbageCollector.h"
#include "userspace/pplObj_fns.h"
#include "userspace/pplObjCmp.h"
#include "userspace/unitsArithmetic.h"
#include "userspace/unitsDisp.h"

#include "pplConstants.h"

#define CAST_TO_NUM2(X) \
 { \
  double d=0; \
  int t=(X)->objType; \
  int rc=(X)->refCount; \
  if (t!=PPLOBJ_NUM) \
   { \
    switch (t) \
     { \
      case PPLOBJ_BOOL: d = ((X)->real!=0); break; \
      case PPLOBJ_STR : \
       { \
        int len; char *c=(char *)(X)->auxil; \
        d = ppl_getFloat(c, &len); \
        if (len>0) { while ((c[len]>'\0')&&(c[len]<=' ')) len++; } \
        if ((len<0)||(c[len]!='\0')) { *status=1; *errType=ERR_TYPE; sprintf(errText,"Attempt to implicitly cast string to number failed: string is not a valid number."); goto cast_fail; } \
        break; \
       } \
      default: \
        { *status=1; *errType=ERR_TYPE; sprintf(errText,"Cannot implicitly cast an object of type <%s> to a number.",pplObjTypeNames[t]); goto cast_fail; } \
     } \
    ppl_garbageObject(X); \
    pplObjNum(X,0,d,0); \
    (X)->refCount=rc; \
   } \
 }

void ppl_opAdd(ppl_context *context, pplObj *a, pplObj *b, pplObj *o, int invertible, int *status, int *errType, char *errText)
 {
  int t1 = a->objType;
  int t2 = b->objType;

  if ((t1==PPLOBJ_NUM)&&(t2==PPLOBJ_NUM))
   {
    goto am_numeric;
   }
  else if ((t1==PPLOBJ_STR)&&(t2==PPLOBJ_STR)) // adding strings: concatenate
   {
    char *tmp;
    int   l1 = strlen((char *)a->auxil);
    int   l2 = strlen((char *)b->auxil);
    int   l  = l1+l2+1;
    tmp = (char *)malloc(l);
    if (tmp==NULL) { *status=1; *errType=ERR_MEMORY; sprintf(errText,"Out of memory."); return; }
    strcpy(tmp    , (char *)a->auxil);
    strcpy(tmp+l1 , (char *)b->auxil);
    pplObjStr(o,0,1,tmp);
   }
  else if ((t1==PPLOBJ_COL)&&(t2==PPLOBJ_COL)) // adding colors
   {
    double r1,g1,b1,r2,g2,b2;
    if      (round(a->exponent[0])==SW_COLSPACE_RGB ) { r1=a->exponent[8]; g1=a->exponent[9]; b1=a->exponent[10]; }
    else if (round(a->exponent[0])==SW_COLSPACE_CMYK) pplcol_CMYKtoRGB(a->exponent[8],a->exponent[9],a->exponent[10],a->exponent[11],&r1,&g1,&b1);
    else                                              pplcol_HSBtoRGB (a->exponent[8],a->exponent[9],a->exponent[10],&r1,&g1,&b1);
    if      (round(b->exponent[0])==SW_COLSPACE_RGB ) { r2=b->exponent[8]; g2=b->exponent[9]; b2=b->exponent[10]; }
    else if (round(b->exponent[0])==SW_COLSPACE_CMYK) pplcol_CMYKtoRGB(b->exponent[8],b->exponent[9],b->exponent[10],b->exponent[11],&r2,&g2,&b2);
    else                                              pplcol_HSBtoRGB (b->exponent[8],b->exponent[9],b->exponent[10],&r2,&g2,&b2);
    r1+=r2; g1+=g2; b1+=b2;
    pplObjColor(o,0,SW_COLSPACE_RGB,r1,g1,b1,0);
   }
  else if ( ((t1==PPLOBJ_DATE)&&(t2==PPLOBJ_NUM)) || ((t1==PPLOBJ_NUM)&&(t2==PPLOBJ_DATE)) ) // adding time interval to date
   {
    int i;
    pplObj *num = (t1==PPLOBJ_NUM) ? a : b;
    for (i=0; i<UNITS_MAX_BASEUNITS; i++) if (num->exponent[i] != (i==UNIT_TIME))
     {
      *status=1; *errType=ERR_UNIT;
      sprintf(errText, "Can only add quantities with units of time to dates. Attempt to add a quantity with units of <%s>.", ppl_printUnit(context,num,NULL,NULL,1,1,0));
      return;
     }
    pplObjDate(o,0,a->real+b->real);
   }
  else if ((t1==PPLOBJ_VEC)&&(t2==PPLOBJ_VEC)) // adding vectors
   {
    int i;
    gsl_vector *v1 = ((pplVector *)(a->auxil))->v;
    gsl_vector *v2 = ((pplVector *)(b->auxil))->v;
    gsl_vector *vo;
    if (!ppl_unitsDimEqual(a, b))
     {
      if (a->dimensionless)
       { sprintf(errText, "Attempt to add quantities with conflicting dimensions: left operand is dimensionless, while right operand has units of <%s>.", ppl_printUnit(context, b, NULL, NULL, 1, 1, 0) ); }
      else if (b->dimensionless)
       { sprintf(errText, "Attempt to add quantities with conflicting dimensions: left operand has units of <%s>, while right operand is dimensionless.", ppl_printUnit(context, a, NULL, NULL, 0, 1, 0) ); }
      else
       { sprintf(errText, "Attempt to add quantities with conflicting dimensions: left operand has units of <%s>, while right operand has units of <%s>.", ppl_printUnit(context, a, NULL, NULL, 0, 1, 0), ppl_printUnit(context, b, NULL, NULL, 1, 1, 0) ); }
      *errType=ERR_UNIT; *status = 1; return;
     }
    if (v1->size != v2->size)
     {
      sprintf(errText, "Can only add vectors of a common size. Left operand has length of %ld, while right operand has length of %ld.", (long)v1->size, (long)v2->size);
      *errType=ERR_RANGE; *status = 1; return;
     }
    if (pplObjVector(o,0,1,v1->size)==NULL) { *status=1; *errType=ERR_MEMORY; sprintf(errText,"Out of memory."); return; }
    vo = ((pplVector *)(o->auxil))->v;
    for (i=0; i<v1->size; i++) gsl_vector_set(vo , i , gsl_vector_get(v1,i) + gsl_vector_get(v2,i));
    ppl_unitsDimCpy(o,a);
   }
  else if ((t1==PPLOBJ_LIST)&&(t2==PPLOBJ_LIST)) // adding lists
   {
    listIterator *li;
    pplObj       *item, buff;
    list         *l;
    if (pplObjList(o,0,1,NULL)==NULL) { *status=1; *errType=ERR_MEMORY; sprintf(errText,"Out of memory."); return; }
    l  = (list *)o->auxil;
    li = ppl_listIterateInit((list *)a->auxil);
    while ((item = (pplObj *)ppl_listIterate(&li))!=NULL) { pplObjCpy(&buff, item, 0, 1, 1); ppl_listAppendCpy(l, (void *)&buff, sizeof(pplObj) ); }
    li = ppl_listIterateInit((list *)b->auxil);
    while ((item = (pplObj *)ppl_listIterate(&li))!=NULL) { pplObjCpy(&buff, item, 0, 1, 1); ppl_listAppendCpy(l, (void *)&buff, sizeof(pplObj) ); }
   }
  else if ((t1==PPLOBJ_LIST)&&(t2==PPLOBJ_VEC)) // adding vector to list
   {
    int i;
    listIterator *li;
    pplObj       *item, buff;
    gsl_vector   *bv = ((pplVector *)(b->auxil))->v;
    list         *l;
    if (pplObjList(o,0,1,NULL)==NULL) { *status=1; *errType=ERR_MEMORY; sprintf(errText,"Out of memory."); return; }
    l  = (list *)o->auxil;
    li = ppl_listIterateInit((list *)a->auxil);
    while ((item = (pplObj *)ppl_listIterate(&li))!=NULL) { pplObjCpy(&buff, item, 0, 1, 1); ppl_listAppendCpy(l, (void *)&buff, sizeof(pplObj) ); }
    pplObjNum(&buff,1,0,0); ppl_unitsDimCpy(&buff,b);
    for (i=0; i<bv->size; i++) { buff.real=gsl_vector_get(bv,i); ppl_listAppendCpy(l, (void *)&buff, sizeof(pplObj) ); }
   }
  else if ((t1==PPLOBJ_VEC)&&(t2==PPLOBJ_LIST)) // adding list to vector
   {
    int i;
    listIterator *li;
    pplObj       *item, buff;
    gsl_vector   *av = ((pplVector *)(a->auxil))->v;
    list         *l;
    if (pplObjList(o,0,1,NULL)==NULL) { *status=1; *errType=ERR_MEMORY; sprintf(errText,"Out of memory."); return; }
    l  = (list *)o->auxil;
    pplObjNum(&buff,1,0,0); ppl_unitsDimCpy(&buff,a);
    for (i=0; i<av->size; i++) { buff.real=gsl_vector_get(av,i); ppl_listAppendCpy(l, (void *)&buff, sizeof(pplObj) ); }
    li = ppl_listIterateInit((list *)b->auxil);
    while ((item = (pplObj *)ppl_listIterate(&li))!=NULL) { pplObjCpy(&buff, item, 0, 1, 1); ppl_listAppendCpy(l, (void *)&buff, sizeof(pplObj) ); }
   }
  else if ( ((t1==PPLOBJ_DICT)&&(t2==PPLOBJ_DICT)) || // adding dictionaries
            ((t1==PPLOBJ_MOD )&&(t2==PPLOBJ_MOD )) )  // adding modules
   {
    dictIterator *di;
    pplObj       *item, buff;
    dict         *d;
    char         *key;
    if ( ((t1==PPLOBJ_DICT)?pplObjDict(o,0,1,NULL):pplObjModule(o,0,1,0)) == NULL )
       { *status=1; *errType=ERR_MEMORY; sprintf(errText,"Out of memory."); return; }
    d  = (dict *)o->auxil;
    di = ppl_dictIterateInit((dict *)a->auxil);
    while ((item = (pplObj *)ppl_dictIterate(&di,&key))!=NULL) { pplObjCpy(&buff, item, 0, 1, 1); ppl_dictAppendCpy(d, key, (void *)&buff, sizeof(pplObj) ); }
    di = ppl_dictIterateInit((dict *)b->auxil);
    while ((item = (pplObj *)ppl_dictIterate(&di,&key))!=NULL) { pplObjCpy(&buff, item, 0, 1, 1); ppl_dictAppendCpy(d, key, (void *)&buff, sizeof(pplObj) ); }
   }
  else if ((t1==PPLOBJ_MAT)&&(t2==PPLOBJ_MAT)) // adding matrices
   {
    int i,j;
    gsl_matrix *m1 = ((pplMatrix *)(a->auxil))->m;
    gsl_matrix *m2 = ((pplMatrix *)(b->auxil))->m;
    gsl_matrix *mo;
    if (!ppl_unitsDimEqual(a, b))
     {
      if (a->dimensionless)
       { sprintf(errText, "Attempt to add quantities with conflicting dimensions: left operand is dimensionless, while right operand has units of <%s>.", ppl_printUnit(context, b, NULL, NULL, 1, 1, 0) ); }
      else if (b->dimensionless)
       { sprintf(errText, "Attempt to add quantities with conflicting dimensions: left operand has units of <%s>, while right operand is dimensionless.", ppl_printUnit(context, a, NULL, NULL, 0, 1, 0) ); }
      else
       { sprintf(errText, "Attempt to add quantities with conflicting dimensions: left operand has units of <%s>, while right operand has units of <%s>.", ppl_printUnit(context, a, NULL, NULL, 0, 1, 0), ppl_printUnit(context, b, NULL, NULL, 1, 1, 0) ); }
      *errType=ERR_UNIT; *status = 1; return;
     }
    if ( (m1->size1 != m2->size1) || (m1->size2 != m2->size2) )
     {
      sprintf(errText, "Can only add matrices of a common size. Left operand has size of %ldx%ld, while right operand has size of %ldx%ld.", (long)m1->size1, (long)m1->size2, (long)m2->size1, (long)m2->size2);
      *errType=ERR_RANGE; *status = 1; return;
     }
    if (pplObjMatrix(o,0,1,m1->size1,m2->size2)==NULL) { *status=1; *errType=ERR_MEMORY; sprintf(errText,"Out of memory."); return; }
    mo = ((pplMatrix *)(o->auxil))->m;
    for (i=0; i<m1->size1; i++) for (j=0; j<m1->size2; j++) gsl_matrix_set(mo , i , j , gsl_matrix_get(m1,i,j) + gsl_matrix_get(m2,i,j));
    ppl_unitsDimCpy(o,a);
   }
  else // adding numbers
   {
    CAST_TO_NUM2(a); CAST_TO_NUM2(b);
am_numeric:
    o->objType = PPLOBJ_NUM; // Do this by hand rather than calling pplObjNum to save time (some field will be set by function call below)
    o->objPrototype = &pplObjPrototypes[PPLOBJ_NUM];
    o->self_lval = NULL; o->self_dval = NULL;
    o->self_this = NULL;
    o->amMalloced = 0;
    o->immutable = 0;
    ppl_uaAdd(context, a, b, o, status, errType, errText);
   }
cast_fail:
  return;
 }

void ppl_opSub(ppl_context *context, pplObj *a, pplObj *b, pplObj *o, int invertible, int *status, int *errType, char *errText)
 {
  int t1 = a->objType;
  int t2 = b->objType;

  if ((t1==PPLOBJ_NUM)&&(t2==PPLOBJ_NUM))
   {
    goto am_numeric;
   }
  else if ((t1==PPLOBJ_DATE)&&(t2==PPLOBJ_DATE)) // subtracting dates: return time interval
   {
    pplObjNum(o,0,a->real-b->real,0);
    o->dimensionless=0;
    o->exponent[UNIT_TIME]=1;
   }
  else if ((t1==PPLOBJ_DATE)&&(t2==PPLOBJ_NUM)) // subtracting time interval from date
   {
    int i;
    for (i=0; i<UNITS_MAX_BASEUNITS; i++) if (b->exponent[i] != (i==UNIT_TIME)) 
     { 
      *status=1; *errType=ERR_UNIT;
      sprintf(errText, "Can only subtract quantities with units of time from dates. Attempt to subtract a quantity with units of <%s>.", ppl_printUnit(context,b,NULL,NULL,1,1,0));
      return;
     }
    pplObjDate(o,0,a->real-b->real);
   }
  else if ((t1==PPLOBJ_COL)&&(t2==PPLOBJ_COL)) // subtracting colors
   {
    double r1,g1,b1,r2,g2,b2;
    if      (round(a->exponent[0])==SW_COLSPACE_RGB ) { r1=a->exponent[8]; g1=a->exponent[9]; b1=a->exponent[10]; }
    else if (round(a->exponent[0])==SW_COLSPACE_CMYK) pplcol_CMYKtoRGB(a->exponent[8],a->exponent[9],a->exponent[10],a->exponent[11],&r1,&g1,&b1);
    else                                              pplcol_HSBtoRGB (a->exponent[8],a->exponent[9],a->exponent[10],&r1,&g1,&b1);
    if      (round(b->exponent[0])==SW_COLSPACE_RGB ) { r2=b->exponent[8]; g2=b->exponent[9]; b2=b->exponent[10]; }
    else if (round(b->exponent[0])==SW_COLSPACE_CMYK) pplcol_CMYKtoRGB(b->exponent[8],b->exponent[9],b->exponent[10],b->exponent[11],&r2,&g2,&b2);
    else                                              pplcol_HSBtoRGB (b->exponent[8],b->exponent[9],b->exponent[10],&r2,&g2,&b2);
    r1-=r2; g1-=g2; b1-=b2;
    pplObjColor(o,0,SW_COLSPACE_RGB,r1,g1,b1,0);
   }
  else if ((t1==PPLOBJ_VEC)&&(t2==PPLOBJ_VEC)) // subtracting vectors
   {
    int i;
    gsl_vector *v1 = ((pplVector *)(a->auxil))->v;
    gsl_vector *v2 = ((pplVector *)(b->auxil))->v;
    gsl_vector *vo;
    if (ppl_unitsDimEqual(a, b) == 0)
     {
      if (a->dimensionless)
       { sprintf(errText, "Attempt to subtract quantities with conflicting dimensions: left operand is dimensionless, while right operand has units of <%s>.", ppl_printUnit(context, b, NULL, NULL, 1, 1, 0) ); }
      else if (b->dimensionless)
       { sprintf(errText, "Attempt to subtract quantities with conflicting dimensions: left operand has units of <%s>, while right operand is dimensionless.", ppl_printUnit(context, a, NULL, NULL, 0, 1, 0) ); }
      else
       { sprintf(errText, "Attempt to subtract quantities with conflicting dimensions: left operand has units of <%s>, while right operand has units of <%s>.", ppl_printUnit(context, a, NULL, NULL, 0, 1, 0), ppl_printUnit(context, b, NULL, NULL, 1, 1, 0) ); }
      *errType=ERR_UNIT; *status = 1; return;
     }
    if (v1->size != v2->size)
     {
      sprintf(errText, "Can only subtract vectors of a common size. Left operand has length of %ld, while right operand has length of %ld.", (long)v1->size, (long)v2->size);
      *errType=ERR_RANGE; *status = 1; return;
     }
    if (pplObjVector(o,0,1,v1->size)==NULL) { *status=1; *errType=ERR_MEMORY; sprintf(errText,"Out of memory."); return; }
    vo = ((pplVector *)(o->auxil))->v;
    for (i=0; i<v1->size; i++) gsl_vector_set(vo , i , gsl_vector_get(v1,i) - gsl_vector_get(v2,i));
    ppl_unitsDimCpy(o,a);
   }
  else if ((t1==PPLOBJ_MAT)&&(t2==PPLOBJ_MAT)) // subtracting matrices
   {
    int i,j;
    gsl_matrix *m1 = ((pplMatrix *)(a->auxil))->m;
    gsl_matrix *m2 = ((pplMatrix *)(b->auxil))->m;
    gsl_matrix *mo;
    if (ppl_unitsDimEqual(a, b) == 0)
     {
      if (a->dimensionless)
       { sprintf(errText, "Attempt to subtract quantities with conflicting dimensions: left operand is dimensionless, while right operand has units of <%s>.", ppl_printUnit(context, b, NULL, NULL, 1, 1, 0) ); }
      else if (b->dimensionless)
       { sprintf(errText, "Attempt to subtract quantities with conflicting dimensions: left operand has units of <%s>, while right operand is dimensionless.", ppl_printUnit(context, a, NULL, NULL, 0, 1, 0) ); }
      else
       { sprintf(errText, "Attempt to subtract quantities with conflicting dimensions: left operand has units of <%s>, while right operand has units of <%s>.", ppl_printUnit(context, a, NULL, NULL, 0, 1, 0), ppl_printUnit(context, b, NULL, NULL, 1, 1, 0) ); }
      *errType=ERR_UNIT; *status = 1; return;
     }
    if ( (m1->size1 != m2->size1) || (m1->size2 != m2->size2) )
     {
      sprintf(errText, "Can only subtract matrices of a common size. Left operand has size of %ldx%ld, while right operand has size of %ldx%ld.", (long)m1->size1, (long)m1->size2, (long)m2->size1, (long)m2->size2);
      *errType=ERR_RANGE; *status = 1; return;
     }
    if (pplObjMatrix(o,0,1,m1->size1,m2->size2)==NULL) { *status=1; *errType=ERR_MEMORY; sprintf(errText,"Out of memory."); return; }
    mo = ((pplMatrix *)(o->auxil))->m;
    for (i=0; i<m1->size1; i++) for (j=0; j<m1->size2; j++) gsl_matrix_set(mo , i , j , gsl_matrix_get(m1,i,j) - gsl_matrix_get(m2,i,j));
    ppl_unitsDimCpy(o,a);
   }
  else // subtracting numbers
   {
    CAST_TO_NUM2(a); CAST_TO_NUM2(b);
am_numeric:
    o->objType = PPLOBJ_NUM; // Do this by hand rather than calling pplObjNum to save time (some field will be set by function call below)
    o->objPrototype = &pplObjPrototypes[PPLOBJ_NUM];
    o->self_lval = NULL; o->self_dval = NULL;
    o->self_this = NULL;
    o->amMalloced = 0;
    o->immutable = 0;
    ppl_uaSub(context, a, b, o, status, errType, errText);
   }
cast_fail:
  return;
 }

void ppl_opMul(ppl_context *context, pplObj *a, pplObj *b, pplObj *o, int invertible, int *status, int *errType, char *errText)
 {
  int t1 = a->objType;
  int t2 = b->objType;

  if ((t1==PPLOBJ_NUM)&&(t2==PPLOBJ_NUM))
   {
    goto am_numeric;
   }
  else if ( ((t1==PPLOBJ_COL)&&(t2==PPLOBJ_NUM)) || ((t1==PPLOBJ_NUM)&&(t2==PPLOBJ_COL)) ) // multiplying colors by numbers
   {
    double  r1,g1,b1;
    pplObj *c = (t1==PPLOBJ_COL)?a:b;
    pplObj *n = (t1==PPLOBJ_COL)?b:a;
    double  m = n->real;
    if ((!n->dimensionless) || (n->flagComplex) || (!gsl_finite(m)) || (m<0))
     {
      sprintf(errText, "Can only multiply colors by dimensionless, real, positive numbers.");
      *errType=ERR_RANGE; *status=1; return;
     }
    if      (round(c->exponent[0])==SW_COLSPACE_RGB ) { r1=c->exponent[8]; g1=c->exponent[9]; b1=c->exponent[10]; }
    else if (round(c->exponent[0])==SW_COLSPACE_CMYK) pplcol_CMYKtoRGB(c->exponent[8],c->exponent[9],c->exponent[10],c->exponent[11],&r1,&g1,&b1);
    else                                              pplcol_HSBtoRGB (c->exponent[8],c->exponent[9],c->exponent[10],&r1,&g1,&b1);
    r1*=m; g1*=m; b1*=m;
    pplObjColor(o,0,SW_COLSPACE_RGB,r1,g1,b1,0);
   }
  else if ((t1==PPLOBJ_VEC)&&(t2==PPLOBJ_VEC)) // multiplying vectors (scalar dot product)
   {
    gsl_vector *v1 = ((pplVector *)(a->auxil))->v;
    gsl_vector *v2 = ((pplVector *)(b->auxil))->v;
    int i;
    double acc=0;
    if (v1->size != v2->size)
     {
      sprintf(errText, "Can only form dot-product of vectors of a common size. Left operand has length of %ld, while right operand has length of %ld.", (long)v1->size, (long)v2->size);
      *errType=ERR_RANGE; *status=1; return;
     }
    a->real=a->imag=b->real=b->imag=0 ; a->flagComplex=b->flagComplex=0;
    ppl_uaMul(context, a, b, o, status, errType, errText);
    for (i=0; i<v1->size; i++) acc += gsl_vector_get(v1,i) * gsl_vector_get(v2,i);
    o->real=acc; o->imag=0; o->flagComplex=0;
   }
  else if ( ((t1==PPLOBJ_VEC)&&(t2==PPLOBJ_NUM)) || // multiplying vector by number
            ((t1==PPLOBJ_NUM)&&(t2==PPLOBJ_VEC)) )  // multiplying number by vector
   {
    int         i;
    pplObj     *num = (t1==PPLOBJ_NUM) ? a : b;
    pplObj     *vec = (t1==PPLOBJ_VEC) ? a : b;
    gsl_vector *v   = ((pplVector *)(vec->auxil))->v;
    gsl_vector *vo  = NULL;
    if (num->flagComplex) { sprintf(errText, "Vectors can only contain real numbers, and cannot be multiplied by complex numbers."); *errType=ERR_NUMERIC; *status = 1; return; }
    vec->real=1; vec->imag=0; vec->flagComplex=0;
    if (pplObjVector(o,0,1,v->size)==NULL) { *status=1; *errType=ERR_MEMORY; sprintf(errText,"Out of memory."); return; }
    ppl_uaMul(context, a, b, o, status, errType, errText);
    vo = ((pplVector *)(o->auxil))->v;
    for (i=0; i<v->size; i++) gsl_vector_set(vo, i, gsl_vector_get(v,i) * o->real);
    o->real=0;
   }
  else if ( ((t1==PPLOBJ_MAT)&&(t2==PPLOBJ_NUM)) || // multiplying matrix by number
            ((t1==PPLOBJ_NUM)&&(t2==PPLOBJ_MAT)) )  // multiplying number by matrix
   {
    int         i,j;
    pplObj     *num = (t1==PPLOBJ_NUM) ? a : b;
    pplObj     *mat = (t1==PPLOBJ_MAT) ? a : b;
    gsl_matrix *m   = ((pplMatrix *)(mat->auxil))->m;
    gsl_matrix *mo  = NULL;
    if (num->flagComplex) { sprintf(errText, "Matrices can only contain real numbers, and cannot be multiplied by complex numbers."); *errType=ERR_NUMERIC; *status = 1; return; }
    mat->real=1; mat->imag=0; mat->flagComplex=0;
    if (pplObjMatrix(o,0,1,m->size1,m->size2)==NULL) { *status=1; *errType=ERR_MEMORY; sprintf(errText,"Out of memory."); return; }
    ppl_uaMul(context, a, b, o, status, errType, errText);
    mo = ((pplMatrix *)(o->auxil))->m;
    for (i=0; i<m->size1; i++) for (j=0; j<m->size2; j++) gsl_matrix_set(mo, i, j, gsl_matrix_get(m,i,j) * o->real);
    o->real=0;
   }
  else if ((t1==PPLOBJ_MAT)&&(t2==PPLOBJ_VEC)) // matrix-vector multiplication
   {
    int gslerr=0;
    gsl_matrix *m   = ((pplMatrix *)(a->auxil))->m;
    gsl_vector *v   = ((pplVector *)(b->auxil))->v;
    gsl_vector *vo  = NULL;
    if (m->size2 != v->size) { sprintf(errText, "Matrices can only be multiplied by vectors when the number of matrix columns (%ld) equals the number of vector rows (%ld).", (long)m->size2, (long)v->size); *errType=ERR_NUMERIC; *status = 1; return; }
    if (pplObjVector(o,0,1,m->size1)==NULL) { *status=1; *errType=ERR_MEMORY; sprintf(errText,"Out of memory."); return; }
    a->real=b->real=1; a->imag=b->imag=0; a->flagComplex=b->flagComplex=0;
    ppl_uaMul(context, a, b, o, status, errType, errText);
    vo = ((pplVector *)(o->auxil))->v;
    if ((gslerr = gsl_blas_dgemv(CblasNoTrans, 1, m, v, 0, vo))!=0) { *status=1; *errType=ERR_NUMERIC; strcpy(errText, gsl_strerror(gslerr)); return; }
   }
  else if ((t1==PPLOBJ_MAT)&&(t2==PPLOBJ_MAT)) // matrix-matrix multiplication
   {
    int gslerr=0;
    gsl_matrix *m1  = ((pplMatrix *)(a->auxil))->m;
    gsl_matrix *m2  = ((pplMatrix *)(b->auxil))->m;
    gsl_matrix *mo  = NULL;
    if (m1->size2 != m2->size1) { sprintf(errText, "Matrices can only be multiplied when the number of matrix columns (%ld) in the left matrix equals the number of rows (%ld) in the right matrix.", (long)m1->size2, (long)m2->size1); *errType=ERR_NUMERIC; *status = 1; return; }
    if (pplObjMatrix(o,0,1,m1->size1,m2->size2)==NULL) { *status=1; *errType=ERR_MEMORY; sprintf(errText,"Out of memory."); return; }
    a->real=b->real=1; a->imag=b->imag=0; a->flagComplex=b->flagComplex=0;
    ppl_uaMul(context, a, b, o, status, errType, errText);
    mo = ((pplMatrix *)(o->auxil))->m;
    if ((gslerr = gsl_blas_dgemm(CblasNoTrans, CblasNoTrans, 1, m1, m2, 0, mo))!=0) { *status=1; *errType=ERR_NUMERIC; strcpy(errText, gsl_strerror(gslerr)); return; }
   }
  else // multiplying numbers
   {
    CAST_TO_NUM2(a); CAST_TO_NUM2(b);
am_numeric:
    o->objType = PPLOBJ_NUM; // Do this by hand rather than calling pplObjNum to save time (some field will be set by function call below)
    o->objPrototype = &pplObjPrototypes[PPLOBJ_NUM];
    o->self_lval = NULL; o->self_dval = NULL;
    o->self_this = NULL;
    o->amMalloced = 0;
    o->immutable = 0;
    ppl_uaMul(context, a, b, o, status, errType, errText);
   }
cast_fail:
  return;
 }

void ppl_opDiv(ppl_context *context, pplObj *a, pplObj *b, pplObj *o, int invertible, int *status, int *errType, char *errText)
 {
  int t1 = a->objType;
  int t2 = b->objType;

  if ((t1==PPLOBJ_NUM)&&(t2==PPLOBJ_NUM))
   {
    goto am_numeric;
   }
  else if ((t1==PPLOBJ_COL)&&(t2==PPLOBJ_NUM)) // dividing colors by numbers
   {
    double  r1,g1,b1;
    double  m = b->real;
    if ((!b->dimensionless) || (b->flagComplex) || (!gsl_finite(m)) || (m<0))
     {
      sprintf(errText, "Can only divide colors by dimensionless, real, positive numbers.");
      *errType=ERR_RANGE; *status=1; return;
     }
    if      (round(a->exponent[0])==SW_COLSPACE_RGB ) { r1=a->exponent[8]; g1=a->exponent[9]; b1=a->exponent[10]; }
    else if (round(a->exponent[0])==SW_COLSPACE_CMYK) pplcol_CMYKtoRGB(a->exponent[8],a->exponent[9],a->exponent[10],a->exponent[11],&r1,&g1,&b1);
    else                                              pplcol_HSBtoRGB (a->exponent[8],a->exponent[9],a->exponent[10],&r1,&g1,&b1);
    r1/=m; g1/=m; b1/=m;
    pplObjColor(o,0,SW_COLSPACE_RGB,r1,g1,b1,0);
   }
  else if ((t1==PPLOBJ_VEC)&&(t2==PPLOBJ_NUM)) // dividing vector by number
   {
    int         i;
    gsl_vector *v   = ((pplVector *)(a->auxil))->v;
    gsl_vector *vo  = NULL;
    if (b->flagComplex) { sprintf(errText, "Vectors can only contain real numbers, and cannot be divided by complex numbers."); *errType=ERR_NUMERIC; *status = 1; return; }
    a->real=1; a->imag=0; a->flagComplex=0;
    if (pplObjVector(o,0,1,v->size)==NULL) { *status=1; *errType=ERR_MEMORY; sprintf(errText,"Out of memory."); return; }
    ppl_uaDiv(context, a, b, o, status, errType, errText);
    vo = ((pplVector *)(o->auxil))->v;
    for (i=0; i<v->size; i++) gsl_vector_set(vo, i, gsl_vector_get(v,i) * o->real);
    o->real=0;
   }
  else if ((t1==PPLOBJ_MAT)&&(t2==PPLOBJ_NUM)) // dividing matrix by number
   {
    int         i,j;
    gsl_matrix *m   = ((pplMatrix *)(a->auxil))->m;
    gsl_matrix *mo  = NULL;
    if (b->flagComplex) { sprintf(errText, "Matrices can only contain real numbers, and cannot be divided by complex numbers."); *errType=ERR_NUMERIC; *status = 1; return; }
    a->real=1; a->imag=0; a->flagComplex=0;
    if (pplObjMatrix(o,0,1,m->size1,m->size2)==NULL) { *status=1; *errType=ERR_MEMORY; sprintf(errText,"Out of memory."); return; }
    ppl_uaDiv(context, a, b, o, status, errType, errText);
    mo = ((pplMatrix *)(o->auxil))->m;
    for (i=0; i<m->size1; i++) for (j=0; j<m->size2; j++) gsl_matrix_set(mo, i, j, gsl_matrix_get(m,i,j) * o->real);
    o->real=0;
   }
  else // dividing numbers
   {
    CAST_TO_NUM2(a); CAST_TO_NUM2(b);
am_numeric:
    o->objType = PPLOBJ_NUM; // Do this by hand rather than calling pplObjNum to save time (some field will be set by function call below)
    o->objPrototype = &pplObjPrototypes[PPLOBJ_NUM];
    o->self_lval = NULL; o->self_dval = NULL;
    o->self_this = NULL;
    o->amMalloced = 0;
    o->immutable = 0;
    ppl_uaDiv(context, a, b, o, status, errType, errText);
   }
cast_fail:
  return;
 }

