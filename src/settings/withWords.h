// withWords.h
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

#ifndef _WITHWORDS_H
#define _WITHWORDS_H 1

typedef struct withWords {
 int    colour, fillcolour, linespoints, linetype, pointtype, style; // Core style settings which can be placed after the 'with' modifier
 double linewidth, pointlinewidth, pointsize;
 int    Col1234Space, FillCol1234Space;
 double colour1, colour2, colour3, colour4; // Alternatives to the colour and fillcolour settings, RGB settings
 double fillcolour1, fillcolour2, fillcolour3, fillcolour4;
 char  *STRlinetype, *STRlinewidth, *STRpointlinewidth, *STRpointsize, *STRpointtype; // Alternatives to the above, where expressions are evaluated per use, e.g. $4
 char  *STRcolour, *STRcolour1, *STRcolour2, *STRcolour3, *STRcolour4, *STRfillcolour, *STRfillcolour1, *STRfillcolour2, *STRfillcolour3, *STRfillcolour4;
 unsigned char USEcolour, USEfillcolour, USElinespoints, USElinetype, USElinewidth, USEpointlinewidth, USEpointsize, USEpointtype, USEstyle, USEcolour1234, USEfillcolour1234; // Set to 1 to indicate settings to be used
 int    AUTOcolour, AUTOlinetype, AUTOpointtype;
 unsigned char malloced; // Indicates whether we need to free strings
 } withWords;

void  ppl_withWordsZero    (withWords *a, const unsigned char malloced);
int   ppl_withWordsCmp     (const withWords *a, const withWords *b);
int   ppl_withWordsCmp_zero(const withWords *a);
void  ppl_withWordsMerge   (withWords *out, const withWords *a, const withWords *b, const withWords *c, const withWords *d, const withWords *e, const unsigned char ExpandStyles);
void  ppl_withWordsPrint   (const withWords *defn, char *out);
void  ppl_withWordsDestroy (withWords *a);
void  ppl_withWordsCpy     (withWords *out, const withWords *in);

#endif
