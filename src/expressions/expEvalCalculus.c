// ppl_calculus.c
//
// The code in this file is part of PyXPlot
// <http://www.pyxplot.org.uk>
//
// Copyright (C) 2006-2011 Dominic Ford <coders@pyxplot.org.uk>
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

#define _PPL_CALCULUS_C 1

#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <string.h>

#include <gsl/gsl_deriv.h>
#include <gsl/gsl_integration.h>
#include <gsl/gsl_math.h>

#include "ppl_settings.h"
#include "ppl_setting_types.h"
#include "ppl_units.h"
#include "ppl_units_fns.h"
#include "ppl_userspace.h"

typedef struct IntComm {
 char         *expr;
 value        *dummy;
 value         first;
 double        dummyReal, dummyImag;
 unsigned char IsFirst;
 unsigned char testingReal, varyingReal;
 int          *errPos;
 char         *errText;
 int           recursionDepth;
 } IntComm;

double CalculusSlave(double x, void *params)
 {
  value output;
  IntComm *data = (IntComm *)params;

  if (*(data->errPos)>=0) return GSL_NAN; // We've previously had an error... so don't do any more work

  if (data->varyingReal) { data->dummy->real = x; data->dummy->imag = data->dummyImag; data->dummy->flagComplex = !ppl_units_DblEqual(data->dummy->imag,0); }
  else                   { data->dummy->imag = x; data->dummy->real = data->dummyReal; data->dummy->flagComplex = !ppl_units_DblEqual(data->dummy->imag,0); }

  ppl_EvaluateAlgebra(data->expr, &output, 0, NULL, 0, data->errPos, data->errText, data->recursionDepth+1);
  if (*(data->errPos)>=0) return GSL_NAN;

  if (data->IsFirst)
   {
    memcpy(&data->first, &output, sizeof(value));
    data->IsFirst = 0;
   } else {
    if (!ppl_units_DimEqual(&data->first,&output))
     {
      *(data->errPos)=0;
      strcpy(data->errText, "This operand does not have consistent units across the range where calculus is being attempted.");
      return GSL_NAN;
     }
   }

  // Integrand was complex, but complex arithmetic is turned off
  if ((!ppl_units_DblEqual(output.imag, 0)) && (c->set->term_current.ComplexNumbers == SW_ONOFF_OFF)) return GSL_NAN;

  if (data->testingReal) return output.real;
  else                   return output.imag;
 }

void ppl_expIntegrate    (char *expr, char *dummy, pplObj *min  , pplObj *max , pplObj *out, int *errPos, int *errType, char *errText);
 {
  IntComm                    commlink;
  value                     *dummyVar;
  value                      dummyTemp;
  gsl_integration_workspace *ws;
  gsl_function               fn;
  double                     resultReal=0, resultImag=0, error;

  if (!ppl_units_DimEqual(min,max))
   {
    *errPos=0;
    strcpy(errText, "The minimum and maximum limits of this integration operation are not dimensionally compatible.");
    return;
   }

  if (min->flagComplex || max->flagComplex)
   {
    *errPos=0;
    strcpy(errText, "The minimum and maximum limits of this integration operation must be real numbers; supplied values are complex.");
    return;
   }

  commlink.expr    = expr;
  commlink.IsFirst = 1;
  commlink.testingReal = 1;
  commlink.varyingReal = 1;
  commlink.errPos  = errPos;
  commlink.errText = errText;
  commlink.recursionDepth = recursionDepth;
  ppl_units_zero(&commlink.first);

  ppl_UserSpace_GetVarPointer(dummy, &dummyVar, &dummyTemp);
  memcpy(dummyVar, min, sizeof(value)); // Get units of dummyVar right
  commlink.dummy     = dummyVar;
  commlink.dummyReal = dummyVar->real;
  commlink.dummyImag = dummyVar->imag;

  ws          = gsl_integration_workspace_alloc(1000);
  fn.function = &CalculusSlave;
  fn.params   = &commlink;

  gsl_integration_qags (&fn, min->real, max->real, 0, 1e-7, 1000, ws, &resultReal, &error);

  if ((*errPos < 0) && (c->set->term_current.ComplexNumbers == SW_ONOFF_ON))
   {
    commlink.testingReal = 0;
    gsl_integration_qags (&fn, min->real, max->real, 0, 1e-7, 1000, ws, &resultImag, &error);
   }

  gsl_integration_workspace_free(ws);

  ppl_UserSpace_RestoreVarPointer(&dummyVar, &dummyTemp); // Restore old value of the dummy variable we've been using

  if (*errPos < 0)
   {
    ppl_units_mult( &commlink.first , min , out , errPos, errText ); // Get units of output right
    out->real = resultReal;
    out->imag = resultImag;
    out->flagComplex = !ppl_units_DblEqual(resultImag, 0);
    if (!out->flagComplex) out->imag=0.0; // Enforce that real numbers have positive zero imaginary components

    if ((!gsl_finite(out->real)) || (!gsl_finite(out->imag)) || ((out->flagComplex) && (c->set->term_current.ComplexNumbers == SW_ONOFF_OFF)))
     {
      if (c->set->term_current.ExplicitErrors == SW_ONOFF_ON) { *errPos=0; sprintf(errText, "Integral does not evaluate to a finite value."); return; }
      else { out->real = GSL_NAN; out->imag = 0; out->flagComplex=0; }
     }
   }
  return;
 }

