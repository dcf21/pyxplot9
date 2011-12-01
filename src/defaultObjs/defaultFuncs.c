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
#include <string.h>

#include <gsl/gsl_cdf.h>
#include <gsl/gsl_complex.h>
#include <gsl/gsl_complex_math.h>
#include <gsl/gsl_const_mksa.h>
#include <gsl/gsl_const_num.h>
#include <gsl/gsl_math.h>
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

#include "defaultObjs/airyFuncs.h"
#include "defaultObjs/defaultFuncs.h"
#include "defaultObjs/defaultFuncsMacros.h"
#include "defaultObjs/zetaRiemann.h"

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

void pplfunc_abs         (pplset_terminal *term, pplObj *in, int nArgs, int *status, int *errType, char *errText)
 {
  char *FunctionDescription = "abs(z)";
  IF_1COMPLEX { OUTPUT.real = hypot(in[0].real , in[0].imag); }
  ELSE_REAL   { OUTPUT.real = fabs(in[0].real); }
  ENDIF;
  ppl_unitsDimCpy(&OUTPUT, in);
  CHECK_OUTPUT_OKAY;
 }

void pplfunc_acos        (pplset_terminal *term, pplObj *in, int nArgs, int *status, int *errType, char *errText)
 {
  char *FunctionDescription = "acos(z)";
  gsl_complex z;
  IF_1COMPLEX { GSL_SET_COMPLEX(&z,in[0].real,in[0].imag); z=gsl_complex_arccos(z); }
  ELSE_REAL   { z=gsl_complex_arccos_real(in[0].real); }
  ENDIF;
  CLEANUP_GSLCOMPLEX;
  CLEANUP_APPLYUNIT(UNIT_ANGLE);
  CHECK_OUTPUT_OKAY;
 }

void pplfunc_acosh       (pplset_terminal *term, pplObj *in, int nArgs, int *status, int *errType, char *errText)
 {
  char *FunctionDescription = "acosh(z)";
  gsl_complex z;
  IF_1COMPLEX { GSL_SET_COMPLEX(&z,in[0].real,in[0].imag); z=gsl_complex_arccosh(z); }
  ELSE_REAL   { z=gsl_complex_arccosh_real(in[0].real); }
  ENDIF;
  CHECK_OUTPUT_OKAY;
 }

void pplfunc_acot        (pplset_terminal *term, pplObj *in, int nArgs, int *status, int *errType, char *errText)
 {
  char *FunctionDescription = "acot(z)";
  gsl_complex z;
  GSL_SET_COMPLEX(&z,in[0].real,in[0].imag); z=gsl_complex_arccot(z);
  CLEANUP_GSLCOMPLEX;
  CLEANUP_APPLYUNIT(UNIT_ANGLE);
  CHECK_OUTPUT_OKAY;
 }

void pplfunc_acoth       (pplset_terminal *term, pplObj *in, int nArgs, int *status, int *errType, char *errText)
 {
  char *FunctionDescription = "acoth(z)";
  gsl_complex z;
  GSL_SET_COMPLEX(&z,in[0].real,in[0].imag); z=gsl_complex_arccoth(z);
  CLEANUP_GSLCOMPLEX;
  CLEANUP_APPLYUNIT(UNIT_ANGLE);
  CHECK_OUTPUT_OKAY;
 }

void pplfunc_acsc        (pplset_terminal *term, pplObj *in, int nArgs, int *status, int *errType, char *errText)
 {
  char *FunctionDescription = "acsc(z)";
  gsl_complex z;
  IF_1COMPLEX { GSL_SET_COMPLEX(&z,in[0].real,in[0].imag); z=gsl_complex_arccsc(z); }
  ELSE_REAL   { z=gsl_complex_arccsc_real(in[0].real); }
  ENDIF;
  CLEANUP_GSLCOMPLEX;
  CLEANUP_APPLYUNIT(UNIT_ANGLE);
  CHECK_OUTPUT_OKAY;
 }

void pplfunc_acsch       (pplset_terminal *term, pplObj *in, int nArgs, int *status, int *errType, char *errText)
 {
  char *FunctionDescription = "acsch(z)";
  gsl_complex z;
  GSL_SET_COMPLEX(&z,in[0].real,in[0].imag); z=gsl_complex_arccsch(z);
  CLEANUP_GSLCOMPLEX;
  CLEANUP_APPLYUNIT(UNIT_ANGLE);
  CHECK_OUTPUT_OKAY;
 }

void pplfunc_airy_ai     (pplset_terminal *term, pplObj *in, int nArgs, int *status, int *errType, char *errText)
 {
  char *FunctionDescription = "airy_ai(z)";
  gsl_complex zi, z;
  GSL_SET_COMPLEX(&zi,in[0].real,in[0].imag);
  airy_ai(zi,&z,status,errText);
  if (*status) { *errType = ERR_NUMERIC; return; }
  CLEANUP_GSLCOMPLEX;
  CHECK_OUTPUT_OKAY;
 }

void pplfunc_airy_ai_diff(pplset_terminal *term, pplObj *in, int nArgs, int *status, int *errType, char *errText)
 {
  char *FunctionDescription = "airy_ai_diff(z)";
  gsl_complex zi,z;
  GSL_SET_COMPLEX(&zi,in[0].real,in[0].imag);
  airy_ai_diff(zi,&z,status,errText);
  if (*status) { *errType = ERR_NUMERIC; return; }
  CLEANUP_GSLCOMPLEX;
  CHECK_OUTPUT_OKAY;
 }

void pplfunc_airy_bi     (pplset_terminal *term, pplObj *in, int nArgs, int *status, int *errType, char *errText)
 {
  char *FunctionDescription = "airy_bi(z)";
  gsl_complex zi,z;
  GSL_SET_COMPLEX(&zi,in[0].real,in[0].imag);
  airy_bi(zi,&z,status,errText);
  if (*status) { *errType = ERR_NUMERIC; return; }
  CLEANUP_GSLCOMPLEX;
  CHECK_OUTPUT_OKAY;
 }

