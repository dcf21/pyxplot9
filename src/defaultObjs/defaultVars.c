// defaultVars.c
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
#include <string.h>

#include <gsl/gsl_const_mksa.h>
#include <gsl/gsl_const_num.h>
#include <gsl/gsl_math.h>
#include <gsl/gsl_sf.h>

#include "coreUtils/errorReport.h"
#include "coreUtils/dict.h"

#include "settings/epsColors.h"
#include "settings/settingTypes.h"

#include "stringTools/strConstants.h"

#include "userspace/context.h"
#include "userspace/pplObj.h"
#include "userspace/pplObjUnits.h"

#include "defaultObjs/defaultFuncs.h"
#include "defaultObjs/moduleAst.h"
#include "defaultObjs/moduleFractals.h"
#include "defaultObjs/moduleOs.h"
#include "defaultObjs/modulePhy.h"
#include "defaultObjs/moduleRandom.h"
#include "defaultObjs/moduleStats.h"

void ppl_makeDefaultVars(ppl_context *out)
 {
  out->ns_ptr    = 1;
  out->ns_branch = 0;
  out->namespaces[0] = NULL; // Will be defaults namespace
  out->namespaces[1] = ppl_dictInit(HASHSIZE_LARGE,1); // Will be root namespace

  // Default variables
   {
    pplObj  v, *m;
    dict   *d, *d2, *d3;
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
    v.auxilLen = strlen(VERSION)+1;
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
    for (i=0; SW_COLOR_INT[i]>=0; i++)
     {
      v.exponent[0] = SW_COLOR_CMYK_C[i];
      v.exponent[1] = SW_COLOR_CMYK_M[i];
      v.exponent[2] = SW_COLOR_CMYK_Y[i];
      v.exponent[3] = SW_COLOR_CMYK_K[i];
      ppl_dictAppendCpy(d2 , SW_COLOR_STR[i] , (void *)&v , sizeof(v));
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
    ppl_addSystemFunc(d2,"Bv"            ,2,2,1,1,1,0,(void *)&pplfunc_planck_Bv   , "Bv(nu,T)", "\\mathrm{B_\\nu}@<@1,@2@>", "Bv(nu,T) returns the power emitted by a blackbody of temperature T per unit area, per unit solid angle, per unit frequency");
    ppl_addSystemFunc(d2,"Bvmax"         ,1,1,1,1,1,0,(void *)&pplfunc_planck_Bvmax, "Bvmax(T)", "\\mathrm{B_{\\nu,max}}@<@1@>", "Bvmax(T) returns the frequency of the maximum of the function Bv(nu,T)");

    // OS module
    ppl_dictAppendCpy(d  , "os"        , m=pplNewModule(frozen) , sizeof(v));
    d2 = (dict *)m->auxil;
    ppl_addSystemFunc(d2,"getegid"       ,0,0,1,1,1,1,(void *)&pplfunc_osGetEgid , "getegid()" , "\\mathrm{getegid}@<@>", "getegid() returns the effective group id of the PyXPlot process");
    ppl_addSystemFunc(d2,"geteuid"       ,0,0,1,1,1,1,(void *)&pplfunc_osGetEuid , "geteuid()" , "\\mathrm{geteuid}@<@>", "geteuid() returns the effective user id of the PyXPlot process");
    ppl_addSystemFunc(d2,"getgid"        ,0,0,1,1,1,1,(void *)&pplfunc_osGetGid  , "getgid()"  , "\\mathrm{getgid}@<@>", "getgid() returns the group id of the PyXPlot process");
    ppl_addSystemFunc(d2,"getpid"        ,0,0,1,1,1,1,(void *)&pplfunc_osGetPid  , "getpid()"  , "\\mathrm{getpid}@<@>", "getpid() returns the process id of the PyXPlot process");
    ppl_addSystemFunc(d2,"getpgrp"       ,0,0,1,1,1,1,(void *)&pplfunc_osGetPgrp , "getpgrp()" , "\\mathrm{getpgrp}@<@>", "getpgrp() returns the process group id of the PyXPlot process");
    ppl_addSystemFunc(d2,"getppid"       ,0,0,1,1,1,1,(void *)&pplfunc_osGetPpid , "getppid()" , "\\mathrm{getppid}@<@>", "getpid() returns the parent process id of the PyXPlot process");
    ppl_addSystemFunc(d2,"getuid"        ,0,0,1,1,1,1,(void *)&pplfunc_osGetUid  , "getuid()"  , "\\mathrm{getuid}@<@>", "getuid() returns the user id of the PyXPlot process");
    ppl_addSystemFunc(d2,"gethomedir"    ,0,0,1,1,1,1,(void *)&pplfunc_osGetHome , "gethomedir()" , "\\mathrm{gethomedir}@<@>", "gethomedir() returns the path of the user's home directory");
    ppl_addSystemFunc(d2,"gethostname"   ,0,0,1,1,1,1,(void *)&pplfunc_osGetHost , "gethostname()", "\\mathrm{gethostname}@<@>", "gethostname() returns the system's host name");
    ppl_addSystemFunc(d2,"getlogin"      ,0,0,1,1,1,1,(void *)&pplfunc_osGetLogin, "getlogin()", "\\mathrm{getlogin}@<@>", "getlogin() returns the system login of the user");
    ppl_addSystemFunc(d2,"getrealname"   ,0,0,1,1,1,1,(void *)&pplfunc_osGetRealName, "getrealname()", "\\mathrm{getrealname}@<@>", "getrealname() returns the user's real name");
    ppl_addSystemFunc(d2,"system"        ,1,1,0,0,0,0,(void *)&pplfunc_osSystem  , "system()", "\\mathrm{system}@<@>", "system() executes a command in a subshell");
    ppl_dictAppendCpy(d2 , "path"      , m=pplNewModule(frozen) , sizeof(v));
    d3 = (dict *)m->auxil;
    ppl_addSystemFunc(d3,"exists"        ,1,1,0,0,0,0,(void *)&pplfunc_osPathExists, "os.path.exists(x)", "\\mathrm{os.path.exists}@<@0@>", "os.path.exists(x) returns a boolean flag indicating whether a file with pathname x exists");
    ppl_addSystemFunc(d3,"expanduser"    ,1,1,0,0,0,0,(void *)&pplfunc_osPathExpandUser, "os.path.expanduser(x)", "\\mathrm{os.path.expanduser@<@0@>", "os.path.expanduser(x) returns its argument with ~s indicating home directories expanded");
    ppl_addSystemFunc(d3,"join"          ,1,9999,0,0,0,0,(void *)&pplfunc_osPathJoin , "os.path.join(...)", "\\mathrm{os.path.join}@<@0@>", "os.path.join(...) joins a series of strings intelligently into a pathname");


    // Default maths functions
    ppl_addSystemFunc(d,"abs"           ,1,1,1,1,0,0,(void *)&pplfunc_abs         , "abs(z)", "\\mathrm{abs}@<@1@>", "abs(z) returns the absolute magnitude of z");
    ppl_addSystemFunc(d,"acos"          ,1,1,1,1,0,1,(void *)&pplfunc_acos        , "acos(z)", "\\mathrm{acos}@<@1@>", "acos(z) returns the arccosine of z");
    ppl_addSystemFunc(d,"acosh"         ,1,1,1,1,0,1,(void *)&pplfunc_acosh       , "acosh(z)", "\\mathrm{acosh}@<@1@>", "acosh(z) returns the hyperbolic arccosine of z");
    ppl_addSystemFunc(d,"acot"          ,1,1,1,1,0,1,(void *)&pplfunc_acot        , "acot(z)", "\\mathrm{acot}@<@1@>", "acot(z) returns the arccotangent of z");
    ppl_addSystemFunc(d,"acoth"         ,1,1,1,1,0,1,(void *)&pplfunc_acoth       , "acoth(z)", "\\mathrm{acoth}@<@1@>", "acoth(z) returns the hyperbolic arccotangent of z");
    ppl_addSystemFunc(d,"acsc"          ,1,1,1,1,0,1,(void *)&pplfunc_acsc        , "acsc(z)", "\\mathrm{acsc}@<@1@>", "acsc(z) returns the arccosecant of z");
    ppl_addSystemFunc(d,"acsch"         ,1,1,1,1,0,1,(void *)&pplfunc_acsch       , "acsch(z)", "\\mathrm{acsch}@<@1@>", "acsch(z) returns the hyperbolic arccosecant of z");
    ppl_addSystemFunc(d,"airy_ai"       ,1,1,1,1,0,1,(void *)&pplfunc_airy_ai     , "airy_ai(z)", "\\mathrm{airy\\_ai}@<@1@>", "airy_ai(z) returns the Airy function Ai evaluated at z");
    ppl_addSystemFunc(d,"airy_ai_diff"  ,1,1,1,1,0,1,(void *)&pplfunc_airy_ai_diff, "airy_ai_diff(z)", "\\mathrm{airy\\_ai\\_diff}@<@1@>", "airy_ai_diff(z) returns the first derivative of the Airy function Ai evaluated at z");
    ppl_addSystemFunc(d,"airy_bi"       ,1,1,1,1,0,1,(void *)&pplfunc_airy_bi     , "airy_bi(z)", "\\mathrm{airy\\_bi}@<@1@>", "airy_bi(z) returns the Airy function Bi evaluated at z");
    ppl_addSystemFunc(d,"airy_bi_diff"  ,1,1,1,1,0,1,(void *)&pplfunc_airy_bi_diff, "airy_bi_diff(z)", "\\mathrm{airy\\_bi\\_diff}@<@1@>", "airy_bi_diff(z) returns the first derivative of the Airy function Bi evaluated at z");
    ppl_addSystemFunc(d,"arg"           ,1,1,1,1,0,0,(void *)&pplfunc_arg         , "arg(z)", "\\mathrm{arg}@<@1@>", "arg(z) returns the argument of the complex number z");
    ppl_addSystemFunc(d,"asec"          ,1,1,1,1,0,1,(void *)&pplfunc_asec        , "asec(z)", "\\mathrm{asec}@<@1@>", "asec(z) returns the arcsecant of z");
    ppl_addSystemFunc(d,"asech"         ,1,1,1,1,0,1,(void *)&pplfunc_asech       , "asech(z)", "\\mathrm{asech}@<@1@>", "asech(z) returns the hyperbolic arcsecant of z");
    ppl_addSystemFunc(d,"asin"          ,1,1,1,1,0,1,(void *)&pplfunc_asin        , "asin(z)", "\\mathrm{asin}@<@1@>", "asin(z) returns the arcsine of z");
    ppl_addSystemFunc(d,"asinh"         ,1,1,1,1,0,1,(void *)&pplfunc_asinh       , "asinh(z)", "\\mathrm{asinh}@<@1@>", "asinh(z) returns the hyperbolic arcsine of z");
    ppl_addSystemFunc(d,"atan"          ,1,1,1,1,0,1,(void *)&pplfunc_atan        , "atan(z)", "\\mathrm{atan}@<@1@>", "atan(z) returns the arctangent of z");
    ppl_addSystemFunc(d,"atan2"         ,2,2,1,1,1,0,(void *)&pplfunc_atan2       , "atan2(x,y)", "\\mathrm{atan2}@<@1,@2@>", "atan2(x,y) returns the arctangent of x/y. Unlike atan(y/x), atan2(x,y) takes account of the signs of both x and y to remove the degeneracy between (1,1) and (-1,-1)");
    ppl_addSystemFunc(d,"atanh"         ,1,1,1,1,0,1,(void *)&pplfunc_atanh       , "atanh(z)", "\\mathrm{atanh}@<@1@>", "atanh(z) returns the hyperbolic arctangent of z");
    ppl_addSystemFunc(d,"besseli"       ,2,2,1,1,1,1,(void *)&pplfunc_besseli     , "besseli(l,x)", "\\mathrm{besseli}@<@1,@2@>", "besseli(l,x) evaluates the lth regular modified spherical Bessel function at x");
    ppl_addSystemFunc(d,"besselI"       ,2,2,1,1,1,1,(void *)&pplfunc_besselI     , "besselI(l,x)", "\\mathrm{besselI}@<@1,@2@>", "besselI(l,x) evaluates the lth regular modified cylindrical Bessel function at x");
    ppl_addSystemFunc(d,"besselj"       ,2,2,1,1,1,1,(void *)&pplfunc_besselj     , "besselj(l,x)", "\\mathrm{besselj}@<@1,@2@>", "besselj(l,x) evaluates the lth regular spherical Bessel function at x");
    ppl_addSystemFunc(d,"besselJ"       ,2,2,1,1,1,1,(void *)&pplfunc_besselJ     , "besselJ(l,x)", "\\mathrm{besselJ}@<@1,@2@>", "besselJ(l,x) evaluates the lth regular cylindrical Bessel function at x");
    ppl_addSystemFunc(d,"besselk"       ,2,2,1,1,1,1,(void *)&pplfunc_besselk     , "besselk(l,x)", "\\mathrm{besselk}@<@1,@2@>", "besselk(l,x) evaluates the lth irregular modified spherical Bessel function at x");
    ppl_addSystemFunc(d,"besselK"       ,2,2,1,1,1,1,(void *)&pplfunc_besselK     , "besselK(l,x)", "\\mathrm{besselK}@<@1,@2@>", "besselK(l,x) evaluates the lth irregular modified cylindrical Bessel function at x");
    ppl_addSystemFunc(d,"bessely"       ,2,2,1,1,1,1,(void *)&pplfunc_bessely     , "bessely(l,x)", "\\mathrm{bessely}@<@1,@2@>", "bessely(l,x) evaluates the lth irregular spherical Bessel function at x");
    ppl_addSystemFunc(d,"besselY"       ,2,2,1,1,1,1,(void *)&pplfunc_besselY     , "besselY(l,x)", "\\mathrm{besselY}@<@1,@2@>", "besselY(l,x) evaluates the lth irregular cylindrical Bessel function at x");
    ppl_addSystemFunc(d,"beta"          ,2,2,1,1,1,1,(void *)&pplfunc_beta        , "beta(a,b)", "\\mathrm{B}@<@1,@2@>", "beta(a,b) evaluates the beta function B(a,b)");
    ppl_addSystemFunc(d,"ceil"          ,1,1,1,1,1,1,(void *)&pplfunc_ceil        , "ceil(x)", "\\mathrm{ceil}@<@1@>", "ceil(x) returns the smallest integer value greater than or equal to x");
    ppl_addSystemFunc(d,"conjugate"     ,1,1,1,1,0,0,(void *)&pplfunc_conjugate   , "conjugate(z)", "\\mathrm{conjugate}@<@1@>", "conjugate(z) returns the complex conjugate of z");
    ppl_addSystemFunc(d,"cos"           ,1,1,1,1,0,1,(void *)&pplfunc_cos         , "cos(z)", "\\mathrm{cos}@<@1@>", "cos(x) returns the cosine of x. If x is dimensionless, it is assumed to be measured in radians");
    ppl_addSystemFunc(d,"cosh"          ,1,1,1,1,0,1,(void *)&pplfunc_cosh        , "cosh(z)", "\\mathrm{cosh}@<@1@>", "cosh(x) returns the hyperbolic cosine of x. x may either be a dimensionless number or may have units of angle");
    ppl_addSystemFunc(d,"cot"           ,1,1,1,1,0,1,(void *)&pplfunc_cot         , "cot(z)", "\\mathrm{cot}@<@1@>", "cot(x) returns the cotangent of x. If x is dimensionless, it is assumed to be measured in radians");
    ppl_addSystemFunc(d,"coth"          ,1,1,1,1,0,1,(void *)&pplfunc_coth        , "coth(z)", "\\mathrm{coth}@<@1@>", "coth(x) returns the hyperbolic cotangent of x. x may either be a dimensionless number or may have units of angle");
    ppl_addSystemFunc(d,"csc"           ,1,1,1,1,0,1,(void *)&pplfunc_csc         , "csc(z)", "\\mathrm{csc}@<@1@>", "csc(x) returns the cosecant of x. If x is dimensionless, it is assumed to be measured in radians");
    ppl_addSystemFunc(d,"csch"          ,1,1,1,1,0,1,(void *)&pplfunc_csch        , "csch(z)", "\\mathrm{csch}@<@1@>", "csch(x) returns the hyperbolic cosecant of x. x may either be a dimensionless number or may have units of angle");
    ppl_addSystemFunc(d,"degrees"       ,1,1,1,1,1,0,(void *)&pplfunc_degrees     , "degrees(x)", "\\mathrm{degrees}@<@1@>", "degrees(x) converts angles measured in radians into degrees");
    ppl_addSystemFunc(d,"ellK"          ,1,1,1,1,1,1,(void *)&pplfunc_ellK        , "ellipticintK(k)", "\\mathrm{ellipticintK}@<@1@>", "ellipticintK(k) evaluates the complete elliptic integral K(k)");
    ppl_addSystemFunc(d,"ellE"          ,1,1,1,1,1,1,(void *)&pplfunc_ellE        , "ellipticintE(k)", "\\mathrm{ellipticintE}@<@1@>", "ellipticintE(k) evaluates the complete elliptic integral E(k)");
    ppl_addSystemFunc(d,"ellP"          ,2,2,1,1,1,1,(void *)&pplfunc_ellP        , "ellipticintP(k,n)", "\\mathrm{ellipticintP}@<@1,@2@>", "ellipticintP(k,n) evaluates the complete elliptic integral P(k,n)");
    ppl_addSystemFunc(d,"erf"           ,1,1,1,1,1,1,(void *)&pplfunc_erf         , "erf(x)", "\\mathrm{erf}@<@1@>", "erf(x) evaluates the error function at x");
    ppl_addSystemFunc(d,"erfc"          ,1,1,1,1,1,1,(void *)&pplfunc_erfc        , "erfc(x)", "\\mathrm{erfc}@<@1@>", "erfc(x) evaluates the complimentary error function at x");
    ppl_addSystemFunc(d,"exp"           ,1,1,1,1,0,0,(void *)&pplfunc_exp         , "exp(z)", "\\mathrm{exp}@<@1@>", "exp(x) returns e to the power of x. x may either be a dimensionless number or may have units of angle");
    ppl_addSystemFunc(d,"expm1"         ,1,1,1,1,0,0,(void *)&pplfunc_expm1       , "expm1(x)", "\\mathrm{expm1}@<@1@>", "expm1(x) accurately evaluates exp(x)-1");
    ppl_addSystemFunc(d,"expint"        ,2,2,1,1,1,1,(void *)&pplfunc_expint      , "expint(n,x)", "\\mathrm{expint}@<@1,@2@>", "expint(n,x) evaluates the integral of exp(-xt)/t**n between one and infinity");
    ppl_addSystemFunc(d,"finite"        ,1,1,1,0,0,0,(void *)&pplfunc_finite      , "finite(z)", "\\mathrm{finite}@<@1@>", "finite(x) returns 1 if x is a finite number, and 0 otherwise");
    ppl_addSystemFunc(d,"floor"         ,1,1,1,1,1,1,(void *)&pplfunc_floor       , "floor(x)", "\\mathrm{floor}@<@1@>", "floor(x) returns the largest integer value smaller than or equal to x");
    ppl_addSystemFunc(d,"gamma"         ,1,1,1,1,1,1,(void *)&pplfunc_gamma       , "gamma(x)", "\\mathrm{\\Gamma}@<@1@>", "gamma(x) evaluates the gamma function at x");
    ppl_addSystemFunc(d,"heaviside"     ,1,1,1,1,1,0,(void *)&pplfunc_heaviside   , "heaviside(x)", "\\mathrm{heaviside}@<@1@>", "heaviside(x) returns the Heaviside function, defined to be one for x>=0 and zero otherwise");
    ppl_addSystemFunc(d,"hyperg_0F1"    ,2,2,1,1,1,1,(void *)&pplfunc_hyperg_0F1  , "hyperg_0F1(c,x)", "\\mathrm{hyperg\\_}_0\\mathrm{F}_1}@<@1,@2@>", "hyperg_0F1(c,x) evaluates the hypergeometric function 0F1(c,x)");
    ppl_addSystemFunc(d,"hyperg_1F1"    ,3,3,1,1,1,1,(void *)&pplfunc_hyperg_1F1  , "hyperg_1F1(a,b,x)", "\\mathrm{hyperg\\_}_1\\mathrm{F}_1}@<@1,@2,@3@>", "hyperg_1F1(a,b,x) evaluates the confluent hypergeometric function 1F1(a,b,x)");
    ppl_addSystemFunc(d,"hyperg_2F0"    ,3,3,1,1,1,1,(void *)&pplfunc_hyperg_2F0  , "hyperg_2F0(a,b,x)", "\\mathrm{hyperg\\_}_2\\mathrm{F}_0}@<@1,@2,@3@>", "hyperg_2F0(a,b,x) evaluates the hypergeometric function 2F0(a,b,x)");
    ppl_addSystemFunc(d,"hyperg_2F1"    ,4,4,1,1,1,1,(void *)&pplfunc_hyperg_2F1  , "hyperg_2F1(a,b,c,x)", "\\mathrm{hyperg\\_}_2\\mathrm{F}_1}@<@1,@2,@3,@4@>", "hyperg_2F1(a,b,c,x) evaluates the Gauss hypergeometric function 2F1(a,b,c,x)");
    ppl_addSystemFunc(d,"hyperg_U"      ,3,3,1,1,1,1,(void *)&pplfunc_hyperg_U    , "hyperg_U(a,b,x)", "\\mathrm{hyperg\\_U}@<@1,@2,@3@>", "hyperg_U(a,b,x) evaluates the confluent hypergeometric function U(m,n,x)");
    ppl_addSystemFunc(d,"hypot"         ,2,2,1,1,0,0,(void *)&pplfunc_hypot       , "hypot(x,...)", "\\mathrm{hypot}@<@1,@2@>", "hypot(x,...) returns the quadrature sum of its arguments");
    ppl_addSystemFunc(d,"imag"          ,1,1,1,1,0,0,(void *)&pplfunc_imag        , "Im(z)", "\\mathrm{Im}@<@1@>", "Im(z) returns the magnitude of the imaginary part of z");
    ppl_addSystemFunc(d,"jacobi_cn"     ,2,2,1,1,1,1,(void *)&pplfunc_jacobi_cn   , "jacobi_cn(u,m)", "\\mathrm{jacobi\\_cn}@<@1,@2@>", "jacobi_cn(u,m) returns the Jacobi elliptic function cn(u,m)");
    ppl_addSystemFunc(d,"jacobi_dn"     ,2,2,1,1,1,1,(void *)&pplfunc_jacobi_dn   , "jacobi_dn(u,m)", "\\mathrm{jacobi\\_dn}@<@1,@2@>", "jacobi_dn(u,m) returns the Jacobi elliptic function dn(u,m)");
    ppl_addSystemFunc(d,"jacobi_sn"     ,2,2,1,1,1,1,(void *)&pplfunc_jacobi_sn   , "jacobi_sn(u,m)", "\\mathrm{jacobi\\_sn}@<@1,@2@>", "jacobi_sn(u,m) returns the Jacobi elliptic function sn(u,m)");
    ppl_addSystemFunc(d,"lambert_W0"    ,1,1,1,1,1,1,(void *)&pplfunc_lambert_W0  , "lambert_W0(x)", "\\mathrm{lambert\\_W0}@<@1@>", "lambert_W0(x) returns the principal branch of the Lambert W function");
    ppl_addSystemFunc(d,"lambert_W1"    ,1,1,1,1,1,1,(void *)&pplfunc_lambert_W1  , "lambert_W1(x)", "\\mathrm{lambert\\_W1}@<@1@>", "lambert_W1(x) returns the secondary branch of the Lambert W function");
    ppl_addSystemFunc(d,"ldexp"         ,2,2,1,1,1,1,(void *)&pplfunc_ldexp       , "ldexp(x,y)", "\\mathrm{ldexp}@<@1,@2@>", "ldexp(x,y) returns x times 2 to the power of an integer y");
    ppl_addSystemFunc(d,"legendreP"     ,2,2,1,1,1,1,(void *)&pplfunc_legendreP   , "legendreP(l,x)", "\\mathrm{legendreP}@<@1,@2@>", "legendreP(l,x) evaluates the lth Legendre polynomial at x");
    ppl_addSystemFunc(d,"legendreQ"     ,2,2,1,1,1,1,(void *)&pplfunc_legendreQ   , "legendreQ(l,x)", "\\mathrm{legendreQ}@<@1,@2@>", "legendreQ(l,x) evaluates the lth Legendre function at x");
    ppl_addSystemFunc(d,"log"           ,1,1,1,1,0,1,(void *)&pplfunc_log         , "log(z)", "\\mathrm{log}@<@1@>", "log(x) returns the natural logarithm of x");
    ppl_addSystemFunc(d,"log10"         ,1,1,1,1,0,1,(void *)&pplfunc_log10       , "log10(z)", "\\mathrm{log_{10}}@<@1@>", "log10(x) returns the logarithm of x to base 10");
    ppl_addSystemFunc(d,"logn"          ,2,2,1,1,0,1,(void *)&pplfunc_logn        , "logn(z,n)", "\\mathrm{log}_n@<@1@>", "logn(x,n) returns the logarithm of x to base n");
    ppl_addSystemFunc(d,"ln"            ,1,1,1,1,0,1,(void *)&pplfunc_log         , "ln(z)", "\\mathrm{ln}@<@1@>", "ln(x) is an alias for log(x): it returns the natural logarithm of x");
    ppl_addSystemFunc(d,"max"           ,2,2,1,1,0,1,(void *)&pplfunc_max         , "max(x,...)", "\\mathrm{max}@<@1,@2@>", "max(x,...) returns the greatest of its arguments");
    ppl_addSystemFunc(d,"min"           ,2,2,1,1,0,1,(void *)&pplfunc_min         , "min(x,...)", "\\mathrm{min}@<@1,@2@>", "min(x,...) returns the least of its arguments");
    ppl_addSystemFunc(d,"mod"           ,2,2,1,1,1,0,(void *)&pplfunc_mod         , "mod(x,y)", "\\mathrm{mod}@<@1,@2@>", "mod(x,y) returns the remainder of x/y");
    ppl_addSystemFunc(d,"pow"           ,2,2,1,1,0,0,(void *)&pplfunc_pow         , "pow(x,y)", "\\mathrm{pow}@<@1,@2@>", "pow(x,y) returns x to the power of y");
    ppl_addSystemFunc(d,"prime"         ,1,1,1,1,1,1,(void *)&pplfunc_prime       , "prime(x)", "\\mathrm{prime}@<@1@>", "prime(x) returns one if floor(x) is a prime number; zero otherwise");
    ppl_addSystemFunc(d,"radians"       ,1,1,1,1,1,0,(void *)&pplfunc_radians     , "radians(x)", "\\mathrm{radians}@<@1@>", "radians(x) converts angles measured in degrees into radians");
    ppl_addSystemFunc(d,"real"          ,1,1,1,1,0,0,(void *)&pplfunc_real        , "Re(z)", "\\mathrm{Re}@<@1@>", "Re(z) returns the magnitude of the real part of z");
    ppl_addSystemFunc(d,"root"          ,2,2,1,1,0,1,(void *)&pplfunc_root        , "root(z,n)", "\\mathrm{root}@<@1,@2@>", "root(z,n) returns the nth root of z");
    ppl_addSystemFunc(d,"sec"           ,1,1,1,1,0,0,(void *)&pplfunc_sec         , "sec(z)", "\\mathrm{sec}@<@1@>", "sec(x) returns the secant of x. If x is dimensionless, it is assumed to be measured in radians");
    ppl_addSystemFunc(d,"sech"          ,1,1,1,1,0,0,(void *)&pplfunc_sech        , "sech(z)", "\\mathrm{sech}@<@1@>", "sech(x) returns the hyperbolic secant of x. x may either be a dimensionless number or may have units of angle");
    ppl_addSystemFunc(d,"sin"           ,1,1,1,1,0,0,(void *)&pplfunc_sin         , "sin(z)", "\\mathrm{sin}@<@1@>", "sin(x) returns the sine of x. If x is dimensionless, it is assumed to be measured in radians");
    ppl_addSystemFunc(d,"sinc"          ,1,1,1,1,0,0,(void *)&pplfunc_sinc        , "sinc(z)", "\\mathrm{sinc}@<@1@>", "sinc(x) returns the function sin(pi*x)/(pi*x). If x is dimensionless, it is assumed to be measured in radians. The output is dimensionless.");
    ppl_addSystemFunc(d,"sinh"          ,1,1,1,1,0,0,(void *)&pplfunc_sinh        , "sinh(z)", "\\mathrm{sinh}@<@1@>", "sinh(x) returns the hyperbolic sine of x. x may either be a dimensionless number or may have units of angle");
    ppl_addSystemFunc(d,"sqrt"          ,1,1,1,1,0,0,(void *)&pplfunc_sqrt        , "sqrt(z)", "\\sqrt{@1}", "sqrt(x) returns the square root of x");
    ppl_addSystemFunc(d,"tan"           ,1,1,1,1,0,0,(void *)&pplfunc_tan         , "tan(z)", "\\mathrm{tan}@<@1@>", "tan(x) returns the tangent of x. If x is dimensionless, it is assumed to be measured in radians");
    ppl_addSystemFunc(d,"tanh"          ,1,1,1,1,0,0,(void *)&pplfunc_tanh        , "tanh(z)", "\\mathrm{tanh}@<@1@>", "tanh(x) returns the hyperbolic tangent of x. x may either be a dimensionless number or may have units of angle");
    ppl_addSystemFunc(d,"tophat"        ,2,2,1,1,1,0,(void *)&pplfunc_tophat      , "tophat(x,sigma)", "\\mathrm{tophat}@<@1,@2@>", "tophat(x,sigma) returns one if |x| <= |sigma|, and zero otherwise");
    ppl_addSystemFunc(d,"zernike"       ,4,4,1,1,1,0,(void *)&pplfunc_zernike     , "zernike(n,m,r,phi)", "\\mathrm{zernike}@<@1,@2,@3,@4@>", "zernike(n,m,r,phi) evaluates the (n,m)th Zernike polynomial at radius r and position angle phi");
    ppl_addSystemFunc(d,"zernikeR"      ,3,3,1,1,1,1,(void *)&pplfunc_zernikeR    , "zernikeR(n,m,r)", "\\mathrm{zernikeR}@<@1,@2,@3@>", "zernikeR(n,m,r) evaluates the (n,m)th radial Zernike polynomial at radius r");
    ppl_addSystemFunc(d,"zeta"          ,1,1,1,1,1,1,(void *)&pplfunc_zeta        , "zeta(z)", "\\zeta@<@1@>", "zeta(x) evaluates the Riemann zeta function at x");

    // Ast module
    ppl_dictAppendCpy(d, "ast", m=pplNewModule(frozen) , sizeof(v));
    d2 = (dict *)m->auxil;
    ppl_addSystemFunc(d2,"Lcdm_age"     ,3,3,1,1,1,0,(void *)&pplfunc_Lcdm_age     , "Lcdm_age(H0,w_m,w_l)", "\\mathrm{\\Lambda_{CDM}\\_age@<@1,@2,@3@>", "Lcdm_age(H0,w_m,w_l) returns the current age of the Universe in an L_CDM cosmology");
    ppl_addSystemFunc(d2,"Lcdm_angscale",4,4,1,1,1,0,(void *)&pplfunc_Lcdm_angscale, "Lcdm_age(z,H0,w_m,w_l)", "\\mathrm{\\Lambda_{CDM}\\_angscale@<@1,@2,@3,@4@>", "Lcdm_angscale(z,H0,w_m,w_l) returns the angular scale of the sky in distance per unit angle for an L_CDM cosmology");
    ppl_addSystemFunc(d2,"Lcdm_DA"      ,4,4,1,1,1,0,(void *)&pplfunc_Lcdm_DA      , "Lcdm_DA(z,H0,w_m,w_l)", "\\mathrm{\\Lambda_{CDM}\\_D_A@<@1,@2,@3,@4@>", "Lcdm_DA(z,H0,w_m,w_l) returns the angular size distance corresponding to redshift z in an L_CDM cosmology");
    ppl_addSystemFunc(d2,"Lcdm_DL"      ,4,4,1,1,1,0,(void *)&pplfunc_Lcdm_DL      , "Lcdm_DL(z,H0,w_m,w_l)", "\\mathrm{\\Lambda_{CDM}\\_D_L@<@1,@2,@3,@4@>", "Lcdm_DL(z,H0,w_m,w_l) returns the luminosity distance corresponding to redshift z in an L_CDM cosmology");
    ppl_addSystemFunc(d2,"Lcdm_DM"      ,4,4,1,1,1,0,(void *)&pplfunc_Lcdm_DM      , "Lcdm_DM(z,H0,w_m,w_l)", "\\mathrm{\\Lambda_{CDM}\\_D_M@<@1,@2,@3,@4@>", "Lcdm_DM(z,H0,w_m,w_l) returns the comoving distance corresponding to redshift z in an L_CDM cosmology");
    ppl_addSystemFunc(d2,"Lcdm_t"       ,4,4,1,1,1,0,(void *)&pplfunc_Lcdm_t       , "Lcdm_t(z,H0,w_m,w_l)", "\\mathrm{\\Lambda_{CDM}\\_t@<@1,@2,@3,@4@>", "Lcdm_t(z,H0,w_m,w_l) returns the lookback time corresponding to redshift z in an L_CDM cosmology");
    ppl_addSystemFunc(d2,"Lcdm_z"       ,4,4,1,1,1,0,(void *)&pplfunc_Lcdm_z       , "Lcdm_z(t,H0,w_m,w_l)", "\\mathrm{\\Lambda_{CDM}\\_z@<@1,@2,@3,@4@>", "Lcdm_z(t,H0,w_m,w_l) returns the redshift corresponding to a lookback time t in an L_CDM cosmology");

    // Fractals module
    ppl_dictAppendCpy(d, "fractals", m=pplNewModule(frozen) , sizeof(v));
    d2 = (dict *)m->auxil;
    ppl_addSystemFunc(d2,"julia"      ,3,3,1,1,0,1,(void *)&pplfunc_julia      ,"julia(z,cz,MaxIter)", "\\mathrm{julia}@<@1,@2,@3@>", "julia(z,cz,MaxIter) returns the number of iterations required before the Julia set iterator diverges outside the circle |z'|<2");
    ppl_addSystemFunc(d2,"mandelbrot" ,2,2,1,1,0,1,(void *)&pplfunc_mandelbrot ,"mandelbrot(z,MaxIter)", "\\mathrm{mandelbrot}@<@1,@2@>", "mandelbrot(z,MaxIter) returns the number of iterations required before the Mandelbrot set iterator diverges outside the circle |z'|<2");


    // Stats module
    ppl_dictAppendCpy(d, "stats", m=pplNewModule(frozen) , sizeof(v));
    d2 = (dict *)m->auxil;
    ppl_addSystemFunc(d2,"binomialPDF"   ,3,3,1,1,1,1,(void *)&pplfunc_binomialPDF , "binomialPDF(k,p,n)", "\\mathrm{binomialPDF}@<@1,@2,@3@>", "binomialPDF(k,p,n) evaulates the probability of getting k successes out of n trials in a binomial distribution with success probability p");
    ppl_addSystemFunc(d2,"binomialCDF"   ,3,3,1,1,1,1,(void *)&pplfunc_binomialCDF , "binomialCDF(k,p,n)", "\\mathrm{binomialCDF}@<@1,@2,@3@>", "binomialCDF(k,p,n) evaulates the probability of getting fewer than or exactly k successes out of n trials in a binomial distribution with success probability p");
    ppl_addSystemFunc(d2,"chisqPDF"      ,2,2,1,1,1,1,(void *)&pplfunc_chisqPDF    , "chisqPDF(x,nu)", "\\mathrm{\\chi^2 PDF}@<@1,@2@>", "chisqPDF(x,nu) returns the probability density at x in a chi-squared distribution with nu degrees of freedom");
    ppl_addSystemFunc(d2,"chisqCDF"      ,2,2,1,1,1,1,(void *)&pplfunc_chisqCDF    , "chisqCDF(x,nu)", "\\mathrm{\\chi^2 CDF}@<@1,@2@>", "chisqCDF(x,nu) returns the cumulative probability density at x in a chi-squared distribution with nu degrees of freedom");
    ppl_addSystemFunc(d2,"chisqCDFi"     ,2,2,1,1,1,1,(void *)&pplfunc_chisqCDFi   , "chisqCDFi(P,nu)", "\\mathrm{\\chi^2 CDFi}@<@1,@2@>", "chisqCDFi(P,nu) returns the point x at which the cumulative probability density in a chi-squared distribution with nu degrees of freedom is P");
    ppl_addSystemFunc(d2,"gaussianPDF"   ,2,2,1,1,1,0,(void *)&pplfunc_gaussianPDF , "gaussianPDF(x,sigma)", "\\mathrm{gaussianPDF}@<@1,@2@>", "gaussianPDF(x,sigma) evaluates the Gaussian probability density function of standard deviation sigma at x");
    ppl_addSystemFunc(d2,"gaussianCDF"   ,2,2,1,1,1,0,(void *)&pplfunc_gaussianCDF , "gaussianCDF(x,sigma)", "\\mathrm{gaussianCDF}@<@1,@2@>", "gaussianCDF(x,sigma) evaluates the Gaussian cumulative distribution function of standard deviation sigma at x");
    ppl_addSystemFunc(d2,"gaussianCDFi"  ,2,2,1,1,1,0,(void *)&pplfunc_gaussianCDFi, "gaussianCDFi(x,sigma)", "\\mathrm{gaussianCDFi}@<@1,@2@>", "gaussianCDFi(x,sigma) evaluates the inverse Gaussian cumulative distribution function of standard deviation sigma at x");
    ppl_addSystemFunc(d2,"lognormalPDF"  ,3,3,1,1,1,0,(void *)&pplfunc_lognormalPDF, "lognormalPDF(x,zeta,sigma)", "\\mathrm{lognormalPDF}@<@1,@2,@3@>", "lognormalPDF(x,zeta,sigma) evaluates the log normal probability density function of standard deviation sigma at x");
    ppl_addSystemFunc(d2,"lognormalCDF"  ,3,3,1,1,1,0,(void *)&pplfunc_lognormalCDF, "lognormalCDF(x,zeta,sigma)", "\\mathrm{lognormalCDF}@<@1,@2,@3@>", "lognormalCDF(x,zeta,sigma) evaluates the log normal cumulative distribution function of standard deviation sigma at x");
    ppl_addSystemFunc(d2,"lognormalCDFi" ,3,3,1,1,1,0,(void *)&pplfunc_lognormalCDFi,"lognormalCDFi(x,zeta,sigma)", "\\mathrm{lognormalCDFi}@<@1,@2,@3@>", "lognormalCDFi(x,zeta,sigma) evaluates the inverse log normal cumulative distribution function of standard deviation sigma at x");
    ppl_addSystemFunc(d2,"poissonPDF"    ,2,2,1,1,1,1,(void *)&pplfunc_poissonPDF  , "poissonPDF(x,mu)", "\\mathrm{poissonPDF}@<@1,@2@>", "poissonPDF(x,mu) returns the probability of getting x from a Poisson distribution with mean mu");
    ppl_addSystemFunc(d2,"poissonCDF"    ,2,2,1,1,1,1,(void *)&pplfunc_poissonCDF  , "poissonCDF(x,mu)", "\\mathrm{poissonCDF}@<@1,@2@>", "poissonCDF(x,mu) returns the probability of getting <= x from a Poisson distribution with mean mu");
    ppl_addSystemFunc(d2,"tdistPDF"      ,2,2,1,1,1,1,(void *)&pplfunc_tdistPDF    , "tdistPDF(x,nu)", "\\mathrm{tdistPDF}@<@1,@2@>", "tdistPDF(x,nu) returns the probability density at x in a t-distribution with nu degrees of freedom");
    ppl_addSystemFunc(d2,"tdistCDF"      ,2,2,1,1,1,1,(void *)&pplfunc_tdistCDF    , "tdistCDF(x,nu)", "\\mathrm{tdistCDF}@<@1,@2@>", "tdistCDF(x,nu) returns the cumulative probability density at x in a t-distribution with nu degrees of freedom");
    ppl_addSystemFunc(d2,"tdistCDFi"     ,2,2,1,1,1,1,(void *)&pplfunc_tdistCDFi   , "tdistCDFi(P,nu)", "\\mathrm{tdistCDFi}@<@1,@2@>", "tdistCDFi(P,nu) returns the point x at which the cumulative probability density in a t-distribution with nu degrees of freedom is P");

    // Random module
    ppl_dictAppendCpy(d  , "random", m=pplNewModule(frozen) , sizeof(v));
    d2 = (dict *)m->auxil;
    ppl_addSystemFunc(d2,"random"        ,0,0,1,1,1,1,(void *)&pplfunc_frandom     , "", "\\mathrm{random}@<@>", "random(x) returns a random number between 0 and 1");
    ppl_addSystemFunc(d2,"randombin"     ,2,2,1,1,1,1,(void *)&pplfunc_frandombin  , "", "\\mathrm{random\\_binomial}@<@1,@2@>", "random_binomial(p,n) returns a random sample from a binomial distribution with n independent trials and a success probability p");
    ppl_addSystemFunc(d2,"randomcs"      ,1,1,1,1,1,1,(void *)&pplfunc_frandomcs   , "", "\\mathrm{random\\_\\chi^2}@<@1@>", "random_chisq(nu) returns a random sample from a chi-squared distribution with nu degrees of freedom");
    ppl_addSystemFunc(d2,"randomg"       ,1,1,1,1,1,0,(void *)&pplfunc_frandomg    , "", "\\mathrm{random\\_gaussian}@<@1@>", "random_gaussian(sigma) returns a random sample from a Gaussian (normal) distribution of standard deviation sigma");
    ppl_addSystemFunc(d2,"randomln"      ,2,2,1,1,1,0,(void *)&pplfunc_frandomln   , "", "\\mathrm{random\\_lognormal}@<@1,@2@>", "random_lognormal(zeta,sigma) returns a random sample from the log normal distribution centred on zeta, and of width sigma");
    ppl_addSystemFunc(d2,"randomp"       ,1,1,1,1,1,1,(void *)&pplfunc_frandomp    , "", "\\mathrm{random\\_poisson}@<@1@>", "random_poisson(n) returns a random integer from a Poisson distribution with mean n");
    ppl_addSystemFunc(d2,"randomt"       ,1,1,1,1,1,1,(void *)&pplfunc_frandomt    , "", "\\mathrm{random\\_tdist}@<@1@>", "random_tdist(nu) returns a random sample from a t-distribution with nu degrees of freedom");

   }

  return;
 }

