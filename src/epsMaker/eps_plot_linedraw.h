// eps_plot_linedraw.h
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

#ifndef _PPL_EPS_PLOT_LINEDRAW_H
#define _PPL_EPS_PLOT_LINEDRAW_H 1

#include "epsMaker/eps_comm.h"
#include "settings/settings.h"

#define FACE_TOP    1
#define FACE_LEFT   2
#define FACE_BOTTOM 3
#define FACE_RIGHT  4

typedef struct LineDrawHandle {
 EPSComm *x;
 pplset_graph *sg;
 pplset_axis *xa, *ya, *za;
 int xrn, yrn, zrn;
 unsigned char ThreeDim;
 double origin_x, origin_y, width, height, zdepth;
 unsigned char x0set, x1set;
 double x0, y0;
 double x1  , y1  , z1  ;
 double xpo1, ypo1, zpo1;
 double xap1, yap1, zap1;
 } LineDrawHandle;

void LineDraw_FindCrossingPoints(EPSComm *x, double x1, double y1, double z1, double xap1, double yap1, double zap1, double x2, double y2, double z2, double xap2, double yap2, double zap2, int *Inside1, int *Inside2, double *cx1, double *cy1, double *cz1, double *cx2, double *cy2, double *cz2, unsigned char *face1, double *AxisPos1, unsigned char *face2, double *AxisPos2, int *NCrossings);

LineDrawHandle *LineDraw_Init (EPSComm *x, pplset_axis *xa, pplset_axis *ya, pplset_axis *za, int xrn, int yrn, int zrn, pplset_graph *sg, unsigned char ThreeDim, double origin_x, double origin_y, double width, double height, double zdepth);
void LineDraw_Point(EPSComm *X, LineDrawHandle *ld, double x, double y, double z, double x_offset, double y_offset, double z_offset, double x_perpoffset, double y_perpoffset, double z_perpoffset, int linetype, double linewidth, char *colstr);
void LineDraw_PenUp(EPSComm *x, LineDrawHandle *ld);

#endif

