// eps_comm.h
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

#ifndef _PPL_EPS_COMM_H
#define _PPL_EPS_COMM_H 1

#include <stdio.h>

#include "coreUtils/list.h"
#include "epsMaker/dvi_read.h"
#include "epsMaker/eps_style.h"
#include "userspace/context.h"
#include "canvasItems.h"

typedef struct CanvasTextItem {
  char *text;
  int   CanvasMultiplotID;
  int   LaTeXstartline, LaTeXendline;
 } CanvasTextItem;

typedef struct EPSComm {
  ppl_context *c;
  int iterDepth;
  canvas_itemlist *itemlist;
  canvas_item *current;
  double bb_left, bb_right, bb_top, bb_bottom;
  unsigned char bb_set;
  unsigned char PointTypesUsed[N_POINTTYPES], StarTypesUsed[N_STARTYPES];
  list *TextItems;
  int NTextItems;
  dviInterpreterState *dvi;
  char *EPSFilename, *TeXFilename, *FinalFilename, *title;
  char  LastPSColor[256], CurrentColor[256], CurrentFillColor[256];
  double LastLinewidth;
  int LastLinetype;
  FILE *epsbuffer;
  int *status, termtype, LaTeXpageno;
 } EPSComm;

#endif

