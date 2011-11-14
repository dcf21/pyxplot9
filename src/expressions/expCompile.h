// expCompile.h
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

#ifndef _EXPCOMPILE_H
#define _EXPCOMPILE_H 1

void ppl_expTokenise       (char *in, int *end, int dollarAllowed, int collectCommas, int isDict, unsigned char *out, int *outlen, int *errPos, char *errText);
void ppl_tokenPrint        (char *in, unsigned char *tdat, int len);
void ppl_expCompile        (char *in, int *end, int dollarAllowed, void *out, int *outlen, void *tmp, int tmplen, int *errPos, char *errText);
void ppl_reversePolishPrint(void *in, char *out);

#endif

