// defaultFuncsMacros.h
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

#ifndef _PPL_DEFAULT_FUNCTIONS_MACROS_H
#define _PPL_DEFAULT_FUNCTIONS_MACROS_H 1

#include "settings/settings.h"
#include "settings/settingTypes.h"
#include "userspace/pplObj.h"
#include "userspace/pplObjUnits.h"
#include "userspace/unitsDisp.h"

#define OUTPUT in[nArgs]

#define NULL_OUTPUT \
 { OUTPUT.real = GSL_NAN; OUTPUT.imag = 0; OUTPUT.flagComplex=0; return; }

#define QUERY_OUT_OF_RANGE \
 { \
  if (term->ExplicitErrors == SW_ONOFF_ON) { *status=1; *errType=ERR_RANGE; sprintf(errText, "The function %s is not defined at the requested point in parameter space.", FunctionDescription); return; } \
  else { NULL_OUTPUT; } \
 }

#define QUERY_MUST_BE_REAL \
 { \
  if (term->ExplicitErrors == SW_ONOFF_ON) { *status=1; *errType=ERR_RANGE; sprintf(errText, "The function %s only accepts real arguments; the supplied arguments are complex.", FunctionDescription); return; } \
  else { NULL_OUTPUT; } \
 }

#define CHECK_NEEDLONG(X, VAR, DESCRIPTION) \
 { \
  if (((X)->flagComplex) || ((X)->real < 0) || ((X)->real >= LONG_MAX)) \
   { \
    if (term->ExplicitErrors == SW_ONOFF_ON) { *status = 1; *errType=ERR_RANGE; sprintf(errText, "The %s %s in the range 0 <= %s < %ld.",FunctionDescription,DESCRIPTION,VAR,LONG_MAX); return; } \
    else { NULL_OUTPUT; } \
   } \
 }

#define CHECK_NEEDINT(X, VAR, DESCRIPTION) \
 { \
  if (((X)->flagComplex) || ((X)->real < 0) || ((X)->real >= INT_MAX)) \
   { \
    if (term->ExplicitErrors == SW_ONOFF_ON) { *status = 1; *errType=ERR_RANGE; sprintf(errText, "The %s %s in the range 0 <= %s < %d.",FunctionDescription,DESCRIPTION,VAR,INT_MAX); return; } \
    else { NULL_OUTPUT; } \
   } \
 }

#define CHECK_NEEDSINT(X, VAR, DESCRIPTION) \
 { \
  if (((X)->real <= INT_MIN) || ((X)->real >= INT_MAX)) \
   { \
    if (term->ExplicitErrors == SW_ONOFF_ON) { *status = 1; *errType=ERR_RANGE; sprintf(errText, "The %s %s in the range %d <= %s < %d.",FunctionDescription,DESCRIPTION,INT_MIN,VAR,INT_MAX); return; } \
    else { NULL_OUTPUT; } \
   } \
 }

#define WRAPPER_INIT \
 { \
  *status = 0; \
  ppl_units_zero(OUTPUT); \
 }

#define NAN_CHECK_FAIL \
 { \
  if (term->ExplicitErrors == SW_ONOFF_ON) { *status = 1; *errType=ERR_RANGE; sprintf(errText, "The function %s has received a non-finite input.",FunctionDescription); return; } \
  else { NULL_OUTPUT; } \
 }

#define CHECK_1NOTNAN \
 { \
  WRAPPER_INIT; \
  if ((term->ComplexNumbers == SW_ONOFF_OFF) && (in[0].flagComplex)) { NAN_CHECK_FAIL; } \
  if ((!gsl_finite(in[0].real)) || (!gsl_finite(in[0].imag))) { NAN_CHECK_FAIL; } \
 }

