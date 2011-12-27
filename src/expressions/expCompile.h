// expCompile.h
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

#ifndef _EXPCOMPILE_H
#define _EXPCOMPILE_H 1

#include "userspace/context.h"

void ppl_expTokenise       (ppl_context *context, char *in, int *end, int dollarAllowed, int allowCommaOperator, int collectCommas, int isDict, int outOffset, int *outlen, int *errPos, int *errType, char *errText);
void ppl_tokenPrint        (ppl_context *context, char *in, int len);
void ppl_expCompile        (ppl_context *context, char *in, int *end, int dollarAllowed, int allowCommaOperator, void *out, int *outlen, int *errPos, int *errType, char *errText);
void ppl_reversePolishPrint(ppl_context *context, void *in, char *out);

#endif

