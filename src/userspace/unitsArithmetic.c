// unitsArithmetic.c
//
// The code in this file is part of PyXPlot
// <http://www.pyxplot.org.uk>
//
// Copyright (C) 2006-2012 Dominic Ford <coders@pyxplot.org.uk>
//               2008-2011 Ross Church
//
// $Id: ppl_units.c 344 2009-09-04 00:46:52Z dcf21 $
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
#include <ctype.h>
#include <math.h>
#include <string.h>

#include <gsl/gsl_complex.h>
#include <gsl/gsl_complex_math.h>
#include <gsl/gsl_math.h>

#include "stringTools/asciidouble.h"
#include "userspace/context.h"
#include "userspace/unitsDisp.h"
#include "userspace/unitsArithmetic.h"

// -------------------------------------------------------------------
// Functions for comparing the units of quantities
// -------------------------------------------------------------------

// Useful function for checking whether two doubles are roughly equal to one another
unsigned char ppl_units_DblEqual(double a, double b)
 {
  if ( (fabs(a) < 1e-100) && (fabs(b) < 1e-100) ) return 1;
  if ( (fabs(a-b) > fabs(1e-7*a)) || (fabs(a-b) > fabs(1e-7*b)) ) return 0;
  return 1;
 }

unsigned char ppl_units_DblApprox(double a, double b, double err)
 {
  if ( fabs(a-b) > (fabs(err)+1e-50) ) return 0;
  return 1;
 }

void ppl_units_DimCpy(pplObj *o, const pplObj *i)
 {
  int j;
  o->dimensionless = i->dimensionless;
  o->tempType      = i->tempType;
  for (j=0; j<UNITS_MAX_BASEUNITS; j++) o->exponent[j] = i->exponent[j];
  return;
 }

void ppl_units_DimInverse(pplObj *o, const pplObj *i)
 {
  int j;
  o->dimensionless = i->dimensionless;
  for (j=0; j<UNITS_MAX_BASEUNITS; j++) o->exponent[j] = -i->exponent[j];
  o->tempType = 0; // Either input was per temperature, or we are now per temperature.
  return;
 }

int ppl_units_DimEqual(const pplObj *a, const pplObj *b)
 {
  int j;
  for (j=0; j<UNITS_MAX_BASEUNITS; j++) if (ppl_units_DblEqual(a->exponent[j] , b->exponent[j]) == 0) return 0;
  return 1;
 }

int ppl_units_DimEqual2(const pplObj *a, const unit *b)
 {
  int j;
  for (j=0; j<UNITS_MAX_BASEUNITS; j++) if (ppl_units_DblEqual(a->exponent[j] , b->exponent[j]) == 0) return 0;
  return 1;
 }

int ppl_units_UnitDimEqual(const unit *a, const unit *b)
 {
  int j;
  for (j=0; j<UNITS_MAX_BASEUNITS; j++) if (ppl_units_DblEqual(a->exponent[j] , b->exponent[j]) == 0) return 0;
  return 1;
 }

unsigned char ppl_tempTypeMatch(unsigned char a, unsigned char b)
 {
  if ((a<1)||(b<1)) return 1; // anything is compatible with something which doesn't have dimensions of temperature
  if (a==b) return 1; // oC, oF and oR are only ever used when called for
  return 0;
 }

// -------------------------------
// ARITHMETIC OPERATIONS ON VALUES
// -------------------------------

