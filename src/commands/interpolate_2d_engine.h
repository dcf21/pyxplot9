// interpolate_2d_engine.h
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

#ifndef _INTERPOLATE2D_H
#define _INTERPOLATE2D_H 1

#include "datafile.h"
#include "settings/settings.h"
#include "userspace/context.h"

void ppl_interp2d_eval(ppl_context *c, double *output, const pplset_graph *sg, const double *in, const long InSize, const int ColNum, const int NCols, const double x, const double y);
void ppl_interp2d_grid(ppl_context *c, dataTable **output, const pplset_graph *sg, dataTable *in, pplset_axis *axis_x, pplset_axis *axis_y, unsigned char SampleToEdge, int *XSizeOut, int *YSizeOut);

#endif

