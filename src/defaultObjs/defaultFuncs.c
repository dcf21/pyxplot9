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
#define GSL_RANGE_CHECK_OFF 1

#include "coreUtils/dict.h"
#include "mathsTools/dcfmath.h"
#include "settings/settings.h"
#include "stringTools/asciidouble.h"

#include "expressions/expEvalOps.h"
#include "userspace/garbageCollector.h"
#include "userspace/pplObj_fns.h"
#include "userspace/pplObjCmp.h"
#include "userspace/pplObjFunc.h"
#include "userspace/unitsArithmetic.h"

#include "defaultObjs/airyFuncs.h"
#include "defaultObjs/defaultFuncs.h"
#include "defaultObjs/defaultFuncsMacros.h"
#include "defaultObjs/zetaRiemann.h"

void ppl_addMagicFunction(dict *n, char *name, int id, char *shortdesc, char *latex, char *desc)
 {
  pplObj   v;
  pplFunc *f = malloc(sizeof(pplFunc));
  if (f==NULL) return;
  f->functionType = PPL_FUNC_MAGIC;
  f->refCount     = 1;
  f->minArgs      = id;
  f->maxArgs      = 0;
  f->functionPtr  = NULL;
  f->argList      = NULL;
  f->minActive    = NULL;
  f->maxActive    = NULL;
  f->numOnly      = 0;
  f->notNan       = 0;
  f->realOnly     = 0;
  f->dimlessOnly  = 0;
  f->next         = NULL;
  f->LaTeX        = latex;
  f->descriptionShort = shortdesc;
  f->description  = desc;
  if (pplObjFunc(&v,1,1,f)!=NULL) ppl_dictAppendCpy(n , name , (void *)&v , sizeof(v));
  return;
 }

void ppl_addSystemFunc(dict *n, char *name, int minArgs, int maxArgs, int numOnly, int notNan, int realOnly, int dimlessOnly, void *fn, char *shortdesc, char *latex, char *desc)
 {
  pplObj   v;
  pplFunc *f = malloc(sizeof(pplFunc));
  if (f==NULL) return;
  f->functionType = PPL_FUNC_SYSTEM;
  f->refCount     = 1;
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
  if (pplObjFunc(&v,1,1,f)!=NULL) ppl_dictAppendCpy(n , name , (void *)&v , sizeof(v));
  return;
 }

void pplfunc_abs         (ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText)
 {
  char *FunctionDescription = "abs(z)";
  IF_1COMPLEX { OUTPUT.real = hypot(in[0].real , in[0].imag); }
  ELSE_REAL   { OUTPUT.real = fabs(in[0].real); }
  ENDIF;
  ppl_unitsDimCpy(&OUTPUT, in);
  CHECK_OUTPUT_OKAY;
 }

void pplfunc_acos        (ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText)
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

void pplfunc_acosh       (ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText)
 {
  char *FunctionDescription = "acosh(z)";
  gsl_complex z;
  IF_1COMPLEX { GSL_SET_COMPLEX(&z,in[0].real,in[0].imag); z=gsl_complex_arccosh(z); }
  ELSE_REAL   { z=gsl_complex_arccosh_real(in[0].real); }
  ENDIF;
  CHECK_OUTPUT_OKAY;
 }

void pplfunc_acot        (ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText)
 {
  char *FunctionDescription = "acot(z)";
  gsl_complex z;
  GSL_SET_COMPLEX(&z,in[0].real,in[0].imag); z=gsl_complex_arccot(z);
  CLEANUP_GSLCOMPLEX;
  CLEANUP_APPLYUNIT(UNIT_ANGLE);
  CHECK_OUTPUT_OKAY;
 }

void pplfunc_acoth       (ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText)
 {
  char *FunctionDescription = "acoth(z)";
  gsl_complex z;
  GSL_SET_COMPLEX(&z,in[0].real,in[0].imag); z=gsl_complex_arccoth(z);
  CLEANUP_GSLCOMPLEX;
  CLEANUP_APPLYUNIT(UNIT_ANGLE);
  CHECK_OUTPUT_OKAY;
 }

void pplfunc_acsc        (ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText)
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

void pplfunc_acsch       (ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText)
 {
  char *FunctionDescription = "acsch(z)";
  gsl_complex z;
  GSL_SET_COMPLEX(&z,in[0].real,in[0].imag); z=gsl_complex_arccsch(z);
  CLEANUP_GSLCOMPLEX;
  CLEANUP_APPLYUNIT(UNIT_ANGLE);
  CHECK_OUTPUT_OKAY;
 }

void pplfunc_airy_ai     (ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText)
 {
  char *FunctionDescription = "airy_ai(z)";
  gsl_complex zi, z;
  GSL_SET_COMPLEX(&zi,in[0].real,in[0].imag);
  airy_ai(zi,&z,status,errText);
  if (*status) { *errType = ERR_NUMERIC; return; }
  CLEANUP_GSLCOMPLEX;
  CHECK_OUTPUT_OKAY;
 }

void pplfunc_airy_ai_diff(ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText)
 {
  char *FunctionDescription = "airy_ai_diff(z)";
  gsl_complex zi,z;
  GSL_SET_COMPLEX(&zi,in[0].real,in[0].imag);
  airy_ai_diff(zi,&z,status,errText);
  if (*status) { *errType = ERR_NUMERIC; return; }
  CLEANUP_GSLCOMPLEX;
  CHECK_OUTPUT_OKAY;
 }

void pplfunc_airy_bi     (ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText)
 {
  char *FunctionDescription = "airy_bi(z)";
  gsl_complex zi,z;
  GSL_SET_COMPLEX(&zi,in[0].real,in[0].imag);
  airy_bi(zi,&z,status,errText);
  if (*status) { *errType = ERR_NUMERIC; return; }
  CLEANUP_GSLCOMPLEX;
  CHECK_OUTPUT_OKAY;
 }

void pplfunc_airy_bi_diff(ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText)
 {
  char *FunctionDescription = "airy_bi_diff(z)";
  gsl_complex zi,z;
  GSL_SET_COMPLEX(&zi,in[0].real,in[0].imag);
  airy_bi_diff(zi,&z,status,errText);
  if (*status) { *errType = ERR_NUMERIC; return; }
  CLEANUP_GSLCOMPLEX;
  CHECK_OUTPUT_OKAY;
 }

void pplfunc_arg         (ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText)
 {
  char *FunctionDescription = "arg(z)";
  gsl_complex z;
  GSL_SET_COMPLEX(&z,in[0].real,in[0].imag);
  OUTPUT.real = gsl_complex_arg(z);
  CLEANUP_APPLYUNIT(UNIT_ANGLE);
  CHECK_OUTPUT_OKAY;
 }