#define CHECK_2NOTNAN \
 { \
  WRAPPER_INIT; \
  if ((term->ComplexNumbers == SW_ONOFF_OFF) && ((in[0].flagComplex) || (in[1].flagComplex))) { NAN_CHECK_FAIL; } \
  if ((!gsl_finite(in[0].real)) || (!gsl_finite(in[0].imag))) { NAN_CHECK_FAIL; } \
  if ((!gsl_finite(in[1].real)) || (!gsl_finite(in[1].imag))) { NAN_CHECK_FAIL; } \
 }

#define CHECK_3NOTNAN \
 { \
  CHECK_2NOTNAN; \
  if ((term->ComplexNumbers == SW_ONOFF_OFF) && (in[2].flagComplex)) { NAN_CHECK_FAIL; } \
  if ((!gsl_finite(in[2].real)) || (!gsl_finite(in[2].imag))) { NAN_CHECK_FAIL; } \
 }

#define CHECK_4NOTNAN \
 { \
  CHECK_3NOTNAN; \
  if ((term->ComplexNumbers == SW_ONOFF_OFF) && (in[3].flagComplex)) { NAN_CHECK_FAIL; } \
  if ((!gsl_finite(in[3].real)) || (!gsl_finite(in[3].imag))) { NAN_CHECK_FAIL; } \
 }

#define CHECK_5NOTNAN \
 { \
  CHECK_4NOTNAN; \
  if ((term->ComplexNumbers == SW_ONOFF_OFF) && (in[4].flagComplex)) { NAN_CHECK_FAIL; } \
  if ((!gsl_finite(in[4].real)) || (!gsl_finite(in[4].imag))) { NAN_CHECK_FAIL; } \
 }

#define CHECK_6NOTNAN \
 { \
  CHECK_5NOTNAN; \
  if ((term->ComplexNumbers == SW_ONOFF_OFF) && (in[5].flagComplex)) { NAN_CHECK_FAIL; } \
  if ((!gsl_finite(in[5].real)) || (!gsl_finite(in[5].imag))) { NAN_CHECK_FAIL; } \
 }

#define CHECK_1INPUT_DIMLESS \
 { \
  if (!(in[0].dimensionless)) \
   { \
    *status = 1; \
    *errType=ERR_UNIT; \
    sprintf(errText, "The %s function can only act upon dimensionless inputs. Supplied input has dimensions of <%s>.", FunctionDescription, ppl_units_GetUnitStr(in, NULL, NULL, 1, 1, 0)); \
    return; \
   } \
 }

#define CHECK_2INPUT_DIMLESS \
 { \
  if (!(in[0].dimensionless && in[1].dimensionless)) \
   { \
    *status = 1; \
    *errType=ERR_UNIT; \
    sprintf(errText, "The %s function can only act upon dimensionless inputs. Supplied inputs have dimensions of <%s> and <%s>.", FunctionDescription, ppl_units_GetUnitStr(in[0], NULL, NULL, 0, 1, 0), ppl_units_GetUnitStr(in[1], NULL, NULL, 1, 1, 0)); \
    return; \
   } \
 }

#define CHECK_3INPUT_DIMLESS \
 { \
  if (!(in[0].dimensionless && in[1].dimensionless && in[2].dimensionless)) \
   { \
    *status = 1; \
    *errType=ERR_UNIT; \
    sprintf(errText, "The %s function can only act upon dimensionless inputs. Supplied inputs have dimensions of <%s>, <%s> and <%s>.", FunctionDescription, ppl_units_GetUnitStr(in[0], NULL, NULL, 0, 1, 0), ppl_units_GetUnitStr(in[1], NULL, NULL, 1, 1, 0), ppl_units_GetUnitStr(in[2], NULL, NULL, 2, 1, 0)); \
    return; \
   } \
 }

#define CHECK_4INPUT_DIMLESS \
 { \
  if (!(in[0].dimensionless && in[1].dimensionless && in[2].dimensionless && in[3].dimensionless)) \
   { \
    *status = 1; \
    *errType=ERR_UNIT; \
    sprintf(errText, "The %s function can only act upon dimensionless inputs.", FunctionDescription); \
    return; \
   } \
 }

