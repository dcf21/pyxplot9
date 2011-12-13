// expEvalCalculus.c
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

#define _EXPEVALCALCULUS_C 1

#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <string.h>

#include <gsl/gsl_deriv.h>
#include <gsl/gsl_integration.h>
#include <gsl/gsl_math.h>

#include "expressions/expCompile.h"
#include "expressions/expEval.h"
#include "settings/settingTypes.h"
#include "stringTools/asciidouble.h"
#include "userspace/context.h"
#include "userspace/contextVarDef.h"
#include "userspace/garbageCollector.h"
#include "userspace/pplObj_fns.h"
#include "userspace/unitsArithmetic.h"

typedef struct calculusComm {
 void         *expr;
 ppl_context  *context;
 pplObj       *dummy;
 pplObj        first;
 double        dummyReal, dummyImag;
 unsigned char isFirst;
 unsigned char testingReal, varyingReal;
 int           dollarAllowed;
 int          *errPos;
 int          *errType;
 char         *errText;
 int           iterDepth;
 } calculusComm;

double ppl_expEvalCalculusSlave(double x, void *params)
 {
  int           lastOpAssign;
  pplObj       *output;
  calculusComm *data = (calculusComm *)params;

  if (*(data->errPos)>=0) return GSL_NAN; // We've previously had an error... so don't do any more work

  if (data->varyingReal) { data->dummy->real = x; data->dummy->imag = data->dummyImag; data->dummy->flagComplex = !ppl_dblEqual(data->dummy->imag,0); }
  else                   { data->dummy->imag = x; data->dummy->real = data->dummyReal; data->dummy->flagComplex = !ppl_dblEqual(data->dummy->imag,0); }

  output = ppl_expEval(data->context, data->expr, &lastOpAssign, data->dollarAllowed, data->iterDepth+1, data->errPos, data->errType, data->errText);
  if (*(data->errPos)>=0) return GSL_NAN;

  // Check that integrand is a number
  if (output->objType!=PPLOBJ_NUM)
   {
    *(data->errPos)=0; *(data->errType)=ERR_TYPE;
    strcpy(data->errText, "This operand is not a number across the range where calculus is being attempted.");
    ppl_garbageObject(&data->context->stack[--data->context->stackPtr]); // trash and pop output from stack
    return GSL_NAN;
   }
  data->context->stackPtr--; // pop output from stack

  // Check that integrand is dimensionally consistent over integration range
  if (data->isFirst)
   {
    memcpy(&data->first, output, sizeof(pplObj));
    data->isFirst = 0;
   }
  else
   {
    if (!ppl_unitsDimEqual(&data->first,output))
     {
      *(data->errPos)=0; *(data->errType)=ERR_UNIT;
      strcpy(data->errText, "This operand does not have consistent units across the range where calculus is being attempted.");
      return GSL_NAN;
     }
   }

  // Integrand was complex, but complex arithmetic is turned off
  if ((!ppl_dblEqual(output->imag, 0)) && (data->context->set->term_current.ComplexNumbers == SW_ONOFF_OFF)) return GSL_NAN;

  if (data->testingReal) return output->real;
  else                   return output->imag;
 }