void pplfunc_asec        (ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText)
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

void pplfunc_asech       (ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText)
 {
  char *FunctionDescription = "asech(z)";
  gsl_complex z;
  GSL_SET_COMPLEX(&z,in[0].real,in[0].imag); z=gsl_complex_arcsech(z);
  CLEANUP_GSLCOMPLEX;
  CLEANUP_APPLYUNIT(UNIT_ANGLE);
  CHECK_OUTPUT_OKAY;
 }

void pplfunc_asin        (ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText)
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

void pplfunc_asinh       (ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText)
 {
  char *FunctionDescription = "asinh(z)";
  gsl_complex z;
  GSL_SET_COMPLEX(&z,in[0].real,in[0].imag); z=gsl_complex_arcsinh(z);
  CLEANUP_GSLCOMPLEX;
  CLEANUP_APPLYUNIT(UNIT_ANGLE);
  CHECK_OUTPUT_OKAY;
 }

void pplfunc_atan        (ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText)
 {
  char *FunctionDescription = "atan(z)";
  gsl_complex z;
  GSL_SET_COMPLEX(&z,in[0].real,in[0].imag); z=gsl_complex_arctan(z);
  CLEANUP_GSLCOMPLEX;
  CLEANUP_APPLYUNIT(UNIT_ANGLE);
  CHECK_OUTPUT_OKAY;
 }

void pplfunc_atanh       (ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText)
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

void pplfunc_atan2       (ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText)
 {
  char *FunctionDescription = "atan2(x,y)";
  CHECK_2INPUT_DIMMATCH;
  OUTPUT.real = atan2(in[0].real, in[1].real);
  CLEANUP_APPLYUNIT(UNIT_ANGLE);
  CHECK_OUTPUT_OKAY;
 }

void pplfunc_besseli     (ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText)
 {
  char *FunctionDescription = "besseli(l,x)";
  CHECK_NEEDINT(in[0], "l", "function can only evaluate Bessel functions");
  OUTPUT.real = gsl_sf_bessel_il_scaled((int)in[0].real, in[1].real);
  CHECK_OUTPUT_OKAY;
 }

void pplfunc_besselI     (ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText)
 {
  char *FunctionDescription = "besselI(l,x)";
  CHECK_NEEDINT(in[0], "l", "function can only evaluate Bessel functions");
  OUTPUT.real = gsl_sf_bessel_In((int)in[0].real, in[1].real);
  CHECK_OUTPUT_OKAY;
 }

void pplfunc_besselj     (ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText)
 {
  char *FunctionDescription = "besselj(l,x)";
  CHECK_NEEDINT(in[0], "l", "function can only evaluate Bessel functions");
  OUTPUT.real = gsl_sf_bessel_jl((int)in[0].real, in[1].real);
  CHECK_OUTPUT_OKAY;
 }

void pplfunc_besselJ     (ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText)
 {
  char *FunctionDescription = "besselJ(l,x)";
  CHECK_NEEDINT(in[0], "l", "function can only evaluate Bessel functions");
  OUTPUT.real = gsl_sf_bessel_Jn((int)in[0].real, in[1].real);
  CHECK_OUTPUT_OKAY;
 }

void pplfunc_besselk     (ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText)
 {
  char *FunctionDescription = "besselk(l,x)";
  CHECK_NEEDINT(in[0], "l", "function can only evaluate Bessel functions");
  OUTPUT.real = gsl_sf_bessel_kl_scaled((int)in[0].real, in[1].real);
  CHECK_OUTPUT_OKAY;
 }

void pplfunc_besselK     (ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText)
 {
  char *FunctionDescription = "besselK(l,x)";
  CHECK_NEEDINT(in[0], "l", "function can only evaluate Bessel functions");
  OUTPUT.real = gsl_sf_bessel_Kn((int)in[0].real, in[1].real);
  CHECK_OUTPUT_OKAY;
 }

void pplfunc_bessely     (ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText)
 {
  char *FunctionDescription = "bessely(l,x)";
  CHECK_NEEDINT(in[0], "l", "function can only evaluate Bessel functions");
  OUTPUT.real = gsl_sf_bessel_yl((int)in[0].real, in[1].real);
  CHECK_OUTPUT_OKAY;
 }

void pplfunc_besselY     (ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText)
 {
  char *FunctionDescription = "besselY(l,x)";
  CHECK_NEEDINT(in[0], "l", "function can only evaluate Bessel functions");
  OUTPUT.real = gsl_sf_bessel_Yn((int)in[0].real, in[1].real);
  CHECK_OUTPUT_OKAY;
 }

void pplfunc_beta        (ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText)
 {
  char *FunctionDescription = "beta(a,b)";
  OUTPUT.real = gsl_sf_beta(in[0].real, in[1].real);
  CHECK_OUTPUT_OKAY;
 }

void pplfunc_ceil        (ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText)
 {
  char *FunctionDescription = "ceil(x)";
  OUTPUT.real = ceil(in[0].real);
  CHECK_OUTPUT_OKAY;
 }

void pplfunc_classOf     (ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText)
 {
  pplObjCpy(&OUTPUT,in[0].objPrototype,0,1);
  OUTPUT.self_lval = NULL;
 }

void pplfunc_cmyk        (ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText)
 {
  pplObjColor(&OUTPUT,0,SW_COLSPACE_CMYK,in[0].real,in[1].real,in[2].real,in[3].real);
 }

void pplfunc_conjugate   (ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText)
 {
  char *FunctionDescription = "conjugate(z)";
  memcpy(&OUTPUT, in, sizeof(pplObj));
  OUTPUT.imag *= -1;
  CHECK_OUTPUT_OKAY;
 }

void pplfunc_cos         (ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText)
 {
  char *FunctionDescription = "cos(x)";
  int i;
  gsl_complex z;
  CHECK_DIMLESS_OR_HAS_UNIT(in[0] , "first", "an angle", UNIT_ANGLE, 1);
  IF_1COMPLEX { GSL_SET_COMPLEX(&z,in[0].real,in[0].imag); z=gsl_complex_cos(z); CLEANUP_GSLCOMPLEX; }
  ELSE_REAL   { OUTPUT.real = cos(in[0].real); }
  ENDIF
  CHECK_OUTPUT_OKAY;
 }

