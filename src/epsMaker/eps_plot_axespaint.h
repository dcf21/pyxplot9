// eps_plot_axespaint.h
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

#ifndef _PPL_EPS_PLOT_AXESPAINT_H
#define _PPL_EPS_PLOT_AXESPAINT_H 1

#include "settings/settings.h"
#include "settings/withWords.h"
#include "epsMaker/eps_comm.h"

void eps_plot_axispaint(EPSComm *x, withWords *ww, pplset_axis *a, const int xyz, const double CP2, const unsigned char Lr, const double x1, const double y1, const double *z1, const double x2, const double y2, const double *z2, const double theta_a, const double theta_b, double *OutputWidth, const unsigned char PrintLabels);
void eps_plot_axespaint(EPSComm *x, double origin_x, double origin_y, double width, double height, double zdepth, int pass);

#endif

