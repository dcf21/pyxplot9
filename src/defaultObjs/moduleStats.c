// moduleStats.c
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
#include <gsl/gsl_sf_bessel.h>
#include <gsl/gsl_sf_ellint.h>
#include <gsl/gsl_sf_elljac.h>
#include <gsl/gsl_sf_erf.h>
#include <gsl/gsl_sf_expint.h>
#include <gsl/gsl_sf_gamma.h>
#include <gsl/gsl_sf_hyperg.h>
#include <gsl/gsl_sf_lambert.h>
#include <gsl/gsl_sf_legendre.h>
#include <gsl/gsl_sf_zeta.h>

#include "coreUtils/dict.h"

#include "settings/settings.h"

#include "stringTools/asciidouble.h"

#include "userspace/pplObj.h"
#include "userspace/pplObjFunc.h"
#include "userspace/unitsArithmetic.h"

#include "defaultObjs/moduleStats.h"
#include "defaultObjs/defaultFuncs.h"
#include "defaultObjs/defaultFuncsMacros.h"

void pplfunc_binomialPDF  (ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText)
 {
  char *FunctionDescription = "binomialPDF(k,p,n)";
  CHECK_NEEDINT(in[0] , "k", "function's first parameter must be");
  CHECK_NEEDINT(in[2] , "n", "function's 3rd  parameter must be");
  OUTPUT.real = gsl_ran_binomial_pdf((unsigned int)in[0].real, in[1].real, (unsigned int)in[2].real);
  CHECK_OUTPUT_OKAY;
 }

void pplfunc_binomialCDF  (ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText)
 {
  char *FunctionDescription = "binomialCDF(k,p,n)";
  CHECK_NEEDINT(in[0] , "k", "function's first parameter must be");
  CHECK_NEEDINT(in[2] , "n", "function's 3rd  parameter must be");
  OUTPUT.real = gsl_cdf_binomial_P((unsigned int)in[0].real, in[1].real, (unsigned int)in[2].real);
  CHECK_OUTPUT_OKAY;
 }

void pplfunc_chisqPDF     (ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText)
 {
  char *FunctionDescription = "chisqPDF(x,nu)";
  OUTPUT.real = gsl_ran_chisq_pdf(in[0].real , in[1].real);
  CHECK_OUTPUT_OKAY;
 }

void pplfunc_chisqCDF     (ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText)
 {
  char *FunctionDescription = "chisqCDF(x,nu)";
  OUTPUT.real = gsl_cdf_chisq_P(in[0].real , in[1].real);
  CHECK_OUTPUT_OKAY;
 }

void pplfunc_chisqCDFi    (ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText)
 {
  char *FunctionDescription = "chisqCDFi(P,nu)";
  OUTPUT.real = gsl_cdf_chisq_Pinv(in[0].real , in[1].real);
  CHECK_OUTPUT_OKAY;
 }

void pplfunc_gaussianPDF  (ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText)
 {
  char *FunctionDescription = "gaussianPDF(x,sigma)";
  CHECK_2INPUT_DIMMATCH;
  OUTPUT.real = gsl_ran_gaussian_pdf(in[0].real, in[1].real);
  CHECK_OUTPUT_OKAY;
  ppl_unitsDimInverse(&OUTPUT, &in[0]);
 }

void pplfunc_gaussianCDF  (ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText)
 {
  char *FunctionDescription = "gaussianCDF(x,sigma)";
  CHECK_2INPUT_DIMMATCH;
  OUTPUT.real = gsl_cdf_gaussian_P(in[0].real, in[1].real);
  CHECK_OUTPUT_OKAY;
 }

void pplfunc_gaussianCDFi (ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText)
 {
  char *FunctionDescription = "gaussianCDFi(x,sigma)";
  CHECK_1INPUT_DIMLESS;
  OUTPUT.real = gsl_cdf_gaussian_Pinv(in[0].real, in[1].real);
  CHECK_OUTPUT_OKAY;
  ppl_unitsDimCpy(&OUTPUT, &in[1]);
 }

void pplfunc_lognormalPDF (ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText)
 {
  char *FunctionDescription = "lognormalPDF(x,zeta,sigma)";
  in+=2;
  CHECK_1INPUT_DIMLESS;
  in-=2;
  CHECK_2INPUT_DIMMATCH;
  OUTPUT.real = gsl_ran_lognormal_pdf(in[0].real, in[1].real, in[2].real);
  CHECK_OUTPUT_OKAY;
  ppl_unitsDimInverse(&OUTPUT, &in[0]);
 }

void pplfunc_lognormalCDF (ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText)
 {
  char *FunctionDescription = "lognormalCDF(x,zeta,sigma)";
  in+=2;
  CHECK_1INPUT_DIMLESS;
  in-=2;
  CHECK_2INPUT_DIMMATCH;
  OUTPUT.real = gsl_cdf_lognormal_P(in[0].real, in[1].real, in[2].real);
  CHECK_OUTPUT_OKAY;
 }

void pplfunc_lognormalCDFi(ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText)
 {
  char *FunctionDescription = "lognormalCDFi(x,zeta,sigma)";
  in+=2;
  CHECK_1INPUT_DIMLESS;
  in-=2;
  CHECK_2INPUT_DIMMATCH;
  OUTPUT.real = gsl_cdf_lognormal_Pinv(in[0].real, in[1].real, in[2].real);
  CHECK_OUTPUT_OKAY;
  ppl_unitsDimCpy(&OUTPUT, &in[1]);
 }

void pplfunc_poissonPDF   (ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText)
 {
  char *FunctionDescription = "poissonPDF(x,mu)";
  OUTPUT.real = gsl_ran_poisson_pdf(in[0].real , in[1].real);
  CHECK_OUTPUT_OKAY;
 }

void pplfunc_poissonCDF   (ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText)
 {
  char *FunctionDescription = "poissonCDF(x,mu)";
  OUTPUT.real = gsl_cdf_poisson_P(in[0].real , in[1].real);
  CHECK_OUTPUT_OKAY;
 }

void pplfunc_tdistPDF     (ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText)
 {
  char *FunctionDescription = "tdistPDF(x,nu)";
  OUTPUT.real = gsl_ran_tdist_pdf(in[0].real , in[1].real);
  CHECK_OUTPUT_OKAY;
 }

void pplfunc_tdistCDF     (ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText)
 {
  char *FunctionDescription = "tdistCDF(x,nu)";
  OUTPUT.real = gsl_cdf_tdist_P(in[0].real , in[1].real);
  CHECK_OUTPUT_OKAY;
 }

void pplfunc_tdistCDFi    (ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText)
 {
  char *FunctionDescription = "tdistCDFi(P,nu)";
  OUTPUT.real = gsl_cdf_tdist_Pinv(in[0].real , in[1].real);
  CHECK_OUTPUT_OKAY;
 }

