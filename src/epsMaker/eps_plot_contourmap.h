// eps_plot_contourmap.h
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

#ifndef _PPL_EPS_PLOT_CONTOURMAP_H
#define _PPL_EPS_PLOT_CONTOURMAP_H 1

#include "settings/settings.h"

void eps_plot_contourmap_YieldText(EPSComm *x, dataTable *data, pplset_graph *sg, canvas_plotdesc *pd);
int  eps_plot_contourmap(EPSComm *x, dataTable *data, unsigned char ThreeDim, int xn, int yn, int zn, pplset_graph *sg, canvas_plotdesc *pd, int pdn, double origin_x, double origin_y, double width, double height, double zdepth);

#endif

