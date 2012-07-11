// canvasDraw.h
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

#ifndef _CANVASDRAW_H
#define _CANVASDRAW_H 1

#include "epsMaker/eps_comm.h"
#include "userspace/context.h"

void ppl_canvas_draw(ppl_context *c, unsigned char *unsuccessful_ops, int iterDepth);
void canvas_CallLaTeX(EPSComm *x);
void canvas_MakeEPSBuffer(EPSComm *x);
void canvas_EPSWrite(EPSComm *x);
void canvas_EPSRenderTextItem(EPSComm *x, char **strout, int pageno, double xpos, double ypos, int halign, int valign, char *colstr, double fontsize, double rotate, double *width, double *height);
void canvas_EPSLandscapify(EPSComm *x, char *transform);
void canvas_EPSEnlarge(EPSComm *x, char *transform);

#endif
