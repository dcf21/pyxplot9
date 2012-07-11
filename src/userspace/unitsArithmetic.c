// unitsArithmetic.c
//
// The code in this file is part of Pyxplot
// <http://www.pyxplot.org.uk>
//
// Copyright (C) 2006-2012 Dominic Ford <coders@pyxplot.org.uk>
//               2008-2012 Ross Church
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
#include <ctype.h>
#include <math.h>
#include <string.h>

#include <gsl/gsl_complex.h>
#include <gsl/gsl_complex_math.h>
#include <gsl/gsl_math.h>

#include "stringTools/asciidouble.h"
#include "settings/settingTypes.h"
#include "userspace/context.h"
#include "userspace/pplObj_fns.h"
#include "userspace/pplObjUnits.h"
#include "userspace/unitsDisp.h"
#include "userspace/unitsArithmetic.h"

// -------------------------------------------------------------------
// Functions for comparing the units of quantities
// -------------------------------------------------------------------

void ppl_unitsDimCpy(pplObj *o, const pplObj *i)
 {
  int j;
  o->dimensionless = i->dimensionless;
  o->tempType      = i->tempType;
  for (j=0; j<UNITS_MAX_BASEUNITS; j++) o->exponent[j] = i->exponent[j];
  return;
 }

void ppl_unitsDimInverse(pplObj *o, const pplObj *i)
 {
  int j;
  o->dimensionless = i->dimensionless;
  for (j=0; j<UNITS_MAX_BASEUNITS; j++) o->exponent[j] = -i->exponent[j];
  o->tempType = 0; // Either input was per temperature, or we are now per temperature.
  return;
 }

int ppl_unitsDimEqual(const pplObj *a, const pplObj *b)
 {
  int j;
  if (a->dimensionless && b->dimensionless) return 1;
  for (j=0; j<UNITS_MAX_BASEUNITS; j++) if (ppl_dblEqual(a->exponent[j] , b->exponent[j]) == 0) return 0;
  return 1;
 }

int ppl_unitsDimEqual2(const pplObj *a, const unit *b)
 {
  int j;
  for (j=0; j<UNITS_MAX_BASEUNITS; j++) if (ppl_dblEqual(a->exponent[j] , b->exponent[j]) == 0) return 0;
  return 1;
 }

int ppl_unitsDimEqual3(const unit *a, const unit *b)
 {
  int j;
  for (j=0; j<UNITS_MAX_BASEUNITS; j++) if (ppl_dblEqual(a->exponent[j] , b->exponent[j]) == 0) return 0;
  return 1;
 }

unsigned char ppl_tempTypeMatch(unsigned char a, unsigned char b)
 {
  if ((a<1)||(b<1)) return 1; // anything is compatible with something which doesn't have dimensions of temperature
  return (a==b);
 }

// -------------------------------
// ARITHMETIC OPERATIONS ON VALUES
// -------------------------------

