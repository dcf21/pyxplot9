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
#include "userspace/unitsArithmetic.h"

#include "pplConstants.h"

void ppl_opAdd(ppl_context *context, pplObj *a, pplObj *b, pplObj *o, int *status, int *errType, char *errText)
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
  else // adding numbers
   {
    CAST_TO_NUM(a); CAST_TO_NUM(b);
    ppl_uaAdd(context, a, b, o, status, errType, errText);
   }
  return;
  cast_fail: *status=1;
 }

void ppl_opSub(ppl_context *context, pplObj *a, pplObj *b, pplObj *o, int *status, int *errType, char *errText)
 {
  CAST_TO_NUM(a); CAST_TO_NUM(b);
  ppl_uaSub(context, a, b, o, status, errType, errText);
  return;
  cast_fail: *status=1;
 }

void ppl_opMul(ppl_context *context, pplObj *a, pplObj *b, pplObj *o, int *status, int *errType, char *errText)
 {
  CAST_TO_NUM(a); CAST_TO_NUM(b);
  ppl_uaMul(context, a, b, o, status, errType, errText);
  return;
  cast_fail: *status=1;
 }

void ppl_opDiv(ppl_context *context, pplObj *a, pplObj *b, pplObj *o, int *status, int *errType, char *errText)
 {
  CAST_TO_NUM(a); CAST_TO_NUM(b);
  ppl_uaDiv(context, a, b, o, status, errType, errText);
  return;
  cast_fail: *status=1;
 }

