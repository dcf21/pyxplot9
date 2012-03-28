// eps_style.h
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

#ifndef _PPL_EPS_STYLE_H
#define _PPL_EPS_STYLE_H 1

#define N_POINTTYPES 44
#define N_STARTYPES   9

char *eps_LineType(int lt, double lw, double offset);

#ifndef _PPL_EPS_STYLE_C
extern char   *eps_PointTypes[];
extern double  eps_PointSize [];
extern char   *eps_StarTypes [];
extern double  eps_StarSize  [];
extern char   *eps_StarCore;
#endif

#endif