void ppl_units_pow (const pplObj *a, const pplObj *b, pplObj *o, int *status, char *errText)
 {
  int i;
  double exponent;
  gsl_complex ac, bc;
  unsigned char DimLess=1;

  exponent = b->real; // We may overwrite this when we set o->real, so store a copy

  if (b->dimensionless == 0)
   {
    if (c->set->term_current.ExplicitErrors == SW_ONOFF_OFF) { ppl_units_zero(o); o->real = GSL_NAN; o->imag = 0; o->FlagComplex=0; return; }
    else { sprintf(errText, "Exponent should be dimensionless, but instead has dimensions of <%s>.", ppl_units_GetUnitStr(b, NULL, NULL, 0, 1, 0)); *status = 1; return; }
   }

  if (c->set->term_current.ComplexNumbers == SW_ONOFF_OFF) // Real pow()
   {
    if (a->FlagComplex || b->FlagComplex) { o->real = GSL_NAN; }
    else
     {
      o->real = pow(a->real, exponent);
      if ((!gsl_finite(o->real))&&(c->set->term_current.ExplicitErrors == SW_ONOFF_ON)) { sprintf(errText, "Exponentiation operator produced an overflow error or a complex number result. To enable complex arithmetic, type 'set numerics complex'."); *status = 1; return; }
     }
    o->imag = 0.0;
    o->FlagComplex=0;
   }
  else // Complex pow()
   {
    if ((a->dimensionless == 0) && (b->FlagComplex))
     {
      if (c->set->term_current.ExplicitErrors == SW_ONOFF_OFF) { ppl_units_zero(o); o->real = GSL_NAN; o->imag = 0; o->FlagComplex=0; return; }
      else { sprintf(errText, "Raising quantities with physical units to complex powers produces quantities with complex physical dimensions, which is forbidden. The operand in question has dimensions of <%s>.", ppl_units_GetUnitStr(a, NULL, NULL, 0, 1, 0)); *status = 1; return; }
     }
    else
     {
      if (!b->FlagComplex)
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
      o->FlagComplex = fabs(o->imag)>(fabs(o->real)*1e-15);
      if (!o->FlagComplex) o->imag=0.0; // Enforce that real numbers have positive zero imaginary components
      if ((!gsl_finite(o->real))||(!gsl_finite(o->imag)))
       {
        if (c->set->term_current.ExplicitErrors == SW_ONOFF_ON) { sprintf(errText, "Exponentiation operator produced an overflow error."); *status = 1; return; }
        else { ppl_units_zero(o); o->real = GSL_NAN; o->imag = 0; o->FlagComplex=0; return; }
       }
     }
   }

  if (a->dimensionless != 0) { if ((o != a) && (o != b)) ppl_units_DimCpy(o,a); return; }

  for (i=0; i<UNITS_MAX_BASEUNITS; i++)
   {
    o->exponent[i] = a->exponent[i] * exponent;
    if (ppl_units_DblEqual(o->exponent[i], 0) == 0) DimLess=0;
    if (fabs(o->exponent[i]) > 20000 )
     {
      if (c->set->term_current.ExplicitErrors == SW_ONOFF_OFF) { ppl_units_zero(o); o->real = GSL_NAN; o->imag = 0; o->FlagComplex=0; return; }
      else { sprintf(errText, "Overflow of physical dimensions of argument."); *status = 1; return; }
     }
   }
  o->tempType = a->tempType;
  o->dimensionless = DimLess;
  return;
 }

