// context.h
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

#ifndef _CONTEXT_H
#define _CONTEXT_H 1

#include "settings/settings.h"

#include "stringTools/strConstants.h"

#include "coreUtils/dict.h"
#include "coreUtils/list.h"
#include "coreUtils/errorReport.h"

#include "expressions/traceback.h"

#include "pplConstants.h"

typedef struct ppl_context_struc
 {

  // Shell status
  int       willBeInteractive;
  char     *inputLineBuffer;
  int       inputLineBufferLen;
  char     *inputLineAddBuffer;
  int       shellBreakable, shellReturnable, shellBreakLevel;
  int       shellExiting, shellBroken, shellContinued, shellReturned;
  char     *shellLoopName[MAX_RECURSION_DEPTH+8];
  pplObj    shellReturnVal;
  long int  historyNLinesWritten;
  int       termtypeSetInConfigfile;

  // CSP status
  char      pplcsp_ghostView_fname[FNAME_LENGTH];

  // Code position to report when ppl_error() is called
  pplerr_context errcontext;

  // traceback
  errStatus errStat;

  // Buffers for parsing and evaluating expressions
  unsigned char tokenBuff[ALGEBRA_MAXLEN];
  pplObj        stack    [ALGEBRA_STACK];
  int           stackPtr;

  // Settings
  ppl_settings *set;

  // Units settings
  unit  *unit_database;
  int    unit_pos;
  int    baseunit_pos;
  list  *unit_PreferredUnits;
  list  *unit_PreferredUnits_default;
  double tempTypeMultiplier[8]; // These are filled in by ppl_userspace_init.c
  double tempTypeOffset    [8]; // They store the offsets and multiplier for each of the units of temperature

  // Buffers used by ppl_printUnit()
  char udBuffA[LSTR_LENGTH], udBuffB[LSTR_LENGTH], udBuffC[LSTR_LENGTH];
  char udNumDispA[LSTR_LENGTH], udNumDispB[LSTR_LENGTH];
  char numdispBuff[4][128];

  // Namespace hierarchy
  int   ns_ptr , ns_branch;
  dict *namespaces[CONTEXT_DEPTH];

 } ppl_context;

ppl_context *ppl_contextInit();
void         ppl_contextFree(ppl_context *in);

#endif

