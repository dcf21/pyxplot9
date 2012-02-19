// traceback.c
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

#include <stdlib.h>
#include <string.h>

#include "expressions/traceback_fns.h"
#include "stringTools/strConstants.h"
#include "userspace/context.h"

void ppl_tbClear(ppl_context *c)
 {
  int i;
  for (i=c->errStat.tracebackDepth-1 ; i>=0 ; i--)
   {
    traceback *t = &c->errStat.tbLevel[i];
    if (t->linetext!=NULL) free(t->linetext);
   }
  c->errStat.status = c->errStat.tracebackDepth = 0;
  c->errStat.errType = -1;
  c->errStat.errPosCmd = c->errStat.errPosExpr = -1;
  c->errStat.errMsgExpr[0] = c->errStat.errMsgCmd[0] = c->errStat.errBuff[0];
  c->errStat.sourceIdExpr = c->errStat.sourceIdCmd = -1;
  return;
 }

void ppl_tbAdd(ppl_context *c, int cmdOrExpr, int errType, int errPos, char *linetext)
 {
  int i = c->errStat.tracebackDepth;
  if ((!cmdOrExpr) || (i==0))
   {
    traceback *t = &c->errStat.tbLevel[i];
    if ((i<0)||(i>=TB_MAXLEVEL)) return;
    c->errStat.tracebackDepth++;
    t->errPos    = errPos;
    t->errLine   = c->errcontext.error_input_linenumber;
    t->sourceId  = c->errcontext.error_input_sourceId;
    strncpy(t->source, c->errcontext.error_input_filename, FNAME_LENGTH);
    t->amErrMsgExpr = t->amErrMsgCmd = 0;
    t->source[FNAME_LENGTH-1]='\0';
    t->context[0] = '\0';
    if (linetext==NULL) { t->linetext=NULL; }
    else                { t->linetext = malloc(strlen(linetext)+1); if (t->linetext!=NULL) strcpy(t->linetext,linetext); }

    c->errStat.status  = 1;
    if (c->errStat.errType < 0) c->errStat.errType = errType;
    if ((!cmdOrExpr)&&(c->errStat.sourceIdExpr<0))
     {
      t->amErrMsgExpr = 1;
      c->errStat.sourceIdExpr = c->errcontext.error_input_sourceId;
      c->errStat.errPosExpr = errPos;
      strcpy(c->errStat.errMsgExpr, c->errStat.errBuff);
     }
    if ((cmdOrExpr)&&(c->errStat.sourceIdCmd<0)) t->amErrMsgCmd = 1;
   }
  if ((cmdOrExpr)&&(c->errStat.sourceIdCmd<0))
   {
    c->errStat.sourceIdCmd = c->errcontext.error_input_sourceId;
    c->errStat.errPosCmd   = errPos;
    strcpy(c->errStat.errMsgCmd, c->errStat.errBuff);
   }
  return;
 }

void ppl_tbWasInSubstring(ppl_context *c, int errPosAdd, char *linetext)
 {
  int i = c->errStat.tracebackDepth-1;
  traceback *t = &c->errStat.tbLevel[i];
  if ((i<0)||(i>=TB_MAXLEVEL)) return;
  t->errPos += errPosAdd;
  if (t->amErrMsgExpr) c->errStat.errPosExpr += errPosAdd;
  if (t->amErrMsgCmd ) c->errStat.errPosCmd  += errPosAdd;
  if (linetext!=NULL)
   {
    if (t->linetext!=NULL) free(t->linetext);
    t->linetext = malloc(strlen(linetext)+1);
    if (t->linetext!=NULL) strcpy(t->linetext,linetext);
   }
  return;
 }

void ppl_tbAddContext(ppl_context *c, char *context)
 {
  int i = c->errStat.tracebackDepth-1;
  if ((i<0)||(i>=TB_MAXLEVEL)) return;
  strncpy(c->errStat.tbLevel[i].context, context, FNAME_LENGTH);
  c->errStat.tbLevel[i].context[FNAME_LENGTH-1] = '\0';
  return;
 }