void ppl_units_mult(const pplObj *a, const pplObj *b, pplObj *o, int *status, char *errText)
 {
  int i;
  double tmp, areal, breal, aimag, bimag;
  unsigned char DimLess=1;

  areal = a->real ; aimag = a->imag;
  breal = b->real ; bimag = b->imag;

  // Two inputs have conflicting temperature units. This is only allowed in the special case of oC/oF and friends, when temperature conversion happens.
  if (!ppl_tempTypeMatch(a->tempType, b->tempType))
   {
    if      (ppl_units_DblEqual(a->exponent[UNIT_TEMPERATURE], 1.0) && (ppl_units_DblEqual(b->exponent[UNIT_TEMPERATURE],-1.0)))
     {
      areal = areal + tempTypeOffset[a->tempType] - tempTypeOffset[b->tempType]; // Remember, areal and breal have already had multipliers applied.
     }
    else if (ppl_units_DblEqual(a->exponent[UNIT_TEMPERATURE],-1.0) && (ppl_units_DblEqual(b->exponent[UNIT_TEMPERATURE], 1.0)))
     {
      breal = breal + tempTypeOffset[b->tempType] - tempTypeOffset[a->tempType]; // Imaginary part needs to conversion... multiplication already done.
     }
    else
     {*status = 1; sprintf(errText, "Attempt to multiply quantities with different temperature units: left operand has units of <%s>, while right operand has units of <%s>. These must be explicitly cast onto the same temperature scale before multiplication is allowed. Type 'help units temperatures' for more details.", ppl_units_GetUnitStr(a, NULL, NULL, 0, 1, 0), ppl_units_GetUnitStr(b, NULL, NULL, 1, 1, 0) );}
   }

  // If one or other input has temperature dependence, we need to propagate which unit of temperature is being used.
  // If we're in one of the cases handled above, don't worry as temperature exponent is about to end up as zero after, e.g. oC/oF
  o->tempType = (a->tempType > b->tempType) ? a->tempType : b->tempType;

  if ((c->set->term_current.ComplexNumbers == SW_ONOFF_OFF) || ((!a->FlagComplex) && (!b->FlagComplex))) // Real multiplication
   {
    if (a->FlagComplex || b->FlagComplex) { o->real = GSL_NAN; }
    else                                  { o->real = areal * breal; }
    o->imag = 0.0;
    o->FlagComplex=0;
   }
  else // Complex multiplication
   {
    if (c->set->term_current.ComplexNumbers == SW_ONOFF_OFF) { o->real = GSL_NAN; o->imag = 0; o->FlagComplex=0; }
    else
     {
      tmp            = (areal * breal - aimag * bimag);
      o->imag        = (aimag * breal + areal * bimag);
      o->real        = tmp;
      o->FlagComplex = !ppl_units_DblEqual(o->imag, 0);
      if (!o->FlagComplex) o->imag=0.0; // Enforce that real numbers have positive zero imaginary components
     }
   }

  if ((!gsl_finite(o->real))||(!gsl_finite(o->imag)))
   {
    if (c->set->term_current.ExplicitErrors == SW_ONOFF_ON) { sprintf(errText, "Multiplication produced an overflow error."); *status = 1; return; }
    else { ppl_units_zero(o); o->real = GSL_NAN; o->imag = 0; o->FlagComplex=0; return; }
   }

  if ((a->dimensionless != 0) && (b->dimensionless != 0)) { if ((o != a) && (o != b)) ppl_units_DimCpy(o,a); return; }

  for (i=0; i<UNITS_MAX_BASEUNITS; i++)
   {
    o->exponent[i] = a->exponent[i] + b->exponent[i];
    if (ppl_units_DblEqual(o->exponent[i], 0) == 0) DimLess=0;
    if (fabs(o->exponent[i]) > 20000 )
     {
      if (c->set->term_current.ExplicitErrors == SW_ONOFF_OFF) { ppl_units_zero(o); o->real = GSL_NAN; o->imag = 0; o->FlagComplex=0; return; }
      else { sprintf(errText, "Overflow of physical dimensions of argument."); *status = 1; return; }
     }
   }
  o->dimensionless = DimLess;
  if (o->exponent[UNIT_TEMPERATURE]==0) o->tempType = 0; // We've lost our temperature dependence
  return;
 }