void ppl_uaPow(ppl_context *c, const pplObj *a, const pplObj *b, pplObj *o, int *status, int *errType, char *errText)
 {
  int i;
  double exponent;
  gsl_complex ac, bc;
  unsigned char DimLess=1;

  exponent = b->real; // We may overwrite this when we set o->real, so store a copy

  if (b->dimensionless == 0)
   {
    if (c->set->term_current.ExplicitErrors == SW_ONOFF_OFF) { pplObjNum(o, o->amMalloced, GSL_NAN, 0.0); return; }
    else { sprintf(errText, "Exponent should be dimensionless, but instead has dimensions of <%s>.", ppl_printUnit(c, b, NULL, NULL, 0, 1, 0)); *errType=ERR_UNIT; *status = 1; return; }
   }

  if (c->set->term_current.ComplexNumbers == SW_ONOFF_OFF) // Real pow()
   {
    if (a->flagComplex || b->flagComplex) { o->real = GSL_NAN; }
    else
     {
      o->real = pow(a->real, exponent);
      if ((!gsl_finite(o->real))&&(c->set->term_current.ExplicitErrors == SW_ONOFF_ON)) { sprintf(errText, "Exponentiation operator produced an overflow error or a complex number result. To enable complex arithmetic, type 'set numerics complex'."); *errType=ERR_NUMERICAL; *status = 1; return; }
     }
    o->imag = 0.0;
    o->flagComplex=0;
   }
  else // Complex pow()
   {
    if ((a->dimensionless == 0) && (b->flagComplex))
     {
      if (c->set->term_current.ExplicitErrors == SW_ONOFF_OFF) { pplObjNum(o, o->amMalloced, GSL_NAN, 0.0); return; }
      else { sprintf(errText, "Raising quantities with physical units to complex powers produces quantities with complex physical dimensions, which is forbidden. The operand in question has dimensions of <%s>.", ppl_printUnit(c, a, NULL, NULL, 0, 1, 0)); *errType=ERR_UNIT; *status = 1; return; }
     }
    else
     {
      if (!b->flagComplex)
       {
        GSL_SET_COMPLEX(&ac, a->real, a->imag);
        ac = gsl_complex_pow_real(ac, b->real);
       }
      else
       {
        GSL_SET_COMPLEX(&ac, a->real, a->imag);
        GSL_SET_COMPLEX(&bc, b->real, b->imag);
        ac = gsl_complex_pow(ac, bc);
       }
      o->real = GSL_REAL(ac);
      o->imag = GSL_IMAG(ac);
      o->flagComplex = fabs(o->imag)>(fabs(o->real)*1e-15);
      if (!o->flagComplex) o->imag=0.0; // Enforce that real numbers have positive zero imaginary components
      if ((!gsl_finite(o->real))||(!gsl_finite(o->imag)))
       {
        if (c->set->term_current.ExplicitErrors == SW_ONOFF_ON) { sprintf(errText, "Exponentiation operator produced an overflow error."); *errType=ERR_OVERFLOW; *status = 1; return; }
        else { pplObjNum(o, o->amMalloced, GSL_NAN, 0.0); return; }
       }
     }
   }

  if (a->dimensionless != 0) { if ((o != a) && (o != b)) ppl_unitsDimCpy(o,a); return; }

  for (i=0; i<UNITS_MAX_BASEUNITS; i++)
   {
    o->exponent[i] = a->exponent[i] * exponent;
    if (ppl_dblEqual(o->exponent[i], 0) == 0) DimLess=0;
    if (fabs(o->exponent[i]) > 20000 )
     {
      if (c->set->term_current.ExplicitErrors == SW_ONOFF_OFF) { pplObjNum(o, o->amMalloced, GSL_NAN, 0.0); return; }
      else { sprintf(errText, "Overflow of physical dimensions of argument."); *errType=ERR_OVERFLOW; *status = 1; return; }
     }
   }
  o->tempType = a->tempType;
  o->dimensionless = DimLess;
  return;
 }