void ppl_expDifferentiate(char *expr, char *dummy, pplObj *point, pplObj *step, pplObj *out, int *errPos, int *errType, char *errText);
 {
  IntComm                    commlink;
  value                     *dummyVar;
  value                      dummyTemp;
  gsl_function               fn;
  double                     resultReal=0, resultImag=0, dIdI, dRdI;
  double                     resultReal_error, resultImag_error, dIdI_error, dRdI_error;

  if (!ppl_units_DimEqual(point, step))
   {
    *errPos=0;
    strcpy(errText, "The arguments x and step to this differentiation operation are not dimensionally compatible.");
    return;
   }

  if (step->flagComplex)
   {
    *errPos=0;
    strcpy(errText, "The argument 'step' to this differentiation operation must be a real number; supplied value is complex.");
    return;
   }

  commlink.expr    = expr;
  commlink.IsFirst = 1;
  commlink.testingReal = 1;
  commlink.varyingReal = 1;
  commlink.errPos  = errPos;
  commlink.errText = errText;
  commlink.recursionDepth = recursionDepth;
  ppl_units_zero(&commlink.first);

  ppl_UserSpace_GetVarPointer(dummy, &dummyVar, &dummyTemp);
  memcpy(dummyVar, point, sizeof(value)); // Get units of dummyVar right
  commlink.dummy     = dummyVar;
  commlink.dummyReal = dummyVar->real;
  commlink.dummyImag = dummyVar->imag;

  fn.function = &CalculusSlave;
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

    if ((!ppl_units_DblApprox(resultReal, dIdI, 2*(resultReal_error+dIdI_error))) || (!ppl_units_DblApprox(resultImag, -dRdI, 2*(resultImag_error+dRdI_error))))
     { *errPos = 0; sprintf(errText, "The Cauchy-Riemann equations are not satisfied at this point in the complex plane. It does not therefore appear possible to perform complex differentiation. In the notation f(x+iy)=u+iv, the offending derivatives were: du/dx=%e, dv/dy=%e, du/dy=%e and dv/dx=%e.", resultReal, dIdI, dRdI, resultImag); return; }
   }

  ppl_UserSpace_RestoreVarPointer(&dummyVar, &dummyTemp); // Restore old value of the dummy variable we've been using

  if (*errPos < 0)
   {
    point->real = 1.0; point->imag = 0.0; point->flagComplex = 0;
    ppl_units_div( &commlink.first , point , out , errPos, errText ); // Get units of output right
    out->real = resultReal;
    out->imag = resultImag;
    out->flagComplex = !ppl_units_DblEqual(resultImag, 0);
    if (!out->flagComplex) out->imag=0.0; // Enforce that real numbers have positive zero imaginary components
   }

  if ((!gsl_finite(out->real)) || (!gsl_finite(out->imag)) || ((out->flagComplex) && (c->set->term_current.ComplexNumbers == SW_ONOFF_OFF)))
   {
    if (c->set->term_current.ExplicitErrors == SW_ONOFF_ON) { *errPos=0; sprintf(errText, "Differential does not evaluate to a finite value."); return; }
    else { out->real = GSL_NAN; out->imag = 0; out->flagComplex=0; }
   }
  return;
 }

