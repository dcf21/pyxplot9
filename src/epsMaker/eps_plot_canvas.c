// eps_plot_canvas.c
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

// Provides routines for mapping between coordinate positions on axes and
// positions on a postscript page. Outputs xpos and ypos are measured in
// postscript points.

#define _PPL_EPS_PLOT_CANVAS_C 1

#include <stdlib.h>
#include <stdio.h>
#include <math.h>

#include <gsl/gsl_math.h>

#include "settings/settings.h"
#include "settings/settingTypes.h"

// Where along this axis, in the range 0 (left) to 1 (right) should the value
// xin go? xrn = Region Number for interpolated axes which do not have
// monotonically increasing ordinate values.
double eps_plot_axis_GetPosition(double xin, pplset_axis *xa, int xrn, unsigned char AllowOffBounds)
 {
  int imin, imax, i;
  if (xa==NULL) return xin;
  if (xa->AxisLinearInterpolation != NULL) // Axis is linearly interpolated
   {
    imin = xa->AxisTurnings[xrn  ];
    imax = xa->AxisTurnings[xrn+1];
    for (i=imin; i<imax; i++)
     {
      if (   ((xa->AxisLinearInterpolation[i] < xin) && (xa->AxisLinearInterpolation[i+1] >= xin))
          || ((xa->AxisLinearInterpolation[i] > xin) && (xa->AxisLinearInterpolation[i+1] <= xin)) )
       return (i + (xin-xa->AxisLinearInterpolation[i])/(xa->AxisLinearInterpolation[i+1]-xa->AxisLinearInterpolation[i])) / (AXISLINEARINTERPOLATION_NPOINTS-1);
     }
    return GSL_NAN;
   }
  if (!AllowOffBounds)
   {
    if (xa->MaxFinal > xa->MinFinal)
     { if ((xin<xa->MinFinal) || (xin>xa->MaxFinal)) return GSL_NAN; }
    else
     { if ((xin>xa->MinFinal) || (xin<xa->MaxFinal)) return GSL_NAN; }
   }
  if ((xa->LogFinal==SW_BOOL_TRUE) && (xin <= 0)) return GSL_NAN;
  if (xa->LogFinal!=SW_BOOL_TRUE) return (xin - xa->MinFinal) / (xa->MaxFinal - xa->MinFinal); // Either linear...
  else                            return (log(xin)-log(xa->MinFinal)) / (log(xa->MaxFinal)-log(xa->MinFinal)); // ... or logarithmic
 }

// What is the value of this axis at point xin, in the range 0 (left) to 1 (right)?
double eps_plot_axis_InvGetPosition(double xin, pplset_axis *xa)
 {
  if (xa->AxisLinearInterpolation != NULL) // Axis is linearly interpolated
   {
    int    i = floor(xin * (AXISLINEARINTERPOLATION_NPOINTS-1));
    double x = xin * (AXISLINEARINTERPOLATION_NPOINTS-1) - i;
    if (i>=AXISLINEARINTERPOLATION_NPOINTS-1) return xa->AxisLinearInterpolation[AXISLINEARINTERPOLATION_NPOINTS-1];
    if (i<                                 0) return xa->AxisLinearInterpolation[0];
    return xa->AxisLinearInterpolation[i]*(1-x) + xa->AxisLinearInterpolation[i+1]*x;
   }
  if (xa->LogFinal!=SW_BOOL_TRUE) return xa->MinFinal + xin * (xa->MaxFinal - xa->MinFinal); // Either linear...
  else                            return xa->MinFinal * pow(xa->MaxFinal / xa->MinFinal , xin); // ... or logarithmic
 }

// Query whether the value xin occurs within the range of this axis
int eps_plot_axis_InRange(pplset_axis *xa, double xin)
 {
  int xrn, swapI, xminset=0, xmaxset=0;
  double xmin=0, xmax=0, swapD;

  if (xa->AxisLinearInterpolation != NULL)
   {
    for (xrn=0; xrn<=xa->AxisValueTurnings; xrn++) if (gsl_finite(eps_plot_axis_GetPosition(xin,xa,xrn,0))) return 1;
    return 0;
   }

  if (xa->HardMinSet)     { xminset=1; xmin=xa->HardMin; }
  if (xa->HardMaxSet)     { xmaxset=1; xmax=xa->HardMax; }
  if (xa->HardAutoMinSet) { xminset=0; }
  if (xa->HardAutoMaxSet) { xmaxset=0; }

  if (xa->RangeReversed)  { swapI=xminset; xminset=xmaxset; xmaxset=swapI; swapD=xmin; xmin=xmax; xmax=swapD; }

  if (xminset && xmaxset) return (((xin>=xmin)&&(xin<=xmax))||((xin<=xmin)&&(xin>=xmax)));
  if (xminset           ) return (xin>xmin);
  if (xmaxset           ) return (xin<xmax);
  return 1; // Axis range is not fixed
 }