void ppl_uaMul(ppl_context *c, const pplObj *a, const pplObj *b, pplObj *o, int *status, int *errType, char *errText)
 {
  int i;
  double tmp, areal, breal, aimag, bimag;
  unsigned char DimLess=1;

  areal = a->real ; aimag = a->imag;
  breal = b->real ; bimag = b->imag;

  // Two inputs have conflicting temperature units. This is only allowed in the special case of oC/oF and friends, when temperature conversion happens.
  if (!ppl_tempTypeMatch(a->tempType, b->tempType))
   {
    if      (ppl_dblEqual(a->exponent[UNIT_TEMPERATURE], 1.0) && (ppl_dblEqual(b->exponent[UNIT_TEMPERATURE],-1.0)))
     {
      areal = areal + c->tempTypeOffset[a->tempType] - c->tempTypeOffset[b->tempType]; // Remember, areal and breal have already had multipliers applied.
     }
    else if (ppl_dblEqual(a->exponent[UNIT_TEMPERATURE],-1.0) && (ppl_dblEqual(b->exponent[UNIT_TEMPERATURE], 1.0)))
     {
      breal = breal + c->tempTypeOffset[b->tempType] - c->tempTypeOffset[a->tempType]; // Imaginary part needs to conversion... multiplication already done.
     }
    else
     { *status = 1; *errType=ERR_UNIT; sprintf(errText, "Attempt to multiply quantities with different temperature units: left operand has units of <%s>, while right operand has units of <%s>. These must be explicitly cast onto the same temperature scale before multiplication is allowed. Type 'help units temperatures' for more details.", ppl_printUnit(c, a, NULL, NULL, 0, 1, 0), ppl_printUnit(c, b, NULL, NULL, 1, 1, 0) );}
   }

  // If one or other input has temperature dependence, we need to propagate which unit of temperature is being used.
  // If we're in one of the cases handled above, don't worry as temperature exponent is about to end up as zero after, e.g. oC/oF
  o->tempType = (a->tempType > b->tempType) ? a->tempType : b->tempType;

  if ((c->set->term_current.ComplexNumbers == SW_ONOFF_OFF) || ((!a->flagComplex) && (!b->flagComplex))) // Real multiplication
   {
    if (a->flagComplex || b->flagComplex) { o->real = GSL_NAN; }
    else                                  { o->real = areal * breal; }
    o->imag = 0.0;
    o->flagComplex=0;
   }
  else // Complex multiplication
   {
    if (c->set->term_current.ComplexNumbers == SW_ONOFF_OFF) { o->real = GSL_NAN; o->imag = 0; o->flagComplex=0; }
    else
     {
      tmp            = (areal * breal - aimag * bimag);
      o->imag        = (aimag * breal + areal * bimag);
      o->real        = tmp;
      o->flagComplex = !ppl_dblEqual(o->imag, 0);
      if (!o->flagComplex) o->imag=0.0; // Enforce that real numbers have positive zero imaginary components
     }
   }

  if ((!gsl_finite(o->real))||(!gsl_finite(o->imag)))
   {
    if (c->set->term_current.ExplicitErrors == SW_ONOFF_ON) { sprintf(errText, "Multiplication produced an overflow error."); *errType=ERR_OVERFLOW; *status = 1; return; }
    else { pplObjNum(o, o->amMalloced, GSL_NAN, 0.0); return; }
   }

  if ((a->dimensionless != 0) && (b->dimensionless != 0)) { if ((o != a) && (o != b)) ppl_unitsDimCpy(o,a); return; }

  for (i=0; i<UNITS_MAX_BASEUNITS; i++)
   {
    o->exponent[i] = a->exponent[i] + b->exponent[i];
    if (ppl_dblEqual(o->exponent[i], 0) == 0) DimLess=0;
    if (fabs(o->exponent[i]) > 20000 )
     {
      if (c->set->term_current.ExplicitErrors == SW_ONOFF_OFF) { pplObjNum(o, o->amMalloced, GSL_NAN, 0.0); return; }
      else { sprintf(errText, "Overflow of physical dimensions of argument."); *errType=ERR_OVERFLOW; *status = 1; return; }
     }
   }
  o->dimensionless = DimLess;
  if (o->exponent[UNIT_TEMPERATURE]==0) o->tempType = 0; // We've lost our temperature dependence
  return;
 }