void pplfunc_airy_bi_diff(pplset_terminal *term, pplObj *in, int nArgs, int *status, int *errType, char *errText)
 {
  char *FunctionDescription = "airy_bi_diff(z)";
  gsl_complex zi,z;
  GSL_SET_COMPLEX(&zi,in[0].real,in[0].imag);
  airy_bi_diff(zi,&z,status,errText);
  if (*status) { *errType = ERR_NUMERIC; return; }
  CLEANUP_GSLCOMPLEX;
  CHECK_OUTPUT_OKAY;
 }

void pplfunc_arg         (pplset_terminal *term, pplObj *in, int nArgs, int *status, int *errType, char *errText)
 {
  char *FunctionDescription = "arg(z)";
  gsl_complex z;
  GSL_SET_COMPLEX(&z,in[0].real,in[0].imag);
  OUTPUT.real = gsl_complex_arg(z);
  CLEANUP_APPLYUNIT(UNIT_ANGLE);
  CHECK_OUTPUT_OKAY;
 }

void pplfunc_asec        (pplset_terminal *term, pplObj *in, int nArgs, int *status, int *errType, char *errText)
 {
  char *FunctionDescription = "asec(z)";
  gsl_complex z;
  IF_1COMPLEX { GSL_SET_COMPLEX(&z,in[0].real,in[0].imag); z=gsl_complex_arcsec(z); }
  ELSE_REAL   { z=gsl_complex_arcsec_real(in[0].real); }
  ENDIF;
  CLEANUP_GSLCOMPLEX;
  CLEANUP_APPLYUNIT(UNIT_ANGLE);
  CHECK_OUTPUT_OKAY;
 }

void pplfunc_asech       (pplset_terminal *term, pplObj *in, int nArgs, int *status, int *errType, char *errText)
 {
  char *FunctionDescription = "asech(z)";
  gsl_complex z;
  GSL_SET_COMPLEX(&z,in[0].real,in[0].imag); z=gsl_complex_arcsech(z);
  CLEANUP_GSLCOMPLEX;
  CLEANUP_APPLYUNIT(UNIT_ANGLE);
  CHECK_OUTPUT_OKAY;
 }

void pplfunc_asin        (pplset_terminal *term, pplObj *in, int nArgs, int *status, int *errType, char *errText)
 {
  char *FunctionDescription = "asin(z)";
  gsl_complex z;
  IF_1COMPLEX { GSL_SET_COMPLEX(&z,in[0].real,in[0].imag); z=gsl_complex_arcsin(z); }
  ELSE_REAL   { z=gsl_complex_arcsin_real(in[0].real); }
  ENDIF;
  CLEANUP_GSLCOMPLEX;
  CLEANUP_APPLYUNIT(UNIT_ANGLE);
  CHECK_OUTPUT_OKAY;
 }

void pplfunc_asinh       (pplset_terminal *term, pplObj *in, int nArgs, int *status, int *errType, char *errText)
 {
  char *FunctionDescription = "asinh(z)";
  gsl_complex z;
  GSL_SET_COMPLEX(&z,in[0].real,in[0].imag); z=gsl_complex_arcsinh(z);
  CLEANUP_GSLCOMPLEX;
  CLEANUP_APPLYUNIT(UNIT_ANGLE);
  CHECK_OUTPUT_OKAY;
 }

void pplfunc_atan        (pplset_terminal *term, pplObj *in, int nArgs, int *status, int *errType, char *errText)
 {
  char *FunctionDescription = "atan(z)";
  gsl_complex z;
  GSL_SET_COMPLEX(&z,in[0].real,in[0].imag); z=gsl_complex_arctan(z);
  CLEANUP_GSLCOMPLEX;
  CLEANUP_APPLYUNIT(UNIT_ANGLE);
  CHECK_OUTPUT_OKAY;
 }

void pplfunc_atanh       (pplset_terminal *term, pplObj *in, int nArgs, int *status, int *errType, char *errText)
 {
  char *FunctionDescription = "atanh(z)";
  gsl_complex z;
  IF_1COMPLEX { GSL_SET_COMPLEX(&z,in[0].real,in[0].imag); z=gsl_complex_arctanh(z); }
  ELSE_REAL   { z=gsl_complex_arctanh_real(in[0].real); }
  ENDIF;
  CLEANUP_GSLCOMPLEX;
  CLEANUP_APPLYUNIT(UNIT_ANGLE);
  CHECK_OUTPUT_OKAY;
 }

void pplfunc_atan2       (pplset_terminal *term, pplObj *in, int nArgs, int *status, int *errType, char *errText)
 {
  char *FunctionDescription = "atan2(x,y)";
  CHECK_2INPUT_DIMMATCH;
  OUTPUT.real = atan2(in[0].real, in[1].real);
  CLEANUP_APPLYUNIT(UNIT_ANGLE);
  CHECK_OUTPUT_OKAY;
 }

void pplfunc_besseli     (pplset_terminal *term, pplObj *in, int nArgs, int *status, int *errType, char *errText)
 {
  char *FunctionDescription = "besseli(l,x)";
  CHECK_NEEDINT(in[0], "l", "function can only evaluate Bessel functions");
  OUTPUT.real = gsl_sf_bessel_il_scaled((int)in[0].real, in[1].real);
  CHECK_OUTPUT_OKAY;
 }

void pplfunc_besselI     (pplset_terminal *term, pplObj *in, int nArgs, int *status, int *errType, char *errText)
 {
  char *FunctionDescription = "besselI(l,x)";
  CHECK_NEEDINT(in[0], "l", "function can only evaluate Bessel functions");
  OUTPUT.real = gsl_sf_bessel_In((int)in[0].real, in[1].real);
  CHECK_OUTPUT_OKAY;
 }