void pplfunc_cosh        (ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText)
 {
  char *FunctionDescription = "cosh(x)";
  int i;
  gsl_complex z;
  CHECK_DIMLESS_OR_HAS_UNIT(in[0] , "first", "an angle", UNIT_ANGLE, 1);
  IF_1COMPLEX { GSL_SET_COMPLEX(&z,in[0].real,in[0].imag); z=gsl_complex_cosh(z); CLEANUP_GSLCOMPLEX; }
  ELSE_REAL   { OUTPUT.real = cosh(in[0].real); }
  ENDIF
  CHECK_OUTPUT_OKAY;
 }

void pplfunc_cot         (ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText)
 {
  char *FunctionDescription = "cot(x)";
  int i;
  gsl_complex z;
  CHECK_DIMLESS_OR_HAS_UNIT(in[0] , "first", "an angle", UNIT_ANGLE, 1);
  GSL_SET_COMPLEX(&z,in[0].real,in[0].imag); z=gsl_complex_cot(z); CLEANUP_GSLCOMPLEX;
  CHECK_OUTPUT_OKAY;
 }

void pplfunc_coth        (ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText)
 {
  char *FunctionDescription = "coth(x)";
  int i;
  gsl_complex z;
  CHECK_DIMLESS_OR_HAS_UNIT(in[0] , "first", "an angle", UNIT_ANGLE, 1);
  GSL_SET_COMPLEX(&z,in[0].real,in[0].imag); z=gsl_complex_coth(z); CLEANUP_GSLCOMPLEX;
  CHECK_OUTPUT_OKAY;
 }

void pplfunc_csc         (ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText)
 {
  char *FunctionDescription = "csc(x)";
  int i;
  gsl_complex z;
  CHECK_DIMLESS_OR_HAS_UNIT(in[0] , "first", "an angle", UNIT_ANGLE, 1);
  GSL_SET_COMPLEX(&z,in[0].real,in[0].imag); z=gsl_complex_csc(z); CLEANUP_GSLCOMPLEX;
  CHECK_OUTPUT_OKAY;
 }

void pplfunc_csch        (ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText)
 {
  char *FunctionDescription = "csch(x)";
  int i;
  gsl_complex z;
  CHECK_DIMLESS_OR_HAS_UNIT(in[0] , "first", "an angle", UNIT_ANGLE, 1);
  GSL_SET_COMPLEX(&z,in[0].real,in[0].imag); z=gsl_complex_csch(z); CLEANUP_GSLCOMPLEX;
  CHECK_OUTPUT_OKAY;
 }

void pplfunc_degrees     (ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText)
 {
  char *FunctionDescription = "degrees(x)";
  int i;
  CHECK_DIMLESS_OR_HAS_UNIT(in[0] , "first", "an angle", UNIT_ANGLE, 1);
  OUTPUT.real = ppl_degs(in[0].real);
  CHECK_OUTPUT_OKAY;
 }

void pplfunc_ellK        (ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText)
 {
  char *FunctionDescription = "ellipticintK(k)";
  OUTPUT.real = gsl_sf_ellint_Kcomp(in[0].real , GSL_PREC_DOUBLE);
  CHECK_OUTPUT_OKAY;
 }

void pplfunc_ellE        (ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText)
 {
  char *FunctionDescription = "ellipticintE(k)";
  OUTPUT.real = gsl_sf_ellint_Ecomp(in[0].real , GSL_PREC_DOUBLE);
  CHECK_OUTPUT_OKAY;
 }

void pplfunc_ellP        (ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText)
 {
  char *FunctionDescription = "ellipticintP(k,n)";
  OUTPUT.real = gsl_sf_ellint_Pcomp(in[0].real , in[1].real , GSL_PREC_DOUBLE);
  CHECK_OUTPUT_OKAY;
 }

void pplfunc_erf         (ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText)
 {
  char *FunctionDescription = "erf(x)";
  OUTPUT.real = gsl_sf_erf(in[0].real);
  CHECK_OUTPUT_OKAY;
 }

void pplfunc_erfc        (ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText)
 {
  char *FunctionDescription = "erfc(x)";
  OUTPUT.real = gsl_sf_erfc(in[0].real);
  CHECK_OUTPUT_OKAY;
 }

void pplfunc_exp         (ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText)
 {
  char *FunctionDescription = "exp(z)";
  int i;
  gsl_complex z;
  CHECK_DIMLESS_OR_HAS_UNIT(in[0] , "first", "dimensionless or an angle", UNIT_ANGLE, 1);
  IF_1COMPLEX { GSL_SET_COMPLEX(&z,in[0].real,in[0].imag); z=gsl_complex_exp(z); CLEANUP_GSLCOMPLEX; }
  ELSE_REAL   { OUTPUT.real = exp(in[0].real); }
  ENDIF
  CHECK_OUTPUT_OKAY;
 }

void pplfunc_expm1       (ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText)
 {
  char *FunctionDescription = "expm1(x)";
  int i;
  CHECK_DIMLESS_OR_HAS_UNIT(in[0] , "first", "dimensionless or an angle", UNIT_ANGLE, 1);
  OUTPUT.real = expm1(in[0].real);
  CHECK_OUTPUT_OKAY;
 }

void pplfunc_expint      (ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText)
 {
  char *FunctionDescription = "expint(n,x)";
  CHECK_NEEDSINT(in[0], "n", "function's first argument must be");
  OUTPUT.real = gsl_sf_expint_En((int)in[0].real, in[1].real);
  CHECK_OUTPUT_OKAY;
 }

void pplfunc_finite      (ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText)
 {
  if ((c->set->term_current.ComplexNumbers == SW_ONOFF_OFF) && (in[0].flagComplex)) return;
  if ((!gsl_finite(in[0].real)) || (!gsl_finite(in[0].imag))) return;
  OUTPUT.real = 1;
 }

void pplfunc_floor       (ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText)
 {
  char *FunctionDescription = "floor(x)";
  OUTPUT.real = floor(in[0].real);
  CHECK_OUTPUT_OKAY;
 }

void pplfunc_gamma       (ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText)
 {
  char *FunctionDescription = "gamma(x)";
  OUTPUT.real = gsl_sf_gamma(in[0].real);
  CHECK_OUTPUT_OKAY;
 }

void pplfunc_heaviside   (ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText)
 {
  char *FunctionDescription = "heaviside(x)";
  if (in[0].real >= 0) OUTPUT.real = 1.0;
  CHECK_OUTPUT_OKAY;
 }

void pplfunc_hsb         (ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText)
 {
  pplObjColor(&OUTPUT,0,SW_COLSPACE_HSB,in[0].real,in[1].real,in[2].real,0);
 }

