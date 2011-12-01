// modulePhy.c
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

#include "coreUtils/dict.h"

#include "settings/settings.h"

#include "stringTools/asciidouble.h"

#include "userspace/pplObj.h"
#include "userspace/pplObjFunc.h"
#include "userspace/unitsArithmetic.h"

#include "defaultObjs/defaultFuncs.h"
#include "defaultObjs/defaultFuncsMacros.h"

void pplfunc_planck_Bv   (ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText)
 {
  char *FunctionDescription = "Bv(nu,T)";
  int i;
  pplObj kelvin;

  CHECK_DIMLESS_OR_HAS_UNIT(in[0], "first", "a frequency", UNIT_TIME, -1);
  CHECK_DIMLESS_OR_HAS_UNIT(in[1], "second", "a temperature", UNIT_TEMPERATURE, 1);
  pplObjZero(&kelvin,0);
  kelvin.real = 1.0;
  kelvin.exponent[UNIT_TEMPERATURE] = 1;
  kelvin.tempType = 1;
  ppl_uaDiv(c, &in[1], &kelvin, &kelvin, status, errType, errText); // Convert in[1] into kelvin
  if (*status) kelvin.real = GSL_NAN;
  OUTPUT.dimensionless = 0;
  OUTPUT.exponent[UNIT_MASS]  =  1;
  OUTPUT.exponent[UNIT_TIME]  = -2;
  OUTPUT.exponent[UNIT_ANGLE] = -2;
  OUTPUT.real                 =  2 * GSL_CONST_MKSA_PLANCKS_CONSTANT_H / pow(GSL_CONST_MKSA_SPEED_OF_LIGHT, 2) * pow(in[0].real,3) / expm1(GSL_CONST_MKSA_PLANCKS_CONSTANT_H * in[0].real / GSL_CONST_MKSA_BOLTZMANN / kelvin.real);
  CHECK_OUTPUT_OKAY;
 }

void pplfunc_planck_Bvmax(ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText)
 {
  char *FunctionDescription = "Bvmax(T)";
  int i;
  pplObj kelvin;

  CHECK_DIMLESS_OR_HAS_UNIT(in[0] , "first" , "a temperature", UNIT_TEMPERATURE, 1);
  pplObjZero(&kelvin,0);
  kelvin.real = 1.0;
  kelvin.exponent[UNIT_TEMPERATURE] = 1;
  kelvin.tempType = 1;
  ppl_uaDiv(c, &in[0], &kelvin, &kelvin, status, errType, errText); // Convert in into kelvin
  if (*status) kelvin.real = GSL_NAN;
  OUTPUT.dimensionless = 0;
  OUTPUT.exponent[UNIT_TIME] = -1;
  OUTPUT.real = 2.821439 * GSL_CONST_MKSA_BOLTZMANN / GSL_CONST_MKSA_PLANCKS_CONSTANT_H * kelvin.real; // Wien displacement law
  CHECK_OUTPUT_OKAY;
 }

