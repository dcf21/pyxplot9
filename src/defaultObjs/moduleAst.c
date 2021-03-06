// moduleAst.c
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

#include <gsl/gsl_const_mksa.h>
#include <gsl/gsl_integration.h>
#include <gsl/gsl_math.h>
#include <gsl/gsl_multimin.h>

#include "coreUtils/dict.h"

#include "settings/settings.h"

#include "stringTools/asciidouble.h"

#include "userspace/pplObj.h"
#include "userspace/pplObjFunc.h"
#include "userspace/unitsArithmetic.h"

#include "defaultObjs/moduleAst.h"
#include "defaultObjs/defaultFuncs.h"
#include "defaultObjs/defaultFuncsMacros.h"

// Lambda Cold Dark Matter Cosmology Engine

typedef struct Lcdm_comm {
 double OmegaM, OmegaL, H, t;
 } Lcdm_comm;

double __Lcdm_DA_integrand(double z, void *params)
 {
  // All equations below are from
  // Distance Measured in Cosmology
  // David W. Hogg
  // http://arxiv.org/abs/astro-ph/9905116
  Lcdm_comm *p = (Lcdm_comm *)params;
  return 1.0 / sqrt(p->OmegaM*pow(1+z,3) + p->OmegaL); // Eq (14), as substituted into eq (15) and (16). Assume omega_k is zero
 }

double __Lcdm_t_integrand(double z, void *params)
 {
  // The equations for the following are taken from:
  // THE COSMOLOGICAL CONSTANT
  // Sean M. Carroll and William H. Press
  // http://nedwww.ipac.caltech.edu/level5/Carroll/frames.html
  Lcdm_comm *p = (Lcdm_comm *)params;
  return pow(1+z,-1) * pow(pow(1+z,2)*(1+p->OmegaM*z) - z*(2+z)*p->OmegaL,-0.5); // Equation (16)
 }


double Lcdm_age(double H, double OmegaM, double OmegaL)
 {
  double OmegaA = OmegaM - 0.3*(OmegaM+OmegaL) + 0.3;
  if (OmegaA<=1.0) return 2.0/3.0/H * gsl_asinh(sqrt(fabs(1.0-OmegaA)/OmegaA)) / sqrt(fabs(1.0-OmegaA));
  else             return 2.0/3.0/H *     asin (sqrt(fabs(1.0-OmegaA)/OmegaA)) / sqrt(fabs(1.0-OmegaA));
 }

double Lcdm_DA(double z, double H, double OmegaM, double OmegaL)
 {
  Lcdm_comm                  p;
  gsl_integration_workspace *ws;
  gsl_function               fn;
  double result, error;

  if (z<=0) return GSL_NAN;
  p.OmegaM = OmegaM; p.OmegaL = OmegaL;

  ws          = gsl_integration_workspace_alloc(1000);
  fn.function = &__Lcdm_DA_integrand;
  fn.params   = &p;
  gsl_integration_qags (&fn, 1e-8, z, 0, 1e-7, 1000, ws, &result, &error);
  gsl_integration_workspace_free(ws);

  if ((result<=0)||(error>result*0.01)) return GSL_NAN; // Something went wrong
  return (GSL_CONST_MKSA_SPEED_OF_LIGHT / H) * result / (1+z);
 }

double Lcdm_t_from_z(double z, double H, double OmegaM, double OmegaL)
 {
  Lcdm_comm                  p;
  gsl_integration_workspace *ws;
  gsl_function               fn;
  double result, error;

  if (z<=0) return GSL_NAN;
  p.OmegaM = OmegaM; p.OmegaL = OmegaL;

  ws          = gsl_integration_workspace_alloc(1000);
  fn.function = &__Lcdm_t_integrand;
  fn.params   = &p;
  gsl_integration_qags (&fn, 1e-8, z, 0, 1e-7, 1000, ws, &result, &error);
  gsl_integration_workspace_free(ws);

  if ((result<=0)||(error>result*0.01)) return GSL_NAN; // Something went wrong
  return (1/H) * result;
 }

double __Lcdm_z_from_t_slave(double z, void *params)
 {
  Lcdm_comm *p = (Lcdm_comm *)params;
  return fabs(Lcdm_t_from_z(pow(10.0,z), p->H, p->OmegaM, p->OmegaL) - p->t);
 }