void pplfunc_besselj     (pplset_terminal *term, pplObj *in, int nArgs, int *status, int *errType, char *errText)
 {
  char *FunctionDescription = "besselj(l,x)";
  CHECK_NEEDINT(in[0], "l", "function can only evaluate Bessel functions");
  OUTPUT.real = gsl_sf_bessel_jl((int)in[0].real, in[1].real);
  CHECK_OUTPUT_OKAY;
 }

void pplfunc_besselJ     (pplset_terminal *term, pplObj *in, int nArgs, int *status, int *errType, char *errText)
 {
  char *FunctionDescription = "besselJ(l,x)";
  CHECK_NEEDINT(in[0], "l", "function can only evaluate Bessel functions");
  OUTPUT.real = gsl_sf_bessel_Jn((int)in[0].real, in[1].real);
  CHECK_OUTPUT_OKAY;
 }

void pplfunc_besselk     (pplset_terminal *term, pplObj *in, int nArgs, int *status, int *errType, char *errText)
 {
  char *FunctionDescription = "besselk(l,x)";
  CHECK_NEEDINT(in[0], "l", "function can only evaluate Bessel functions");
  OUTPUT.real = gsl_sf_bessel_kl_scaled((int)in[0].real, in[1].real);
  CHECK_OUTPUT_OKAY;
 }

void pplfunc_besselK     (pplset_terminal *term, pplObj *in, int nArgs, int *status, int *errType, char *errText)
 {
  char *FunctionDescription = "besselK(l,x)";
  CHECK_NEEDINT(in[0], "l", "function can only evaluate Bessel functions");
  OUTPUT.real = gsl_sf_bessel_Kn((int)in[0].real, in[1].real);
  CHECK_OUTPUT_OKAY;
 }

void pplfunc_bessely     (pplset_terminal *term, pplObj *in, int nArgs, int *status, int *errType, char *errText)
 {
  char *FunctionDescription = "bessely(l,x)";
  CHECK_NEEDINT(in[0], "l", "function can only evaluate Bessel functions");
  OUTPUT.real = gsl_sf_bessel_yl((int)in[0].real, in[1].real);
  CHECK_OUTPUT_OKAY;
 }

void pplfunc_besselY     (pplset_terminal *term, pplObj *in, int nArgs, int *status, int *errType, char *errText)
 {
  char *FunctionDescription = "besselY(l,x)";
  CHECK_NEEDINT(in[0], "l", "function can only evaluate Bessel functions");
  OUTPUT.real = gsl_sf_bessel_Yn((int)in[0].real, in[1].real);
  CHECK_OUTPUT_OKAY;
 }

void pplfunc_beta        (pplset_terminal *term, pplObj *in, int nArgs, int *status, int *errType, char *errText)
 {
  char *FunctionDescription = "beta(a,b)";
  OUTPUT.real = gsl_sf_beta(in[0].real, in[1].real);
  CHECK_OUTPUT_OKAY;
 }

void pplfunc_ceil        (pplset_terminal *term, pplObj *in, int nArgs, int *status, int *errType, char *errText)
 {
  char *FunctionDescription = "ceil(x)";
  OUTPUT.real = ceil(in[0].real);
  CHECK_OUTPUT_OKAY;
 }

void pplfunc_conjugate   (pplset_terminal *term, pplObj *in, int nArgs, int *status, int *errType, char *errText)
 {
  char *FunctionDescription = "conjugate(z)";
  memcpy(OUTPUT, in, sizeof(pplObj));
  OUTPUT.imag *= -1;
  CHECK_OUTPUT_OKAY;
 }

void pplfunc_cos         (pplset_terminal *term, pplObj *in, int nArgs, int *status, int *errType, char *errText)
 {
  char *FunctionDescription = "cos(x)";
  int i;
  gsl_complex z;
  CHECK_DIMLESS_OR_HAS_UNIT(in , "first", "an angle", UNIT_ANGLE, 1);
  IF_1COMPLEX { GSL_SET_COMPLEX(&z,in[0].real,in[0].imag); z=gsl_complex_cos(z); CLEANUP_GSLCOMPLEX; }
  ELSE_REAL   { OUTPUT.real = cos(in[0].real); }
  ENDIF
  CHECK_OUTPUT_OKAY;
 }

void pplfunc_cosh        (pplset_terminal *term, pplObj *in, int nArgs, int *status, int *errType, char *errText)
 {
  char *FunctionDescription = "cosh(x)";
  int i;
  gsl_complex z;
  CHECK_DIMLESS_OR_HAS_UNIT(in , "first", "an angle", UNIT_ANGLE, 1);
  IF_1COMPLEX { GSL_SET_COMPLEX(&z,in[0].real,in[0].imag); z=gsl_complex_cosh(z); CLEANUP_GSLCOMPLEX; }
  ELSE_REAL   { OUTPUT.real = cosh(in[0].real); }
  ENDIF
  CHECK_OUTPUT_OKAY;
 }

void pplfunc_cot         (pplset_terminal *term, pplObj *in, int nArgs, int *status, int *errType, char *errText)
 {
  char *FunctionDescription = "cot(x)";
  int i;
  gsl_complex z;
  CHECK_DIMLESS_OR_HAS_UNIT(in , "first", "an angle", UNIT_ANGLE, 1);
  GSL_SET_COMPLEX(&z,in[0].real,in[0].imag); z=gsl_complex_cot(z); CLEANUP_GSLCOMPLEX;
  CHECK_OUTPUT_OKAY;
 }

void pplfunc_coth        (pplset_terminal *term, pplObj *in, int nArgs, int *status, int *errType, char *errText)
 {
  char *FunctionDescription = "coth(x)";
  int i;
  gsl_complex z;
  CHECK_DIMLESS_OR_HAS_UNIT(in , "first", "an angle", UNIT_ANGLE, 1);
  GSL_SET_COMPLEX(&z,in[0].real,in[0].imag); z=gsl_complex_coth(z); CLEANUP_GSLCOMPLEX;
  CHECK_OUTPUT_OKAY;
 }