#define CHECK_5INPUT_DIMLESS \
 { \
  if (!(in[0].dimensionless && in[1].dimensionless && in[2].dimensionless && in[3].dimensionless && in[4].dimensionless)) \
   { \
    *status = 1; \
    *errType=ERR_UNIT; \
    sprintf(errText, "The %s function can only act upon dimensionless inputs.", FunctionDescription); \
    return; \
   } \
 }
#define CHECK_6INPUT_DIMLESS \
 { \
  if (!(in[0].dimensionless && in[1].dimensionless && in[2].dimensionless && in[3].dimensionless && in[4].dimensionless && in[5].dimensionless)) \
   { \
    *status = 1; \
    *errType=ERR_UNIT; \
    sprintf(errText, "The %s function can only act upon dimensionless inputs.", FunctionDescription); \
    return; \
   } \
 }

#define CHECK_2INPUT_DIMMATCH \
 { \
  if ((!(in[0].dimensionless && in[1].dimensionless)) && (!(ppl_units_DimEqual(in[0], in[1])))) \
   { \
    *status = 1; \
    *errType=ERR_UNIT; \
    sprintf(errText, "The %s function can only act upon inputs with matching dimensions. Supplied inputs have dimensions of <%s> and <%s>.", FunctionDescription, ppl_units_GetUnitStr(in[0], NULL, NULL, 0, 1, 0), ppl_units_GetUnitStr(in[1], NULL, NULL, 1, 1, 0)); \
    return; \
   } \
 }

#define CHECK_DIMLESS_OR_HAS_UNIT(X, DESCRIPTION, UNITNAME, UNIT, UNITN) \
 { \
  if (!((X)->dimensionless)) \
   for (i=0; i<UNITS_MAX_BASEUNITS; i++) \
    if ((X)->exponent[i] != UNITN*(i==UNIT)) \
     { \
      *status = 1; \
      *errType=ERR_UNIT; \
      sprintf(errText, "The %s argument to the %s function must be %s. Supplied input has dimensions of <%s>.", DESCRIPTION, FunctionDescription, UNITNAME, ppl_units_GetUnitStr((X), NULL, NULL, 1, 1, 0)); \
      return; \
     } \
 } \

#define IF_1COMPLEX if (in[0].flagComplex) {
#define IF_2COMPLEX if ((in[0].flagComplex) || (in[1].flagComplex)) {
#define IF_3COMPLEX if ((in[0].flagComplex) || (in[1].flagComplex) || (in[2].flagComplex)) {
#define IF_4COMPLEX if ((in[0].flagComplex) || (in[1].flagComplex) || (in[2].flagComplex) || (in[3].flagComplex)) {
#define IF_5COMPLEX if ((in[0].flagComplex) || (in[1].flagComplex) || (in[2].flagComplex) || (in[3].flagComplex) || (in[4].flagComplex)) {
#define IF_6COMPLEX if ((in[0].flagComplex) || (in[1].flagComplex) || (in[2].flagComplex) || (in[3].flagComplex) || (in[4].flagComplex) || (in[5].flagComplex)) {
#define ELSE_REAL   } else {
#define ENDIF       }

#define CLEANUP_GSLCOMPLEX \
  OUTPUT.real = GSL_REAL(z); \
  OUTPUT.imag = GSL_IMAG(z); \
  OUTPUT.flagComplex = !ppl_dblEqual(OUTPUT.imag,0); \
  if (!OUTPUT.flagComplex) OUTPUT.imag=0.0;

#define CLEANUP_APPLYUNIT(UNIT) \
  OUTPUT.dimensionless = 0; \
  OUTPUT.exponent[UNIT] = 1; \


#define CHECK_OUTPUT_OKAY \
 if ((!gsl_finite(OUTPUT.real)) || (!gsl_finite(OUTPUT.imag)) || ((OUTPUT.flagComplex) && (term->ComplexNumbers == SW_ONOFF_OFF))) \
  { QUERY_OUT_OF_RANGE; }

#endif