void ppl_uaDiv(ppl_context *c, const pplObj *a, const pplObj *b, pplObj *o, int *status, int *errType, char *errText)
 {
  int i;
  double mag, tmp, areal, breal, aimag, bimag;
  unsigned char DimLess=1;

  areal = a->real ; aimag = a->imag;
  breal = b->real ; bimag = b->imag;

  // Two inputs have conflicting temperature units. This is only allowed in the special case of oC/oF and friends, when temperature conversion happens.
  if (!ppl_tempTypeMatch(a->tempType, b->tempType))
   {
    if      (ppl_dblEqual(a->exponent[UNIT_TEMPERATURE], 1.0) && (ppl_dblEqual(b->exponent[UNIT_TEMPERATURE], 1.0)))
     {
      areal = areal + c->tempTypeOffset[a->tempType] - c->tempTypeOffset[b->tempType]; // Remember, areal and breal have already had multipliers applied.
     }
    else if (ppl_dblEqual(a->exponent[UNIT_TEMPERATURE],-1.0) && (ppl_dblEqual(b->exponent[UNIT_TEMPERATURE],-1.0)))
     {
      breal = breal + c->tempTypeOffset[b->tempType] - c->tempTypeOffset[a->tempType]; // Imaginary part needs to conversion... multiplication already done.
     }
    else
     {* status = 1; *errType=ERR_UNIT; sprintf(errText, "Attempt to divide quantities with different temperature units: left operand has units of <%s>, while right operand has units of <%s>. These must be explicitly cast onto the same temperature scale before division is allowed. Type 'help units temperatures' for more details.", ppl_printUnit(c, a, NULL, NULL, 0, 1, 0), ppl_printUnit(c, b, NULL, NULL, 1, 1, 0) );}
   }

  // If one or other input has temperature dependence, we need to propagate which unit of temperature is being used.
  // If we're in one of the cases handled above, don't worry as temperature exponent is about to end up as zero after, e.g. oC/oF
  o->tempType = (a->tempType > b->tempType) ? a->tempType : b->tempType;

  if ((c->set->term_current.ComplexNumbers == SW_ONOFF_OFF) || ((!a->flagComplex) && (!b->flagComplex))) // Real division
   {
    if (a->flagComplex || b->flagComplex) { o->real = GSL_NAN; o->imag = 0; o->flagComplex=0; }
    else if (fabs(breal) < 1e-200)
     {
      if (c->set->term_current.ExplicitErrors == SW_ONOFF_OFF) { o->real = GSL_NAN; o->imag = 0; o->flagComplex=0; }
      else                                                      { sprintf(errText, "Division by zero error."); *errType=ERR_NUMERICAL; *status = 1; return; }
     }
    else
     {
      if (a->flagComplex || b->flagComplex) { o->real = GSL_NAN; }
      else                                  { o->real = areal / breal; }
      o->imag = 0.0;
      o->flagComplex=0;
     }
   }
  else // Complex division
   {
    if (c->set->term_current.ComplexNumbers == SW_ONOFF_OFF) { o->real = GSL_NAN; o->imag = 0; o->flagComplex=0; }
    else if ((mag = pow(breal,2)+pow(bimag,2)) < 1e-200)
     {
      if (c->set->term_current.ExplicitErrors == SW_ONOFF_OFF) { o->real = GSL_NAN; o->imag = 0; o->flagComplex=0; }
      else                                                      { sprintf(errText, "Division by zero error."); *errType=ERR_NUMERICAL; *status = 1; return; }
     }
    else
     {
      tmp            = (areal * breal + aimag * bimag) / mag;
      o->imag        = (aimag * breal - areal * bimag) / mag;
      o->real        = tmp;
      o->flagComplex = !ppl_dblEqual(o->imag, 0);
      if (!o->flagComplex) o->imag=0.0; // Enforce that real numbers have positive zero imaginary components
     }
   }

  if ((!gsl_finite(o->real))||(!gsl_finite(o->imag)))
   {
    if (c->set->term_current.ExplicitErrors == SW_ONOFF_ON) { sprintf(errText, "Division produced an overflow error."); *errType=ERR_OVERFLOW; *status = 1; return; }
    else { pplObjNum(o, o->amMalloced, GSL_NAN, 0.0); return; }
   }

  if ((a->dimensionless != 0) && (b->dimensionless != 0)) { if ((o != a) && (o != b)) ppl_unitsDimCpy(o,a); return; }

  for (i=0; i<UNITS_MAX_BASEUNITS; i++)
   {
    o->exponent[i] = a->exponent[i] - b->exponent[i];
    if (ppl_dblEqual(o->exponent[i], 0) == 0) DimLess=0;
    if (fabs(o->exponent[i]) > 20000 )
     {
      if (c->set->term_current.ExplicitErrors == SW_ONOFF_OFF) { pplObjNum(o, o->amMalloced, GSL_NAN, 0.0); return; }
      else { sprintf(errText, "Overflow of physical dimensions of argument."); *errType=ERR_UNIT; *status = 1; return; }
     }
   }
  o->dimensionless = DimLess;
  if (o->exponent[UNIT_TEMPERATURE]==0) o->tempType = 0; // We've lost our temperature dependence
  return;
 }

