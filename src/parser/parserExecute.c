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

#include "expressions/expEval.h"
#include "expressions/traceback_fns.h"

#include "stringTools/asciidouble.h"
#include "stringTools/strConstants.h"

#include "parser/parser.h"

#include "userspace/context.h"
#include "userspace/garbageCollector.h"
#include "userspace/pplObj_fns.h"

#include "pplConstants.h"

#define TBADD(et,pos,lt) ppl_tbAdd(c,in->srcLineN,in->srcId,in->srcFname,0,et,pos,lt)

void ppl_parserOutFree(parserOutput *in)
 {
  int i;
  if (in==NULL) return;
  for (i=0; i<in->stackLen; i++) ppl_garbageObject(&in->stk[i]);
  free(in->stk);
  free(in);
  return;
 }

void ppl_parserExecute(ppl_context *c, parserLine *in, int iterDepth)
 {
  int           i;
  parserOutput *out  = NULL;
  parserAtom   *item = in->firstAtom;
  pplObj       *stk  = NULL;
  char         *eB   = c->errStat.errBuff;

  // Check that recursion depth has not been exceeded
  if (iterDepth > MAX_RECURSION_DEPTH) { strcpy(eB,"Maximum recursion depth exceeded."); TBADD(ERR_OVERFLOW,0,in->linetxt); return; }

  while (in != NULL)
   {
    // If line contains macros, need to recompile it now
    if (in->containsMacros)
     {
      parserLine *pl = ppl_parserCompile(c, in->srcLineN, in->srcId, in->srcFname, in->linetxt, 1);
      if (c->errStat.status) break;
      ppl_parserExecute(c, pl, iterDepth+1);
      ppl_parserLineFree(pl);
      in = in->next;
      continue;
     }

    // Initialise output stack
    out = (parserOutput *)malloc(sizeof(parserOutput));
    if (out==NULL) { strcpy(eB,"Out of memory."); TBADD(ERR_MEMORY,0,in->linetxt); return; }
    stk = (pplObj *)malloc(in->stackLen*sizeof(pplObj));
    if (stk==NULL) { free(out); strcpy(eB,"Out of memory."); TBADD(ERR_MEMORY,0,in->linetxt); return; }
    for (i=0; i<in->stackLen; i++) pplObjZom(&stk[i],0);
    out->stk = stk;
    out->stackLen = in->stackLen;

    // Loop over atoms
    while (item != NULL)
     {
      if (item->literal!=NULL)
       {
        // Atom is a literal: simply copy it
        pplObjCpy(&stk[i],item->literal,0,0,1);
       }
      else
       {
        // Atom is an expression: evaluate it
        int lastOpAssign=0, passed=0, j=0, k=0;
        pplObj *val = ppl_expEval(c, item->expr, &lastOpAssign, 1, iterDepth+1);
        if (c->errStat.status) { ppl_tbWasInSubstring(c, item->linePos, in->linetxt); break; } // On error, stop

        // Check type of pplObj produced by expression, and complain if the wrong type
        strcpy(eB+k,"Expression evaluates to the wrong type: needed "); k+=strlen(eB+k);
        for (j=0; ((j<PARSER_TYPE_OPTIONS)&&(item->options!='\0')&&(!passed)); j++)
         {
         }

        // If expression has produced a value of the wrong type, throw an error
        if (!passed)
         {
          TBADD(ERR_TYPE, item->linePos, in->linetxt);
          break;
         }

        // Copy value produced by atom
        pplObjCpy(&stk[i], val, 0, 0, 1);
       }
      item = item->next;
     }

    // If no error, pass command to shell
    if (!c->errStat.status)
     {
      ppl_parserShell(c, out, iterDepth);
     }

    // Free output stack
    ppl_parserOutFree(out);

    in = in->next;
   }
  return;
 }

