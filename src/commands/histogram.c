// histogram.c
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

#define _HISTOGRAM_C 1

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

#include <gsl/gsl_math.h>

#include "commands/histogram.h"
#include "parser/parser.h"
#include "settings/settingTypes.h"
#include "userspace/context.h"
#include "userspace/pplObj_fns.h"
#include "userspace/pplObjFunc.h"
#include "userspace/unitsArithmetic.h"
#include "userspace/unitsDisp.h"

void ppl_histogram_evaluate(ppl_context *c, char *FuncName, histogramDescriptor *desc, pplObj *in, pplObj *out, int *status, char *errout)
 {
  long   i, Nsteps, len, ss, pos;
  double dblin;

  if (!ppl_unitsDimEqual(in, &desc->unit))
   {
    if (c->set->term_current.ExplicitErrors == SW_ONOFF_ON) { sprintf(errout, "The %s(x) function expects an argument with dimensions of <%s>, but has instead received an argument with dimensions of <%s>.", FuncName, ppl_printUnit(c, &desc->unit, NULL, NULL, 0, 1, 0), ppl_printUnit(c, in, NULL, NULL, 1, 1, 0)); }
    else { pplObjNum(out,0,0,0); out->real = GSL_NAN; out->imag = 0; }
    *status=1;
    return;
   }
  if (in->flagComplex)
   {
    if (c->set->term_current.ExplicitErrors == SW_ONOFF_ON) { sprintf(errout, "The %s(x) function expects a real argument, but the supplied argument has an imaginary component.", FuncName); }
    else { pplObjNum(out,0,0,0); out->real = GSL_NAN; out->imag = 0; }
    *status=1;
    return;
   }

  dblin = in->real;

  *out = desc->unit;
  out->imag = 0.0;
  out->real = 0.0;
  out->flagComplex = 0;
  for (i=0; i<UNITS_MAX_BASEUNITS; i++) if (out->exponent[i]!=0.0) out->exponent[i]*=-1; // Output has units of 'per x'
  if ((desc->Nbins<1) || (dblin<desc->bins[0]) || (dblin>desc->bins[desc->Nbins-1])) return; // Query is outside range of histogram

  len    = desc->Nbins;
  Nsteps = (long)ceil(log(len)/log(2));
  for (pos=i=0; i<Nsteps; i++)
   {
    ss = 1<<(Nsteps-1-i);
    if (pos+ss>=len) continue;
    if (desc->bins[pos+ss]<=dblin) pos+=ss;
   }
  if      (desc->bins[pos]>dblin) return; // Off left end
  else if (pos==len-1)            return; // Off right end

  out->real = desc->binvals[pos];
  return;
 }

