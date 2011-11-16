// context.c
//
// The code in this file is part of PyXPlot
// <http://www.pyxplot.org.uk>
//
// Copyright (C) 2006-2012 Dominic Ford <coders@pyxplot.org.uk>
//               2008-2011 Ross Church
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

#define _CONTEXT_C 1

#include <stdlib.h>
#include <string.h>

#include "coreUtils/errorReport.h"
#include "coreUtils/dict.h"

#include "stringTools/strConstants.h"

#include "userspace/context.h"

ppl_context *ppl_contextInit()
 {
  ppl_context *out = malloc(sizeof(ppl_context));
  out->willBeInteractive = 1;
  out->inputLineBuffer = NULL;
  out->inputLineAddBuffer = NULL;
  out->shellExiting = 0;
  out->historyNLinesWritten = 0;
  out->errcontext.error_input_linenumber = -1;
  out->errcontext.error_input_filename[0] = '\0';
  strcpy(out->errcontext.error_source,"main     ");
  return out;
 }

void ppl_contextFree(ppl_context *in)
 {
  free(in);
  return;
 }

