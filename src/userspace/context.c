// context.c
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

#define _CONTEXT_C 1

#include <stdlib.h>
#include <string.h>

#include "coreUtils/errorReport.h"
#include "coreUtils/dict.h"
#include "coreUtils/memAlloc.h"

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
  ppl_context *out = malloc(sizeof(ppl_context));
  if (out==NULL) return NULL;

  out->set = (ppl_settings *)malloc(sizeof(ppl_settings));
  if (out->set==NULL) { free(out); return NULL; }

  out->errcontext.error_input_linenumber = -1;
  out->errcontext.error_input_filename[0] = '\0';
  strcpy(out->errcontext.error_source,"main     ");
  ppl_memAlloc_MemoryInit(&out->errcontext, &ppl_error, &ppl_log);
  pplset_makedefault(out);

  out->stackPtr = 0;
  out->willBeInteractive = 1;
  out->inputLineBuffer = NULL;
  out->inputLineAddBuffer = NULL;
  out->shellExiting = 0;
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

