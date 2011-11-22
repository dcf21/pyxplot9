// defaultFuncs.c
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

#include "userspace/airyFuncs.h"
#include "userspace/defaultFuncs.h"
#include "userspace/defaultFuncsMacros.h"
#include "userspace/pplObj.h"
#include "userspace/pplObjFunc.h"
#include "userspace/zetaRiemann.h"

void ppl_addSystemFunc(dict *n, char *name, int minArgs, int maxArgs, int numOnly, int notNan, int realOnly, int dimlessOnly, void *fn, char *shortdesc, char *latex, char *desc)
 {
  pplObj   v;
  pplFunc *f = malloc(sizeof(pplFunc));
  if (f==NULL) return;
  f->functionType = PPL_FUNC_SYSTEM;
  f->iNodeCount   = 1;
  f->minArgs      = minArgs;
  f->maxArgs      = maxArgs;
  f->functionPtr  = fn;
  f->argList      = NULL;
  f->minActive    = NULL;
  f->maxActive    = NULL;
  f->numOnly      = numOnly;
  f->notNan       = notNan;
  f->realOnly     = realOnly;
  f->dimlessOnly  = dimlessOnly;
  f->next         = NULL;
  f->LaTeX        = latex;
  f->descriptionShort = shortdesc;
  f->description  = desc;
  pplObjZero(&v,1);
  v.objType       = PPLOBJ_FUNC;
  v.auxilLen      = sizeof(f);
  v.auxil         = (void *)f;
  ppl_dictAppendCpy(n , name , (void *)&v , sizeof(v));
  return;
 }

void pplfunc_abs         (pplObj *in, int nArgs, int *status, char *errText)
 {
  char *FunctionDescription = "abs(x)";
  IF_1COMPLEX { OUTPUT.real = hypot(in[0].real , in[0].imag); }
  ELSE_REAL   { OUTPUT.real = fabs(in[0].real); }
  ENDIF;
  ppl_units_DimCpy(&OUTPUT, in);
  CHECK_OUTPUT_OKAY;
 }

void pplfunc_acos        (pplObj *in, int nArgs, int *status, char *errText)
 {
  char *FunctionDescription = "acos(x)";
  gsl_complex z;
  IF_1COMPLEX { GSL_SET_COMPLEX(&z,in[0].real,in[0].imag); z=gsl_complex_arccos(z); }
  ELSE_REAL   { z=gsl_complex_arccos_real(in[0].real); }
  ENDIF;
  CLEANUP_GSLCOMPLEX;
  CLEANUP_APPLYUNIT(UNIT_ANGLE);
  CHECK_OUTPUT_OKAY;
 }

void pplfunc_acosh       (pplObj *in, int nArgs, int *status, char *errText)
 {
  char *FunctionDescription = "acosh(x)";
  gsl_complex z;
  IF_1COMPLEX { GSL_SET_COMPLEX(&z,in[0].real,in[0].imag); z=gsl_complex_arccosh(z); }
  ELSE_REAL   { z=gsl_complex_arccosh_real(in[0].real); }
  ENDIF;
  CHECK_OUTPUT_OKAY;
 }

void pplfunc_acot        (pplObj *in, int nArgs, int *status, char *errText)
 {
  char *FunctionDescription = "acot(x)";
  gsl_complex z;
  GSL_SET_COMPLEX(&z,in[0].real,in[0].imag); z=gsl_complex_arccot(z);
  CLEANUP_GSLCOMPLEX;
  CLEANUP_APPLYUNIT(UNIT_ANGLE);
  CHECK_OUTPUT_OKAY;
 }

void pplfunc_acoth       (pplObj *in, int nArgs, int *status, char *errText)
 {
  char *FunctionDescription = "acoth(x)";
  gsl_complex z;
  GSL_SET_COMPLEX(&z,in[0].real,in[0].imag); z=gsl_complex_arccoth(z);
  CLEANUP_GSLCOMPLEX;
  CLEANUP_APPLYUNIT(UNIT_ANGLE);
  CHECK_OUTPUT_OKAY;
 }

void pplfunc_acsc        (pplObj *in, int nArgs, int *status, char *errText)
 {
  char *FunctionDescription = "acsc(x)";
  gsl_complex z;
  IF_1COMPLEX { GSL_SET_COMPLEX(&z,in[0].real,in[0].imag); z=gsl_complex_arccsc(z); }
  ELSE_REAL   { z=gsl_complex_arccsc_real(in[0].real); }
  ENDIF;
  CLEANUP_GSLCOMPLEX;
  CLEANUP_APPLYUNIT(UNIT_ANGLE);
  CHECK_OUTPUT_OKAY;
 }

void pplfunc_acsch       (pplObj *in, int nArgs, int *status, char *errText)
 {
  char *FunctionDescription = "acsch(x)";
  gsl_complex z;
  GSL_SET_COMPLEX(&z,in[0].real,in[0].imag); z=gsl_complex_arccsch(z);
  CLEANUP_GSLCOMPLEX;
  CLEANUP_APPLYUNIT(UNIT_ANGLE);
  CHECK_OUTPUT_OKAY;
 }

