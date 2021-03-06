// moduleRandom.c
//
// The code in this file is part of Pyxplot
// <http://www.pyxplot.org.uk>
//
// Copyright (C) 2006-2013 Dominic Ford <coders@pyxplot.org.uk>
//               2008-2013 Ross Church
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

#include "defaultObjs/moduleRandom.h"
#include "defaultObjs/defaultFuncs.h"
#include "defaultObjs/defaultFuncsMacros.h"

static gsl_rng *rndgen = NULL; // Random number generator

void pplfunc_setRandomSeed(long i)
 {
  if (rndgen==NULL) rndgen = gsl_rng_alloc(gsl_rng_default);
  gsl_rng_set(rndgen, i);
  return;
 }

void pplfunc_frandom   (ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText)
 {
  if (rndgen==NULL) { rndgen = gsl_rng_alloc(gsl_rng_default); gsl_rng_set(rndgen, 0); }
  OUTPUT.real = gsl_rng_uniform(rndgen);
 }

void pplfunc_frandombin(ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText)
 {
  char *FunctionDescription = "binomial(p,n)";
  if (rndgen==NULL) { rndgen = gsl_rng_alloc(gsl_rng_default); gsl_rng_set(rndgen, 0); }
  CHECK_NEEDINT(in[1], "n", "function's second argument must be an integer in the range");
  OUTPUT.real = gsl_ran_binomial(rndgen, in[0].real, (unsigned int)in[1].real);
  CHECK_OUTPUT_OKAY;
 }

void pplfunc_frandomcs (ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText)
 {
  char *FunctionDescription = "chisq(nu)";
  if (rndgen==NULL) { rndgen = gsl_rng_alloc(gsl_rng_default); gsl_rng_set(rndgen, 0); }
  OUTPUT.real = gsl_ran_chisq(rndgen, in[0].real);
  CHECK_OUTPUT_OKAY;
 }

void pplfunc_frandomg  (ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText)
 {
  char *FunctionDescription = "gaussian(sigma)";
  if (rndgen==NULL) { rndgen = gsl_rng_alloc(gsl_rng_default); gsl_rng_set(rndgen, 0); }
  OUTPUT.real = gsl_ran_gaussian(rndgen, in[0].real);
  CHECK_OUTPUT_OKAY;
 }

void pplfunc_frandomln (ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText)
 {
  char *FunctionDescription = "lognormal(zeta,sigma)";
  if (rndgen==NULL) { rndgen = gsl_rng_alloc(gsl_rng_default); gsl_rng_set(rndgen, 0); }
  in++;
  CHECK_1INPUT_DIMLESS; // THIS IS CORRECT. Only check in[1]
  in--;
  OUTPUT.real = gsl_ran_lognormal(rndgen, in[0].real, in[1].real);
  CHECK_OUTPUT_OKAY;
  ppl_unitsDimCpy(&OUTPUT, &in[0]);
 }

void pplfunc_frandomp  (ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText)
 {
  char *FunctionDescription = "poisson(n)";
  if (rndgen==NULL) { rndgen = gsl_rng_alloc(gsl_rng_default); gsl_rng_set(rndgen, 0); }
  OUTPUT.real = gsl_ran_poisson(rndgen, in[0].real);
  CHECK_OUTPUT_OKAY;
 }

void pplfunc_frandomt  (ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText)
 {
  char *FunctionDescription = "tdist(nu)";
  if (rndgen==NULL) { rndgen = gsl_rng_alloc(gsl_rng_default); gsl_rng_set(rndgen, 0); }
  OUTPUT.real = gsl_ran_tdist(rndgen, in[0].real);
  CHECK_OUTPUT_OKAY;
 }