void ppl_tbWrite(ppl_context *c, char *out, int outLen, int *HighlightPos1, int *HighlightPos2)
 {
  int i=0,j,k,l;
  out[i] = out[outLen-1] = '\0';
  outLen--;

  *HighlightPos1 = *HighlightPos2 = -1;

  if (!c->errStat.status) return; // No error!

  snprintf(out+i, outLen-i, "\n"); i+=strlen(out+i);

  switch (c->errStat.errType)
   {
    case ERR_INTERNAL : snprintf(out+i, outLen-i, "Internal Error:");  break;
    case ERR_SYNTAX   : snprintf(out+i, outLen-i, "Syntax Error:");    break;
    case ERR_NUMERIC  : snprintf(out+i, outLen-i, "Numerical Error:"); break;
    case ERR_FILE     : snprintf(out+i, outLen-i, "File Error:");      break;
    case ERR_RANGE    : snprintf(out+i, outLen-i, "Range Error:");     break;
    case ERR_UNIT     : snprintf(out+i, outLen-i, "Unit Error:");      break;
    case ERR_OVERFLOW : snprintf(out+i, outLen-i, "Overflow Error:");  break;
    case ERR_NAMESPACE: snprintf(out+i, outLen-i, "Namespace Error:"); break;
    case ERR_TYPE     : snprintf(out+i, outLen-i, "Type Error:");      break;
    case ERR_INTERRUPT: snprintf(out+i, outLen-i, "Interrupt Error:"); break;
    case ERR_DICTKEY  : snprintf(out+i, outLen-i, "Key Error:");       break;
    case ERR_MEMORY   :
    case ERR_GENERAL  :
    default           : snprintf(out+i, outLen-i, "Error:");           break;
   }
  i+=strlen(out+i);
  snprintf(out+i, outLen-i, "\n\n"); i+=strlen(out+i);
  for (j=c->errStat.tracebackDepth-1 ; j>=0 ; j--)
   {
    traceback *t = &c->errStat.tbLevel[j];
    snprintf(out+i, outLen-i, "In %s", (t->source[0]=='\0')?"keyboard entry":t->source); i+=strlen(out+i);
    if (t->errLine>=0)       { snprintf(out+i, outLen-i, ", line %d", t->errLine); i+=strlen(out+i); }
    if (t->errPos >=0)       { snprintf(out+i, outLen-i, ", position %d", t->errPos); i+=strlen(out+i); }
    if (t->context[0]!='\0') { snprintf(out+i, outLen-i, " (%s)", t->context); i+=strlen(out+i); }
    for (k=l=0; k<j; k++) if (c->errStat.tbLevel[k].sourceId == t->sourceId) { l=1; break; }
    if ((t->linetext!=NULL) && (j>0) && (!l))
     {
      snprintf(out+i, outLen-i, ":\n %s\n", t->linetext); i+=strlen(out+i);
     }
    else                     { snprintf(out+i, outLen-i, ".\n"); i+=strlen(out+i); }
   }
  snprintf(out+i, outLen-i, "\n"); i+=strlen(out+i);

  if ((c->errStat.tracebackDepth>0) && (c->errStat.tbLevel[0].linetext!=NULL))
   {
    const int lp  = c->errStat.errPosCmd;
    const int lp2 = c->errStat.errPosExpr;
    traceback *t = &c->errStat.tbLevel[0];
    if (t->sourceId == c->errStat.sourceIdCmd)
     {
      snprintf(out+i, outLen-i, "%s\n", c->errStat.errMsgCmd); i+=strlen(out+i);
      for (k=0;((k<lp)&&(i<outLen));k++,i++) out[i]=' ';
      snprintf(out+i, outLen-i, " |\n"); i+=strlen(out+i);
      for (k=0;((k<lp)&&(i<outLen));k++,i++) out[i]=' ';
      snprintf(out+i, outLen-i, "\\|/\n"); i+=strlen(out+i);
      *HighlightPos1 = i + 1 + lp;
     }
    *HighlightPos2 = i + 1 + lp2;
    snprintf(out+i, outLen-i, " %s\n", t->linetext); i+=strlen(out+i);
    if (t->sourceId == c->errStat.sourceIdExpr)
     {
      for (k=0;((k<lp2)&&(i<outLen));k++,i++) out[i]=' ';
      snprintf(out+i, outLen-i, "/|\\\n"); i+=strlen(out+i);
      for (k=0;((k<lp2)&&(i<outLen));k++,i++) out[i]=' ';
      snprintf(out+i, outLen-i, " |\n"); i+=strlen(out+i);
      snprintf(out+i, outLen-i, "%s\n", c->errStat.errMsgExpr); i+=strlen(out+i);
     }
    else { *HighlightPos2 = -1; }
   }
  return;
 }

