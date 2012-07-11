// flowctrl.h
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

#ifndef _FLOWCTRL_H
#define _FLOWCTRL_H 1

#include "parser/parser.h"
#include "userspace/context.h"
#include "userspace/pplObj.h"

void ppl_directive_break   (ppl_context *c, parserLine *pl, parserOutput *in, int interactive, int iterDepth);
void ppl_directive_continue(ppl_context *c, parserLine *pl, parserOutput *in, int interactive, int iterDepth);
void ppl_directive_return  (ppl_context *c, parserLine *pl, parserOutput *in, int interactive, int iterDepth);

void ppl_directive_do      (ppl_context *c, parserLine *pl, parserOutput *in, int interactive, int iterDepth);
void ppl_directive_for     (ppl_context *c, parserLine *pl, parserOutput *in, int interactive, int iterDepth);
void ppl_directive_foreach (ppl_context *c, parserLine *pl, parserOutput *in, int interactive, int iterDepth);
void ppl_directive_fordata (ppl_context *c, parserLine *pl, parserOutput *in, int interactive, int iterDepth);
void ppl_directive_if      (ppl_context *c, parserLine *pl, parserOutput *in, int interactive, int iterDepth);
void ppl_directive_subrt   (ppl_context *c, parserLine *pl, parserOutput *in, int interactive, int iterDepth);
void ppl_directive_while   (ppl_context *c, parserLine *pl, parserOutput *in, int interactive, int iterDepth);
void ppl_directive_with    (ppl_context *c, parserLine *pl, parserOutput *in, int interactive, int iterDepth);

#endif