void ppl_units_div (const pplObj *a, const pplObj *b, pplObj *o, int *status, char *errText)
 {
  int i;
  double mag, tmp, areal, breal, aimag, bimag;
  unsigned char DimLess=1;

  areal = a->real ; aimag = a->imag;
  breal = b->real ; bimag = b->imag;

  // Two inputs have conflicting temperature units. This is only allowed in the special case of oC/oF and friends, when temperature conversion happens.
  if (!ppl_tempTypeMatch(a->tempType, b->tempType))
   {
    if      (ppl_units_DblEqual(a->exponent[UNIT_TEMPERATURE], 1.0) && (ppl_units_DblEqual(b->exponent[UNIT_TEMPERATURE], 1.0)))
     {
      areal = areal + tempTypeOffset[a->tempType] - tempTypeOffset[b->tempType]; // Remember, areal and breal have already had multipliers applied.
     }
    else if (ppl_units_DblEqual(a->exponent[UNIT_TEMPERATURE],-1.0) && (ppl_units_DblEqual(b->exponent[UNIT_TEMPERATURE],-1.0)))
     {
      breal = breal + tempTypeOffset[b->tempType] - tempTypeOffset[a->tempType]; // Imaginary part needs to conversion... multiplication already done.
     }
    else
     {*status = 1; sprintf(errText, "Attempt to divide quantities with different temperature units: left operand has units of <%s>, while right operand has units of <%s>. These must be explicitly cast onto the same temperature scale before division is allowed. Type 'help units temperatures' for more details.", ppl_units_GetUnitStr(a, NULL, NULL, 0, 1, 0), ppl_units_GetUnitStr(b, NULL, NULL, 1, 1, 0) );}
   }

  // If one or other input has temperature dependence, we need to propagate which unit of temperature is being used.
  // If we're in one of the cases handled above, don't worry as temperature exponent is about to end up as zero after, e.g. oC/oF
  o->tempType = (a->tempType > b->tempType) ? a->tempType : b->tempType;

  if ((c->set->term_current.ComplexNumbers == SW_ONOFF_OFF) || ((!a->FlagComplex) && (!b->FlagComplex))) // Real division
   {
    if (a->FlagComplex || b->FlagComplex) { o->real = GSL_NAN; o->imag = 0; o->FlagComplex=0; }
    else if (fabs(breal) < 1e-200)
     {
      if (c->set->term_current.ExplicitErrors == SW_ONOFF_OFF) { o->real = GSL_NAN; o->imag = 0; o->FlagComplex=0; }
      else                                                      { sprintf(errText, "Division by zero error."); *status = 1; return; }
     }
    else
     {
      if (a->FlagComplex || b->FlagComplex) { o->real = GSL_NAN; }
      else                                  { o->real = areal / breal; }
      o->imag = 0.0;
      o->FlagComplex=0;
     }
   }
  else // Complex division
   {
    if (c->set->term_current.ComplexNumbers == SW_ONOFF_OFF) { o->real = GSL_NAN; o->imag = 0; o->FlagComplex=0; }
    else if ((mag = pow(breal,2)+pow(bimag,2)) < 1e-200)
     {
      if (c->set->term_current.ExplicitErrors == SW_ONOFF_OFF) { o->real = GSL_NAN; o->imag = 0; o->FlagComplex=0; }
      else                                                      { sprintf(errText, "Division by zero error."); *status = 1; return; }
     }
    else
     {
      tmp            = (areal * breal + aimag * bimag) / mag;
      o->imag        = (aimag * breal - areal * bimag) / mag;
      o->real        = tmp;
      o->FlagComplex = !ppl_units_DblEqual(o->imag, 0);
      if (!o->FlagComplex) o->imag=0.0; // Enforce that real numbers have positive zero imaginary components
     }
   }

  if ((!gsl_finite(o->real))||(!gsl_finite(o->imag)))
   {
    if (c->set->term_current.ExplicitErrors == SW_ONOFF_ON) { sprintf(errText, "Division produced an overflow error."); *status = 1; return; }
    else { ppl_units_zero(o); o->real = GSL_NAN; o->imag = 0; o->FlagComplex=0; return; }
   }

  if ((a->dimensionless != 0) && (b->dimensionless != 0)) { if ((o != a) && (o != b)) ppl_units_DimCpy(o,a); return; }

  for (i=0; i<UNITS_MAX_BASEUNITS; i++)
   {
    o->exponent[i] = a->exponent[i] - b->exponent[i];
    if (ppl_units_DblEqual(o->exponent[i], 0) == 0) DimLess=0;
    if (fabs(o->exponent[i]) > 20000 )
     {
      if (c->set->term_current.ExplicitErrors == SW_ONOFF_OFF) { ppl_units_zero(o); o->real = GSL_NAN; o->imag = 0; o->FlagComplex=0; return; }
      else { sprintf(errText, "Overflow of physical dimensions of argument."); *status = 1; return; }
     }
   }
  o->dimensionless = DimLess;
  if (o->exponent[UNIT_TEMPERATURE]==0) o->tempType = 0; // We've lost our temperature dependence
  return;
 }

