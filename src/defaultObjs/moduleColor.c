// moduleColor.c
//
// The code in this file is part of PyXPlot
// <http://www.pyxplot.org.uk>
//
// Copyright (C) 2006-2012 Dominic Ford <coders@pyxplot.org.uk>
//               2008-2012 Ross Church
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

// Parts of this source file are modified from public domain code by John
// Walker, including corrections by Andrew J. S. Hamilton.

// See <http://www.fourmilab.ch/documents/specrend/>
// Source code at <http://www.fourmilab.ch/documents/specrend/specrend.c>

// Referenced from <http://stackoverflow.com/questions/1472514/convert-light-frequency-to-rgb>

// John Walker's homepage is at <http://www.fourmilab.ch/>

#include <stdlib.h>
#include <stdio.h>
#include <limits.h>
#include <math.h>
#include <string.h>

#include <gsl/gsl_const_mksa.h>
#include <gsl/gsl_math.h>

#include "expressions/expCompile.h"
#include "expressions/fnCall.h"
#include "settings/settings.h"
#include "stringTools/asciidouble.h"
#include "userspace/context.h"
#include "userspace/garbageCollector.h"
#include "userspace/pplObj_fns.h"
#include "userspace/pplObjFunc.h"
#include "userspace/unitsArithmetic.h"

#include "defaultObjs/moduleColor.h"
#include "defaultObjs/defaultFuncs.h"
#include "defaultObjs/defaultFuncsMacros.h"

#define GAMMA_FUDGE 0.45

struct colourSystem
 {
  char *name;             // Colour system name
  double xRed, yRed,      // Red x, y
         xGreen, yGreen,  // Green x, y
         xBlue, yBlue,    // Blue x, y
         xWhite, yWhite,  // White point x, y
         gamma;           // Gamma correction for system
 };

// White point chromaticities
#define IlluminantC     0.3101, 0.3162       // For NTSC television
#define IlluminantD65   0.3127, 0.3291       // For EBU and SMPTE
#define IlluminantE  0.33333333, 0.33333333  // CIE equal-energy illuminant

#define GAMMA_REC709 0  // Rec. 709

static struct colourSystem
                  // Name                  xRed    yRed    xGreen  yGreen  xBlue  yBlue    White point        Gamma
//  NTSCsystem  =  { "NTSC",               0.67,   0.33,   0.21,   0.71,   0.14,   0.08,   IlluminantC,    GAMMA_REC709 },
//  EBUsystem   =  { "EBU (PAL/SECAM)",    0.64,   0.33,   0.29,   0.60,   0.15,   0.06,   IlluminantD65,  GAMMA_REC709 },
    SMPTEsystem =  { "SMPTE",              0.630,  0.340,  0.310,  0.595,  0.155,  0.070,  IlluminantD65,  GAMMA_REC709 };
//  HDTVsystem  =  { "HDTV",               0.670,  0.330,  0.210,  0.710,  0.150,  0.060,  IlluminantD65,  GAMMA_REC709 },
//  CIEsystem   =  { "CIE",                0.7355, 0.2645, 0.2658, 0.7243, 0.1669, 0.0085, IlluminantE,    GAMMA_REC709 },
//  Rec709system = { "CIE REC 709",        0.64,   0.33,   0.30,   0.60,   0.15,   0.06,   IlluminantD65,  GAMMA_REC709 };

#define CIE_WLEN_MIN      380
#define CIE_WLEN_STEPSIZE   5
#define N_CIE_STEPS        81