void ppl_expIntegrate(ppl_context *c, char *expr, char *dummy, pplObj *min, pplObj *max, pplObj *out, int dollarAllowed, int *errPos, int *errType, char *errText, int iterDepth)
 {
  calculusComm     commlink;
  pplObj          *dummyVar;
  pplObj           dummyTemp;
  gsl_integration_workspace *ws;
  gsl_function     fn;
  void            *expr2, *tmp;
  int              elen = 4*strlen(expr)+1024;
  int              explen;
  double           resultReal=0, resultImag=0, error;

  if (!ppl_unitsDimEqual(min,max))
   {
    *errPos=0; *errType=ERR_UNIT;
    strcpy(errText, "The minimum and maximum limits of this integration operation are not dimensionally compatible.");
    return;
   }

  if (min->flagComplex || max->flagComplex)
   {
    *errPos=0; *errType=ERR_NUMERIC;
    strcpy(errText, "The minimum and maximum limits of this integration operation must be real numbers; supplied values are complex.");
    return;
   }

  expr2 = malloc(elen);
  ppl_expCompile(c,expr,&explen,dollarAllowed,1,expr2,&elen,errPos,errType,errText);
  if (*errPos>=0) { free(expr2); return; }
  if (explen<strlen(expr)) { *errPos=explen; *errType=ERR_SYNTAX; strcpy(errText, "Unexpected trailing matter at the end of integrand."); free(expr2); return; }
  tmp = realloc(expr2,elen+16);
  if (tmp==NULL) { *errPos=0; *errType=ERR_MEMORY; strcpy(errText, "Out of memory."); free(expr2); return; }
  expr2 = tmp;

  commlink.context = c;
  commlink.expr    = expr2;
  commlink.isFirst = 1;
  commlink.testingReal = 1;
  commlink.varyingReal = 1;
  commlink.dollarAllowed = dollarAllowed;
  commlink.errPos  = errPos;
  commlink.errType = errType;
  commlink.errText = errText;
  commlink.iterDepth = iterDepth;
  pplObjNum(&commlink.first,0,0,0);

  ppl_contextGetVarPointer(c, dummy, &dummyVar, &dummyTemp);
  memcpy(dummyVar, min, sizeof(pplObj)); // Get units of dummyVar right
  commlink.dummy     = dummyVar;
  commlink.dummyReal = dummyVar->real;
  commlink.dummyImag = dummyVar->imag;

  ws          = gsl_integration_workspace_alloc(1000);
  fn.function = &ppl_expEvalCalculusSlave;
  fn.params   = &commlink;

  gsl_integration_qags (&fn, min->real, max->real, 0, 1e-7, 1000, ws, &resultReal, &error);

  if ((*errPos < 0) && (c->set->term_current.ComplexNumbers == SW_ONOFF_ON))
   {
    commlink.testingReal = 0;
    gsl_integration_qags (&fn, min->real, max->real, 0, 1e-7, 1000, ws, &resultImag, &error);
   }

  gsl_integration_workspace_free(ws);
  free(expr2);

  ppl_contextRestoreVarPointer(c, dummy, &dummyTemp); // Restore old value of the dummy variable we've been using

  if (*errPos < 0)
   {
    int status=0;
    ppl_uaMul( c, &commlink.first , min , out , &status, errType, errText ); // Get units of output right
    if (status) { *errPos=0; return; }
    out->real = resultReal;
    out->imag = resultImag;
    out->flagComplex = !ppl_dblEqual(resultImag, 0);
    if (!out->flagComplex) out->imag=0.0; // Enforce that real numbers have positive zero imaginary components

    if ((!gsl_finite(out->real)) || (!gsl_finite(out->imag)) || ((out->flagComplex) && (c->set->term_current.ComplexNumbers == SW_ONOFF_OFF)))
     {
      if (c->set->term_current.ExplicitErrors == SW_ONOFF_ON) { *errPos=0; *errType=ERR_NUMERIC; sprintf(errText, "Integral does not evaluate to a finite value."); return; }
      else { out->real = GSL_NAN; out->imag = 0; out->flagComplex=0; }
     }
   }
  return;
 }