void ppl_units_add (const pplObj *a, const pplObj *b, pplObj *o, int *status, char *errText)
 {
  o->real = a->real + b->real;
  if ((o != a) && (o != b)) { ppl_units_DimCpy(o,a); o->imag = 0.0; o->FlagComplex=0; }
  if ((a->dimensionless == 0) || (b->dimensionless == 0))
   {
    if (ppl_units_DimEqual(a, b) == 0)
     {
      if (a->dimensionless)
       { sprintf(errText, "Attempt to add quantities with conflicting dimensions: left operand is dimensionless, while right operand has units of <%s>.", ppl_units_GetUnitStr(b, NULL, NULL, 1, 1, 0) ); }
      else if (b->dimensionless)
       { sprintf(errText, "Attempt to add quantities with conflicting dimensions: left operand has units of <%s>, while right operand is dimensionless.", ppl_units_GetUnitStr(a, NULL, NULL, 0, 1, 0) ); }
      else
       { sprintf(errText, "Attempt to add quantities with conflicting dimensions: left operand has units of <%s>, while right operand has units of <%s>.", ppl_units_GetUnitStr(a, NULL, NULL, 0, 1, 0), ppl_units_GetUnitStr(b, NULL, NULL, 1, 1, 0) ); }
      *status = 1; return;
     }
   }
  if (!ppl_tempTypeMatch(a->tempType, b->tempType))
   { *status = 1; sprintf(errText, "Attempt to add quantities with different temperature units: left operand has units of <%s>, while right operand has units of <%s>. These must be explicitly cast onto the same temperature scale before addition is allowed. Type 'help units temperatures' for more details.", ppl_units_GetUnitStr(a, NULL, NULL, 0, 1, 0), ppl_units_GetUnitStr(b, NULL, NULL, 1, 1, 0) ); }
  if (a->FlagComplex || b->FlagComplex)
   {
    if (c->set->term_current.ComplexNumbers == SW_ONOFF_OFF) { o->real = GSL_NAN; o->imag = 0; o->FlagComplex=0; return; }
    o->imag = a->imag + b->imag;
    o->FlagComplex = !ppl_units_DblEqual(o->imag, 0);
    if (!o->FlagComplex) o->imag=0.0; // Enforce that real numbers have positive zero imaginary components
   }

  if ((!gsl_finite(o->real))||(!gsl_finite(o->imag)))
   {
    if (c->set->term_current.ExplicitErrors == SW_ONOFF_ON) { sprintf(errText, "Addition produced an overflow error."); *status = 1; return; }
    else { ppl_units_zero(o); o->real = GSL_NAN; o->imag = 0; o->FlagComplex=0; return; }
   }

  return;
 }