void pplfunc_hyperg_0F1  (ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText)
 {
  char *FunctionDescription = "hyperg_0F1(c,x)";
  OUTPUT.real = gsl_sf_hyperg_0F1(in[0].real,in[1].real);
  CHECK_OUTPUT_OKAY;
 }

void pplfunc_hyperg_1F1  (ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText)
 {
  char *FunctionDescription = "hyperg_1F1(a,b,x)";
  OUTPUT.real = gsl_sf_hyperg_1F1(in[0].real,in[1].real,in[2].real);
  CHECK_OUTPUT_OKAY;
 }

void pplfunc_hyperg_2F0  (ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText)
 {
  char *FunctionDescription = "hyperg_2F0(a,b,x)";
  OUTPUT.real = gsl_sf_hyperg_2F0(in[0].real,in[1].real,in[2].real);
  CHECK_OUTPUT_OKAY;
 }

void pplfunc_hyperg_2F1  (ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText)
 {
  char *FunctionDescription = "hyperg_2F1(a,b,c,x)";
  OUTPUT.real = gsl_sf_hyperg_2F1(in[0].real,in[1].real,in[2].real,in[3].real);
  CHECK_OUTPUT_OKAY;
 }

void pplfunc_hyperg_U    (ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText)
 {
  char *FunctionDescription = "hyperg_U(a,b,x)";
  OUTPUT.real = gsl_sf_hyperg_U(in[0].real,in[1].real,in[2].real);
  CHECK_OUTPUT_OKAY;
 }

void pplfunc_hypot       (ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText)
 {
  char *FunctionDescription = "hypot(...)";
  int i;
  double acc=0, *buffer;
  if (nArgs<1) { pplObjNum(&OUTPUT,0,0,0); return; }
  for (i=1; i<nArgs; i++) // Check matching dimensions
   if (!ppl_unitsDimEqual(&in[0], &in[i]))
   {
    *status = 1;
    *errType=ERR_UNIT;
    sprintf(errText, "The %s function can only act upon inputs with matching dimensions. Input 1 has dimensions of <%s>, but input %d has dimensions of <%s>.", FunctionDescription, ppl_printUnit(c, &in[0], NULL, NULL, 0, 1, 0), i+1, ppl_printUnit(c, &in[i], NULL, NULL, 1, 1, 0));
    return;
   }
  buffer = (double *)malloc(nArgs * sizeof(double));
  if (buffer==NULL) { *status = 1; *errType=ERR_MEMORY; sprintf(errText, "Out of memory."); return; }
  for (i=0; i<nArgs; i++) buffer[i] = hypot(in[i].real, in[i].imag);
  qsort((void *)buffer, nArgs, sizeof(double), ppl_dblSort);
  for (i=0; i<nArgs; i++) acc += gsl_pow_2(buffer[i]);
  free(buffer);
  OUTPUT.real = sqrt(acc);
  CHECK_OUTPUT_OKAY;
  ppl_unitsDimCpy(&OUTPUT, in);
 }

void pplfunc_globals     (ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText)
 {
  pplObjDict(&OUTPUT, 0, 1, c->namespaces[1]);
  c->namespaces[1]->refCount++;
 }

void pplfunc_imag        (ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText)
 {
  char *FunctionDescription = "Im(z)";
  if (c->set->term_current.ComplexNumbers == SW_ONOFF_OFF)
   {
    if (c->set->term_current.ExplicitErrors == SW_ONOFF_ON) { *status=1; *errText=ERR_NUMERIC; sprintf(errText, "The function %s can only be used when complex arithmetic is enabled; type 'set numerics complex' first.", FunctionDescription); return; }
    else { NULL_OUTPUT; }
   }
  OUTPUT.real = in[0].imag;
  CHECK_OUTPUT_OKAY;
  ppl_unitsDimCpy(&OUTPUT, in);
 }

void pplfunc_jacobi_cn   (ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText)
 {
  char *FunctionDescription = "jacobi_cn(u,m)";
  { double t1,t2,t3; if (gsl_sf_elljac_e(in[0].real,in[1].real,&t1,&t2,&t3)!=GSL_SUCCESS) { OUTPUT.real=GSL_NAN; } else { OUTPUT.real=t2; } }
  CHECK_OUTPUT_OKAY;
 }

void pplfunc_jacobi_dn   (ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText)
 {
  char *FunctionDescription = "jacobi_dn(u,m)";
  { double t1,t2,t3; if (gsl_sf_elljac_e(in[0].real,in[1].real,&t1,&t2,&t3)!=GSL_SUCCESS) { OUTPUT.real=GSL_NAN; } else { OUTPUT.real=t3; } }
  CHECK_OUTPUT_OKAY;
 }

void pplfunc_jacobi_sn   (ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText)
 {
  char *FunctionDescription = "jacobi_sn(u,m)";
  { double t1,t2,t3; if (gsl_sf_elljac_e(in[0].real,in[1].real,&t1,&t2,&t3)!=GSL_SUCCESS) { OUTPUT.real=GSL_NAN; } else { OUTPUT.real=t1; } }
  CHECK_OUTPUT_OKAY;
 }

void pplfunc_lambert_W0  (ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText)
 {
  char *FunctionDescription = "lambert_W0(x)";
  OUTPUT.real = gsl_sf_lambert_W0(in[0].real);
  CHECK_OUTPUT_OKAY;
 }

void pplfunc_lambert_W1  (ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText)
 {
  char *FunctionDescription = "lambert_W1(x)";
  OUTPUT.real = gsl_sf_lambert_Wm1(in[0].real);
  CHECK_OUTPUT_OKAY;
 }

void pplfunc_ldexp       (ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText)
 {
  char *FunctionDescription = "ldexp(x,y)";
  CHECK_NEEDSINT(in[1], "y", "function's second parameter must be");
  OUTPUT.real = ldexp(in[0].real, (int)in[1].real);
  CHECK_OUTPUT_OKAY;
 }

void pplfunc_legendreP   (ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText)
 {
  char *FunctionDescription = "legendreP(l,x)";
  CHECK_NEEDINT(in[0] , "l", "function's first parameter must be");
  OUTPUT.real = gsl_sf_legendre_Pl((int)in[0].real, in[1].real);
  CHECK_OUTPUT_OKAY;
 }

void pplfunc_legendreQ   (ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText)
 {
  char *FunctionDescription = "legendreQ(l,x)";
  CHECK_NEEDINT(in[0] , "l", "function's first parameter must be");
  OUTPUT.real = gsl_sf_legendre_Ql((int)in[0].real, in[1].real);
  CHECK_OUTPUT_OKAY;
 }

