// context.c
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

#define _CONTEXT_C 1

#include <stdlib.h>
#include <string.h>

#include "coreUtils/errorReport.h"
#include "coreUtils/dict.h"
#include "coreUtils/memAlloc.h"

#include "expressions/dollarOp.h"
#include "expressions/traceback_fns.h"

#include "settings/settings_fns.h"

#include "stringTools/strConstants.h"

#include "userspace/context.h"
#include "userspace/garbageCollector.h"
#include "userspace/pplObj_fns.h"
#include "userspace/pplObjUnits.h"

#include "defaultObjs/defaultUnits.h"
#include "defaultObjs/defaultVars.h"

ppl_context *ppl_contextInit()
 {
  int i;
  ppl_context *out = malloc(sizeof(ppl_context));
  if (out==NULL) return NULL;

  out->set = (ppl_settings *)malloc(sizeof(ppl_settings));
  if (out->set==NULL) { free(out); return NULL; }

  out->errcontext.error_input_linenumber = -1;
  out->errcontext.error_input_filename[0] = '\0';
  strcpy(out->errcontext.error_source,"main     ");
  ppl_memAlloc_MemoryInit(&out->errcontext, &ppl_error, &ppl_log);
  pplset_makedefault(out);
  ppl_dollarOp_deconfig(out);
  out->dollarStat.lastFilename[0]='\0';

  out->errStat.status = out->errStat.tracebackDepth = 0; ppl_tbClear(out);

  out->canvas_items = NULL;
  out->replotFocus  = -1;
  out->algebraErrPos = -1;

  out->tokenBuff = NULL;   out->tokenBuffLen = 0;
  out->parserStack = NULL; out->parserStackLen = 0;
  out->stackPtr = 0;
  out->willBeInteractive = 1;
  out->inputLineBuffer = NULL;
  out->inputLineAddBuffer = NULL;
  out->shellExiting = 0;
  out->shellBreakable = 0;
  out->shellReturnable = 0;
  out->shellBroken = 0;
  out->shellContinued = 0;
  out->shellReturned = 0;
  for (i=0; i<MAX_RECURSION_DEPTH+8; i++) out->shellLoopName[i]=NULL;
  out->shellReturnVal.refCount = 1;
  pplObjNum(&out->shellReturnVal,0,0,0);
  out->historyNLinesWritten = 0;
  out->termtypeSetInConfigfile = 0;
  out->errcontext.error_input_linenumber = -1;
  out->errcontext.error_input_filename[0] = '\0';
  strcpy(out->errcontext.error_source,"main     ");

  pplObjInit(out);
  ppl_makeDefaultUnits(out);
  ppl_makeDefaultVars(out);
  return out;
 }

void ppl_contextFree(ppl_context *in)
 {
  int i;
  for (i=in->ns_ptr; i>in->ns_branch; i--) ppl_garbageNamespace(in->namespaces[i]);
  free(in);
  return;
 }