void pplfunc_airy_ai     (pplObj *in, int nArgs, int *status, char *errText)
 {
  char *FunctionDescription = "airy_ai(x)";
  gsl_complex zi, z;
  GSL_SET_COMPLEX(&zi,in[0].real,in[0].imag);
  airy_ai(zi,&z,status,errText);
  if (*status) CHECK_OUTPUT_OKAY;
  CLEANUP_GSLCOMPLEX;
  CHECK_OUTPUT_OKAY;
 }

void pplfunc_airy_ai_diff(pplObj *in, int nArgs, int *status, char *errText)
 {
  char *FunctionDescription = "airy_ai_diff(x)";
  gsl_complex zi,z;
  GSL_SET_COMPLEX(&zi,in[0].real,in[0].imag);
  airy_ai_diff(zi,&z,status,errText);
  if (*status) return;
  CLEANUP_GSLCOMPLEX;
  CHECK_OUTPUT_OKAY;
 }

void pplfunc_airy_bi     (pplObj *in, int nArgs, int *status, char *errText)
 {
  char *FunctionDescription = "airy_bi(x)";
  gsl_complex zi,z;
  GSL_SET_COMPLEX(&zi,in[0].real,in[0].imag);
  airy_bi(zi,&z,status,errText);
  if (*status) return;
  CLEANUP_GSLCOMPLEX;
  CHECK_OUTPUT_OKAY;
 }

void pplfunc_airy_bi_diff(pplObj *in, int nArgs, int *status, char *errText)
 {
  char *FunctionDescription = "airy_bi_diff(x)";
  gsl_complex zi,z;
  GSL_SET_COMPLEX(&zi,in[0].real,in[0].imag);
  airy_bi_diff(zi,&z,status,errText);
  if (*status) return;
  CLEANUP_GSLCOMPLEX;
  CHECK_OUTPUT_OKAY;
 }

void pplfunc_arg         (pplObj *in, int nArgs, int *status, char *errText)
 {
  char *FunctionDescription = "arg(x)";
  gsl_complex z;
  GSL_SET_COMPLEX(&z,in[0].real,in[0].imag);
  OUTPUT.real = gsl_complex_arg(z);
  CLEANUP_APPLYUNIT(UNIT_ANGLE);
  CHECK_OUTPUT_OKAY;
 }

void pplfunc_asec        (pplObj *in, int nArgs, int *status, char *errText)
 {
  char *FunctionDescription = "asec(x)";
  gsl_complex z;
  IF_1COMPLEX { GSL_SET_COMPLEX(&z,in[0].real,in[0].imag); z=gsl_complex_arcsec(z); }
  ELSE_REAL   { z=gsl_complex_arcsec_real(in[0].real); }
  ENDIF;
  CLEANUP_GSLCOMPLEX;
  CLEANUP_APPLYUNIT(UNIT_ANGLE);
  CHECK_OUTPUT_OKAY;
 }

void pplfunc_asech       (pplObj *in, int nArgs, int *status, char *errText)
 {
  char *FunctionDescription = "asech(x)";
  gsl_complex z;
  GSL_SET_COMPLEX(&z,in[0].real,in[0].imag); z=gsl_complex_arcsech(z);
  CLEANUP_GSLCOMPLEX;
  CLEANUP_APPLYUNIT(UNIT_ANGLE);
  CHECK_OUTPUT_OKAY;
 }

void pplfunc_asin        (pplObj *in, int nArgs, int *status, char *errText)
 {
  char *FunctionDescription = "asin(x)";
  gsl_complex z;
  IF_1COMPLEX { GSL_SET_COMPLEX(&z,in[0].real,in[0].imag); z=gsl_complex_arcsin(z); }
  ELSE_REAL   { z=gsl_complex_arcsin_real(in[0].real); }
  ENDIF;
  CLEANUP_GSLCOMPLEX;
  CLEANUP_APPLYUNIT(UNIT_ANGLE);
  CHECK_OUTPUT_OKAY;
 }

void pplfunc_asinh       (pplObj *in, int nArgs, int *status, char *errText)
 {
  char *FunctionDescription = "asinh(x)";
  gsl_complex z;
  GSL_SET_COMPLEX(&z,in[0].real,in[0].imag); z=gsl_complex_arcsinh(z);
  CLEANUP_GSLCOMPLEX;
  CLEANUP_APPLYUNIT(UNIT_ANGLE);
  CHECK_OUTPUT_OKAY;
 }

void pplfunc_atan        (pplObj *in, int nArgs, int *status, char *errText)
 {
  char *FunctionDescription = "atan(x)";
  gsl_complex z;
  GSL_SET_COMPLEX(&z,in[0].real,in[0].imag); z=gsl_complex_arctan(z);
  CLEANUP_GSLCOMPLEX;
  CLEANUP_APPLYUNIT(UNIT_ANGLE);
  CHECK_OUTPUT_OKAY;
 }

void pplfunc_atanh       (pplObj *in, int nArgs, int *status, char *errText)
 {
  char *FunctionDescription = "atanh(x)";
  gsl_complex z;
  IF_1COMPLEX { GSL_SET_COMPLEX(&z,in[0].real,in[0].imag); z=gsl_complex_arctanh(z); }
  ELSE_REAL   { z=gsl_complex_arctanh_real(in[0].real); }
  ENDIF;
  CLEANUP_GSLCOMPLEX;
  CLEANUP_APPLYUNIT(UNIT_ANGLE);
  CHECK_OUTPUT_OKAY;
 }