void pplfunc_len         (ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText)
 {
  int t = in->objType;
  if      ((t==PPLOBJ_DICT)||(t==PPLOBJ_MOD)||(t==PPLOBJ_USER)) OUTPUT.real = ((dict *)in->auxil)->length;
  else if  (t==PPLOBJ_LIST)                                     OUTPUT.real = ((list *)in->auxil)->length;
  else if ((t==PPLOBJ_STR)||(t==PPLOBJ_EXC))                    OUTPUT.real = strlen((char *)in->auxil);
  else if  (t==PPLOBJ_VEC)                                      OUTPUT.real = ((pplVector *)in->auxil)->v->size;
  else if  (t==PPLOBJ_MAT)                                      OUTPUT.real = ((pplMatrix *)in->auxil)->m->size1;
  else { *status=1; *errType=ERR_TYPE; sprintf(errText, "Object of type <%s> is not a compound object and has no property of length", pplObjTypeNames[t]); return; }
  return;
 }

void pplfunc_locals      (ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText)
 {
  pplObjDict(&OUTPUT, 0, 1, c->namespaces[c->ns_ptr]);
  c->namespaces[c->ns_ptr]->refCount++;
 }

void pplfunc_log         (ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText)
 {
  char *FunctionDescription = "log(z)";
  gsl_complex z;
  GSL_SET_COMPLEX(&z,in[0].real,in[0].imag); z=gsl_complex_log(z); CLEANUP_GSLCOMPLEX;
  CHECK_OUTPUT_OKAY;
 }

void pplfunc_log10       (ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText)
 {
  char *FunctionDescription = "log10(z)";
  gsl_complex z;
  GSL_SET_COMPLEX(&z,in[0].real,in[0].imag); z=gsl_complex_log10(z); CLEANUP_GSLCOMPLEX;
  CHECK_OUTPUT_OKAY;
 }

void pplfunc_logn        (ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText)
 {
  char *FunctionDescription = "logn(z,n)";
  gsl_complex z;
  pplObj base;
  GSL_SET_COMPLEX(&z,in[1].real,in[1].imag); z=gsl_complex_log(z); CLEANUP_GSLCOMPLEX; base=OUTPUT;
  GSL_SET_COMPLEX(&z,in[0].real,in[0].imag); z=gsl_complex_log(z); CLEANUP_GSLCOMPLEX;
  ppl_uaDiv(c, &OUTPUT, &base, &OUTPUT, status, errType, errText);
  CHECK_OUTPUT_OKAY;
 }

void pplfunc_lrange       (ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText)
 {
  char  *FunctionDescription = "lrange([f],l,[s])";
  double start, end, step, n;
  int    i;
  if (nArgs>1) { CHECK_2INPUT_DIMMATCH; }
  if (nArgs>2) { in++; CHECK_2INPUT_DIMMATCH; in--; }
 
  if      (nArgs==1) { start=1; end=in[0].real; step=2; }
  else if (nArgs==2) { start=in[0].real; end=in[1].real; step=2; }
  else               { start=in[0].real; end=in[1].real; step=in[2].real; }
  n = ceil(log(end/start) / log(step));
  if ((!gsl_finite(n))||(n>INT_MAX)) { *status=1; *errType=ERR_NUMERIC; strcpy(errText,"Invalid step size."); return; }
  if (n<0) n=0;
  if (pplObjVector(&OUTPUT,0,1,n)==NULL) { *status=1; *errType=ERR_MEMORY; strcpy(errText,"Out of memory."); return; }
  for (i=0; i<n; i++) gsl_vector_set(((pplVector *)OUTPUT.auxil)->v, i, start * pow(step,i));
  ppl_unitsDimCpy(&OUTPUT, &in[0]);
 }

void pplfunc_max         (ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText)
 {
  if (nArgs==0) { pplObjNull(&OUTPUT,0); return; }
  if ((nArgs==1)&&(in[0].objType==PPLOBJ_LIST))
   {
    pplObj *item, *best=NULL;
    listIterator *li = ppl_listIterateInit((list *)in[0].auxil);
    while ((item = (pplObj *)ppl_listIterate(&li))!=NULL)
     {
      if ((best==NULL)||(pplObjCmpQuiet((void*)&item, (void*)&best)==1)) best=item;
      if (status) return;
     }
    if (best==NULL) pplObjNull(&OUTPUT,0);
    else            pplObjCpy (&OUTPUT,best,0,1);
   }
  else if ((nArgs==1)&&((in[0].objType==PPLOBJ_DICT)||(in[0].objType==PPLOBJ_MOD)||(in[0].objType==PPLOBJ_USER)))
   {
    char *key;
    pplObj *item, *best=NULL;
    dictIterator *di = ppl_dictIterateInit((dict *)in[0].auxil);
    while ((item = (pplObj *)ppl_dictIterate(&di,&key))!=NULL)
     {
      if ((best==NULL)||(pplObjCmpQuiet((void*)&item, (void*)&best)==1)) best=item;
      if (status) return;
     }
    if (best==NULL) pplObjNull(&OUTPUT,0);
    else            pplObjCpy (&OUTPUT,best,0,1);
   }
  else if ((nArgs==1)&&(in[0].objType==PPLOBJ_VEC))
   {
    int i;
    double *best=NULL;
    gsl_vector *v=((pplVector *)(in[0].auxil))->v;
    for (i=0; i<v->size; i++) if ((best==NULL)||(gsl_vector_get(v,i)>*best)) best=gsl_vector_ptr(v,i);
    if (best==NULL) pplObjNull(&OUTPUT,0);
    else            pplObjNum (&OUTPUT,0,*best,0);
    ppl_unitsDimCpy(&OUTPUT, &in[0]);
   }
  else if ((nArgs==1)&&(in[0].objType==PPLOBJ_MAT))
   {
    int i,j;
    double *best=NULL;
    gsl_matrix *m=((pplMatrix *)(in[0].auxil))->m;
    for (i=0; i<m->size1; i++) for (j=0; j<m->size2; j++) if ((best==NULL)||(gsl_matrix_get(m,i,j)>*best)) best=gsl_matrix_ptr(m,i,j);
    if (best==NULL) pplObjNull(&OUTPUT,0);
    else            pplObjNum (&OUTPUT,0,*best,0);
    ppl_unitsDimCpy(&OUTPUT, &in[0]);
   }
  else
   {
    int i;
    pplObj *best = in;
    for (i=1; i<nArgs; i++)
     {
      pplObj *x=&in[i];
      if (pplObjCmpQuiet((void*)&x, (void*)&best)==1) best=&in[i];
      if (status) return;
     }
    if (best==NULL) pplObjNull(&OUTPUT,0);
    else            pplObjCpy (&OUTPUT,best,0,1);
   }
 }

