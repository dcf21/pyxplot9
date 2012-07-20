// asciidouble.h
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

#ifndef _ASCIIDOUBLE_H
#define _ASCIIDOUBLE_H 1

#include <stdio.h>

unsigned char ppl_dblEqual(double a, double b);
unsigned char ppl_dblApprox(double a, double b, double err);

double ppl_getFloat                 (const char *str, int *Nchars);
int    ppl_validFloat               (const char *str, int *end);
char  *ppl_numericDisplay           (double in, char *output, int SigFig, int latex);
void   ppl_file_readline            (FILE *file, char **output, int *MaxLenPtr, int MaxLength);
void   ppl_getWord                  (char *out, const char *in, int max);
char  *ppl_nextWord                 (char *in);
char  *ppl_friendlyTimestring       ();
char  *ppl_strStrip                 (const char *in, char *out);
char  *ppl_strUpper                 (const char *in, char *out);
char  *ppl_strLower                 (const char *in, char *out);
char  *ppl_strUnderline             (const char *in, char *out);
char  *ppl_strRemoveCompleteLine    (char *in, char *out);
char  *ppl_strSlice                 (const char *in, char *out, int start, int end);
char  *ppl_strCommaSeparatedListScan(char **inscan, char *out);
int    ppl_strAutocomplete          (const char *candidate, char *test, int Nmin);
void   ppl_strWordWrap              (const char *in, char *out, int width);
void   ppl_strBracketMatch          (const char *in, char open, char close, int *CommaPositions, int *Nargs, int *ClosingBracketPos, int MaxCommaPoses);
int    ppl_strCmpNoCase             (const char *a, const char *b);
char  *ppl_strEscapify              (const char *in, char *out);
int    ppl_strWildcardTest          (const char *test, const char *wildcard);
#endif