void pplfunc_csc         (pplset_terminal *term, pplObj *in, int nArgs, int *status, int *errType, char *errText)
 {
  char *FunctionDescription = "csc(x)";
  int i;
  gsl_complex z;
  CHECK_DIMLESS_OR_HAS_UNIT(in , "first", "an angle", UNIT_ANGLE, 1);
  GSL_SET_COMPLEX(&z,in[0].real,in[0].imag); z=gsl_complex_csc(z); CLEANUP_GSLCOMPLEX;
  CHECK_OUTPUT_OKAY;
 }

void pplfunc_csch        (pplset_terminal *term, pplObj *in, int nArgs, int *status, int *errType, char *errText)
 {
  char *FunctionDescription = "csch(x)";
  int i;
  gsl_complex z;
  CHECK_DIMLESS_OR_HAS_UNIT(in , "first", "an angle", UNIT_ANGLE, 1);
  GSL_SET_COMPLEX(&z,in[0].real,in[0].imag); z=gsl_complex_csch(z); CLEANUP_GSLCOMPLEX;
  CHECK_OUTPUT_OKAY;
 }

void pplfunc_degrees     (pplset_terminal *term, pplObj *in, int nArgs, int *status, int *errType, char *errText)
 {
  char *FunctionDescription = "degrees(x)";
  int i;
  CHECK_DIMLESS_OR_HAS_UNIT(in , "first", "an angle", UNIT_ANGLE, 1);
  OUTPUT.real = degrees(in[0].real);
  CHECK_OUTPUT_OKAY;
 }

void pplfunc_ellK        (pplset_terminal *term, pplObj *in, int nArgs, int *status, int *errType, char *errText)
 {
  char *FunctionDescription = "ellipticintK(k)";
  OUTPUT.real = gsl_sf_ellint_Kcomp(in[0].real , GSL_PREC_DOUBLE);
  CHECK_OUTPUT_OKAY;
 }

void pplfunc_ellE        (pplset_terminal *term, pplObj *in, int nArgs, int *status, int *errType, char *errText)
 {
  char *FunctionDescription = "ellipticintE(k)";
  OUTPUT.real = gsl_sf_ellint_Ecomp(in[0].real , GSL_PREC_DOUBLE);
  CHECK_OUTPUT_OKAY;
 }

void pplfunc_ellP        (pplset_terminal *term, pplObj *in, int nArgs, int *status, int *errType, char *errText)
 {
  char *FunctionDescription = "ellipticintP(k,n)";
  OUTPUT.real = gsl_sf_ellint_Pcomp(in[0].real , in[1].real , GSL_PREC_DOUBLE);
  CHECK_OUTPUT_OKAY;
 }

void pplfunc_erf         (pplset_terminal *term, pplObj *in, int nArgs, int *status, int *errType, char *errText)
 {
  char *FunctionDescription = "erf(x)";
  OUTPUT.real = gsl_sf_erf(in[0].real);
  CHECK_OUTPUT_OKAY;
 }

void pplfunc_erfc        (pplset_terminal *term, pplObj *in, int nArgs, int *status, int *errType, char *errText)
 {
  char *FunctionDescription = "erfc(x)";
  OUTPUT.real = gsl_sf_erfc(in[0].real);
  CHECK_OUTPUT_OKAY;
 }

void pplfunc_exp         (pplset_terminal *term, pplObj *in, int nArgs, int *status, int *errType, char *errText)
 {
  char *FunctionDescription = "exp(z)";
  int i;
  gsl_complex z;
  CHECK_DIMLESS_OR_HAS_UNIT(in , "first", "dimensionless or an angle", UNIT_ANGLE, 1);
  IF_1COMPLEX { GSL_SET_COMPLEX(&z,in[0].real,in[0].imag); z=gsl_complex_exp(z); CLEANUP_GSLCOMPLEX; }
  ELSE_REAL   { OUTPUT.real = exp(in[0].real); }
  ENDIF
  CHECK_OUTPUT_OKAY;
 }

void pplfunc_expm1       (pplset_terminal *term, pplObj *in, int nArgs, int *status, int *errType, char *errText)
 {
  char *FunctionDescription = "expm1(x)";
  int i;
  CHECK_DIMLESS_OR_HAS_UNIT(in , "first", "dimensionless or an angle", UNIT_ANGLE, 1);
  OUTPUT.real = expm1(in[0].real);
  CHECK_OUTPUT_OKAY;
 }

void pplfunc_expint      (pplset_terminal *term, pplObj *in, int nArgs, int *status, int *errType, char *errText)
 {
  char *FunctionDescription = "expint(n,x)";
  CHECK_NEEDSINT(in[0], "n", "function's first argument must be");
  OUTPUT.real = gsl_sf_expint_En((int)in[0].real, in[1].real);
  CHECK_OUTPUT_OKAY;
 }

void pplfunc_finite      (pplset_terminal *term, pplObj *in, int nArgs, int *status, int *errType, char *errText)
 {
  if ((settings_term_current.ComplexNumbers == SW_ONOFF_OFF) && (in[0].FlagComplex)) return;
  if ((!gsl_finite(in[0].real)) || (!gsl_finite(in[0].imag))) return;
  OUTPUT.real = 1;
 }

void pplfunc_floor       (pplset_terminal *term, pplObj *in, int nArgs, int *status, int *errType, char *errText)
 {
  char *FunctionDescription = "floor(x)";
  OUTPUT.real = floor(in[0].real);
  CHECK_OUTPUT_OKAY;
 }

