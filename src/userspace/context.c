// context.c
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

#define _CONTEXT_C 1

#include <stdlib.h>
#include <string.h>

#include <gsl/gsl_const_mksa.h>
#include <gsl/gsl_const_num.h>
#include <gsl/gsl_math.h>
#include <gsl/gsl_sf.h>

#include "coreUtils/errorReport.h"
#include "coreUtils/dict.h"

#include "settings/epsColours.h"

#include "stringTools/strConstants.h"

#include "userspace/context.h"
#include "userspace/defaultFuncs.h"
#include "userspace/garbageCollector.h"
#include "userspace/pplObj.h"
#include "userspace/pplObjUnits.h"

ppl_context *ppl_contextInit()
 {
  ppl_context *out = malloc(sizeof(ppl_context));

  out->willBeInteractive = 1;
  out->inputLineBuffer = NULL;
  out->inputLineAddBuffer = NULL;
  out->shellExiting = 0;
  out->historyNLinesWritten = 0;
  out->errcontext.error_input_linenumber = -1;
  out->errcontext.error_input_filename[0] = '\0';
  strcpy(out->errcontext.error_source,"main     ");

  out->ns_ptr    = 1;
  out->ns_branch = 0;
  out->namespaces[0] = NULL; // Will be defaults namespace
  out->namespaces[1] = ppl_dictInit(HASHSIZE_LARGE,1); // Will be root namespace

  // Default variables
   {
    pplObj  v, *m;
    dict   *d, *d2;
    int     i;
    const int frozen=1;
    ppl_dictAppendCpy(out->namespaces[1] , "defaults" , m=pplNewModule(frozen) , sizeof(v));
    d = (dict *)m->auxil;
    out->namespaces[0] = d;

    // Root namespace
    pplObjZero(&v,1);
    v.real = M_PI;
    ppl_dictAppendCpy(d  , "pi"        , (void *)&v , sizeof(v)); // pi
    v.real = M_E;
    ppl_dictAppendCpy(d  , "e"         , (void *)&v , sizeof(v)); // e
    v.real = M_EULER;
    ppl_dictAppendCpy(d  , "euler"     , (void *)&v , sizeof(v)); // Euler constant
    v.real = (1.0+sqrt(5))/2.0;
    ppl_dictAppendCpy(d  , "goldenRatio", (void *)&v , sizeof(v)); // Golden Ratio
    v.real = 0.0;
    v.imag = 1.0;
    v.flagComplex = 1;
    ppl_dictAppendCpy(d  , "i"         , (void *)&v , sizeof(v)); // i

    pplObjZero(&v,1);
    v.objType = PPLOBJ_BOOL;
    v.real = 1.0;
    ppl_dictAppendCpy(d  , "true"      , (void *)&v , sizeof(v)); // True
    v.real = 0.0;
    ppl_dictAppendCpy(d  , "false"     , (void *)&v , sizeof(v)); // False

    pplObjNullStr(&v,0);
    v.auxil = (void *)VERSION;
    ppl_dictAppendCpy(d  , "version"   , (void *)&v , sizeof(v)); // PyXPlot version string

    // types module
    ppl_dictAppendCpy(d  , "types"     , m=pplNewModule(frozen) , sizeof(v));
    d2 = (dict *)m->auxil;
    pplObjZero(&v,1);
    v.objType  = PPLOBJ_TYPE;
    v.auxilLen = sizeof(pplType);
    for (i=PPLOBJ_NUM; i<PPLOBJ_GLOB; i++)
     {
      pplType *t = (pplType *)malloc(sizeof(pplType));
      if (t==NULL) { ppl_fatal(&out->errcontext, __FILE__, __LINE__, "Malloc fail."); exit(1); }
      t->iNodeCount = 1;
      t->type       = NULL;
      t->id         = i;
      v.auxil       = (void *)t;
      ppl_dictAppendCpy(d2 , pplObjTypeNames[i] , (void *)&v , sizeof(v));
     }

    // colors module
    ppl_dictAppendCpy(d  , "colors"    , m=pplNewModule(frozen) , sizeof(v));
    d2 = (dict *)m->auxil;
    pplObjZero(&v,1);
    v.objType  = PPLOBJ_COL;
    v.auxilLen = 0; // CMYK color
    for (i=0; SW_COLOUR_INT[i]>=0; i++)
     {
      v.exponent[0] = SW_COLOUR_CMYK_C[i];
      v.exponent[1] = SW_COLOUR_CMYK_M[i];
      v.exponent[2] = SW_COLOUR_CMYK_Y[i];
      v.exponent[3] = SW_COLOUR_CMYK_K[i];
      ppl_dictAppendCpy(d2 , SW_COLOUR_STR[i] , (void *)&v , sizeof(v));
     }

    // phy module
    ppl_dictAppendCpy(d  , "phy"       , m=pplNewModule(frozen) , sizeof(v));
    d2 = (dict *)m->auxil;
    pplObjZero(&v,1);
    v.real = GSL_CONST_MKSA_SPEED_OF_LIGHT;
    v.dimensionless = 0;
    v.exponent[UNIT_LENGTH]=1 ; v.exponent[UNIT_TIME]=-1;
    ppl_dictAppendCpy(d2 , "c"         , (void *)&v , sizeof(v)); // Speed of light
    pplObjZero(&v,1);
    v.real = GSL_CONST_MKSA_VACUUM_PERMEABILITY;
    v.dimensionless = 0;
    v.exponent[UNIT_LENGTH] = 1; v.exponent[UNIT_MASS] = 1; v.exponent[UNIT_TIME] = -2; v.exponent[UNIT_CURRENT] = -2;
    ppl_dictAppendCpy(d2 , "mu_0"      , (void *)&v , sizeof(v)); // The permeability of free space
    pplObjZero(&v,1);
    v.real = GSL_CONST_MKSA_VACUUM_PERMITTIVITY;
    v.dimensionless = 0;
    v.exponent[UNIT_LENGTH] =-3; v.exponent[UNIT_MASS] =-1; v.exponent[UNIT_TIME] =  4; v.exponent[UNIT_CURRENT] =  2;
    ppl_dictAppendCpy(d2 , "epsilon_0" , (void *)&v , sizeof(v)); // The permittivity of free space
    pplObjZero(&v,1);
    v.real = GSL_CONST_MKSA_ELECTRON_CHARGE;
    v.dimensionless = 0;
    v.exponent[UNIT_CURRENT] = 1; v.exponent[UNIT_TIME] = 1;
    ppl_dictAppendCpy(d2 , "q"         , (void *)&v , sizeof(v)); // The fundamental charge
    pplObjZero(&v,1);
    v.real = GSL_CONST_MKSA_PLANCKS_CONSTANT_H;
    v.dimensionless = 0;
    v.exponent[UNIT_MASS] = 1; v.exponent[UNIT_LENGTH] = 2; v.exponent[UNIT_TIME] =-1;
    ppl_dictAppendCpy(d2 , "h"         , (void *)&v , sizeof(v)); // The Planck constant
    v.real = GSL_CONST_MKSA_PLANCKS_CONSTANT_HBAR;
    ppl_dictAppendCpy(d2 , "hbar"      , (void *)&v , sizeof(v)); // The Planck constant / 2pi
    pplObjZero(&v,1);
    v.real = GSL_CONST_NUM_AVOGADRO;
    v.dimensionless = 0;
    v.exponent[UNIT_MOLE] = -1;
    ppl_dictAppendCpy(d2 , "NA"        , (void *)&v , sizeof(v)); // The Avogadro constant
    pplObjZero(&v,1);
    v.real = 3.839e26;
    v.dimensionless = 0;
    v.exponent[UNIT_MASS] = 1; v.exponent[UNIT_LENGTH] = 2; v.exponent[UNIT_TIME] =-3;
    ppl_dictAppendCpy(d2 , "Lsun"      , (void *)&v , sizeof(v)); // The solar luminosity
    pplObjZero(&v,1);
    v.real = GSL_CONST_MKSA_UNIFIED_ATOMIC_MASS;
    v.dimensionless = 0;
    v.exponent[UNIT_MASS] = 1;
    ppl_dictAppendCpy(d2 , "m_u"       , (void *)&v , sizeof(v)); // The universal mass constant
    v.real = GSL_CONST_MKSA_MASS_ELECTRON;
    ppl_dictAppendCpy(d2 , "m_e"       , (void *)&v , sizeof(v)); // The electron mass
    v.real = GSL_CONST_MKSA_MASS_PROTON;
    ppl_dictAppendCpy(d2 , "m_p"       , (void *)&v , sizeof(v)); // The proton mass
    v.real = GSL_CONST_MKSA_MASS_NEUTRON;
    ppl_dictAppendCpy(d2 , "m_n"       , (void *)&v , sizeof(v)); // The neutron mass
    v.real = GSL_CONST_MKSA_MASS_MUON;
    ppl_dictAppendCpy(d2 , "m_muon"    , (void *)&v , sizeof(v)); // The muon mass
    v.real = GSL_CONST_MKSA_SOLAR_MASS;
    ppl_dictAppendCpy(d2 , "Msun"      , (void *)&v , sizeof(v)); // The solar mass
    pplObjZero(&v,1);
    v.real = GSL_CONST_MKSA_RYDBERG / GSL_CONST_MKSA_SPEED_OF_LIGHT / GSL_CONST_MKSA_PLANCKS_CONSTANT_H;
    v.dimensionless = 0;
    v.exponent[UNIT_LENGTH] = -1;
    ppl_dictAppendCpy(d2 , "Ry"        , (void *)&v , sizeof(v)); // The Rydberg constant
    pplObjZero(&v,1);
    v.real = GSL_CONST_NUM_FINE_STRUCTURE;
    v.dimensionless = 1;
    ppl_dictAppendCpy(d2 , "alpha"     , (void *)&v , sizeof(v)); // The fine structure constant
    pplObjZero(&v,1);
    v.real = 6.955e8;
    v.dimensionless = 0;
    v.exponent[UNIT_LENGTH] = 1;
    ppl_dictAppendCpy(d2 , "Rsun"      , (void *)&v , sizeof(v)); // The solar radius
    pplObjZero(&v,1);
    v.real = GSL_CONST_MKSA_BOHR_MAGNETON;
    v.dimensionless = 0;
    v.exponent[UNIT_LENGTH] = 2; v.exponent[UNIT_CURRENT] = 1;
    ppl_dictAppendCpy(d2 , "mu_b"      , (void *)&v , sizeof(v)); // The Bohr magneton
    pplObjZero(&v,1);
    v.real = GSL_CONST_MKSA_MOLAR_GAS;
    v.dimensionless = 0;
    v.exponent[UNIT_MASS] = 1; v.exponent[UNIT_LENGTH] = 2; v.exponent[UNIT_TIME] =-2; v.exponent[UNIT_TEMPERATURE] =-1; v.exponent[UNIT_MOLE] =-1;
    ppl_dictAppendCpy(d2 , "R"         , (void *)&v , sizeof(v)); // The gas constant
    pplObjZero(&v,1);
    v.real = GSL_CONST_MKSA_BOLTZMANN;
    v.dimensionless = 0;
    v.exponent[UNIT_MASS] = 1; v.exponent[UNIT_LENGTH] = 2; v.exponent[UNIT_TIME] =-2; v.exponent[UNIT_TEMPERATURE] =-1;
    ppl_dictAppendCpy(d2 , "kB"        , (void *)&v , sizeof(v)); // The Boltzmann constant
    v.dimensionless = 0;
    v.exponent[UNIT_MASS] = 1; v.exponent[UNIT_LENGTH] = 2; v.exponent[UNIT_TIME] =-2;
    pplObjZero(&v,1);
    v.real = GSL_CONST_MKSA_STEFAN_BOLTZMANN_CONSTANT;
    v.dimensionless = 0;
    v.exponent[UNIT_MASS] = 1; v.exponent[UNIT_TIME] =-3; v.exponent[UNIT_TEMPERATURE] =-4;
    ppl_dictAppendCpy(d2 , "sigma"     , (void *)&v , sizeof(v)); // The Stefan-Boltzmann constant
    pplObjZero(&v,1);
    v.real = GSL_CONST_MKSA_GRAVITATIONAL_CONSTANT;
    v.dimensionless = 0;
    v.exponent[UNIT_LENGTH] = 3; v.exponent[UNIT_TIME] =-2; v.exponent[UNIT_MASS] =-1;
    ppl_dictAppendCpy(d2 , "G"         , (void *)&v , sizeof(v)); // The gravitational constant
    pplObjZero(&v,1);
    v.real = GSL_CONST_MKSA_GRAV_ACCEL;
    v.dimensionless = 0;
    v.exponent[UNIT_LENGTH] = 1; v.exponent[UNIT_TIME] =-2;
    ppl_dictAppendCpy(d2 , "g"         , (void *)&v , sizeof(v)); // The standard acceleration due to gravity on Earth

    // Default maths functions
    ppl_addSystemFunc(d,"abs"           ,1,1,1,1,0,0,(void *)&pplfunc_abs         , "abs(x)", "\\mathrm{abs}@<@1@>", "abs(z) returns the absolute magnitude of z");
    ppl_addSystemFunc(d,"acos"          ,1,1,1,1,0,1,(void *)&pplfunc_acos        , "acos(x)", "\\mathrm{acos}@<@1@>", "acos(x) returns the arccosine of x");
    ppl_addSystemFunc(d,"acosh"         ,1,1,1,1,0,1,(void *)&pplfunc_acosh       , "acosh(x)", "\\mathrm{acosh}@<@1@>", "acosh(x) returns the hyperbolic arccosine of x");
    ppl_addSystemFunc(d,"acot"          ,1,1,1,1,0,1,(void *)&pplfunc_acot        , "acot(x)", "\\mathrm{acot}@<@1@>", "acot(x) returns the arccotangent of x");
    ppl_addSystemFunc(d,"acoth"         ,1,1,1,1,0,1,(void *)&pplfunc_acoth       , "acoth(x)", "\\mathrm{acoth}@<@1@>", "acoth(x) returns the hyperbolic arccotangent of x");
    ppl_addSystemFunc(d,"acsc"          ,1,1,1,1,0,1,(void *)&pplfunc_acsc        , "acsc(x)", "\\mathrm{acsc}@<@1@>", "acsc(x) returns the arccosecant of x");
    ppl_addSystemFunc(d,"acsch"         ,1,1,1,1,0,1,(void *)&pplfunc_acsch       , "acsch(x)", "\\mathrm{acsch}@<@1@>", "acsch(x) returns the hyperbolic arccosecant of x");
    ppl_addSystemFunc(d,"airy_ai"       ,1,1,1,1,0,1,(void *)&pplfunc_airy_ai     , "airy_ai(x)", "\\mathrm{airy\\_ai}@<@1@>", "airy_ai(z) returns the Airy function Ai evaluated at z");
    ppl_addSystemFunc(d,"airy_ai_diff"  ,1,1,1,1,0,1,(void *)&pplfunc_airy_ai_diff, "airy_ai_diff(x)", "\\mathrm{airy\\_ai\\_diff}@<@1@>", "airy_ai_diff(z) returns the first derivative of the Airy function Ai evaluated at z");
    ppl_addSystemFunc(d,"airy_bi"       ,1,1,1,1,0,1,(void *)&pplfunc_airy_bi     , "airy_bi(x)", "\\mathrm{airy\\_bi}@<@1@>", "airy_bi(z) returns the Airy function Bi evaluated at z");
    ppl_addSystemFunc(d,"airy_bi_diff"  ,1,1,1,1,0,1,(void *)&pplfunc_airy_bi_diff, "airy_bi_diff(x)", "\\mathrm{airy\\_bi\\_diff}@<@1@>", "airy_bi_diff(z) returns the first derivative of the Airy function Bi evaluated at z");
    ppl_addSystemFunc(d,"arg"           ,1,1,1,1,0,0,(void *)&pplfunc_arg         , "arg(x)", "\\mathrm{arg}@<@1@>", "arg(z) returns the argument of the complex number z");
    ppl_addSystemFunc(d,"asec"          ,1,1,1,1,0,1,(void *)&pplfunc_asec        , "asec(x)", "\\mathrm{asec}@<@1@>", "asec(x) returns the arcsecant of x");
    ppl_addSystemFunc(d,"asech"         ,1,1,1,1,0,1,(void *)&pplfunc_asech       , "asech(x)", "\\mathrm{asech}@<@1@>", "asech(x) returns the hyperbolic arcsecant of x");
    ppl_addSystemFunc(d,"asin"          ,1,1,1,1,0,1,(void *)&pplfunc_asin        , "asin(x)", "\\mathrm{asin}@<@1@>", "asin(x) returns the arcsine of x");
    ppl_addSystemFunc(d,"asinh"         ,1,1,1,1,0,1,(void *)&pplfunc_asinh       , "asinh(x)", "\\mathrm{asinh}@<@1@>", "asinh(x) returns the hyperbolic arcsine of x");
    ppl_addSystemFunc(d,"atan"          ,1,1,1,1,0,1,(void *)&pplfunc_atan        , "atan(x)", "\\mathrm{atan}@<@1@>", "atan(x) returns the arctangent of x");
    ppl_addSystemFunc(d,"atanh"         ,1,1,1,1,0,1,(void *)&pplfunc_atanh       , "atanh(x)", "\\mathrm{atanh}@<@1@>", "atanh(x) returns the hyperbolic arctangent of x");

   }

  return out;
 }

void ppl_contextFree(ppl_context *in)
 {
  int i;
  for (i=in->ns_ptr; i>in->ns_branch; i--) ppl_garbageNamespace(in->namespaces[i]);
  free(in);
  return;
 }