double Lcdm_z_from_t(double t, double H, double OmegaM, double OmegaL)
 {
  Lcdm_comm p;
  int iter = 0, max_iter = 100, status;
  double m, a, b;
  const gsl_min_fminimizer_type *T = gsl_min_fminimizer_goldensection;
  gsl_min_fminimizer *s;
  gsl_function fn;

  if ((t<=0)||(t>Lcdm_age(H,OmegaM,OmegaL))) return GSL_NAN;
  p.OmegaM = OmegaM; p.OmegaL = OmegaL; p.H = H; p.t = t;

  fn.function = &__Lcdm_z_from_t_slave;
  fn.params   = &p;
  s           = gsl_min_fminimizer_alloc(T);
  if (s==NULL) return GSL_NAN;
  gsl_min_fminimizer_set(s, &fn, 0.0, -3.0, 8.0);

  do
   {
    iter++;
    status = gsl_min_fminimizer_iterate(s);

    m = gsl_min_fminimizer_x_minimum(s);
    a = gsl_min_fminimizer_x_lower(s);
    b = gsl_min_fminimizer_x_upper(s);

    status = gsl_min_test_interval (a, b, 0.001, 0.0);
   }
  while ((status == GSL_CONTINUE) && (iter < max_iter));
  gsl_min_fminimizer_free(s);
  return pow(10.0,m);
 }

// Lambda Cold Dark Matter Cosmology Wrappers

#define CHECK_LCDM_INPUTS(XN,H0NUM) \
 CHECK_DIMLESS_OR_HAS_UNIT(in[XN], H0NUM, "a recession velocity per unit distance", UNIT_TIME, -1); \
 if (in[XN].dimensionless) in[XN].real *= 1e3 / (GSL_CONST_MKSA_PARSEC * 1e6); \
 if (!(in[XN+1].dimensionless && in[XN+2].dimensionless)) \
   { \
    *status = 1; \
    *errType = ERR_UNIT; \
    sprintf(errText, "The %s function can only act upon dimensionless values for w_m and w_l. Supplied values have dimensions of <%s> and <%s>.", FunctionDescription, ppl_printUnit(c, &in[XN+1], NULL, NULL, 0, 1, 0), ppl_printUnit(c, &in[XN+2], NULL, NULL, 1, 1, 0)); \
    return; \
   } \

#define CHECK_LCDM_REDSHIFT \
 if (!(in[0].dimensionless)) \
   { \
    *status = 1; \
    *errType = ERR_UNIT; \
    sprintf(errText, "The %s function can only act upon dimensionless values for redshift. Supplied value has dimensions of <%s>.", FunctionDescription, ppl_printUnit(c, &in[0], NULL, NULL, 0, 1, 0)); \
    return; \
   } \

void pplfunc_Lcdm_age     (ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText)
 {
  char *FunctionDescription = "Lcdm_age(H0,w_m,w_l)";
  int i;
  CHECK_LCDM_INPUTS(0,"first");
  OUTPUT.real = Lcdm_age(in[0].real, in[1].real, in[2].real);
  CLEANUP_APPLYUNIT(UNIT_TIME);
  CHECK_OUTPUT_OKAY;
 }

void pplfunc_Lcdm_angscale(ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText)
 {
  char *FunctionDescription = "Lcdm_age(z,H0,w_m,w_l)";
  int i;
  CHECK_LCDM_INPUTS(1,"second");
  CHECK_LCDM_REDSHIFT;
  OUTPUT.real = Lcdm_DA(in[0].real, in[1].real, in[2].real, in[3].real);
  CLEANUP_APPLYUNIT(UNIT_LENGTH);
  OUTPUT.exponent[UNIT_ANGLE] = -1;
  CHECK_OUTPUT_OKAY;
 }

void pplfunc_Lcdm_DA      (ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText)
 {
  char *FunctionDescription = "Lcdm_DA(z,H0,w_m,w_l)";
  int i;
  CHECK_LCDM_INPUTS(1,"second");
  CHECK_LCDM_REDSHIFT;
  OUTPUT.real = Lcdm_DA(in[0].real, in[1].real, in[2].real, in[3].real);
  CLEANUP_APPLYUNIT(UNIT_LENGTH);
  CHECK_OUTPUT_OKAY;
 }

void pplfunc_Lcdm_DL      (ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText)
 {
  char *FunctionDescription = "Lcdm_DL(z,H0,w_m,w_l)";
  int i;
  CHECK_LCDM_INPUTS(1,"second");
  CHECK_LCDM_REDSHIFT;
  OUTPUT.real = Lcdm_DA(in[0].real, in[1].real, in[2].real, in[3].real) * pow(1+in[0].real, 2);
  CLEANUP_APPLYUNIT(UNIT_LENGTH);
  CHECK_OUTPUT_OKAY;
 }

void pplfunc_Lcdm_DM      (ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText)
 {
  char *FunctionDescription = "Lcdm_DM(z,H0,w_m,w_l)";
  int i;
  CHECK_LCDM_INPUTS(1,"second");
  CHECK_LCDM_REDSHIFT;
  OUTPUT.real = Lcdm_DA(in[0].real, in[1].real, in[2].real, in[3].real) * (1+in[0].real);
  CLEANUP_APPLYUNIT(UNIT_LENGTH);
  CHECK_OUTPUT_OKAY;
 }

