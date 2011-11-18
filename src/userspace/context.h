// context.h
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

#ifndef _CONTEXT_H
#define _CONTEXT_H 1

#include "stringTools/strConstants.h"

#include "coreUtils/dict.h"

#include "pplConstants.h"

typedef struct ppl_context_struc
 {

  // Shell status
  int       willBeInteractive;
  char     *inputLineBuffer;
  char     *inputLineAddBuffer;
  int       shellExiting;
  long int  historyNLinesWritten;

  // CSP status
  char      pplcsp_ghostView_fname[FNAME_LENGTH];

  // Code position to report when ppl_error() is called
  pplerr_context errcontext;

  // Buffers for parsing and evaluating expressions
  unsigned char tokenBuff[ALGEBRA_MAXLEN];

  // Namespace hierarchy
  dict *namespaces[CONTEXT_DEPTH];

 } ppl_context;

ppl_context *ppl_contextInit();
void         ppl_contextFree(ppl_context *in);

#endif