void pplfunc_min         (ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText)
 {
  if (nArgs==0) { pplObjNull(&OUTPUT,0); return; }
  if ((nArgs==1)&&(in[0].objType==PPLOBJ_LIST))
   {
    pplObj *item, *best=NULL;
    listIterator *li = ppl_listIterateInit((list *)in[0].auxil);
    while ((item = (pplObj *)ppl_listIterate(&li))!=NULL)
     {
      if ((best==NULL)||(pplObjCmpQuiet((void*)&item, (void*)&best)==-1)) best=item;
      if (status) return;
     }
    if (best==NULL) pplObjNull(&OUTPUT,0);
    else            pplObjCpy (&OUTPUT,best,0,1);
   }
  else if ((nArgs==1)&&((in[0].objType==PPLOBJ_DICT)||(in[0].objType==PPLOBJ_MOD)||(in[0].objType==PPLOBJ_USER)))
   {
    char *key;
    pplObj *item, *best=NULL;
    dictIterator *di = ppl_dictIterateInit((dict *)in[0].auxil);
    while ((item = (pplObj *)ppl_dictIterate(&di,&key))!=NULL)
     {
      if ((best==NULL)||(pplObjCmpQuiet((void*)&item, (void*)&best)==-1)) best=item;
      if (status) return;
     }
    if (best==NULL) pplObjNull(&OUTPUT,0);
    else            pplObjCpy (&OUTPUT,best,0,1);
   }
  else if ((nArgs==1)&&(in[0].objType==PPLOBJ_VEC))
   {
    int i;
    double *best=NULL;
    gsl_vector *v=((pplVector *)(in[0].auxil))->v;
    for (i=0; i<v->size; i++) if ((best==NULL)||(gsl_vector_get(v,i)<*best)) best=gsl_vector_ptr(v,i);
    if (best==NULL) pplObjNull(&OUTPUT,0);
    else            pplObjNum (&OUTPUT,0,*best,0);
    ppl_unitsDimCpy(&OUTPUT, &in[0]);
   }
  else if ((nArgs==1)&&(in[0].objType==PPLOBJ_MAT))
   {
    int i,j;
    double *best=NULL;
    gsl_matrix *m=((pplMatrix *)(in[0].auxil))->m;
    for (i=0; i<m->size1; i++) for (j=0; j<m->size2; j++) if ((best==NULL)||(gsl_matrix_get(m,i,j)<*best)) best=gsl_matrix_ptr(m,i,j);
    if (best==NULL) pplObjNull(&OUTPUT,0);
    else            pplObjNum (&OUTPUT,0,*best,0);
    ppl_unitsDimCpy(&OUTPUT, &in[0]);
   }
  else
   {
    int i;
    pplObj *best = in;
    for (i=1; i<nArgs; i++)
     {
      pplObj *x=&in[i];
      if (pplObjCmpQuiet((void*)&x, (void*)&best)==-1) best=&in[i];
      if (status) return;
     }
    if (best==NULL) pplObjNull(&OUTPUT,0);
    else            pplObjCpy (&OUTPUT,best,0,1);
   }
 }

void pplfunc_mod         (ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText)
 {
  char *FunctionDescription = "mod(x,y)";
  if (in[0].real*ppl_machineEpsilon*10 > in[1].real)
   {
    if (c->set->term_current.ExplicitErrors == SW_ONOFF_ON) { *status = 1; *errType=ERR_NUMERIC; sprintf(errText, "Loss of accuracy in the function %s; the remainder of this division is lost in floating-point rounding.", FunctionDescription); return; }
    else { NULL_OUTPUT; }
   }
  OUTPUT.real = fmod(in[0].real , in[1].real);
  CHECK_OUTPUT_OKAY;
  ppl_unitsDimCpy(&OUTPUT, &in[0]);
 }

void pplfunc_open        (ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText)
 {
  FILE *f;
  char *mode;
  if ((nArgs<1)||(in[0].objType!=PPLOBJ_STR)) { *status=1; *errType=ERR_TYPE; sprintf(errText,"The open() function requires string arguments."); return; }
  if ((nArgs>1)&&(in[1].objType!=PPLOBJ_STR)) { *status=1; *errType=ERR_TYPE; sprintf(errText,"The open() function requires string arguments."); return; }
  if (nArgs>1) mode=(char*)in[1].auxil;
  else         mode="r";
  f = fopen((char*)in[0].auxil,mode);
  if (f==NULL) { *status=1; *errType=ERR_FILE; sprintf(errText,"File open error."); return; }
  pplObjFile(&OUTPUT,0,1,f);
 }

void pplfunc_ordinal     (ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText)
 {
  char *FunctionDescription = "ordinal(n)";
  char *out;
  int n = (int)ceil(fabs(log10(in[0].real)));
  int i = (int)in[0].real;
  if (in[0].real<0) in[0].real=GSL_NAN;
  if (n<2) n=2;
  if (n>64) in[0].real=GSL_NAN;
  CHECK_NEEDINT(in[0], "n", "function's input must be an integer");
  out = (char *)malloc(n + 8);
  if (out==NULL) { *status=1; sprintf(errText,"Out of memory."); return; }
  if      (((i%100)<21) && ((i%100)>3)) sprintf(out, "%dth", i);
  else if  ((i% 10)==1)                 sprintf(out, "%dst", i);
  else if  ((i% 10)==2)                 sprintf(out, "%dnd", i);
  else if  ((i% 10)==3)                 sprintf(out, "%drd", i);
  else                                  sprintf(out, "%dth", i);
  pplObjStr(&OUTPUT,0,1,out);
 }

void pplfunc_pow         (ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText)
 {
  ppl_uaPow(c, &in[0], &in[1], &OUTPUT, status, errType, errText);
 }