void pplfunc_Lcdm_t       (ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText)
 {
  char *FunctionDescription = "Lcdm_t(z,H0,w_m,w_l)";
  int i;
  CHECK_LCDM_INPUTS(1,"second");
  CHECK_LCDM_REDSHIFT;
  OUTPUT.real = Lcdm_t_from_z(in[0].real, in[1].real, in[2].real, in[3].real);
  CLEANUP_APPLYUNIT(UNIT_TIME);
  CHECK_OUTPUT_OKAY;
 }

void pplfunc_Lcdm_z       (ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText)
 {
  char *FunctionDescription = "Lcdm_z(t,H0,w_m,w_l)";
  int i;
  CHECK_LCDM_INPUTS(1,"second");
  CHECK_DIMLESS_OR_HAS_UNIT(in[0], "first", "a time", UNIT_TIME, 1);
  OUTPUT.real = Lcdm_z_from_t(in[0].real, in[1].real, in[2].real, in[3].real);
  CHECK_OUTPUT_OKAY;
 }

void pplfunc_sidereal_time(ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText)
 {
  char *FunctionDescription = "siderealTime(d)";
  if ((nArgs!=1)&&((in[0].objType!=PPLOBJ_NUM)||(in[0].objType!=PPLOBJ_DATE))) { *status=1; *errType=ERR_TYPE; sprintf(errText, "The function %s requires a date object or numeric Unix time as its first argument; supplied argument had type <%s>.", FunctionDescription, pplObjTypeNames[in[0].objType]); return; }
  CHECK_1NOTNAN;
  CHECK_1INPUT_DIMLESS;
  IF_1COMPLEX { QUERY_MUST_BE_REAL }
  ELSE_REAL
   {
    double JD   = in->real / 86400.0 + 40587.5;
    double T = (JD - 51545.0) / 36525.0; // See pages 87-88 of Astronomical Algorithms, by Jean Meeus
    OUTPUT.real = fmod( M_PI/180 * (
                                    280.46061837 +
                                    360.98564736629 * (JD - 51545.0) +
                                    0.000387933     * T*T +
                                    T*T*T / 38710000.0
                                   ), 2*M_PI);
   }
  ENDIF
  CLEANUP_APPLYUNIT(UNIT_ANGLE);
  CHECK_OUTPUT_OKAY;
 }

void pplfunc_moonphase    (ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText)
 {
  char *FunctionDescription = "moonPhase(d)";
  if ((nArgs!=1)&&((in[0].objType!=PPLOBJ_NUM)||(in[0].objType!=PPLOBJ_DATE))) { *status=1; *errType=ERR_TYPE; sprintf(errText, "The function %s requires a date object or numeric Unix time as its first argument; supplied argument had type <%s>.", FunctionDescription, pplObjTypeNames[in[0].objType]); return; }
  CHECK_1NOTNAN;
  CHECK_1INPUT_DIMLESS;
  IF_1COMPLEX { QUERY_MUST_BE_REAL }
  ELSE_REAL
   {
    double JD   = in->real / 86400.0 + 40587.5;
    double t    = (JD - 51545)/36525; // Time in Julian Centuries since 2000.0
    double Msun = 2*M_PI*fmod(0.993133+99.997361*t, 1);
    double Lsun = 2*M_PI*fmod(0.7859453+Msun/(2*M_PI)+(6893.0*sin(Msun)+72.0*sin(2*Msun)+6191.2*t)/1296e3, 1);
    double L0   = 2*M_PI*fmod(0.606433+1336.855225*t, 1);
    double l    = 2*M_PI*fmod(0.374897+1325.552410*t, 1);
    double ls   = 2*M_PI*fmod(0.993133+99.997361*t, 1);
    double D    = 2*M_PI*fmod(0.827361+1236.853086*t, 1);
    double F    = 2*M_PI*fmod(0.259086+1342.227825*t, 1);
    double dL   = 22640*sin(l) - 4586*sin(l-2*D) + 2370*sin(2*D) + 769*sin(2*l) - 668*sin(ls) - 412*sin(2*F) - 212*sin(2*l-2*D) - 206*sin(l+ls-2*D) + 192*sin(l+2*D) - 165*sin(ls-2*D) - 125*sin(D) - 110*sin(l+ls) + 148*sin(l-ls) - 55*sin(2*F-2*D);
    OUTPUT.real = fmod(L0 + dL/1296e3*2*M_PI - Lsun , 2*M_PI);
    while (OUTPUT.real<0) OUTPUT.real += 2*M_PI;
   }
  ENDIF
  CLEANUP_APPLYUNIT(UNIT_ANGLE);
  CHECK_OUTPUT_OKAY;
 }