static double cie_colour_match[N_CIE_STEPS][3] =
 {
    {0.0014,0.0000,0.0065}, {0.0022,0.0001,0.0105}, {0.0042,0.0001,0.0201},
    {0.0076,0.0002,0.0362}, {0.0143,0.0004,0.0679}, {0.0232,0.0006,0.1102},
    {0.0435,0.0012,0.2074}, {0.0776,0.0022,0.3713}, {0.1344,0.0040,0.6456},
    {0.2148,0.0073,1.0391}, {0.2839,0.0116,1.3856}, {0.3285,0.0168,1.6230},
    {0.3483,0.0230,1.7471}, {0.3481,0.0298,1.7826}, {0.3362,0.0380,1.7721},
    {0.3187,0.0480,1.7441}, {0.2908,0.0600,1.6692}, {0.2511,0.0739,1.5281},
    {0.1954,0.0910,1.2876}, {0.1421,0.1126,1.0419}, {0.0956,0.1390,0.8130},
    {0.0580,0.1693,0.6162}, {0.0320,0.2080,0.4652}, {0.0147,0.2586,0.3533},
    {0.0049,0.3230,0.2720}, {0.0024,0.4073,0.2123}, {0.0093,0.5030,0.1582},
    {0.0291,0.6082,0.1117}, {0.0633,0.7100,0.0782}, {0.1096,0.7932,0.0573},
    {0.1655,0.8620,0.0422}, {0.2257,0.9149,0.0298}, {0.2904,0.9540,0.0203},
    {0.3597,0.9803,0.0134}, {0.4334,0.9950,0.0087}, {0.5121,1.0000,0.0057},
    {0.5945,0.9950,0.0039}, {0.6784,0.9786,0.0027}, {0.7621,0.9520,0.0021},
    {0.8425,0.9154,0.0018}, {0.9163,0.8700,0.0017}, {0.9786,0.8163,0.0014},
    {1.0263,0.7570,0.0011}, {1.0567,0.6949,0.0010}, {1.0622,0.6310,0.0008},
    {1.0456,0.5668,0.0006}, {1.0026,0.5030,0.0003}, {0.9384,0.4412,0.0002},
    {0.8544,0.3810,0.0002}, {0.7514,0.3210,0.0001}, {0.6424,0.2650,0.0000},
    {0.5419,0.2170,0.0000}, {0.4479,0.1750,0.0000}, {0.3608,0.1382,0.0000},
    {0.2835,0.1070,0.0000}, {0.2187,0.0816,0.0000}, {0.1649,0.0610,0.0000},
    {0.1212,0.0446,0.0000}, {0.0874,0.0320,0.0000}, {0.0636,0.0232,0.0000},
    {0.0468,0.0170,0.0000}, {0.0329,0.0119,0.0000}, {0.0227,0.0082,0.0000},
    {0.0158,0.0057,0.0000}, {0.0114,0.0041,0.0000}, {0.0081,0.0029,0.0000},
    {0.0058,0.0021,0.0000}, {0.0041,0.0015,0.0000}, {0.0029,0.0010,0.0000},
    {0.0020,0.0007,0.0000}, {0.0014,0.0005,0.0000}, {0.0010,0.0004,0.0000},
    {0.0007,0.0002,0.0000}, {0.0005,0.0002,0.0000}, {0.0003,0.0001,0.0000},
    {0.0002,0.0001,0.0000}, {0.0002,0.0001,0.0000}, {0.0001,0.0000,0.0000},
    {0.0001,0.0000,0.0000}, {0.0001,0.0000,0.0000}, {0.0000,0.0000,0.0000}
 };


static void xyz_to_rgb(struct colourSystem *cs, double xc, double yc, double zc, double *r, double *g, double *b)
 {
  double xr, yr, zr, xg, yg, zg, xb, yb, zb;
  double xw, yw, zw;
  double rx, ry, rz, gx, gy, gz, bx, by, bz;
  double rw, gw, bw;

  xr = cs->xRed;    yr = cs->yRed;    zr = 1 - (xr + yr);
  xg = cs->xGreen;  yg = cs->yGreen;  zg = 1 - (xg + yg);
  xb = cs->xBlue;   yb = cs->yBlue;   zb = 1 - (xb + yb);

  xw = cs->xWhite;  yw = cs->yWhite;  zw = 1 - (xw + yw);

  // xyz -> rgb matrix, before scaling to white
  rx = (yg * zb) - (yb * zg);  ry = (xb * zg) - (xg * zb);  rz = (xg * yb) - (xb * yg);
  gx = (yb * zr) - (yr * zb);  gy = (xr * zb) - (xb * zr);  gz = (xb * yr) - (xr * yb);
  bx = (yr * zg) - (yg * zr);  by = (xg * zr) - (xr * zg);  bz = (xr * yg) - (xg * yr);

  // White scaling factors. Dividing by yw scales the white luminance to unity, as conventional.
  rw = ((rx * xw) + (ry * yw) + (rz * zw)) / yw;
  gw = ((gx * xw) + (gy * yw) + (gz * zw)) / yw;
  bw = ((bx * xw) + (by * yw) + (bz * zw)) / yw;

  // xyz -> rgb matrix, correctly scaled to white
  rx = rx / rw;  ry = ry / rw;  rz = rz / rw;
  gx = gx / gw;  gy = gy / gw;  gz = gz / gw;
  bx = bx / bw;  by = by / bw;  bz = bz / bw;

  // rgb of the desired point
  *r = (rx * xc) + (ry * yc) + (rz * zc);
  *g = (gx * xc) + (gy * yc) + (gz * zc);
  *b = (bx * xc) + (by * yc) + (bz * zc);
  return;
}

