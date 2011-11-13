// dcfmath.h
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

#ifndef _DCFMATH_H
#define _DCFMATH_H 1

#ifndef _DCFMATH_C
extern double ppl_machineEpsilon;
#endif

void   ppl_makeMachineEpsilon();
double ppl_max      (double x, double y);
double ppl_min      (double x, double y);
int    ppl_sgn      (double x);
void   ppl_linRaster(double *out, double min, double max, int Nsteps);
void   ppl_logRaster(double *out, double min, double max, int Nsteps);
double ppl_degs     (double rad);
double ppl_rads     (double degrees);

#endif

