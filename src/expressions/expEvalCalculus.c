// expEvalCalculus.c
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

#define _EXPEVALCALCULUS_C 1

#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <string.h>

#include <gsl/gsl_deriv.h>
#include <gsl/gsl_integration.h>
#include <gsl/gsl_math.h>

#include "expressions/expCompile_fns.h"
#include "expressions/expEval.h"
#include "expressions/traceback_fns.h"
#include "settings/settingTypes.h"
#include "stringTools/asciidouble.h"
#include "userspace/context.h"
#include "userspace/contextVarDef.h"
#include "userspace/garbageCollector.h"
#include "userspace/pplObj_fns.h"
#include "userspace/unitsArithmetic.h"

typedef struct calculusComm {
 pplExpr      *expr;
 ppl_context  *context;
 pplObj       *dummy;
 pplObj        first;
 double        dummyReal, dummyImag;
 unsigned char isFirst;
 unsigned char testingReal, varyingReal;
 int           dollarAllowed;
 int           integrate;
 int           iterDepth;
 } calculusComm;

double ppl_expEvalCalculusSlave(double x, void *params)
 {
  int           lastOpAssign;
  double        r,i;
  pplObj       *output;
  calculusComm *data = (calculusComm *)params;

  if (data->context->errStat.status) return GSL_NAN; // We've previously had an error... so don't do any more work

  if (data->varyingReal) { data->dummy->real = x; data->dummy->imag = data->dummyImag; data->dummy->flagComplex = !ppl_dblEqual(data->dummy->imag,0); }
  else                   { data->dummy->imag = x; data->dummy->real = data->dummyReal; data->dummy->flagComplex = !ppl_dblEqual(data->dummy->imag,0); }

  output = ppl_expEval(data->context, data->expr, &lastOpAssign, data->dollarAllowed, data->iterDepth+1);
  if (data->context->errStat.status) return GSL_NAN;

  // Check that integrand is a number
  if (output->objType!=PPLOBJ_NUM)
   {
    strcpy(data->context->errStat.errBuff, "This operand is not a number across the range where calculus is being attempted.");
    ppl_tbAdd(data->context,data->expr->srcLineN,data->expr->srcId,data->expr->srcFname,0,ERR_TYPE,0,data->expr->ascii,data->integrate?"integrand":"differentiated expression");
    ppl_garbageObject(&data->context->stack[--data->context->stackPtr]); // trash and pop output from stack
    return GSL_NAN;
   }

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
      strcpy(data->context->errStat.errBuff, "This operand does not have consistent units across the range where calculus is being attempted.");
      ppl_tbAdd(data->context,data->expr->srcLineN,data->expr->srcId,data->expr->srcFname,0,ERR_UNIT,0,data->expr->ascii,data->integrate?"integrand":"differentiated expression");
      ppl_garbageObject(&data->context->stack[--data->context->stackPtr]); // trash and pop output from stack
      return GSL_NAN;
     }
   }

  r = output->real;
  i = output->imag;
  ppl_garbageObject(&data->context->stack[--data->context->stackPtr]); // trash and pop output from stack

  // Integrand was complex, but complex arithmetic is turned off
  if ((!ppl_dblEqual(output->imag, 0)) && (data->context->set->term_current.ComplexNumbers == SW_ONOFF_OFF)) return GSL_NAN;

  if (data->testingReal) return r;
  else                   return i;
 }

