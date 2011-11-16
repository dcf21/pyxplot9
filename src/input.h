// input.h
//
// The code in this file is part of PyXPlot
// <http://www.pyxplot.org.uk>
//
// Copyright (C) 2006-2012 Dominic Ford <coders@pyxplot.org.uk>
//               2008-2011 Ross Church
//               2010-2011 Zoltan Voros
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

#ifndef _INPUT_H
#define _INPUT_H 1

#include "userspace/context.h"

int  ppl_inputInit         (ppl_context *context);
void ppl_interactiveSession(ppl_context *context);
void ppl_processScript     (ppl_context *context, char *input, int iterLevel);
int  ppl_processLine       (ppl_context *context, char *in, int interactive, int iterLevel, int exitOnError);
int  ppl_ProcessStatement  (ppl_context *context,char *line);

#endif

