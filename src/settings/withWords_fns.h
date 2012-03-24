// withWords_fns.h
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

#ifndef _WITHWORDS_FNS_H
#define _WITHWORDS_FNS_H 1

#include "withWords.h"
#include "userspace/context.h"
#include "userspace/pplObj.h"

void  ppl_withWordsZero    (ppl_context *context, withWords *a);
void  ppl_withWordsFromDict(ppl_context *context, pplObj *in, const int *ptab, withWords *out);
int   ppl_withWordsCmp     (ppl_context *context, const withWords *a, const withWords *b);
int   ppl_withWordsCmp_zero(ppl_context *context, const withWords *a);
void  ppl_withWordsMerge   (ppl_context *context, withWords *out, const withWords *a, const withWords *b, const withWords *c, const withWords *d, const withWords *e, const unsigned char ExpandStyles);
void  ppl_withWordsPrint   (ppl_context *context, const withWords *defn, char *out);
void  ppl_withWordsDestroy (ppl_context *context, withWords *a);
void  ppl_withWordsCpy     (ppl_context *context, withWords *out, const withWords *in);

#endif

