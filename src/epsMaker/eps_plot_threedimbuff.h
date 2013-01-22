// eps_plot_threedimbuff.h
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

#ifndef _PPL_EPS_PLOT_THREEDIMBUFF_H
#define _PPL_EPS_PLOT_THREEDIMBUFF_H 1

#include "epsMaker/eps_comm.h"

typedef struct ThreeDimBufferItem {
 unsigned char FlagLineSegment, FirstLineSegment;
 int           linetype;
 double        linewidth, offset, pointsize;
 char         *colstr, *psfrag;
 double        depth, LineLength, x0,y0,x1,y1,x2,y2;
 long          LineSegmentID;
 } ThreeDimBufferItem;

#ifndef _PPL_EPS_PLOT_THREEDIMBUFF_C
extern unsigned char ThreeDimBuffer_ACTIVE;
#endif

void ThreeDimBuffer_Reset(EPSComm *x);
int  ThreeDimBuffer_Activate(EPSComm *x);
int  ThreeDimBuffer_Deactivate(EPSComm *x);
int  ThreeDimBuffer_writeps(EPSComm *x, double z, int linetype, double linewidth, double offset, double pointsize, char *colstr, char *psfrag);
int  ThreeDimBuffer_linesegment(EPSComm *x, double z, int linetype, double linewidth, char *colstr, double x0, double y0, double x1, double y1, double x2, double y2, unsigned char FirstSegment, unsigned char broken, double LengthOffset);
int  ThreeDimBuffer_linepenup(EPSComm *x);

#endif

