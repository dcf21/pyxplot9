// parserCompile.c
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

#define _PARSERCOMPILE_C 1

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "coreUtils/errorReport.h"

#include "expressions/expCompile.h"
#include "expressions/traceback_fns.h"

#include "stringTools/asciidouble.h"
#include "stringTools/strConstants.h"

#include "parser/parser.h"

#include "userspace/context.h"
#include "userspace/garbageCollector.h"
#include "userspace/pplObj_fns.h"

static void parse_descend(ppl_context *c, char *line, int *linepos, parserNode *node, int tabCompNo, char *tabCompTxt,  int topLevel, int *match, parserLine *out)
 {
  return;
 }

void ppl_parserAtomFree(parserAtom **in)
 {
  parserAtom *i;
  if (in==NULL) return;
  i = *in;
  while (i!=NULL)
   {
    parserAtom *next = i->next;
    if (i->literal!=NULL) ppl_garbageObject(i->literal);
    if (i->expr   !=NULL) pplExpr_free(i->expr);
    free(i);
    i = next;
   }
  *in=NULL;
  return;
 }

void ppl_parserLineFree(parserLine *in)
 {
  ppl_parserAtomFree(&in->firstAtom);
  if (in->linetxt !=NULL) free(in->linetxt);
  if (in->srcFname!=NULL) free(in->srcFname);
  free(in);
  return;
 }

parserLine *ppl_parserCompile(ppl_context *c, char *line, int ExpandMacros)
 {
  parserLine   *output=NULL;
  listIterator *cmdIter=NULL;
  int           i, cln;

  // Initialise output structure
  output = (parserLine *)malloc(sizeof(parserLine));
  if (output==NULL) return NULL;
  output->linetxt = (char *)malloc(strlen(line)+1);
  if (output->linetxt==NULL) { free(output); return NULL; }
  strcpy(output->linetxt, line);
  output->srcFname = (char *)malloc(strlen(c->errcontext.error_input_filename)+1);
  if (output->srcFname==NULL) { free(output->linetxt); free(output); return NULL; }
  strcpy(output->srcFname, c->errcontext.error_input_filename);
  output->firstAtom = NULL;
  output->lastAtom  = NULL;
  output->srcLineN  = c->errcontext.error_input_linenumber;
  output->srcId     = c->errcontext.error_input_sourceId;
  output->stackLen  = 16;

  // Fetch first non-whitespace character of command string
  for (i=0; ((line[i]>='\0')&&(line[i]<=' ')); i++);
  if      ((line[i]>='a')&&(line[i]<='z')) cln=(int)(line[i]-'a');
  else if ((line[i]>='A')&&(line[i]<='Z')) cln=(int)(line[i]-'A');
  else                                     cln=-1;

  // Cycle through possible command matches
  cmdIter = ppl_listIterateInit(pplParserCmdList[26]);
  while (1)
   {
    int match, linepos;
    parserNode *item;

    if (cmdIter == NULL) // Once we've finished cycling through commands that start with punctuation, cycle through ones that start with first letter
     {
      if (cln==-1) break;
      cmdIter = ppl_listIterateInit(pplParserCmdList[cln]);
      cln=-1;
      if (cmdIter==NULL) break;
     }

    // Fetch next item from list of possible commands and try to parse it
    item  = (parserNode *)ppl_listIterate(&cmdIter);
    match = linepos = 0;
    parse_descend(c, line, &linepos, item, -1, NULL, 1, &match, output);

    // If this command did not match as far as the '=' token, clear stack and traceback
    if (match==0) { ppl_parserAtomFree(&output->firstAtom); ppl_tbClear(c); continue; }

    // If we did match as far as '=', but got an error traceback, free the stack and return to the user
    if (c->errStat.status) { ppl_parserLineFree(output); return NULL; }

    // Success!
    return output;
   }

  // We have not found a match to this command
  sprintf(c->errStat.errBuff, "Unrecognised command.");
  ppl_tbAdd(c,output->srcLineN,output->srcId,output->srcFname,1,ERR_SYNTAX,0,line);
  ppl_parserLineFree(output);
  return NULL;
 }