static int inside_gamut(double r, double g, double b)
 {
  return (r >= 0) && (g >= 0) && (b >= 0);
 }

static int constrain_rgb(double *r, double *g, double *b)
 {
  double w;

  // Amount of white needed is w = - min(0, *r, *g, *b)
  w = (0 < *r) ? 0 : *r;
  w = (w < *g) ? w : *g;
  w = (w < *b) ? w : *b;
  w = -w;

  // Add just enough white to make r, g, b all positive
  if (w>0)
   {
    double n = (*r+*g+*b) / (*r+*g+*b+w);
    if (!gsl_finite(n)) n=1;
    *r=(*r+w)*n;
    *g=(*g+w)*n;
    *b=(*b+w)*n;
    return 1; // Colour modified to fit RGB gamut
   }
  return 0; // Colour within RGB gamut
 }

void pplfunc_colWavelen (ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText)
 {
  char  *FunctionDescription = "wavelength(lambda,norm)";
  double p0,wi,wj,x,y,z,r,g,b,norm;
  int    i,j;

  CHECK_DIMLESS_OR_HAS_UNIT(in[0], "first", "a wavelength", UNIT_LENGTH, 1);
  if (!in[1].dimensionless) { *status=1; *errType=ERR_UNIT; sprintf(errText, "The %s function requires a dimensionless renormalisation constant as its second argument. Supplied value has dimensions of <%s>.", FunctionDescription, ppl_printUnit(c, &in[1], NULL, NULL, 0, 1, 0)); return; }
  pplObjColor(&OUTPUT,0,SW_COLSPACE_RGB,0,0,0,0);

  norm = in[1].real;
  p0   = ((in[0].real * 1e9) - CIE_WLEN_MIN) / CIE_WLEN_STEPSIZE;
  if ((!gsl_finite(p0))||(p0<0)||(p0>=N_CIE_STEPS-1)) { pplObjColor(&OUTPUT,0,SW_COLSPACE_RGB,0,0,0,0); return; } // Black
  i    = (int)floor(p0); if (i<0) i=0; if (i>N_CIE_STEPS-2) i=N_CIE_STEPS-2;
  j    = i+1;
  wi   = (j-p0); if (wi<0) wi=0; if (wi>1) wi=1;
  wj   = 1-wi;

  x    = cie_colour_match[i][0]*wi + cie_colour_match[j][0]*wj;
  y    = cie_colour_match[i][1]*wi + cie_colour_match[j][1]*wj;
  z    = cie_colour_match[i][2]*wi + cie_colour_match[j][2]*wj;

  // Fudge to brightness of spectrum
  {
   double xyz   = gsl_hypot3(x,y,z); if (xyz>1) xyz=1; if (xyz<0) xyz=0;
   double XYZ   = pow(xyz, GAMMA_FUDGE);
   double gamma = XYZ / xyz;
          x    *= gamma;
          y    *= gamma;
          z    *= gamma;
  }

  xyz_to_rgb(&SMPTEsystem, x, y, z, &r, &g, &b);
  if (!inside_gamut(r,g,b)) constrain_rgb(&r,&g,&b);
  r*=fabs(norm); if (r<0) r=0; if (r>1) r=1;
  g*=fabs(norm); if (g<0) g=0; if (g>1) g=1;
  b*=fabs(norm); if (b<0) b=0; if (b>1) b=1;
  pplObjColor(&OUTPUT,0,SW_COLSPACE_RGB,r,g,b,0);
  return;
 }

#define STACK_POP \
   { \
    c->stackPtr--; \
    ppl_garbageObject(&c->stack[c->stackPtr]); \
    if (c->stack[c->stackPtr].refCount != 0) { *status=1; *errType=ERR_INTERNAL; strcpy(errText,"Stack forward reference detected."); return; } \
   }

