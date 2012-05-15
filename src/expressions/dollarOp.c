// dollarOp.c
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

#define _DOLLAROP_C 1

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <limits.h>

#include <gsl/gsl_math.h>

#include "expressions/dollarOp.h"
#include "expressions/expEval.h"
#include "expressions/traceback_fns.h"
#include "settings/settingTypes.h"
#include "stringTools/asciidouble.h"
#include "userspace/context.h"
#include "userspace/pplObj_fns.h"
#include "userspace/unitsArithmetic.h"

#include "datafile.h"
#include "pplConstants.h"

#define TBADD(et) ppl_tbAdd(c,inExpr->srcLineN,inExpr->srcId,inExpr->srcFname,0,et,inExprCharPos,inExpr->ascii,"")

#define STACK_POP \
   { \
    c->stackPtr--; \
    ppl_garbageObject(&c->stack[context->stackPtr]); \
    if (c->stack[c->stackPtr].refCount != 0) { strcpy(c->errStat.errBuff,"Stack forward reference detected."); TBADD(ERR_INTERNAL); goto cleanup; } \
   }

void ppl_dollarOp_config  (ppl_context *c, 
  char   **columns_str,
  pplObj  *columns_val,
  int      Ncols,
  char    *filename,
  long     file_linenumber,
  long    *file_linenumbers,
  long     linenumber_count,
  long     block_count,
  long     index_number,
  int      usingRowCol,
  char    *usingExpr,
  char   **colHeads,
  int      NcolHeads,
  pplObj  *colUnits,
  int      NcolUnits)
 {
  c->dollarStat.columns_str      = columns_str;
  c->dollarStat.columns_val      = columns_val;
  c->dollarStat.Ncols            = Ncols;
  c->dollarStat.filename         = filename;
  c->dollarStat.file_linenumber  = file_linenumber;
  c->dollarStat.file_linenumbers = file_linenumbers;
  c->dollarStat.linenumber_count = linenumber_count;
  c->dollarStat.block_count      = block_count;
  c->dollarStat.index_number     = index_number;
  c->dollarStat.usingRowCol      = usingRowCol;
  c->dollarStat.usingExpr        = usingExpr;
  c->dollarStat.colHeads         = colHeads;
  c->dollarStat.NcolHeads        = NcolHeads;
  c->dollarStat.colUnits         = colUnits;
  c->dollarStat.NcolUnits        = NcolUnits;
  c->dollarStat.warntxt[0]       = '\0';
  return;
 }

void ppl_dollarOp_deconfig(ppl_context *c)
 {
  c->dollarStat.columns_str      = NULL;
  c->dollarStat.columns_val      = NULL;
  c->dollarStat.Ncols            = -1;
  c->dollarStat.filename         = NULL;
  c->dollarStat.file_linenumber  = -1;
  c->dollarStat.file_linenumbers = NULL;
  c->dollarStat.linenumber_count = -1;
  c->dollarStat.block_count      = -1;
  c->dollarStat.index_number     = -1;
  c->dollarStat.usingRowCol      = -1;
  c->dollarStat.usingExpr        = NULL;
  c->dollarStat.colHeads         = NULL;
  c->dollarStat.NcolHeads        = -1;
  c->dollarStat.colUnits         = NULL;
  c->dollarStat.NcolUnits        = -1;
  c->dollarStat.warntxt[0]       = '\0';
  return;
 }

void ppl_dollarOp_fetchColByNum(ppl_context *c, pplExpr *inExpr, int inExprCharPos, int colNum)
 {
  pplObj *out = &c->stack[c->stackPtr-1];
  pplObjNum(out, 0, 0, 0);
  out->refCount = 1;

  if ((colNum<-3)||(colNum>c->dollarStat.Ncols))
   {
    sprintf(c->dollarStat.warntxt,"%s:%ld: In the expression <%s>, the requested %s number %d does not exist %son line %ld.",
              c->dollarStat.filename, c->dollarStat.file_linenumber, c->dollarStat.usingExpr,
             (c->dollarStat.usingRowCol==DATAFILE_COL)?"column":"row", colNum,
             (c->dollarStat.usingRowCol==DATAFILE_COL)?"":"in the block commencing ", c->dollarStat.file_linenumber);
    sprintf(c->errStat.errBuff,"No %s with number %d.", (c->dollarStat.usingRowCol==DATAFILE_COL)?"column":"row", colNum);
    TBADD(ERR_RANGE);
    return;
   }

  if      (colNum==-3) out->real = c->dollarStat.file_linenumber;
  else if (colNum==-2) out->real = c->dollarStat.index_number;
  else if (colNum==-1) out->real = c->dollarStat.block_count;
  else if (c->dollarStat.columns_val != NULL) pplObjCpy(out, &c->dollarStat.columns_val[colNum], 0, 0, 1);
  else if (colNum== 0) out->real = c->dollarStat.linenumber_count;
  else
   {
    char *s = c->dollarStat.columns_str[colNum-1];
    int j=-1;
    if ( (!ppl_validFloat(s,&j)) || (j<=1) || ((s[j]!='\0')&&(s[j]!=',')&&(s[j-1]>' ')) )
     {
      const long ln = (c->dollarStat.file_linenumbers==NULL) ? c->dollarStat.file_linenumber : c->dollarStat.file_linenumbers[colNum-1];
      sprintf(c->dollarStat.warntxt,"%s:%ld: In the expression <%s>, the requested %s number %d does not contain numeric data.",
                c->dollarStat.filename, ln, c->dollarStat.usingExpr,
               (c->dollarStat.usingRowCol==DATAFILE_COL)?"column":"row", colNum);
      sprintf(c->errStat.errBuff,"%s number %d does not contain numeric data.", (c->dollarStat.usingRowCol==DATAFILE_COL)?"Column":"Row", colNum);
      TBADD(ERR_NUMERIC);
      return;
     }
    if ((colNum>0)&&(colNum<=c->dollarStat.NcolUnits)) ppl_unitsDimCpy(out, &c->dollarStat.colUnits[colNum-1]);
    out->real = ppl_getFloat(s,&j);
   }
  return;
 }

void ppl_dollarOp_fetchColByName(ppl_context *c, pplExpr *inExpr, int inExprCharPos, char *colName)
 {
  int i;
  for (i=0; i<c->dollarStat.NcolHeads; i++)
   if (strcmp(colName, c->dollarStat.colHeads[i])==0)
    {
     ppl_dollarOp_fetchColByNum(c, inExpr, inExprCharPos, i);
     return;
    }
  sprintf(c->dollarStat.warntxt,"%s:%ld: In the expression <%s>, the requested %s named '%s' does not exist %son line %ld.",
              c->dollarStat.filename, c->dollarStat.file_linenumber, c->dollarStat.usingExpr,
             (c->dollarStat.usingRowCol==DATAFILE_COL)?"column":"row", colName,
             (c->dollarStat.usingRowCol==DATAFILE_COL)?"":"in the block commencing ", c->dollarStat.file_linenumber);
  sprintf(c->errStat.errBuff,"No %s with name '%s'.", (c->dollarStat.usingRowCol==DATAFILE_COL)?"column":"row", colName);
  TBADD(ERR_DICTKEY);
  return;
 }