void pplfunc_gamma       (pplset_terminal *term, pplObj *in, int nArgs, int *status, int *errType, char *errText)
 {
  char *FunctionDescription = "gamma(x)";
  OUTPUT.real = gsl_sf_gamma(in[0].real);
  CHECK_OUTPUT_OKAY;
 }

void pplfunc_heaviside   (pplset_terminal *term, pplObj *in, int nArgs, int *status, int *errType, char *errText)
 {
  char *FunctionDescription = "heaviside(x)";
  if (in[0].real >= 0) OUTPUT.real = 1.0;
  CHECK_OUTPUT_OKAY;
 }

void pplfunc_hyperg_0F1  (pplset_terminal *term, pplObj *in, int nArgs, int *status, int *errType, char *errText)
 {
  char *FunctionDescription = "hyperg_0F1(c,x)";
  OUTPUT.real = gsl_sf_hyperg_0F1(in[0].real,in[1].real);
  CHECK_OUTPUT_OKAY;
 }

void pplfunc_hyperg_1F1  (pplset_terminal *term, pplObj *in, int nArgs, int *status, int *errType, char *errText)
 {
  char *FunctionDescription = "hyperg_1F1(a,b,x)";
  OUTPUT.real = gsl_sf_hyperg_1F1(in[0].real,in[1].real,in[2].real);
  CHECK_OUTPUT_OKAY;
 }

void pplfunc_hyperg_2F0  (pplset_terminal *term, pplObj *in, int nArgs, int *status, int *errType, char *errText)
 {
  char *FunctionDescription = "hyperg_2F0(a,b,x)";
  OUTPUT.real = gsl_sf_hyperg_2F0(in[0].real,in[1].real,in[2].real);
  CHECK_OUTPUT_OKAY;
 }

void pplfunc_hyperg_2F1  (pplset_terminal *term, pplObj *in, int nArgs, int *status, int *errType, char *errText)
 {
  char *FunctionDescription = "hyperg_2F1(a,b,c,x)";
  OUTPUT.real = gsl_sf_hyperg_2F1(in[0].real,in[1].real,in[2].real,in[3].real);
  CHECK_OUTPUT_OKAY;
 }

void pplfunc_hyperg_U    (pplset_terminal *term, pplObj *in, int nArgs, int *status, int *errType, char *errText)
 {
  char *FunctionDescription = "hyperg_U(a,b,x)";
  OUTPUT.real = gsl_sf_hyperg_U(in[0].real,in[1].real,in[2].real);
  CHECK_OUTPUT_OKAY;
 }

void pplfunc_hypot       (pplset_terminal *term, pplObj *in, int nArgs, int *status, int *errType, char *errText)
 {
 }

void pplfunc_imag        (pplset_terminal *term, pplObj *in, int nArgs, int *status, int *errType, char *errText)
 {
  char *FunctionDescription = "Im(z)";
  if (settings_term_current.ComplexNumbers == SW_ONOFF_OFF)
   {
    if (term->ExplicitErrors == SW_ONOFF_ON) { *status=1; *errText=ERR_NUMERIC; sprintf(errtext, "The function %s can only be used when complex arithmetic is enabled; type 'set numerics complex' first.", FunctionDescription); return; }
    else { NULL_OUTPUT; }
   }
  OUTPUT.real = in[0].imag;
  CHECK_OUTPUT_OKAY;
  ppl_units_DimCpy(OUTPUT, in);
 }

void pplfunc_jacobi_cn   (pplset_terminal *term, pplObj *in, int nArgs, int *status, int *errType, char *errText)
 {
  char *FunctionDescription = "jacobi_cn(u,m)";
  { double t1,t2,t3; if (gsl_sf_elljac_e(in[0].real,in[1].real,&t1,&t2,&t3)!=GSL_SUCCESS) { OUTPUT.real=GSL_NAN; } else { OUTPUT.real=t2; } }
  CHECK_OUTPUT_OKAY;
 }

void pplfunc_jacobi_dn   (pplset_terminal *term, pplObj *in, int nArgs, int *status, int *errType, char *errText)
 {
  char *FunctionDescription = "jacobi_dn(u,m)";
  { double t1,t2,t3; if (gsl_sf_elljac_e(in[0].real,in[1].real,&t1,&t2,&t3)!=GSL_SUCCESS) { OUTPUT.real=GSL_NAN; } else { OUTPUT.real=t3; } }
  CHECK_OUTPUT_OKAY;
 }

void pplfunc_jacobi_sn   (pplset_terminal *term, pplObj *in, int nArgs, int *status, int *errType, char *errText)
 {
  char *FunctionDescription = "jacobi_sn(u,m)";
  { double t1,t2,t3; if (gsl_sf_elljac_e(in[0].real,in[1].real,&t1,&t2,&t3)!=GSL_SUCCESS) { OUTPUT.real=GSL_NAN; } else { OUTPUT.real=t1; } }
  CHECK_OUTPUT_OKAY;
 }

void pplfunc_lambert_W0  (pplset_terminal *term, pplObj *in, int nArgs, int *status, int *errType, char *errText)
 {
  char *FunctionDescription = "lambert_W0(x)";
  OUTPUT.real = gsl_sf_lambert_W0(in[0].real);
  CHECK_OUTPUT_OKAY;
 }

void pplfunc_lambert_W1  (pplset_terminal *term, pplObj *in, int nArgs, int *status, int *errType, char *errText)
 {
  char *FunctionDescription = "lambert_W1(x)";
  OUTPUT.real = gsl_sf_lambert_Wm1(in[0].real);
  CHECK_OUTPUT_OKAY;
 }

void pplfunc_ldexp       (pplset_terminal *term, pplObj *in, int nArgs, int *status, int *errType, char *errText)
 {
  char *FunctionDescription = "ldexp(x,y)";
  CHECK_NEEDSINT(in[1], "y", "function's second parameter must be");
  OUTPUT.real = ldexp(in[0].real, (int)in[1].real);
  CHECK_OUTPUT_OKAY;
 }

