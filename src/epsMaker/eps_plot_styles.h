// eps_plot_styles.h
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

#ifndef _PPL_EPS_PLOT_STYLES_H
#define _PPL_EPS_PLOT_STYLES_H 1

#include "canvasItems.h"
#include "datafile.h"
#include "epsMaker/eps_comm.h"
#include "settings/settings.h"
#include "settings/withWords.h"

void eps_withwords_default(EPSComm *x, withWords *output, pplset_graph *sg, unsigned char functions, int Ccounter, int LTcounter, int PTcounter, unsigned char colour);
void eps_withwords_default_counterinc(EPSComm *x, int *Ccounter, int *LTcounter, int *PTcounter, unsigned char colour, withWords *ww_final, pplset_graph *sg);
int  eps_plot_styles_NDataColumns(pplerr_context *ec, int style, unsigned char ThreeDim);
int  eps_plot_styles_UpdateUsage(EPSComm *x, dataTable *data, int style, unsigned char ThreeDim, pplset_axis *a1, pplset_axis *a2, pplset_axis *a3, pplset_graph *sg, int xyz1, int xyz2, int xyz3, int n1, int n2, int n3, int id);
int  eps_plot_dataset(EPSComm *x, dataTable *data, int style, unsigned char ThreeDim, pplset_axis *a1, pplset_axis *a2, pplset_axis *a3, int xn, int yn, int zn, pplset_graph *sg, canvas_plotdesc *pd, double origin_x, double origin_y, double width, double height, double zdepth);
void eps_plot_LegendIcon(EPSComm *x, int i, canvas_plotdesc *pd, double xpos, double ypos, double scale, pplset_axis *a1, pplset_axis *a2, pplset_axis *a3, int xn, int yn, int zn);

#endif