void ppl_uaAdd(ppl_context *c, const pplObj *a, const pplObj *b, pplObj *o, int *status, int *errType, char *errText)
 {
  o->real = a->real + b->real;
  if ((o != a) && (o != b)) { ppl_unitsDimCpy(o,a); o->imag = 0.0; o->flagComplex=0; }
  if ((a->dimensionless == 0) || (b->dimensionless == 0))
   {
    if (ppl_unitsDimEqual(a, b) == 0)
     {
      if (a->dimensionless)
       { sprintf(errText, "Attempt to add quantities with conflicting dimensions: left operand is dimensionless, while right operand has units of <%s>.", ppl_printUnit(c, b, NULL, NULL, 1, 1, 0) ); }
      else if (b->dimensionless)
       { sprintf(errText, "Attempt to add quantities with conflicting dimensions: left operand has units of <%s>, while right operand is dimensionless.", ppl_printUnit(c, a, NULL, NULL, 0, 1, 0) ); }
      else
       { sprintf(errText, "Attempt to add quantities with conflicting dimensions: left operand has units of <%s>, while right operand has units of <%s>.", ppl_printUnit(c, a, NULL, NULL, 0, 1, 0), ppl_printUnit(c, b, NULL, NULL, 1, 1, 0) ); }
      *errType=ERR_UNIT; *status = 1; return;
     }
   }
  if (!ppl_tempTypeMatch(a->tempType, b->tempType))
   { *errType=ERR_UNIT; *status = 1; sprintf(errText, "Attempt to add quantities with different temperature units: left operand has units of <%s>, while right operand has units of <%s>. These must be explicitly cast onto the same temperature scale before addition is allowed. Type 'help units temperatures' for more details.", ppl_printUnit(c, a, NULL, NULL, 0, 1, 0), ppl_printUnit(c, b, NULL, NULL, 1, 1, 0) ); }
  if (a->flagComplex || b->flagComplex)
   {
    if (c->set->term_current.ComplexNumbers == SW_ONOFF_OFF) { o->real = GSL_NAN; o->imag = 0; o->flagComplex=0; return; }
    o->imag = a->imag + b->imag;
    o->flagComplex = !ppl_dblEqual(o->imag, 0);
    if (!o->flagComplex) o->imag=0.0; // Enforce that real numbers have positive zero imaginary components
   }

  if ((!gsl_finite(o->real))||(!gsl_finite(o->imag)))
   {
    if (c->set->term_current.ExplicitErrors == SW_ONOFF_ON) { sprintf(errText, "Addition produced an overflow error."); *errType=ERR_OVERFLOW; *status = 1; return; }
    else { pplObjNum(o, o->amMalloced, GSL_NAN, 0.0); return; }
   }

  return;
 }

void ppl_uaSub(ppl_context *c, const pplObj *a, const pplObj *b, pplObj *o, int *status, int *errType, char *errText)
 {
  o->real = a->real - b->real;
  if ((o != a) && (o != b)) { ppl_unitsDimCpy(o,a); o->imag = 0.0; o->flagComplex=0; }
  if ((a->dimensionless == 0) || (b->dimensionless == 0))
   {
    if (ppl_unitsDimEqual(a, b) == 0)
     {
      if (a->dimensionless)
       { sprintf(errText, "Attempt to subtract quantities with conflicting dimensions: left operand is dimensionless, while right operand has units of <%s>.", ppl_printUnit(c, b, NULL, NULL, 1, 1, 0) ); }
      else if (b->dimensionless)
       { sprintf(errText, "Attempt to subtract quantities with conflicting dimensions: left operand has units of <%s>, while right operand is dimensionless.", ppl_printUnit(c, a, NULL, NULL, 0, 1, 0) ); }
      else
       { sprintf(errText, "Attempt to subtract quantities with conflicting dimensions: left operand has units of <%s>, while right operand has units of <%s>.", ppl_printUnit(c, a, NULL, NULL, 0, 1, 0), ppl_printUnit(c, b, NULL, NULL, 1, 1, 0) ); }
      *errType=ERR_UNIT; *status = 1; return;
     }
   }
  if (!ppl_tempTypeMatch(a->tempType, b->tempType))
   { *errType=ERR_UNIT; *status = 1; sprintf(errText, "Attempt to subtract quantities with different temperature units: left operand has units of <%s>, while right operand has units of <%s>. These must be explicitly cast onto the same temperature scale before subtraction is allowed. Type 'help units temperatures' for more details.", ppl_printUnit(c, a, NULL, NULL, 0, 1, 0), ppl_printUnit(c, b, NULL, NULL, 1, 1, 0) ); }
  if (a->flagComplex || b->flagComplex)
   {
    if (c->set->term_current.ComplexNumbers == SW_ONOFF_OFF) { o->real = GSL_NAN; o->imag = 0; o->flagComplex=0; return; }
    o->imag = a->imag - b->imag;
    o->flagComplex = !ppl_dblEqual(o->imag, 0);
    if (!o->flagComplex) o->imag=0.0; // Enforce that real numbers have positive zero imaginary components
   }

  if ((!gsl_finite(o->real))||(!gsl_finite(o->imag)))
   {
    if (c->set->term_current.ExplicitErrors == SW_ONOFF_ON) { sprintf(errText, "Subtraction produced an overflow error."); *errType=ERR_OVERFLOW; *status = 1; return; }
    else { pplObjNum(o, o->amMalloced, GSL_NAN, 0.0); return; }
   }

  return;
 }