void pplfunc_legendreP   (pplset_terminal *term, pplObj *in, int nArgs, int *status, int *errType, char *errText)
 {
  char *FunctionDescription = "legendreP(l,x)";
  CHECK_NEEDINT(in[0] , "l", "function's first parameter must be");
  OUTPUT.real = gsl_sf_legendre_Pl((int)in[0].real, in[1].real);
  CHECK_OUTPUT_OKAY;
 }

void pplfunc_legendreQ   (pplset_terminal *term, pplObj *in, int nArgs, int *status, int *errType, char *errText)
 {
  char *FunctionDescription = "legendreQ(l,x)";
  CHECK_NEEDINT(in[0] , "l", "function's first parameter must be");
  OUTPUT.real = gsl_sf_legendre_Ql((int)in[0].real, in[1].real);
  CHECK_OUTPUT_OKAY;
 }

void pplfunc_log         (pplset_terminal *term, pplObj *in, int nArgs, int *status, int *errType, char *errText)
 {
  char *FunctionDescription = "log(z)";
  gsl_complex z;
  GSL_SET_COMPLEX(&z,in[0].real,in[0].imag); z=gsl_complex_log(z); CLEANUP_GSLCOMPLEX;
  CHECK_OUTPUT_OKAY;
 }

void pplfunc_log10       (pplset_terminal *term, pplObj *in, int nArgs, int *status, int *errType, char *errText)
 {
  char *FunctionDescription = "log10(z)";
  gsl_complex z;
  GSL_SET_COMPLEX(&z,in[0].real,in[0].imag); z=gsl_complex_log10(z); CLEANUP_GSLCOMPLEX;
  CHECK_OUTPUT_OKAY;
 }

void pplfunc_logn        (pplset_terminal *term, pplObj *in, int nArgs, int *status, int *errType, char *errText)
 {
  char *FunctionDescription = "logn(z,n)";
  gsl_complex z;
  pplObj base;
  GSL_SET_COMPLEX(&z,in[1].real,in[1].imag); z=gsl_complex_log(z); CLEANUP_GSLCOMPLEX; base=*OUTPUT;
  GSL_SET_COMPLEX(&z,in[0].real,in[0].imag); z=gsl_complex_log(z); CLEANUP_GSLCOMPLEX;
  ppl_uaDiv(OUTPUT,&base,OUTPUT,status,errtext);
  CHECK_OUTPUT_OKAY;
 }

void pplfunc_max         (pplset_terminal *term, pplObj *in, int nArgs, int *status, int *errType, char *errText)
 {
 }

void pplfunc_min         (pplset_terminal *term, pplObj *in, int nArgs, int *status, int *errType, char *errText)
 {
 }

void pplfunc_mod         (pplset_terminal *term, pplObj *in, int nArgs, int *status, int *errType, char *errText)
 {
  char *FunctionDescription = "mod(x,y)";
  if (in[0].real*machine_epsilon*10 > in[1].real)
   {
    if (term->ExplicitErrors == SW_ONOFF_ON) { *status = 1; *errType=ERR_NUMERIC; sprintf(errtext, "Loss of accuracy in the function %s; the remainder of this division is lost in floating-point rounding.", FunctionDescription); return; }
    else { NULL_OUTPUT; }
   }
  OUTPUT.real = fmod(in[0].real , in[1].real);
  CHECK_OUTPUT_OKAY;
  ppl_units_DimCpy(OUTPUT, in[0]);
 }

void pplfunc_pow         (pplset_terminal *term, pplObj *in, int nArgs, int *status, int *errType, char *errText)
 {
  char *FunctionDescription = "pow(x,y)";
  ppl_uaPow(in[0], in[1], OUTPUT, status, errtext);
 }

void pplfunc_prime       (pplset_terminal *term, pplObj *in, int nArgs, int *status, int *errType, char *errText)
 {
  char *FunctionDescription = "prime(x)";
  CHECK_NEEDINT(in, "x", "function's argument must be an integer in the range");
   {
    long x = floor(in[0].real), m, n;
    if (x<53) // Hardcode primes less than 53
     {
      if ((x==2)||(x==3)||(x==5)||(x==7)||(x==11)||(x==13)||(x==17)||(x==19)||(x==23)||(x==29)||(x==31)||(x==37)||(x==41)||(x==43)||(x==47)) { OUTPUT.real = 1; return; }
      else return;
     }
    if (((x%2)==0)||((x%3)==0)||((x%5)==0)||((x%7)==0)) return;
    m = sqrt(x);
    for (n=11; n<=m; n+=6)
     {
      if ((x% n   )==0) return;
      if ((x%(n+2))==0) return;
     }
    OUTPUT.real = 1;
   }
  CHECK_OUTPUT_OKAY;
 }

void pplfunc_radians     (pplset_terminal *term, pplObj *in, int nArgs, int *status, int *errType, char *errText)
 {
  char *FunctionDescription = "radians(x)";
  int i;
  CHECK_DIMLESS_OR_HAS_UNIT(in , "first", "an angle", UNIT_ANGLE, 1);
  if (in[0].dimensionless) { OUTPUT.real = radians(in[0].real); } else { OUTPUT.real = in[0].real; }
  ENDIF
  CHECK_OUTPUT_OKAY;
 }

void pplfunc_real        (pplset_terminal *term, pplObj *in, int nArgs, int *status, int *errType, char *errText)
 {
  char *FunctionDescription = "Re(z)";
  OUTPUT.real = in[0].real;
  CHECK_OUTPUT_OKAY;
  ppl_units_DimCpy(OUTPUT, in);
 }