void ppl_expIntegrate(ppl_context *c, pplExpr *inExpr, int inExprCharPos, char *expr, int exprPos, char *dummy, pplObj *min, pplObj *max, pplObj *out, int dollarAllowed, int iterDepth)
 {
  calculusComm     commlink;
  pplObj          *dummyVar;
  pplObj           dummyTemp;
  gsl_integration_workspace *ws;
  gsl_function     fn;
  pplExpr         *expr2;
  int              explen;
  double           resultReal=0, resultImag=0, error;

  if (!ppl_unitsDimEqual(min,max))
   {
    strcpy(c->errStat.errBuff, "The minimum and maximum limits of this integration operation are not dimensionally compatible.");
    ppl_tbAdd(c,inExpr->srcLineN,inExpr->srcId,inExpr->srcFname,0,ERR_UNIT,inExprCharPos,inExpr->ascii,"int_d?() function");
    return;
   }

  if (min->flagComplex || max->flagComplex)
   {
    strcpy(c->errStat.errBuff, "The minimum and maximum limits of this integration operation must be real numbers; supplied values are complex.");
    ppl_tbAdd(c,inExpr->srcLineN,inExpr->srcId,inExpr->srcFname,0,ERR_NUMERICAL,inExprCharPos,inExpr->ascii,"int_d?() function");
    return;
   }

  {
   int errPos=-1, errType=-1;
   ppl_expCompile(c,inExpr->srcLineN,inExpr->srcId,inExpr->srcFname,expr,&explen,dollarAllowed,1,1,&expr2,&errPos,&errType,c->errStat.errBuff);
   if (errPos>=0) { pplExpr_free(expr2); ppl_tbAdd(c,inExpr->srcLineN,inExpr->srcId,inExpr->srcFname,0,errType,errPos+exprPos,inExpr->ascii,"int_d?() function"); return; }
   if (explen<strlen(expr)) { strcpy(c->errStat.errBuff, "Unexpected trailing matter at the end of integrand."); ppl_tbAdd(c,inExpr->srcLineN,inExpr->srcId,inExpr->srcFname,0,ERR_SYNTAX,explen+exprPos,inExpr->ascii,"int_d?() function"); pplExpr_free(expr2); return; }
  }

  commlink.context   = c;
  commlink.integrate = 1;
  commlink.expr      = expr2;
  commlink.isFirst   = 1;
  commlink.testingReal = 1;
  commlink.varyingReal = 1;
  commlink.dollarAllowed = dollarAllowed;
  commlink.iterDepth = iterDepth;
  pplObjNum(&commlink.first,0,0,0);

  ppl_contextGetVarPointer(c, dummy, &dummyVar, &dummyTemp);
  dummyVar->objType     = min->objType;
  dummyVar->real        = min->real;
  dummyVar->imag        = min->imag;
  dummyVar->flagComplex = min->flagComplex;
  ppl_unitsDimCpy(dummyVar, min); // Get units of dummyVar right
  commlink.dummy     = dummyVar;
  commlink.dummyReal = dummyVar->real;
  commlink.dummyImag = dummyVar->imag;

  ws          = gsl_integration_workspace_alloc(1000);
  fn.function = &ppl_expEvalCalculusSlave;
  fn.params   = &commlink;

  gsl_integration_qags (&fn, min->real, max->real, 0, 1e-7, 1000, ws, &resultReal, &error);

  if ((!c->errStat.status) && (c->set->term_current.ComplexNumbers == SW_ONOFF_ON))
   {
    commlink.testingReal = 0;
    gsl_integration_qags (&fn, min->real, max->real, 0, 1e-7, 1000, ws, &resultImag, &error);
   }

  gsl_integration_workspace_free(ws);
  pplExpr_free(expr2);

  ppl_contextRestoreVarPointer(c, dummy, &dummyTemp); // Restore old value of the dummy variable we've been using

  if (!c->errStat.status)
   {
    int status=0, errType=-1;
    ppl_uaMul( c, &commlink.first , min , out , &status, &errType, c->errStat.errBuff ); // Get units of output right
    if (status) { ppl_tbAdd(c,inExpr->srcLineN,inExpr->srcId,inExpr->srcFname,0,errType,inExprCharPos,inExpr->ascii,"int_d?() function"); return; }
    out->real = resultReal;
    out->imag = resultImag;
    out->flagComplex = !ppl_dblEqual(resultImag, 0);
    if (!out->flagComplex) out->imag=0.0; // Enforce that real numbers have positive zero imaginary components

    if ((!gsl_finite(out->real)) || (!gsl_finite(out->imag)) || ((out->flagComplex) && (c->set->term_current.ComplexNumbers == SW_ONOFF_OFF)))
     {
      if (c->set->term_current.ExplicitErrors == SW_ONOFF_ON) { sprintf(c->errStat.errBuff, "Integral does not evaluate to a finite value."); ppl_tbAdd(c,inExpr->srcLineN,inExpr->srcId,inExpr->srcFname,0,ERR_NUMERICAL,exprPos,inExpr->ascii,"int_d?() function"); return; }
      else { out->real = GSL_NAN; out->imag = 0; out->flagComplex=0; }
     }
   }
  else
   {
    strcpy(c->errStat.errBuff, "");
    ppl_tbAdd(c,inExpr->srcLineN,inExpr->srcId,inExpr->srcFname,0,ERR_GENERIC,inExprCharPos,inExpr->ascii,"int_d?() function");
   }
  return;
 }

