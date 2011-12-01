// moduleRandom.c
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
#include <stdio.h>
#include <limits.h>
#include <math.h>

#include <gsl/gsl_cdf.h>
#include <gsl/gsl_complex.h>
#include <gsl/gsl_complex_math.h>
#include <gsl/gsl_const_mksa.h>
#include <gsl/gsl_const_num.h>
#include <gsl/gsl_math.h>
#include <gsl/gsl_randist.h>
#include <gsl/gsl_rng.h>
#include <gsl/gsl_sf_erf.h>
#include <gsl/gsl_sf_gamma.h>
#include <gsl/gsl_sf_hyperg.h>

#include "coreUtils/dict.h"

#include "settings/settings.h"

#include "stringTools/asciidouble.h"

#include "userspace/pplObj.h"
#include "userspace/pplObjFunc.h"
#include "userspace/unitsArithmetic.h"

#include "defaultObjs/defaultFuncs.h"
#include "defaultObjs/defaultFuncsMacros.h"

static gsl_rng *rndgen = NULL; // Random number generator

void pplfunc_setRandomSeed(long i)
 {
  if (rndgen==NULL) rndgen = gsl_rng_alloc(gsl_rng_default);
  gsl_rng_set(rndgen, i);
  return;
 }

void pplfunc_frandom   (pplset_terminal *term, pplObj *in, int nArgs, int *status, int *errType, char *errText)
 {
  if (rndgen==NULL) { rndgen = gsl_rng_alloc(gsl_rng_default); gsl_rng_set(rndgen, 0); }
  OUTPUT.real = gsl_rng_uniform(rndgen);
 }

void pplfunc_frandombin(pplset_terminal *term, pplObj *in, int nArgs, int *status, int *errType, char *errText)
 {
  char *FunctionDescription = "random_binomial(p,n)";
  if (rndgen==NULL) { rndgen = gsl_rng_alloc(gsl_rng_default); gsl_rng_set(rndgen, 0); }
  CHECK_NEEDINT(in[1], "n", "function's second argument must be an integer in the range");
  OUTPUT.real = gsl_ran_binomial(rndgen, in[0].real, (unsigned int)in[1].real);
  CHECK_OUTPUT_OKAY;
 }

void pplfunc_frandomcs (pplset_terminal *term, pplObj *in, int nArgs, int *status, int *errType, char *errText)
 {
  char *FunctionDescription = "random_chisq(nu)";
  if (rndgen==NULL) { rndgen = gsl_rng_alloc(gsl_rng_default); gsl_rng_set(rndgen, 0); }
  OUTPUT.real = gsl_ran_chisq(rndgen, in[0].real);
  CHECK_OUTPUT_OKAY;
 }

void pplfunc_frandomg  (pplset_terminal *term, pplObj *in, int nArgs, int *status, int *errType, char *errText)
 {
  char *FunctionDescription = "random_gaussian(sigma)";
  if (rndgen==NULL) { rndgen = gsl_rng_alloc(gsl_rng_default); gsl_rng_set(rndgen, 0); }
  OUTPUT.real = gsl_ran_gaussian(rndgen, in[0].real);
  CHECK_OUTPUT_OKAY;
 }

void pplfunc_frandomln (pplset_terminal *term, pplObj *in, int nArgs, int *status, int *errType, char *errText)
 {
  char *FunctionDescription = "random_lognormal(zeta,sigma)";
  if (rndgen==NULL) { rndgen = gsl_rng_alloc(gsl_rng_default); gsl_rng_set(rndgen, 0); }
  in++;
  CHECK_1INPUT_DIMLESS; // THIS IS CORRECT. Only check in[1]
  in--;
  OUTPUT.real = gsl_ran_lognormal(rndgen, in[0].real, in[1].real);
  CHECK_OUTPUT_OKAY;
  ppl_units_DimCpy(OUTPUT, in[0]);
 }

void pplfunc_frandomp  (pplset_terminal *term, pplObj *in, int nArgs, int *status, int *errType, char *errText)
 {
  char *FunctionDescription = "random_poisson(n)";
  if (rndgen==NULL) { rndgen = gsl_rng_alloc(gsl_rng_default); gsl_rng_set(rndgen, 0); }
  OUTPUT.real = gsl_ran_poisson(rndgen, in[0].real);
  CHECK_OUTPUT_OKAY;
 }

void pplfunc_frandomt  (pplset_terminal *term, pplObj *in, int nArgs, int *status, int *errType, char *errText)
 {
  char *FunctionDescription = "random_tdist(nu)";
  if (rndgen==NULL) { rndgen = gsl_rng_alloc(gsl_rng_default); gsl_rng_set(rndgen, 0); }
  OUTPUT.real = gsl_ran_tdist(rndgen, in[0].real);
  CHECK_OUTPUT_OKAY;
 }

