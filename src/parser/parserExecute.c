// parserExecute.c
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

#define _PARSEREXECUTE_C 1

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "coreUtils/errorReport.h"

#include "stringTools/asciidouble.h"
#include "stringTools/strConstants.h"

#include "parser/parser.h"

#include "userspace/context.h"
#include "userspace/garbageCollector.h"
#include "userspace/pplObj_fns.h"

void ppl_parserOutFree(parserOutput *in)
 {
  int i;
  if (in==NULL) return;
  for (i=0; i<in->stackLen; i++) ppl_garbageObject(&in->stk[i]);
  free(in->stk);
  free(in);
  return;
 }

parserOutput *ppl_parserExecute(ppl_context *c, parserLine *in)
 {
  int           i;
  parserOutput *out=NULL;
  pplObj       *stk=NULL;

  // Initialise output stack
  out = (parserOutput *)malloc(sizeof(parserOutput));
  if (out==NULL) return NULL;
  stk = (pplObj *)malloc(in->stackLen*sizeof(pplObj));
  if (stk==NULL) { free(out); return NULL; }
  for (i=0; i<in->stackLen; i++) pplObjZom(&stk[i],0);
  out->stk = stk;
  out->stackLen = in->stackLen;

  return out;
 }