void pplfunc_colSpectrum(ppl_context *c, pplObj *in, int nArgs, int *status, int *errType, char *errText)
 {
  char      *FunctionDescription = "spectrum(spec,norm)";
  pplFunc   *fi;
  const int  stkLevelOld = c->stackPtr;
  double     x=0,y=0,z=0,r,g,b,norm;
  int        i;

  if (in[0].objType != PPLOBJ_FUNC) { *status=1; *errType=ERR_TYPE; sprintf(errText, "The %s function requires a function object as its first argument. Supplied object is of type <%s>.", FunctionDescription, pplObjTypeNames[in[0].objType]); return; }
  fi = (pplFunc *)in[0].auxil;
  if ((fi==NULL)||(fi->functionType==PPL_FUNC_MAGIC)) { *status=1; *errType=ERR_TYPE; sprintf(errText, "The %s function requires a function object as its first argument. Integration and differentiation operators are not suitable functions.", FunctionDescription); return; }
  CHECK_DIMLESS_OR_HAS_UNIT(in[0], "first", "a wavelength", UNIT_LENGTH, 1);
  if (!in[1].dimensionless) { *status=1; *errType=ERR_UNIT; sprintf(errText, "The %s function requires a dimensionless renormalisation constant as its second argument. Supplied value has dimensions of <%s>.", FunctionDescription, ppl_printUnit(c, &in[1], NULL, NULL, 0, 1, 0)); return; }
  pplObjColor(&OUTPUT,0,SW_COLSPACE_RGB,0,0,0,0);

  norm = in[1].real;

  // Check there's enough space on the stack
  if (c->stackPtr > ALGEBRA_STACK-4) { *status=1; *errType=ERR_MEMORY; sprintf(errText, "stack overflow in the colors.spectrum function."); return; }

  for (i=0; i<N_CIE_STEPS; i++)
   {
    double   wlen = (CIE_WLEN_MIN + i*CIE_WLEN_STEPSIZE) * 1e-9;
    double   w;
    pplExpr  dummy;

    // Dummy expression object with dummy line number information
    dummy.srcLineN = 0;
    dummy.srcId    = 0;
    dummy.srcFname = "<dummy>";
    dummy.ascii    = NULL;

    // Push function object
    pplObjCpy(&c->stack[c->stackPtr], &in[0], 1, 0, 1);
    c->stack[c->stackPtr].refCount=1;
    c->stackPtr++;

    // Push wavelength
    pplObjNum(&c->stack[c->stackPtr], 0 , wlen, 0);
    c->stack[c->stackPtr].dimensionless=0;
    c->stack[c->stackPtr].exponent[UNIT_LENGTH]=1;
    c->stack[c->stackPtr].refCount=1;
    c->stackPtr++;

    // Call function
    c->errStat.errMsgExpr[0]='\0';
    ppl_fnCall(c, &dummy, 0, 1, 1, 1);

    // Propagate error if function failed
    if (c->errStat.status)
     {
      *status=1;
      *errType=ERR_TYPE;
      sprintf(errText, "Error inside spectrum function supplied to the %s function: %s", FunctionDescription, c->errStat.errMsgExpr);
      return;
     }

    // Return error if function didn't return a number
    if (c->stack[c->stackPtr-1].objType!=PPLOBJ_NUM) { *status=1; *errType=ERR_TYPE; sprintf(errText, "The %s function requires a spectrum function that returns a number. Supplied function returned an object of type <%s>.", FunctionDescription, pplObjTypeNames[c->stack[c->stackPtr-1].objType]); return; }

    // Get number back and clean stack
    w = fabs(c->stack[c->stackPtr-1].real) / N_CIE_STEPS;
    if (!gsl_finite(w)) w=0;
    while (c->stackPtr>stkLevelOld) { STACK_POP; }

    // Add color to accumulators
    x += cie_colour_match[i][0]*w;
    y += cie_colour_match[i][1]*w;
    z += cie_colour_match[i][2]*w;
   }

  // Fudge to brightness of spectrum
  {
   double xyz   = gsl_hypot3(x,y,z); if (xyz>1) xyz=1; if (xyz<0) xyz=0;
   double XYZ   = pow(xyz, GAMMA_FUDGE);
   double gamma = XYZ / xyz;
          x    *= gamma;
          y    *= gamma;
          z    *= gamma;
  }

  xyz_to_rgb(&SMPTEsystem, x, y, z, &r, &g, &b);
  if (!inside_gamut(r,g,b)) constrain_rgb(&r,&g,&b);
  r*=fabs(norm); if (r<0) r=0; if (r>1) r=1;
  g*=fabs(norm); if (g<0) g=0; if (g>1) g=1;
  b*=fabs(norm); if (b<0) b=0; if (b>1) b=1;
  pplObjColor(&OUTPUT,0,SW_COLSPACE_RGB,r,g,b,0);
  return;
 }

