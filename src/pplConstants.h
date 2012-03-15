// pplConstants.h
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

#ifndef _PPLCONSTANTS_H
#define _PPLCONSTANTS_H 1

// Context buffer lengths

#define ALGEBRA_MAXLEN    65536 /* Size of buffer for tokenising input expressions */
#define ALGEBRA_STACK      1024 /* Stack size */
#define CONTEXT_DEPTH       256 /* Maximum number of stacked local namespaces */

#define DUMMYVAR_MAXLEN      16 /* Maximum number of characters in a dummy variable name for integration / differentiation */

#define MAX_RECURSION_DEPTH 128 /* The maximum recursion depth */

// Plot related structure sizes

#define MULTIPLOT_MAXINDEX 32768
#define PALETTE_LENGTH       512
#define MAX_PLOTSTYLES       128 // The maximum number of plot styles (e.g. plot sin(x) with style 23) which are be defined. Similar to 'with linestyle 23' in gnuplot
#define MAX_AXES             128
#define MAX_CONTOURS         128 // Maximum number of contours in 'set contour'
#define USING_ITEMS_MAX       32
#define FUNC_MAXARGS        1024

// Axis linear interpolations

#define AXISLINEARINTERPOLATION_NPOINTS 2045

// Command-specific options

#define EQNSOLVE_MAXDIMS      16 // Maximum number of via variables when equation solving / fitting


#endif

