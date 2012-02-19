// traceback_fns.h
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

#ifndef _TRACEBACK_FNS_H
#define _TRACEBACK_FNS_H 1

#include "expressions/traceback.h"
#include "userspace/context.h"

void ppl_tbClear         (ppl_context *c);
void ppl_tbAdd           (ppl_context *c, int cmdOrExpr, int errType, int errPos, char *linetext);
void ppl_tbWasInSubstring(ppl_context *c, int errPosAdd, char *linetext);
void ppl_tbAddContext    (ppl_context *c, char *context);
void ppl_tbWrite         (ppl_context *c, char *out, int outLen, int *HighlightPos1, int *HighlightPos2);

#endif

