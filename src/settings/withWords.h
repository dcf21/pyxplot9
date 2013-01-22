// withWords.h
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

#ifndef _WITHWORDS_H
#define _WITHWORDS_H 1

#include "expressions/expCompile.h"

typedef struct withWords {
 int            color, fillcolor, linespoints, linetype, pointtype, style; // Core style settings which can be placed after the 'with' modifier
 double         linewidth, pointlinewidth, pointsize;
 int            Col1234Space, FillCol1234Space;
 double         color1, color2, color3, color4; // Alternatives to the color and fillcolor settings, RGB settings
 double         fillcolor1, fillcolor2, fillcolor3, fillcolor4;
 pplExpr       *EXPlinetype, *EXPlinewidth, *EXPpointlinewidth, *EXPpointsize, *EXPpointtype; // Alternatives to the above, where expressions are evaluated per use, e.g. $4
 pplExpr       *EXPcolor, *EXPfillcolor;
 unsigned char  USEcolor, USEfillcolor, USElinespoints, USElinetype, USElinewidth, USEpointlinewidth, USEpointsize, USEpointtype, USEstyle, USEcolor1234, USEfillcolor1234; // Set to 1 to indicate settings to be used
 int            AUTOcolor, AUTOlinetype, AUTOpointtype;
 } withWords;

#endif