void pplfunc_prime       (ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText)
 {
  char *FunctionDescription = "prime(x)";
  CHECK_NEEDINT(in[0], "x", "function's argument must be an integer in the range");
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

void pplfunc_radians     (ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText)
 {
  char *FunctionDescription = "radians(x)";
  int i;
  CHECK_DIMLESS_OR_HAS_UNIT(in[0] , "first", "an angle", UNIT_ANGLE, 1);
  if (in[0].dimensionless) { OUTPUT.real = ppl_rads(in[0].real); } else { OUTPUT.real = in[0].real; }
  CHECK_OUTPUT_OKAY;
 }

void pplfunc_range       (ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText)
 {
  char  *FunctionDescription = "range([f],l,[s])";
  double start, end, step, n;
  int    i;
  if (nArgs>1) { CHECK_2INPUT_DIMMATCH; }
  if (nArgs>2) { in++; CHECK_2INPUT_DIMMATCH; in--; }

  if      (nArgs==1) { start=0; end=in[0].real; step=1; }
  else if (nArgs==2) { start=in[0].real; end=in[1].real; step=1; }
  else               { start=in[0].real; end=in[1].real; step=in[2].real; }
  n = ceil((end-start) / step);
  if ((!gsl_finite(n))||(n>INT_MAX)) { *status=1; *errType=ERR_NUMERIC; strcpy(errText,"Invalid step size."); return; }
  if (n<0) n=0;
  if (pplObjVector(&OUTPUT,0,1,n)==NULL) { *status=1; *errType=ERR_MEMORY; strcpy(errText,"Out of memory."); return; }
  for (i=0; i<n; i++) gsl_vector_set(((pplVector *)OUTPUT.auxil)->v, i, start+i*step);
  ppl_unitsDimCpy(&OUTPUT, &in[0]);
 }

void pplfunc_real        (ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText)
 {
  char *FunctionDescription = "Re(z)";
  OUTPUT.real = in[0].real;
  CHECK_OUTPUT_OKAY;
  ppl_unitsDimCpy(&OUTPUT, &in[0]);
 }

void pplfunc_rgb         (ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText)
 {
  pplObjColor(&OUTPUT,0,SW_COLSPACE_RGB,in[0].real,in[1].real,in[2].real,0);
 }

void pplfunc_root        (ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText)
 {
  pplObj x1,x2;
  double tmpdbl;
  char *FunctionDescription = "root(z,n)";
  unsigned char negated = 0;
  in++;
  CHECK_1INPUT_DIMLESS; // THIS IS CORRECT. Only check in[1]
  in--;
  if ((in[1].flagComplex) || (in[1].real < 2) || (in[1].real >= INT_MAX))
   {
    if (c->set->term_current.ExplicitErrors == SW_ONOFF_ON) { *status = 1; sprintf(errText, "The %s %s in the range 2 <= n < %d.",FunctionDescription,"function's second argument must be an integer in the range",INT_MAX); return; }
    else { NULL_OUTPUT; }
   }
  x1=in[0];
  if (x1.real < 0.0) { negated=1; x1.real=-x1.real; if (x1.imag!=0.0) x1.imag=-x1.imag; }
  pplObjNum(&x2, 0, 1.0 / floor(in[1].real), 0);
  ppl_uaPow(c, &x1, &x2, &OUTPUT, status, errType, errText);
  if (*status) return;
  if (negated)
   {
    if (fmod(floor(in[1].real) , 2) == 1)
     {
      OUTPUT.real=-OUTPUT.real; if (OUTPUT.imag!=0.0) OUTPUT.imag=-OUTPUT.imag;
     } else {
      if (c->set->term_current.ComplexNumbers == SW_ONOFF_OFF) { QUERY_OUT_OF_RANGE; }
      else
       {
        tmpdbl = OUTPUT.imag;
        OUTPUT.imag = OUTPUT.real;
        OUTPUT.real = -tmpdbl;
        OUTPUT.flagComplex = !ppl_dblEqual(OUTPUT.imag, 0);
        if (!OUTPUT.flagComplex) OUTPUT.imag=0.0; // Enforce that real numbers have positive zero imaginary components
       }
     }
   }
  CHECK_OUTPUT_OKAY;
 }

void pplfunc_sec         (ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText)
 {
  char *FunctionDescription = "sec(z)";
  int i;
  gsl_complex z;
  CHECK_DIMLESS_OR_HAS_UNIT(in[0] , "first", "an angle", UNIT_ANGLE, 1);
  GSL_SET_COMPLEX(&z,in[0].real,in[0].imag); z=gsl_complex_sec(z); CLEANUP_GSLCOMPLEX;
  CHECK_OUTPUT_OKAY;
 }

void pplfunc_sech        (ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText)
 {
  char *FunctionDescription = "sech(z)";
  int i;
  gsl_complex z;
  CHECK_DIMLESS_OR_HAS_UNIT(in[0] , "first", "an angle", UNIT_ANGLE, 1);
  GSL_SET_COMPLEX(&z,in[0].real,in[0].imag); z=gsl_complex_sech(z); CLEANUP_GSLCOMPLEX;
  CHECK_OUTPUT_OKAY;
 }

void pplfunc_sin         (ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText)
 {
  char *FunctionDescription = "sin(z)";
  int i;
  gsl_complex z;
  CHECK_DIMLESS_OR_HAS_UNIT(in[0] , "first", "an angle", UNIT_ANGLE, 1);
  IF_1COMPLEX { GSL_SET_COMPLEX(&z,in[0].real,in[0].imag); z=gsl_complex_sin(z); CLEANUP_GSLCOMPLEX; }
  ELSE_REAL   { OUTPUT.real = sin(in[0].real); }
  ENDIF
  CHECK_OUTPUT_OKAY;
 }

void pplfunc_sinc        (ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText)
 {
  char *FunctionDescription = "sinc(z)";
  int i;
  gsl_complex z;
  CHECK_DIMLESS_OR_HAS_UNIT(in[0] , "first", "an angle", UNIT_ANGLE, 1);
  if ((in[0].real==0) && (in[0].imag==0)) { OUTPUT.real = 1.0; }
  else
   {
    IF_1COMPLEX { GSL_SET_COMPLEX(&z,in[0].real, in[0].imag); z=gsl_complex_sin(z); CLEANUP_GSLCOMPLEX; ppl_uaDiv(c, &OUTPUT, &in[0], &OUTPUT, status, errType, errText); }
    ELSE_REAL   { OUTPUT.real = sin(in[0].real)/in[0].real; }
    ENDIF
   }
  CHECK_OUTPUT_OKAY;
 }

void pplfunc_sinh        (ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText)
 {
  char *FunctionDescription = "sinh(z)";
  int i;
  gsl_complex z;
  CHECK_DIMLESS_OR_HAS_UNIT(in[0] , "first", "an angle", UNIT_ANGLE, 1);
  IF_1COMPLEX { GSL_SET_COMPLEX(&z,in[0].real,in[0].imag); z=gsl_complex_sinh(z); CLEANUP_GSLCOMPLEX; }
  ELSE_REAL   { OUTPUT.real = sinh(in[0].real); }
  ENDIF
  CHECK_OUTPUT_OKAY;
 }

void pplfunc_sqrt        (ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText)
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

void pplfunc_sum         (ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText)
 {
  if (nArgs==0) { pplObjNum(&OUTPUT,0,0,0); return; }
  if ((nArgs==1)&&(in[0].objType==PPLOBJ_LIST))
   {
    int first=1;
    pplObj acc, acc2, *item;
    listIterator *li = ppl_listIterateInit((list *)in[0].auxil);
    while ((item = (pplObj *)ppl_listIterate(&li))!=NULL)
     {
      if (first)
       {
        pplObjCpy(&acc2, item, 0, 1);
        memcpy(&acc, &acc2, sizeof(pplObj));
        first=0;
       }
      else
       {
        ppl_opAdd(c, &acc, item, &acc2, 1, status, errType, errText);
        if (*status) { ppl_garbageObject(&acc); return; }
        memcpy(&acc, &acc2, sizeof(pplObj));
       }
     }
    if (first) pplObjNum(&OUTPUT,0,0,0);
    else       memcpy(&OUTPUT, &acc, sizeof(pplObj));
   }
  else if ((nArgs==1)&&((in[0].objType==PPLOBJ_DICT)||(in[0].objType==PPLOBJ_MOD)||(in[0].objType==PPLOBJ_USER)))
   {
    int first=1;
    pplObj acc, acc2, *item;
    char *key;
    dictIterator *di = ppl_dictIterateInit((dict *)in[0].auxil);
    while ((item = (pplObj *)ppl_dictIterate(&di,&key))!=NULL)
     {
      if (first)
       {
        pplObjCpy(&acc2, item, 0, 1);
        memcpy(&acc, &acc2, sizeof(pplObj));
        first=0;
       }
      else
       {
        ppl_opAdd(c, &acc, item, &acc2, 1, status, errType, errText);
        if (*status) { ppl_garbageObject(&acc); return; }
        memcpy(&acc, &acc2, sizeof(pplObj));
       }
     }
    if (first) pplObjNum(&OUTPUT,0,0,0);
    else       memcpy(&OUTPUT, &acc, sizeof(pplObj));
   }
  else if ((nArgs==1)&&(in[0].objType==PPLOBJ_VEC))
   {
    int i;
    double acc=0;
    gsl_vector *v=((pplVector *)(in[0].auxil))->v;
    for (i=0; i<v->size; i++) acc += gsl_vector_get(v,i);
    pplObjNum(&OUTPUT,0,acc,0);
   }
  else if ((nArgs==1)&&(in[0].objType==PPLOBJ_MAT))
   {
    int i,j;
    double acc=0;
    gsl_matrix *m=((pplMatrix *)(in[0].auxil))->m;
    for (i=0; i<m->size1; i++) for (j=0; j<m->size2; j++) acc += gsl_matrix_get(m,i,j);
    pplObjNum(&OUTPUT,0,acc,0);
   }
  else
   {
    int i;
    pplObj acc, acc2;
    pplObjCpy(&acc2, &in[0], 0, 1);
    memcpy(&acc, &acc2, sizeof(pplObj));
    for (i=1; i<nArgs; i++)
     {
      ppl_opAdd(c, &acc, &in[i], &acc2, 1, status, errType, errText);
      if (*status) { ppl_garbageObject(&acc); return; }
      memcpy(&acc, &acc2, sizeof(pplObj));
     }
    memcpy(&OUTPUT,&acc,sizeof(pplObj));
   }
 }

void pplfunc_tan         (ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText)
 {
  char *FunctionDescription = "tan(z)";
  int i;
  gsl_complex z;
  CHECK_DIMLESS_OR_HAS_UNIT(in[0] , "first", "an angle", UNIT_ANGLE, 1);
  IF_1COMPLEX { GSL_SET_COMPLEX(&z,in[0].real,in[0].imag); z=gsl_complex_tan(z); CLEANUP_GSLCOMPLEX; }
  ELSE_REAL   { OUTPUT.real = tan(in[0].real); }
  ENDIF
  CHECK_OUTPUT_OKAY;
 }

void pplfunc_tanh        (ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText)
 {
  char *FunctionDescription = "tanh(z)";
  int i;
  gsl_complex z;
  CHECK_DIMLESS_OR_HAS_UNIT(in[0] , "first", "an angle", UNIT_ANGLE, 1);
  IF_1COMPLEX { GSL_SET_COMPLEX(&z,in[0].real,in[0].imag); z=gsl_complex_tanh(z); CLEANUP_GSLCOMPLEX; }
  ELSE_REAL   { OUTPUT.real = tanh(in[0].real); }
  ENDIF
  CHECK_OUTPUT_OKAY;
 }

void pplfunc_tophat      (ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText)
 {
  char *FunctionDescription = "tophat(x,sigma)";
  CHECK_2INPUT_DIMMATCH;
  if ( fabs(in[0].real) <= fabs(in[1].real) ) OUTPUT.real = 1.0;
 }

void pplfunc_typeOf      (ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText)
 {
  pplObjCpy(&OUTPUT,&pplObjPrototypes[in[0].objType],0,1);
  OUTPUT.self_lval = NULL;
 }

void pplfunc_zernike     (ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText)
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
      if (c->set->term_current.ExplicitErrors == SW_ONOFF_ON) { *status=1; *errType=ERR_RANGE; sprintf(errText, "The function %s is only defined for -n<=m<=n.", FunctionDescription); return; }
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

void pplfunc_zernikeR    (ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText)
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
      if (c->set->term_current.ExplicitErrors == SW_ONOFF_ON) { *status=1; *errType=ERR_RANGE; sprintf(errText, "The function %s is only defined for -n<=m<=n.", FunctionDescription); return; }
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
  CHECK_OUTPUT_OKAY;
 }

void pplfunc_zeta        (ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText)
 {
  char *FunctionDescription = "zeta(z)";
  gsl_complex zi,z;
  IF_1COMPLEX { GSL_SET_COMPLEX(&zi,in[0].real,in[0].imag); riemann_zeta_complex(zi,&z,status,errText); if (*status) return; CLEANUP_GSLCOMPLEX; }
  ELSE_REAL   { OUTPUT.real = gsl_sf_zeta(in[0].real); }
  ENDIF
  CHECK_OUTPUT_OKAY;
 }

