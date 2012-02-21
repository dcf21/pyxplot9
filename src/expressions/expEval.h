// expEval.h
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

#ifndef _EXPEVAL_H
#define _EXPEVAL_H 1

#include "expressions/expCompile.h"
#include "userspace/context.h"
#include "userspace/pplObj.h"

#define CAST_TO_NUM(X) \
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
        if ((len<0)||(c[len]!='\0')) { sprintf(context->errStat.errBuff,"Attempt to implicitly cast string to number failed: string is not a valid number."); TBADD(ERR_TYPE,0,NULL); goto cast_fail; } \
        break; \
       } \
      default: \
        { sprintf(context->errStat.errBuff,"Cannot implicitly cast an object of type <%s> to a number.",pplObjTypeNames[t]); TBADD(ERR_TYPE,0,NULL); goto cast_fail; } \
     } \
    ppl_garbageObject(X); \
    pplObjNum(X,0,d,0); \
    (X)->refCount=rc; \
   } \
 }

#define CAST_TO_REAL(X,OP) \
 { \
  CAST_TO_NUM(X); \
  if ((X)->flagComplex) { sprintf(context->errStat.errBuff,"The %s operator can only act on real numbers.",OP); TBADD(ERR_RANGE,0,NULL); goto cast_fail; } \
 }

#define CAST_TO_INT(X,OP) \
 { \
  CAST_TO_REAL(X,OP); \
  if (!(X)->dimensionless) { sprintf(context->errStat.errBuff,"The %s operator is an integer operator which can only act on dimensionless numbers: supplied operand has units of <%s>.",OP,ppl_printUnit(context,X,NULL,NULL,0,1,0)); TBADD(ERR_UNIT,0,NULL); goto cast_fail; } \
  if (((X)->real < INT_MIN) || ((X)->real > INT_MAX)) { sprintf(context->errStat.errBuff,"The %s operator can only act on integers in the range %d to %d.",OP,INT_MIN,INT_MAX); TBADD(ERR_RANGE,0,NULL); goto cast_fail; } \
 }

#define CAST_TO_BOOL(X) \
 { \
  int t=(X)->objType, s; \
  int rc=(X)->refCount; \
  if (t!=PPLOBJ_BOOL) \
   { \
    switch (t) \
     { \
      case PPLOBJ_NUM : s = (((X)->real!=0)||((X)->imag!=0)) && gsl_finite((X)->real) && gsl_finite((X)->imag) && (((X)->imag==0)||(context->set->term_current.ComplexNumbers == SW_ONOFF_ON)); break; \
      case PPLOBJ_STR : s = ((char *)(X)->auxil)[0]!='\0'; break; \
      case PPLOBJ_ZOM : \
      case PPLOBJ_EXC : \
      case PPLOBJ_NULL: s = 0; break; \
      case PPLOBJ_DICT: s = (((dict *)(X)->auxil)->length!=0); break; \
      case PPLOBJ_LIST: s = (((list *)(X)->auxil)->length!=0); break; \
      case PPLOBJ_VEC : s = (((pplVector *)(X)->auxil)->v->size!=0); break; \
      case PPLOBJ_MAT : s = (((pplMatrix *)(X)->auxil)->m->size1!=0) || (((pplMatrix *)(X)->auxil)->m->size2!=0); break; \
      case PPLOBJ_FILE: s = (((pplFile *)(X)->auxil)->open!=0); break; \
      default         : s = 1; break; \
     } \
    ppl_garbageObject(X); \
    pplObjBool(X,0,s); \
    (X)->refCount=rc; \
   } \
 }

pplObj *ppl_expEval(ppl_context *context, pplExpr *inExpr, int *lastOpAssign, int dollarAllowed, int IterDepth);

#endif