void pplfunc_root        (pplset_terminal *term, pplObj *in, int nArgs, int *status, int *errType, char *errText)
 {
  pplObj x1,x2;
  double tmpdbl;
  char *FunctionDescription = "root(z,n)";
  unsigned char negated = 0;
  in++;
  CHECK_1INPUT_DIMLESS; // THIS IS CORRECT. Only check in[1]
  in--;
  if ((in[1].FlagComplex) || (in[1].real < 2) || (in[1].real >= INT_MAX))
   {
    if (term->ExplicitErrors == SW_ONOFF_ON) { *status = 1; sprintf(errtext, "The %s %s in the range 2 <= n < %d.",FunctionDescription,"function's second argument must be an integer in the range",INT_MAX); return; }
    else { NULL_OUTPUT; }
   }
  pplObjZero(&x2);
  x1=*in[0];
  if (x1.real < 0.0) { negated=1; x1.real=-x1.real; if (x1.imag!=0.0) x1.imag=-x1.imag; }
  x2.real = 1.0 / floor(in[1].real);
  ppl_uaPow(&x1, &x2, OUTPUT, status, errtext);
  if (*status) return;
  if (negated)
   {
    if (fmod(floor(in[1].real) , 2) == 1)
     {
      OUTPUT.real=-OUTPUT.real; if (OUTPUT.imag!=0.0) OUTPUT.imag=-OUTPUT.imag;
     } else {
      if (term->ComplexNumbers == SW_ONOFF_OFF) { QUERY_OUT_OF_RANGE; }
      else
       {
        tmpdbl = OUTPUT.imag;
        OUTPUT.imag = OUTPUT.real;
        OUTPUT.real = -tmpdbl;
        OUTPUT.FlagComplex = !ppl_units_DblEqual(OUTPUT.imag, 0);
        if (!OUTPUT.FlagComplex) OUTPUT.imag=0.0; // Enforce that real numbers have positive zero imaginary components
       }
     }
   }
  CHECK_OUTPUT_OKAY;
 }

void pplfunc_sec         (pplset_terminal *term, pplObj *in, int nArgs, int *status, int *errType, char *errText)
 {
  char *FunctionDescription = "sec(z)";
  int i;
  gsl_complex z;
  CHECK_DIMLESS_OR_HAS_UNIT(in , "first", "an angle", UNIT_ANGLE, 1);
  GSL_SET_COMPLEX(&z,in[0].real,in[0].imag); z=gsl_complex_sec(z); CLEANUP_GSLCOMPLEX;
  CHECK_OUTPUT_OKAY;
 }

void pplfunc_sech        (pplset_terminal *term, pplObj *in, int nArgs, int *status, int *errType, char *errText)
 {
  char *FunctionDescription = "sech(z)";
  int i;
  gsl_complex z;
  CHECK_DIMLESS_OR_HAS_UNIT(in , "first", "an angle", UNIT_ANGLE, 1);
  GSL_SET_COMPLEX(&z,in[0].real,in[0].imag); z=gsl_complex_sech(z); CLEANUP_GSLCOMPLEX;
  CHECK_OUTPUT_OKAY;
 }

void pplfunc_sin         (pplset_terminal *term, pplObj *in, int nArgs, int *status, int *errType, char *errText)
 {
  char *FunctionDescription = "sin(z)";
  int i;
  gsl_complex z;
  CHECK_DIMLESS_OR_HAS_UNIT(in , "first", "an angle", UNIT_ANGLE, 1);
  IF_1COMPLEX { GSL_SET_COMPLEX(&z,in[0].real,in[0].imag); z=gsl_complex_sin(z); CLEANUP_GSLCOMPLEX; }
  ELSE_REAL   { OUTPUT.real = sin(in[0].real); }
  ENDIF
  CHECK_OUTPUT_OKAY;
 }

void pplfunc_sinc        (pplset_terminal *term, pplObj *in, int nArgs, int *status, int *errType, char *errText)
 {
  char *FunctionDescription = "sinc(z)";
  int i;
  gsl_complex z;
  CHECK_DIMLESS_OR_HAS_UNIT(in , "first", "an angle", UNIT_ANGLE, 1);
  if ((in[0].real==0) && (in[0].imag==0)) { OUTPUT.real = 1.0; }
  else
   {
    IF_1COMPLEX { GSL_SET_COMPLEX(&z,in[0].real, in[0].imag); z=gsl_complex_sin(z); CLEANUP_GSLCOMPLEX; ppl_uaDiv(OUTPUT, in, OUTPUT, status, errtext); }
    ELSE_REAL   { OUTPUT.real = sin(in[0].real)/in[0].real; }
    ENDIF
   }
  CHECK_OUTPUT_OKAY;
 }

void pplfunc_sinh        (pplset_terminal *term, pplObj *in, int nArgs, int *status, int *errType, char *errText)
 {
  char *FunctionDescription = "sinh(z)";
  int i;
  gsl_complex z;
  CHECK_DIMLESS_OR_HAS_UNIT(in , "first", "an angle", UNIT_ANGLE, 1);
  IF_1COMPLEX { GSL_SET_COMPLEX(&z,in[0].real,in[0].imag); z=gsl_complex_sinh(z); CLEANUP_GSLCOMPLEX; }
  ELSE_REAL   { OUTPUT.real = sinh(in[0].real); }
  ENDIF
  CHECK_OUTPUT_OKAY;
 }

void pplfunc_sqrt        (pplset_terminal *term, pplObj *in, int nArgs, int *status, int *errType, char *errText)
 {
  char *FunctionDescription = "sqrt(z)";
  int i;
  gsl_complex z;
  IF_1COMPLEX { GSL_SET_COMPLEX(&z,in[0].real,in[0].imag); z=gsl_complex_sqrt(z); }
  ELSE_REAL   { z=gsl_complex_sqrt_real(in[0].real); }
  ENDIF
  CLEANUP_GSLCOMPLEX;
  CHECK_OUTPUT_OKAY;
  OUTPUT.dimensionless = in[0].dimensionless;
  OUTPUT.tempType      = in[0].tempType;
  for (i=0; i<UNITS_MAX_BASEUNITS; i++) OUTPUT.exponent[i] = in[0].exponent[i] / 2;
 }

