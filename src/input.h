// input.h
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

#ifndef _INPUT_H
#define _INPUT_H 1

#include "parser/parser.h"
#include "userspace/context.h"

int  ppl_inputInit         (ppl_context *context);
void ppl_interactiveSession(ppl_context *context);
void ppl_processScript     (ppl_context *context, char *input, int iterDepth);
int  ppl_processLine       (ppl_context *context, parserStatus *ps, char *in, int interactive, int iterDepth);
int  ppl_ProcessStatement  (ppl_context *context, parserStatus *ps, char *line, int interactive, int iterDepth);

#endif

