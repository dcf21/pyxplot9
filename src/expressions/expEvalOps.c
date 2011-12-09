// expEvalOps.c
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

#include <stdlib.h>
#include <string.h>

#include <gsl/gsl_math.h>

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

void ppl_opAdd(ppl_context *context, pplObj *a, pplObj *b, pplObj *o, int invertible, int *status, int *errType, char *errText)
 {
  int t1 = a->objType;
  int t2 = b->objType;

  if ((t1==PPLOBJ_STR)&&(t2==PPLOBJ_STR)) // adding strings: concatenate
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
  else if ((t1==PPLOBJ_VEC)&&(t2==PPLOBJ_VEC)) // adding vectors
   {
    int i,j;
    gsl_vector *v1 = ((pplVector *)(a->auxil))->v;
    gsl_vector *v2 = ((pplVector *)(b->auxil))->v;
    gsl_vector *vo;
    if (ppl_unitsDimEqual(a, b) == 0)
     {
      if (a->dimensionless)
       { sprintf(errText, "Attempt to add quantities with conflicting dimensions: left operand is dimensionless, while right operand has units of <%s>.", ppl_printUnit(context, b, NULL, NULL, 1, 1, 0) ); }
      else if (b->dimensionless)
       { sprintf(errText, "Attempt to add quantities with conflicting dimensions: left operand has units of <%s>, while right operand is dimensionless.", ppl_printUnit(context, a, NULL, NULL, 0, 1, 0) ); }
      else
       { sprintf(errText, "Attempt to add quantities with conflicting dimensions: left operand has units of <%s>, while right operand has units of <%s>.", ppl_printUnit(context, a, NULL, NULL, 0, 1, 0), ppl_printUnit(context, b, NULL, NULL, 1, 1, 0) ); }
      *errType=ERR_UNIT; *status = 1; return;
     }
    if (pplObjVector(o,0,1,v1->size+v2->size)==NULL) { *status=1; *errType=ERR_MEMORY; sprintf(errText,"Out of memory."); return; }
    vo = ((pplVector *)(o->auxil))->v;
    for (j=i=0; i<v1->size; i++,j++) vo->data[j] = v1->data[i];
    for (  i=0; i<v2->size; i++,j++) vo->data[j] = v2->data[i];
    ppl_unitsDimCpy(o,a);
   }
  else // adding numbers
   {
    CAST_TO_NUM(a); CAST_TO_NUM(b);
    ppl_uaAdd(context, a, b, o, status, errType, errText);
   }
  return;
  cast_fail: *status=1;
 }

void ppl_opSub(ppl_context *context, pplObj *a, pplObj *b, pplObj *o, int invertible, int *status, int *errType, char *errText)
 {
  int t1 = a->objType;
  int t2 = b->objType;

  if ((t1==PPLOBJ_DATE)&&(t2==PPLOBJ_DATE)) // subtracting dates: return time interval
   {
    pplObjNum(o,0,a->real-b->real,0);
    o->dimensionless=0;
    o->exponent[UNIT_TIME]=1;
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
  else // subtracting numbers
   {
    CAST_TO_NUM(a); CAST_TO_NUM(b);
    ppl_uaSub(context, a, b, o, status, errType, errText);
   }
  return;
  cast_fail: *status=1;
 }

void ppl_opMul(ppl_context *context, pplObj *a, pplObj *b, pplObj *o, int invertible, int *status, int *errType, char *errText)
 {
  CAST_TO_NUM(a); CAST_TO_NUM(b);
  ppl_uaMul(context, a, b, o, status, errType, errText);
  return;
  cast_fail: *status=1;
 }

void ppl_opDiv(ppl_context *context, pplObj *a, pplObj *b, pplObj *o, int invertible, int *status, int *errType, char *errText)
 {
  CAST_TO_NUM(a); CAST_TO_NUM(b);
  ppl_uaDiv(context, a, b, o, status, errType, errText);
  return;
  cast_fail: *status=1;
 }

