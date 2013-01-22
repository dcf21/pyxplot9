// eps_plot_filledregion.h
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

#ifndef _PPL_EPS_PLOT_FILLEDREGION_H
#define _PPL_EPS_PLOT_FILLEDREGION_H 1

#include "epsMaker/eps_comm.h"
#include "coreUtils/list.h"
#include "settings/settings.h"

#define INCOMING 0
#define OUTGOING 1

typedef struct FilledRegionAxisCrossing {
 double x, y, x2, y2;
 double AxisPos;
 listItem *point;
 unsigned char AxisFace, sense, singleton, used;
 int id, twin;
 } FilledRegionAxisCrossing;

typedef struct FilledRegionPoint {
 double x, y, xa, ya, xap, yap;
 unsigned char inside;
 int FillSideFlip_prv, FillSideFlip_fwd;
 } FilledRegionPoint;

typedef struct FilledRegionHandle {
 EPSComm *x;
 pplset_graph *sg;
 pplset_axis *xa, *ya, *za;
 int xrn, yrn, zrn;
 unsigned char ThreeDim;
 double origin_x, origin_y, width, height, zdepth;
 list *points;
 int Naxiscrossings;
 double lastx, lasty, lastxap, lastyap;
 unsigned char first, EverInside;
 } FilledRegionHandle;

FilledRegionHandle *FilledRegion_Init (EPSComm *x, pplset_axis *xa, pplset_axis *ya, pplset_axis *za, int xrn, int yrn, int zrn, pplset_graph *sg, unsigned char ThreeDim, double origin_x, double origin_y, double width, double height, double zdepth);
void FilledRegion_Point(EPSComm *X, FilledRegionHandle *fr, double x, double y);
void FilledRegion_Finish(EPSComm *X, FilledRegionHandle *fr, int linetype, double linewidth, unsigned char StrokeOutline);

#endif

