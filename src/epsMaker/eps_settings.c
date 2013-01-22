// eps_settings.c
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

// This file contains various numerical constants which are used by the eps
// generation routines

#define _PPL_EPS_SETTINGS_C 1

#include <math.h>

#include <gsl/gsl_const_mksa.h>

#include "epsMaker/eps_settings.h"

// Constant to convert between millimetres and 72nds of an inch
double M_TO_PS = 1.0 / (GSL_CONST_MKSA_INCH / 72.0);

double EPS_DEFAULT_LINEWIDTH = 1.0 * EPS_BASE_DEFAULT_LINEWIDTH;
double EPS_DEFAULT_PS        = 1.0 * EPS_BASE_DEFAULT_PS;
double EPS_ARROW_ANGLE       = 1.0 * EPS_BASE_ARROW_ANGLE;
double EPS_ARROW_CONSTRICT   = 1.0 * EPS_BASE_ARROW_CONSTRICT;
double EPS_ARROW_HEADSIZE    = 1.0 * EPS_BASE_ARROW_HEADSIZE;
double EPS_AXES_LINEWIDTH    = 1.0 * EPS_BASE_AXES_LINEWIDTH;
double EPS_AXES_MAJTICKLEN   = 1.0 * EPS_BASE_AXES_MAJTICKLEN;
double EPS_AXES_MINTICKLEN   = 1.0 * EPS_BASE_AXES_MINTICKLEN;
double EPS_AXES_SEPARATION   = 1.0 * EPS_BASE_AXES_SEPARATION;
double EPS_AXES_TEXTGAP      = 1.0 * EPS_BASE_AXES_TEXTGAP;
double EPS_COLORSCALE_MARGIN = 1.0 * EPS_BASE_COLORSCALE_MARG;
double EPS_COLORSCALE_WIDTH  = 1.0 * EPS_BASE_COLORSCALE_WIDTH;
double EPS_GRID_MAJLINEWIDTH = 1.0 * EPS_BASE_GRID_MAJLINEWIDTH;
double EPS_GRID_MINLINEWIDTH = 1.0 * EPS_BASE_GRID_MINLINEWIDTH;