void pplfunc_tan         (pplset_terminal *term, pplObj *in, int nArgs, int *status, int *errType, char *errText)
 {
  char *FunctionDescription = "tan(z)";
  int i;
  gsl_complex z;
  CHECK_DIMLESS_OR_HAS_UNIT(in , "first", "an angle", UNIT_ANGLE, 1);
  IF_1COMPLEX { GSL_SET_COMPLEX(&z,in[0].real,in[0].imag); z=gsl_complex_tan(z); CLEANUP_GSLCOMPLEX; }
  ELSE_REAL   { OUTPUT.real = tan(in[0].real); }
  ENDIF
  CHECK_OUTPUT_OKAY;
 }

void pplfunc_tanh        (pplset_terminal *term, pplObj *in, int nArgs, int *status, int *errType, char *errText)
 {
  char *FunctionDescription = "tanh(z)";
  int i;
  gsl_complex z;
  CHECK_DIMLESS_OR_HAS_UNIT(in , "first", "an angle", UNIT_ANGLE, 1);
  IF_1COMPLEX { GSL_SET_COMPLEX(&z,in[0].real,in[0].imag); z=gsl_complex_tanh(z); CLEANUP_GSLCOMPLEX; }
  ELSE_REAL   { OUTPUT.real = tanh(in[0].real); }
  ENDIF
  CHECK_OUTPUT_OKAY;
 }

void pplfunc_tophat      (pplset_terminal *term, pplObj *in, int nArgs, int *status, int *errType, char *errText)
 {
  char *FunctionDescription = "tophat(x,sigma)";
  CHECK_2INPUT_DIMMATCH;
  if ( fabs(in[0].real) <= fabs(in[1].real) ) OUTPUT.real = 1.0;
  ENDIF
 }

void pplfunc_zernike     (pplset_terminal *term, pplObj *in, int nArgs, int *status, int *errType, char *errText)
 {
  int i;
  char *FunctionDescription = "zernike(n,m,r,phi)";
  CHECK_3INPUT_DIMLESS;
  CHECK_NEEDINT (in[0], "n", "function can only evaluate Zernike polynomials");
  CHECK_NEEDSINT(in[1], "m", "function can only evaluate Zernike polynomials");
  CHECK_DIMLESS_OR_HAS_UNIT(in[3], "fourth", "an angle", UNIT_ANGLE, 1);
   {
    int n,m,ms , sgn=1;
    double r;
    n  = in[0].real;
    ms = in[1].real;
    m  = abs(ms);
    r  = in[2].real;
    if (m>n)
     {
      if (term->ExplicitErrors == SW_ONOFF_ON) { *status=1; *errType=ERR_RANGE; sprintf(errtext, "The function %s is only defined for -n<=m<=n.", FunctionDescription); return; }
      else { NULL_OUTPUT; }
     }
    if ((r<0)||(r>1))
     {
      OUTPUT.real = GSL_NAN; // Defined only within the unit disk
     }
    else
     {
      if ((n%2)!=(m%2)) return; // Defined to be zero
      for (i=0; i<(1+(n-m)/2); i++)
       {
        OUTPUT.real += sgn * gsl_sf_fact(n-i) / ( gsl_sf_fact(i) * gsl_sf_fact((n+m)/2-i) * gsl_sf_fact((n-m)/2-i) ) * pow(r , n-2*i);
        sgn*=-1;
       }
      if      (ms>0) OUTPUT.real *= cos(m*in[3].real);
      else if (ms<0) OUTPUT.real *= sin(m*in[3].real);
     }
   }
  CHECK_OUTPUT_OKAY;
 }

void pplfunc_zernikeR    (pplset_terminal *term, pplObj *in, int nArgs, int *status, int *errType, char *errText)
 {
  int i;
  char *FunctionDescription = "zernikeR(n,m,r)";
  CHECK_NEEDINT (in[0], "n", "function can only evaluate Zernike polynomials");
  CHECK_NEEDSINT(in[1], "m", "function can only evaluate Zernike polynomials");
   {
    int n,m,ms , sgn=1;
    double r;
    n  = in[0].real;
    ms = in[1].real;
    m  = abs(ms);
    r  = in[2].real;
    if (m>n)
     {
      if (term->ExplicitErrors == SW_ONOFF_ON) { *status=1; *errType=ERR_RANGE; sprintf(errtext, "The function %s is only defined for -n<=m<=n.", FunctionDescription); return; }
      else { NULL_OUTPUT; }
     }
    if ((r<0)||(r>1))
     {
      OUTPUT.real = GSL_NAN; // Defined only within the unit disk
     }
    else
     {
      if ((n%2)!=(m%2)) return; // Defined to be zero

      for (i=0; i<(1+(n-m)/2); i++)
       {
        OUTPUT.real += sgn * gsl_sf_fact(n-i) / ( gsl_sf_fact(i) * gsl_sf_fact((n+m)/2-i) * gsl_sf_fact((n-m)/2-i) ) * pow(r , n-2*i);
        sgn*=-1;
       }
     }
   }
  ENDIF
  CHECK_OUTPUT_OKAY;
 }

void pplfunc_zeta        (pplset_terminal *term, pplObj *in, int nArgs, int *status, int *errType, char *errText)
 {
  char *FunctionDescription = "zeta(z)";
  gsl_complex zi,z;
  IF_1COMPLEX { GSL_SET_COMPLEX(&zi,in[0].real,in[0].imag); riemann_zeta_complex(zi,&z,status,errtext); if (*status) return; CLEANUP_GSLCOMPLEX; }
  ELSE_REAL   { OUTPUT.real = gsl_sf_zeta(in[0].real); }
  ENDIF
  CHECK_OUTPUT_OKAY;
 }