void ppl_uaMod(ppl_context *c, const pplObj *a, const pplObj *b, pplObj *o, int *status, int *errType, char *errText)
 {
  o->real = a->real - floor(a->real / b->real) * b->real;
  if ((o != a) && (o != b)) { ppl_unitsDimCpy(o,a); o->imag = 0.0; o->flagComplex=0; }
  if ((a->dimensionless == 0) || (b->dimensionless == 0))
   {
    if (ppl_unitsDimEqual(a, b) == 0)
     {
      if (a->dimensionless)
       { sprintf(errText, "Attempt to apply mod operator to quantities with conflicting dimensions: left operand is dimensionless, while right operand has units of <%s>.", ppl_printUnit(c, b, NULL, NULL, 1, 1, 0) ); }
      else if (b->dimensionless)
       { sprintf(errText, "Attempt to apply mod operator to quantities with conflicting dimensions: left operand has units of <%s>, while right operand is dimensionless.", ppl_printUnit(c, a, NULL, NULL, 0, 1, 0) ); }
      else
       { sprintf(errText, "Attempt to apply mod operator to quantities with conflicting dimensions: left operand has units of <%s>, while right operand has units of <%s>.", ppl_printUnit(c, a, NULL, NULL, 0, 1, 0), ppl_printUnit(c, b, NULL, NULL, 1, 1, 0) ); }
      *errType=ERR_UNIT; *status = 1; return;
     }
   }
  if (!ppl_tempTypeMatch(a->tempType, b->tempType))
   { *errType=ERR_UNIT; *status = 1; sprintf(errText, "Attempt to apply mod operator to quantities with different temperature units: left operand has units of <%s>, while right operand has units of <%s>. These must be explicitly cast onto the same temperature scale before the use of the mod operator is allowed. Type 'help units temperatures' for more details.", ppl_printUnit(c, a, NULL, NULL, 0, 1, 0), ppl_printUnit(c, b, NULL, NULL, 1, 1, 0) ); }
  if (a->flagComplex || b->flagComplex)
   {
    if (c->set->term_current.ComplexNumbers == SW_ONOFF_OFF) { o->real = GSL_NAN; o->imag = 0; o->flagComplex=0; return; }
    sprintf(errText, "Mod operator can only be applied to real operands; complex operands supplied.");
    *errType=ERR_NUMERICAL; *status = 1; return;
   }

  if ((!gsl_finite(o->real))||(!gsl_finite(o->imag)))
   {
    if (c->set->term_current.ExplicitErrors == SW_ONOFF_ON) { sprintf(errText, "Modulo operator produced an overflow error."); *errType=ERR_OVERFLOW; *status = 1; return; }
    else { pplObjNum(o, o->amMalloced, GSL_NAN, 0.0); return; }
   }

  return;
 }