void ppl_units_sub (const pplObj *a, const pplObj *b, pplObj *o, int *status, char *errText)
 {
  o->real = a->real - b->real;
  if ((o != a) && (o != b)) { ppl_units_DimCpy(o,a); o->imag = 0.0; o->FlagComplex=0; }
  if ((a->dimensionless == 0) || (b->dimensionless == 0))
   {
    if (ppl_units_DimEqual(a, b) == 0)
     {
      if (a->dimensionless)
       { sprintf(errText, "Attempt to subtract quantities with conflicting dimensions: left operand is dimensionless, while right operand has units of <%s>.", ppl_units_GetUnitStr(b, NULL, NULL, 1, 1, 0) ); }
      else if (b->dimensionless)
       { sprintf(errText, "Attempt to subtract quantities with conflicting dimensions: left operand has units of <%s>, while right operand is dimensionless.", ppl_units_GetUnitStr(a, NULL, NULL, 0, 1, 0) ); }
      else
       { sprintf(errText, "Attempt to subtract quantities with conflicting dimensions: left operand has units of <%s>, while right operand has units of <%s>.", ppl_units_GetUnitStr(a, NULL, NULL, 0, 1, 0), ppl_units_GetUnitStr(b, NULL, NULL, 1, 1, 0) ); }
      *status = 1; return;
     }
   }
  if (!ppl_tempTypeMatch(a->tempType, b->tempType))
   { *status = 1; sprintf(errText, "Attempt to subtract quantities with different temperature units: left operand has units of <%s>, while right operand has units of <%s>. These must be explicitly cast onto the same temperature scale before subtraction is allowed. Type 'help units temperatures' for more details.", ppl_units_GetUnitStr(a, NULL, NULL, 0, 1, 0), ppl_units_GetUnitStr(b, NULL, NULL, 1, 1, 0) ); }
  if (a->FlagComplex || b->FlagComplex)
   {
    if (c->set->term_current.ComplexNumbers == SW_ONOFF_OFF) { o->real = GSL_NAN; o->imag = 0; o->FlagComplex=0; return; }
    o->imag = a->imag - b->imag;
    o->FlagComplex = !ppl_units_DblEqual(o->imag, 0);
    if (!o->FlagComplex) o->imag=0.0; // Enforce that real numbers have positive zero imaginary components
   }

  if ((!gsl_finite(o->real))||(!gsl_finite(o->imag)))
   {
    if (c->set->term_current.ExplicitErrors == SW_ONOFF_ON) { sprintf(errText, "Subtraction produced an overflow error."); *status = 1; return; }
    else { ppl_units_zero(o); o->real = GSL_NAN; o->imag = 0; o->FlagComplex=0; return; }
   }

  return;
 }

void ppl_units_mod (const pplObj *a, const pplObj *b, pplObj *o, int *status, char *errText)
 {
  o->real = a->real - floor(a->real / b->real) * b->real;
  if ((o != a) && (o != b)) { ppl_units_DimCpy(o,a); o->imag = 0.0; o->FlagComplex=0; }
  if ((a->dimensionless == 0) || (b->dimensionless == 0))
   {
    if (ppl_units_DimEqual(a, b) == 0)
     {
      if (a->dimensionless)
       { sprintf(errText, "Attempt to apply mod operator to quantities with conflicting dimensions: left operand is dimensionless, while right operand has units of <%s>.", ppl_units_GetUnitStr(b, NULL, NULL, 1, 1, 0) ); }
      else if (b->dimensionless)
       { sprintf(errText, "Attempt to apply mod operator to quantities with conflicting dimensions: left operand has units of <%s>, while right operand is dimensionless.", ppl_units_GetUnitStr(a, NULL, NULL, 0, 1, 0) ); }
      else
       { sprintf(errText, "Attempt to apply mod operator to quantities with conflicting dimensions: left operand has units of <%s>, while right operand has units of <%s>.", ppl_units_GetUnitStr(a, NULL, NULL, 0, 1, 0), ppl_units_GetUnitStr(b, NULL, NULL, 1, 1, 0) ); }
      *status = 1; return;
     }
   }
  if (!ppl_tempTypeMatch(a->tempType, b->tempType))
   { *status = 1; sprintf(errText, "Attempt to apply mod operator to quantities with different temperature units: left operand has units of <%s>, while right operand has units of <%s>. These must be explicitly cast onto the same temperature scale before the use of the mod operator is allowed. Type 'help units temperatures' for more details.", ppl_units_GetUnitStr(a, NULL, NULL, 0, 1, 0), ppl_units_GetUnitStr(b, NULL, NULL, 1, 1, 0) ); }
  if (a->FlagComplex || b->FlagComplex)
   {
    if (c->set->term_current.ComplexNumbers == SW_ONOFF_OFF) { o->real = GSL_NAN; o->imag = 0; o->FlagComplex=0; return; }
    sprintf(errText, "Mod operator can only be applied to real operands; complex operands supplied.");
    *status = 1; return;
   }

  if ((!gsl_finite(o->real))||(!gsl_finite(o->imag)))
   {
    if (c->set->term_current.ExplicitErrors == SW_ONOFF_ON) { sprintf(errText, "Modulo operator produced an overflow error."); *status = 1; return; }
    else { ppl_units_zero(o); o->real = GSL_NAN; o->imag = 0; o->FlagComplex=0; return; }
   }

  return;
 }