void ppl_expDifferentiate(ppl_context *c, char *expr, char *dummy, pplObj *point, pplObj *step, pplObj *out, int dollarAllowed, int *errPos, int *errType, char *errText, int iterDepth)
 {
  calculusComm     commlink;
  pplObj          *dummyVar;
  pplObj           dummyTemp;
  gsl_function     fn;
  void            *expr2, *tmp;
  int              elen = 4*strlen(expr)+1024;
  int              explen;
  double           resultReal=0, resultImag=0, dIdI, dRdI;
  double           resultReal_error, resultImag_error, dIdI_error, dRdI_error;

  if (!ppl_unitsDimEqual(point, step))
   {
    *errPos=0; *errType=ERR_NUMERIC;
    strcpy(errText, "The arguments x and step to this differentiation operation are not dimensionally compatible.");
    return;
   }

  if (step->flagComplex)
   {
    *errPos=0; *errType=ERR_NUMERIC;
    strcpy(errText, "The argument 'step' to this differentiation operation must be a real number; supplied value is complex.");
    return;
   }

  expr2 = malloc(elen);
  ppl_expCompile(c,expr,&explen,dollarAllowed,1,expr2,&elen,errPos,errType,errText);
  if (*errPos>=0) { free(expr2); return; }
  if (explen<strlen(expr)) { *errPos=explen; *errType=ERR_SYNTAX; strcpy(errText, "Unexpected trailing matter at the end of integrand."); free(expr2); return; }
  tmp = realloc(expr2,elen+16);
  if (tmp==NULL) { *errPos=0; *errType=ERR_MEMORY; strcpy(errText, "Out of memory."); free(expr2); return; }
  expr2 = tmp;

  commlink.context = c;
  commlink.expr    = expr2;
  commlink.isFirst = 1;
  commlink.testingReal = 1;
  commlink.varyingReal = 1;
  commlink.dollarAllowed = dollarAllowed;
  commlink.errPos  = errPos;
  commlink.errType = errType;
  commlink.errText = errText;
  commlink.iterDepth = iterDepth;
  pplObjNum(&commlink.first,0,0,0);

  ppl_contextGetVarPointer(c, dummy, &dummyVar, &dummyTemp);
  memcpy(dummyVar, point, sizeof(pplObj)); // Get units of dummyVar right
  commlink.dummy     = dummyVar;
  commlink.dummyReal = dummyVar->real;
  commlink.dummyImag = dummyVar->imag;

  fn.function = &ppl_expEvalCalculusSlave;
  fn.params   = &commlink;

  gsl_deriv_central(&fn, point->real, step->real, &resultReal, &resultReal_error);

  if ((*errPos < 0) && (c->set->term_current.ComplexNumbers == SW_ONOFF_ON))
   {
    commlink.testingReal = 0;
    gsl_deriv_central(&fn, point->real, step->real, &resultImag, &resultImag_error);
    commlink.varyingReal = 0;
    gsl_deriv_central(&fn, point->imag, step->real, &dIdI      , &dIdI_error);
    commlink.testingReal = 1;
    gsl_deriv_central(&fn, point->imag, step->real, &dRdI      , &dRdI_error);

    if ((!ppl_dblApprox(resultReal, dIdI, 2*(resultReal_error+dIdI_error))) || (!ppl_dblApprox(resultImag, -dRdI, 2*(resultImag_error+dRdI_error))))
     { *errPos = 0; *errType=ERR_NUMERIC; sprintf(errText, "The Cauchy-Riemann equations are not satisfied at this point in the complex plane. It does not therefore appear possible to perform complex differentiation. In the notation f(x+iy)=u+iv, the offending derivatives were: du/dx=%e, dv/dy=%e, du/dy=%e and dv/dx=%e.", resultReal, dIdI, dRdI, resultImag); return; }
   }

  ppl_contextRestoreVarPointer(c, dummy, &dummyTemp); // Restore old value of the dummy variable we've been using

  if (*errPos < 0)
   {
    int status=0;
    point->real = 1.0; point->imag = 0.0; point->flagComplex = 0;
    ppl_uaDiv( c , &commlink.first , point , out , &status, errType, errText ); // Get units of output right
    if (status) { *errPos=0; return; }
    out->real = resultReal;
    out->imag = resultImag;
    out->flagComplex = !ppl_dblEqual(resultImag, 0);
    if (!out->flagComplex) out->imag=0.0; // Enforce that real numbers have positive zero imaginary components
   }

  if ((!gsl_finite(out->real)) || (!gsl_finite(out->imag)) || ((out->flagComplex) && (c->set->term_current.ComplexNumbers == SW_ONOFF_OFF)))
   {
    if (c->set->term_current.ExplicitErrors == SW_ONOFF_ON) { *errPos=0; *errType=ERR_NUMERIC; sprintf(errText, "Differential does not evaluate to a finite value."); return; }
    else { out->real = GSL_NAN; out->imag = 0; out->flagComplex=0; }
   }
  return;
 }

