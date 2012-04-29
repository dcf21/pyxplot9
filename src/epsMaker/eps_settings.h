// eps_settings.h
//
// The code in this file is part of PyXPlot
// <http://www.pyxplot.org.uk>
//
// Copyright (C) 2006-2012 Dominic Ford <coders@pyxplot.org.uk>
//               2008-2012 Ross Church
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

#ifndef _PPL_EPS_SETTINGS_H
#define _PPL_EPS_SETTINGS_H 1

// Baseline values of settings used by the eps generation routines
#define EPS_BASE_DEFAULT_LINEWIDTH ( 0.566929       ) /* 0.2mm in TeX points */
#define EPS_BASE_DEFAULT_PS        ( 3.0            )
#define EPS_BASE_ARROW_ANGLE       ( 45.0 *M_PI/180 )
#define EPS_BASE_ARROW_CONSTRICT   ( 0.2            )
#define EPS_BASE_ARROW_HEADSIZE    ( 6.0            )
#define EPS_BASE_AXES_LINEWIDTH    ( 1.0            )
#define EPS_BASE_AXES_MAJTICKLEN   ( 0.0012         )
#define EPS_BASE_AXES_MINTICKLEN   ( 0.000848528137 ) /* 0.0012 divided by sqrt(2) */
#define EPS_BASE_AXES_SEPARATION   ( 0.008          )
#define EPS_BASE_AXES_TEXTGAP      ( 0.003          )
#define EPS_BASE_COLORSCALE_MARG   ( 3e-3           )
#define EPS_BASE_COLORSCALE_WIDTH  ( 4e-3           )
#define EPS_BASE_GRID_MAJLINEWIDTH ( 1.0            )
#define EPS_BASE_GRID_MINLINEWIDTH ( 0.5            )

// Copies of the values actually used by the eps generation routines, which may have been scaled relative to their baselines
#ifndef _PPL_EPS_SETTINGS_C
extern double M_TO_PS;
extern double EPS_DEFAULT_LINEWIDTH;
extern double EPS_DEFAULT_PS;
extern double EPS_ARROW_ANGLE;
extern double EPS_ARROW_CONSTRICT;
extern double EPS_ARROW_HEADSIZE;
extern double EPS_AXES_LINEWIDTH;
extern double EPS_AXES_MAJTICKLEN;
extern double EPS_AXES_MINTICKLEN;
extern double EPS_AXES_SEPARATION;
extern double EPS_AXES_TEXTGAP;
extern double EPS_COLORSCALE_MARGIN;
extern double EPS_COLORSCALE_WIDTH;
extern double EPS_GRID_MAJLINEWIDTH;
extern double EPS_GRID_MINLINEWIDTH;
#endif

#endif

