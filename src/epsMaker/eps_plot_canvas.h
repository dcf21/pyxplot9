// eps_plot_canvas.h
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

#ifndef _PPL_EPS_PLOT_CANVAS_H
#define _PPL_EPS_PLOT_CANVAS_H 1

#include <stdlib.h>
#include <stdio.h>

#include "settings/settings.h"

double eps_plot_axis_GetPosition(double xin, pplset_axis *xa, int xrn, unsigned char AllowOffBounds);
double eps_plot_axis_InvGetPosition(double xin, pplset_axis *xa);
int eps_plot_axis_InRange(pplset_axis *xa, double xin);
void eps_plot_ThreeDimProject(double xap, double yap, double zap, pplset_graph *sg, double origin_x, double origin_y, double width, double height, double zdepth, double *xpos, double *ypos, double *depth);
void eps_plot_GetPosition(double *xpos, double *ypos, double *depth, double *xap, double *yap, double *zap, double *theta_x, double *theta_y, double *theta_z, unsigned char ThreeDim, double xin, double yin, double zin, pplset_axis *xa, pplset_axis *ya, pplset_axis *za, int xrn, int yrn, int zrn, pplset_graph *sg, double origin_x, double origin_y, double width, double height, double zdepth, unsigned char AllowOffBounds);

#endif

