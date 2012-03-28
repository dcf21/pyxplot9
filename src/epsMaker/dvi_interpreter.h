// dvi_interpreter.h
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

#ifndef _PPL_DVI_INTERPRETER_H
#define _PPL_DVI_INTERPRETER_H 1

#include "coreUtils/errorReport.h"
#include "epsMaker/dvi_read.h"

// Postscript functions
int dviPostscriptLineto       (pplerr_context *ec, dviInterpreterState *interp);
int dviPostscriptClosepathFill(pplerr_context *ec, dviInterpreterState *interp);
int dviChngFnt                (pplerr_context *ec, dviInterpreterState *interp, int fn);
int dviSpecialColourCommand   (pplerr_context *ec, dviInterpreterState *interp, char *command);
int dviSpecialColourStackPush (pplerr_context *ec, dviInterpreterState *interp, char *psText);
int dviSpecialColourStackPop  (pplerr_context *ec, dviInterpreterState *interp);

// Interpreter functions for various types of dvi operators
int dviInOpChar       (pplerr_context *ec, dviInterpreterState *interp, DVIOperator *op);
int dviInOpSet1234    (pplerr_context *ec, dviInterpreterState *interp, DVIOperator *op);
int dviInOpSetRule    (pplerr_context *ec, dviInterpreterState *interp, DVIOperator *op);
int dviInOpPut1234    (pplerr_context *ec, dviInterpreterState *interp, DVIOperator *op);
int dviInOpPutRule    (pplerr_context *ec, dviInterpreterState *interp, DVIOperator *op);
int dviInOpNop        (pplerr_context *ec, dviInterpreterState *interp, DVIOperator *op);
int dviInOpBop        (pplerr_context *ec, dviInterpreterState *interp, DVIOperator *op);
int dviInOpEop        (pplerr_context *ec, dviInterpreterState *interp, DVIOperator *op);
int dviInOpPush       (pplerr_context *ec, dviInterpreterState *interp, DVIOperator *op);
int dviInOpPop        (pplerr_context *ec, dviInterpreterState *interp, DVIOperator *op);
int dviInOpRight1234  (pplerr_context *ec, dviInterpreterState *interp, DVIOperator *op);
int dviInOpW0         (pplerr_context *ec, dviInterpreterState *interp, DVIOperator *op);
int dviInOpW1234      (pplerr_context *ec, dviInterpreterState *interp, DVIOperator *op);
int dviInOpX0         (pplerr_context *ec, dviInterpreterState *interp, DVIOperator *op);
int dviInOpX1234      (pplerr_context *ec, dviInterpreterState *interp, DVIOperator *op);
int dviInOpDown1234   (pplerr_context *ec, dviInterpreterState *interp, DVIOperator *op);
int dviInOpY0         (pplerr_context *ec, dviInterpreterState *interp, DVIOperator *op);
int dviInOpY1234      (pplerr_context *ec, dviInterpreterState *interp, DVIOperator *op);
int dviInOpZ0         (pplerr_context *ec, dviInterpreterState *interp, DVIOperator *op);
int dviInOpZ1234      (pplerr_context *ec, dviInterpreterState *interp, DVIOperator *op);
int dviInOpFnt        (pplerr_context *ec, dviInterpreterState *interp, DVIOperator *op);
int dviInOpFnt1234    (pplerr_context *ec, dviInterpreterState *interp, DVIOperator *op);
int dviInOpSpecial1234(pplerr_context *ec, dviInterpreterState *interp, DVIOperator *op);
int dviInOpFntdef1234 (pplerr_context *ec, dviInterpreterState *interp, DVIOperator *op);
int dviInOpPre        (pplerr_context *ec, dviInterpreterState *interp, DVIOperator *op);
int dviInOpPost       (pplerr_context *ec, dviInterpreterState *interp, DVIOperator *op);
int dviInOpPostPost   (pplerr_context *ec, dviInterpreterState *interp, DVIOperator *op);

// Functions called by operator interpreter functions
int dviSpecialChar      (pplerr_context *ec, dviInterpreterState *interp, DVIOperator *op);
int dviSpecialImplement (pplerr_context *ec, dviInterpreterState *interp);
int dviNonAsciiChar     (pplerr_context *ec, dviInterpreterState *interp, int c, char move);
int dviUpdateBoundingBox(pplerr_context *ec, dviInterpreterState *interp, double width, double height, double depth);

#endif

