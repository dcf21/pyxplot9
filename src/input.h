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

#define _INPUT_C 1
extern char    *pplinLineBuffer;
extern char    *pplinLineAddBuffer;
extern char    *pplinLineBufferPos;
extern int      ppl_shellExiting;
extern long int history_NLinesWritten;
#endif

int  ppl_inputInit();
void ppl_interactiveSession();
void ppl_processScript     (char *input, int iterLevel);
int  ppl_processLine       (char *in, int interactive, int iterLevel, int exitOnError);
int  ppl_ProcessStatement  (char *line);