void eps_plot_ThreeDimProject(double xap, double yap, double zap, pplset_graph *sg, double origin_x, double origin_y, double width, double height, double zdepth, double *xpos, double *ypos, double *depth)
 {
  double x,y,z,x2,y2,z2,x3,y3,z3;

  x = width  * (xap - 0.5);
  y = height * (yap - 0.5);
  z = zdepth * (zap - 0.5);

  x2 = x*cos(sg->XYview.real) + y*sin(sg->XYview.real);
  y2 =-x*sin(sg->XYview.real) + y*cos(sg->XYview.real);
  z2 = z;

  x3 = x2;
  y3 = y2*cos(sg->YZview.real) - z2*sin(sg->YZview.real);
  z3 = y2*sin(sg->YZview.real) + z2*cos(sg->YZview.real);

  *xpos  = origin_x + x3;
  *ypos  = origin_y + z3;
  *depth =            y3;
  return;
 }

void eps_plot_GetPosition(double *xpos, double *ypos, double *depth, double *xap, double *yap, double *zap, double *theta_x, double *theta_y, double *theta_z, unsigned char ThreeDim, double xin, double yin, double zin, pplset_axis *xa, pplset_axis *ya, pplset_axis *za, int xrn, int yrn, int zrn, pplset_graph *sg, double origin_x, double origin_y, double width, double height, double zdepth, unsigned char AllowOffBounds)
 {
  // Convert (xin,yin,zin) to axis positions on the range of 0-1
  *xap = eps_plot_axis_GetPosition(xin, xa, xrn, 1);
  *yap = eps_plot_axis_GetPosition(yin, ya, yrn, 1);
  *zap = 0.5;
  if (ThreeDim) *zap = eps_plot_axis_GetPosition(zin, za, zrn, 1);

  if ( (!gsl_finite(*xap)) || (!gsl_finite(*yap)) || (!gsl_finite(*zap)) ) { *xpos = *ypos = GSL_NAN; return; }

  // Apply non-flat projections to (xap,yap,zap)
  if (sg->projection == SW_PROJ_GNOM)
   {
   }

  // Crop axis positions to range 0-1
  if ((!AllowOffBounds) && ((*xap<0.0)||(*xap>1.0)||(*yap<0.0)||(*yap>1.0)||(*zap<0.0)||(*zap>1.0))) { *xpos = *ypos = GSL_NAN; return; }

  // 3D plots
  if (ThreeDim)
   {
    eps_plot_ThreeDimProject(*xap,*yap,*zap,sg,origin_x,origin_y,width,height,zdepth,xpos,ypos,depth);
    if (theta_x != NULL) *theta_x = atan2( cos(sg->XYview.real) ,-sin(sg->XYview.real)*sin(sg->YZview.real) );
    if (theta_y != NULL) *theta_y = atan2( sin(sg->XYview.real) , cos(sg->XYview.real)*sin(sg->YZview.real) );
    if (theta_z != NULL) *theta_z = atan2( 0                    ,                      cos(sg->YZview.real) );

    if ((theta_x != NULL) && (!gsl_finite(*theta_x))) *theta_x=0.0;
    if ((theta_y != NULL) && (!gsl_finite(*theta_y))) *theta_y=0.0;
    if ((theta_z != NULL) && (!gsl_finite(*theta_z))) *theta_z=0.0;
   }
  else // 2D plots
   {
    *xpos = origin_x + width  * (*xap);
    *ypos = origin_y + height * (*yap);
    *depth = 0.0;

    if (theta_x != NULL) *theta_x = M_PI/2;
    if (theta_y != NULL) *theta_y = 0.0;
    if (theta_z != NULL) *theta_z = 0.0;
   }

  return;
 }