void ppl_expDifferentiate(ppl_context *c, pplExpr *inExpr, int inExprCharPos, char *expr, int exprPos, char *dummy, pplObj *point, pplObj *step, pplObj *out, int dollarAllowed, int iterDepth)
 {
  calculusComm     commlink;
  pplObj          *dummyVar;
  pplObj           dummyTemp;
  gsl_function     fn;
  pplExpr         *expr2;
  int              explen;
  double           resultReal=0, resultImag=0, dIdI, dRdI;
  double           resultReal_error, resultImag_error, dIdI_error, dRdI_error;

  if (!ppl_unitsDimEqual(point, step))
   {
    strcpy(c->errStat.errBuff, "The arguments x and step to this differentiation operation are not dimensionally compatible.");
    ppl_tbAdd(c,inExpr->srcLineN,inExpr->srcId,inExpr->srcFname,0,ERR_NUMERICAL,inExprCharPos,inExpr->ascii,"diff_d?() function");
    return;
   }

  if (step->flagComplex)
   {
    strcpy(c->errStat.errBuff, "The argument 'step' to this differentiation operation must be a real number; supplied value is complex.");
    ppl_tbAdd(c,inExpr->srcLineN,inExpr->srcId,inExpr->srcFname,0,ERR_NUMERICAL,inExprCharPos,inExpr->ascii,"diff_d?() function");
    return;
   }

  {
   int errPos=-1, errType=-1;
   ppl_expCompile(c,inExpr->srcLineN,inExpr->srcId,inExpr->srcFname,expr,&explen,dollarAllowed,1,1,&expr2,&errPos,&errType,c->errStat.errBuff);
   if (errPos>=0) { pplExpr_free(expr2); ppl_tbAdd(c,inExpr->srcLineN,inExpr->srcId,inExpr->srcFname,0,errType,errPos+exprPos,inExpr->ascii,"diff_d?() function"); return; }
   if (explen<strlen(expr)) { strcpy(c->errStat.errBuff, "Unexpected trailing matter at the end of differentiated expression."); ppl_tbAdd(c,inExpr->srcLineN,inExpr->srcId,inExpr->srcFname,0,ERR_SYNTAX,explen,inExpr->ascii,"diff_d?() function"); pplExpr_free(expr2); return; }
  }

  commlink.context   = c;
  commlink.integrate = 0;
  commlink.expr      = expr2;
  commlink.isFirst   = 1;
  commlink.testingReal = 1;
  commlink.varyingReal = 1;
  commlink.dollarAllowed = dollarAllowed;
  commlink.iterDepth = iterDepth;
  pplObjNum(&commlink.first,0,0,0);

  ppl_contextGetVarPointer(c, dummy, &dummyVar, &dummyTemp);
  dummyVar->objType     = point->objType;
  dummyVar->real        = point->real;
  dummyVar->imag        = point->imag;
  dummyVar->flagComplex = point->flagComplex;
  ppl_unitsDimCpy(dummyVar, point); // Get units of dummyVar right
  commlink.dummy     = dummyVar;
  commlink.dummyReal = dummyVar->real;
  commlink.dummyImag = dummyVar->imag;

  fn.function = &ppl_expEvalCalculusSlave;
  fn.params   = &commlink;

  gsl_deriv_central(&fn, point->real, step->real, &resultReal, &resultReal_error);
  pplExpr_free(expr2);

  if ((!c->errStat.status) && (c->set->term_current.ComplexNumbers == SW_ONOFF_ON))
   {
    commlink.testingReal = 0;
    gsl_deriv_central(&fn, point->real, step->real, &resultImag, &resultImag_error);
    commlink.varyingReal = 0;
    gsl_deriv_central(&fn, point->imag, step->real, &dIdI      , &dIdI_error);
    commlink.testingReal = 1;
    gsl_deriv_central(&fn, point->imag, step->real, &dRdI      , &dRdI_error);

    if ((!ppl_dblApprox(resultReal, dIdI, 2*(resultReal_error+dIdI_error))) || (!ppl_dblApprox(resultImag, -dRdI, 2*(resultImag_error+dRdI_error))))
     { sprintf(c->errStat.errBuff, "The Cauchy-Riemann equations are not satisfied at this point in the complex plane. It does not therefore appear possible to perform complex differentiation. In the notation f(x+iy)=u+iv, the offending derivatives were: du/dx=%e, dv/dy=%e, du/dy=%e and dv/dx=%e.", resultReal, dIdI, dRdI, resultImag); ppl_tbAdd(c,inExpr->srcLineN,inExpr->srcId,inExpr->srcFname,0,ERR_NUMERICAL,exprPos,inExpr->ascii,"diff_d?() function"); return; }
   }

  ppl_contextRestoreVarPointer(c, dummy, &dummyTemp); // Restore old value of the dummy variable we've been using

  if (!c->errStat.status)
   {
    int status=0, errType=-1;
    point->real = 1.0; point->imag = 0.0; point->flagComplex = 0;
    ppl_uaDiv( c , &commlink.first , point , out , &status, &errType, c->errStat.errBuff ); // Get units of output right
    if (status) { ppl_tbAdd(c,inExpr->srcLineN,inExpr->srcId,inExpr->srcFname,0,errType,inExprCharPos,inExpr->ascii,"diff_d?() function"); return; }
    out->real = resultReal;
    out->imag = resultImag;
    out->flagComplex = !ppl_dblEqual(resultImag, 0);
    if (!out->flagComplex) out->imag=0.0; // Enforce that real numbers have positive zero imaginary components
   }

  if ((!gsl_finite(out->real)) || (!gsl_finite(out->imag)) || ((out->flagComplex) && (c->set->term_current.ComplexNumbers == SW_ONOFF_OFF)))
   {
    if (c->set->term_current.ExplicitErrors == SW_ONOFF_ON) { sprintf(c->errStat.errBuff, "Differentiated expression does not evaluate to a finite value."); ppl_tbAdd(c,inExpr->srcLineN,inExpr->srcId,inExpr->srcFname,0,ERR_NUMERICAL,exprPos,inExpr->ascii,"diff_d?() function"); return; }
    else { out->real = GSL_NAN; out->imag = 0; out->flagComplex=0; }
   }
  return;
 }

