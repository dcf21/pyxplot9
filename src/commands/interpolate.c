// interpolate.c
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

#define _FFT_C 1

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

#include <gsl/gsl_math.h>

#include "commands/interpolate.h"
#include "commands/interpolate_2d_engine.h"
#include "parser/parser.h"
#include "settings/settingTypes.h"
#include "userspace/context.h"
#include "userspace/pplObj_fns.h"
#include "userspace/pplObjFunc.h"
#include "userspace/unitsArithmetic.h"
#include "userspace/unitsDisp.h"

void ppl_spline_evaluate(ppl_context *c, char *FuncName, splineDescriptor *desc, pplObj *in, pplObj *out, int *status, char *errout)
 {
  double dblin, dblout;

  if (!ppl_unitsDimEqual(in, &desc->unitX))
   {
    if (c->set->term_current.ExplicitErrors == SW_ONOFF_ON) { *status=1; sprintf(errout, "The %s(x) function expects an argument with dimensions of <%s>, but has instead received an argument with dimensions of <%s>.", FuncName, ppl_printUnit(c, &desc->unitX, NULL, NULL, 0, 1, 0), ppl_printUnit(c, in, NULL, NULL, 1, 1, 0)); }
    else { pplObjNum(out,0,0,0); out->real = GSL_NAN; out->imag = 0; }
    return;
   }
  if (in->flagComplex)
   {
    if (c->set->term_current.ExplicitErrors == SW_ONOFF_ON) { *status=1; sprintf(errout, "The %s(x) function expects a real argument, but the supplied argument has an imaginary component.", FuncName); }
    else { pplObjNum(out,0,0,0); out->real = GSL_NAN; out->imag = 0; }
    return;
   }

  // If loglinear interpolation, log input value
  dblin = in->real;
  if (desc->logInterp) dblin = log(dblin);

  if (desc->splineType[1]!='t') // Not stepwise interpolation
   {
    *status = gsl_spline_eval_e(desc->splineObj, dblin, desc->accelerator, &dblout);
   }
  else // Stepwise interpolation
   {
    long          i, pos, ss, len=desc->sizeX, xystep=desc->sizeY, Nsteps = (long)ceil(log(desc->sizeX)/log(2));
    double       *data = (double *)desc->splineObj;
    for (pos=i=0; i<Nsteps; i++)
     {
      ss = 1<<(Nsteps-1-i);
      if (pos+ss>=len) continue;
      if (data[pos+ss]<=dblin) pos+=ss;
     }
    if      (data[pos]>dblin)                               dblout=data[pos  +xystep]; // Off left end
    else if (pos==len-1)                                    dblout=data[pos  +xystep]; // Off right end
    else if (fabs(dblin-data[pos])<fabs(dblin-data[pos+1])) dblout=data[pos  +xystep];
    else                                                    dblout=data[pos+1+xystep];
   }

  // Catch interpolation failure
  if (*status!=0)
   {
    if (c->set->term_current.ExplicitErrors == SW_ONOFF_ON) { *status=1; sprintf(errout, "Error whilst evaluating the %s(x) function: %s", FuncName, gsl_strerror(*status)); }
    else { *status=0; pplObjNum(out,0,0,0); out->real = GSL_NAN; out->imag = 0; }
    return;
   }

  // If loglinear interpolation, unlog output value
  if (desc->logInterp) dblout = exp(dblout);

  if (!gsl_finite(dblout))
   {
    if (c->set->term_current.ExplicitErrors == SW_ONOFF_ON) { *status=1; sprintf(errout, "Error whilst evaluating the %s(x) function: result was not a finite number.", FuncName); }
    else { pplObjNum(out,0,0,0); out->real = GSL_NAN; out->imag = 0; }
    return;
   }

  // Return output
  pplObjNum(out,0,0,0);
  out->real = dblout;
  ppl_unitsDimCpy(out, &desc->unitY);
  return;
 }


void ppl_interp2d_evaluate(ppl_context *c, const char *FuncName, splineDescriptor *desc, const pplObj *in1, const pplObj *in2, const unsigned char bmp, pplObj *out, int *status, char *errout)
 {
  double dblin1, dblin2, dblout;

  if (!ppl_unitsDimEqual(in1, &desc->unitX))
   {
    if (c->set->term_current.ExplicitErrors == SW_ONOFF_ON) { *status=1; sprintf(errout, "The %s(x,y) function expects its first argument to have dimensions of <%s>, but has instead received an argument with dimensions of <%s>.", FuncName, ppl_printUnit(c, &desc->unitX, NULL, NULL, 0, 1, 0), ppl_printUnit(c, in1, NULL, NULL, 1, 1, 0)); }
    else { pplObjNum(out,0,0,0); out->real = GSL_NAN; out->imag = 0; }
    return;
   }
  if (in1->flagComplex)
   {
    if (c->set->term_current.ExplicitErrors == SW_ONOFF_ON) { *status=1; sprintf(errout, "The %s(x,y) function expects real arguments, but first supplied argument has an imaginary component.", FuncName); }
    else { pplObjNum(out,0,0,0); out->real = GSL_NAN; out->imag = 0; }
    return;
   }

  if (!ppl_unitsDimEqual(in2, &desc->unitY))
   {
    if (c->set->term_current.ExplicitErrors == SW_ONOFF_ON) { *status=1; sprintf(errout, "The %s(x,y) function expects its second argument to have dimensions of <%s>, but has instead received an argument with dimensions of <%s>.", FuncName, ppl_printUnit(c, &desc->unitY, NULL, NULL, 0, 1, 0), ppl_printUnit(c, in2, NULL, NULL, 1, 1, 0)); }
    else { pplObjNum(out,0,0,0); out->real = GSL_NAN; out->imag = 0; }
    return;
   }
  if (in2->flagComplex)
   {
    if (c->set->term_current.ExplicitErrors == SW_ONOFF_ON) { *status=1; sprintf(errout, "The %s(x,y) function expects real arguments, but second supplied argument has an imaginary component.", FuncName); }
    else { pplObjNum(out,0,0,0); out->real = GSL_NAN; out->imag = 0; }
    return;
   }

  dblin1 = in1->real;
  dblin2 = in2->real;

  if (!bmp)
   {
    ppl_interp2d_eval(c, &dblout, &c->set->graph_current, (double *)desc->splineObj, desc->sizeX, 2, 3, dblin1, dblin2);
   } else {
    int x = floor(dblin1);
    int y = floor(dblin2);
    if ((x<0) || (x>=desc->sizeX) || (y<0) || (y>=desc->sizeY)) dblout = GSL_NAN;
    else                                                        dblout = ((double)(((unsigned char *)desc->splineObj)[x+y*desc->sizeX]))/255.0;
   }

  if (!gsl_finite(dblout))
   {
    if (c->set->term_current.ExplicitErrors == SW_ONOFF_ON) { *status=1; sprintf(errout, "Error whilst evaluating the %s(x,y) function: result was not a finite number.", FuncName); }
    else { pplObjNum(out,0,0,0); out->real = GSL_NAN; out->imag = 0; }
    return;
   }

  // Return output
  pplObjNum(out,0,0,0);
  out->real = dblout;
  ppl_unitsDimCpy(out, &desc->unitZ);
  return;
 }

