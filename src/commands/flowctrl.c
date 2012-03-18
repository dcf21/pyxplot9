// flowctrl.c
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

#define _FLOWCTRL_C 1

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <glob.h>
#include <wordexp.h>

#include <gsl/gsl_math.h>
#include <gsl/gsl_vector.h>

#include "commands/flowctrl.h"

#include "coreUtils/errorReport.h"

#include "expressions/expEval.h"
#include "expressions/traceback_fns.h"

#include "settings/settingTypes.h"
#include "settings/textConstants.h"

#include "stringTools/asciidouble.h"
#include "stringTools/strConstants.h"

#include "parser/cmdList.h"
#include "parser/parser.h"

#include "expressions/expEval.h"
#include "expressions/traceback_fns.h"

#include "userspace/context.h"
#include "userspace/contextVarDef.h"
#include "userspace/garbageCollector.h"
#include "userspace/pplObj_fns.h"
#include "userspace/pplObjFunc_fns.h"
#include "userspace/unitsArithmetic.h"
#include "userspace/unitsDisp.h"

#include "pplConstants.h"

#define TBADD(et,cont) ppl_tbAdd(c,pl->srcLineN,pl->srcId,pl->srcFname,0,et,0,pl->linetxt,cont)

#define TBADD2(et,pos) ppl_tbAdd(c,pl->srcLineN,pl->srcId,pl->srcFname,0,et,pos,pl->linetxt,"")

#define STACK_POP \
   { \
    c->stackPtr--; \
    if (c->stack[c->stackPtr].objType!=PPLOBJ_NUM) /* optimisation: Don't waste time garbage collecting numbers */ \
     { \
      ppl_garbageObject(&c->stack[c->stackPtr]); \
      if (c->stack[c->stackPtr].refCount != 0) { strcpy(c->errStat.errBuff,"Stack forward reference detected."); TBADD(ERR_INTERNAL,0); return; } \
    } \
   } \

void directive_break(ppl_context *c, parserLine *pl, parserOutput *in, int interactive, int iterDepth)
 {
  pplObj      *stk   = in->stk;
  int          named = (stk[PARSE_break_loopname].objType == PPLOBJ_STR);
  char        *name  = (char *)stk[PARSE_break_loopname].auxil;

  if (named)
   {
    int i;
    for (i=iterDepth; i>=0; i--)
     if ((c->shellLoopName[i]!=NULL)&&(strcmp(c->shellLoopName[i],name)==0))
      break;
    if (i<0) { snprintf(c->errStat.errBuff, LSTR_LENGTH, "Not inside any loop with the name '%s'.", name); TBADD2(ERR_SYNTAX, in->stkCharPos[PARSE_break_loopname]); return; }
    c->shellBreakLevel = i;
   }
  else
   {
    if (!c->shellBreakable) { snprintf(c->errStat.errBuff, LSTR_LENGTH, "The break statement can only be placed inside a loop structure."); TBADD2(ERR_SYNTAX, 0); return; }
    c->shellBreakLevel = -1;
   }

  c->shellBroken = 1;
  return;
 }

void directive_continue(ppl_context *c, parserLine *pl, parserOutput *in, int interactive, int iterDepth)
 {
  pplObj      *stk   = in->stk;
  int          named = (stk[PARSE_continue_loopname].objType == PPLOBJ_STR);
  char        *name  = (char *)stk[PARSE_continue_loopname].auxil;

  if (named)
   {
    int i;
    for (i=iterDepth; i>=0; i--)
     if ((c->shellLoopName[i]!=NULL)&&(strcmp(c->shellLoopName[i],name)==0))
      break;
    if (i<0) { snprintf(c->errStat.errBuff, LSTR_LENGTH, "Not inside any loop with the name '%s'.", name); TBADD2(ERR_SYNTAX, in->stkCharPos[PARSE_continue_loopname]); return; }
    c->shellBreakLevel = i;
   }
  else
   {
    if (!c->shellBreakable) { snprintf(c->errStat.errBuff, LSTR_LENGTH, "The continue statement can only be placed inside a loop structure."); TBADD2(ERR_SYNTAX, 0); return; }
    c->shellBreakLevel = -1;
   }

  c->shellContinued = 1;
  return;
 }

void directive_return(ppl_context *c, parserLine *pl, parserOutput *in, int interactive, int iterDepth)
 {
  pplObj *stk    = in->stk;
  pplObj *retval = &stk[PARSE_return_return_value];
  int     gotval = (retval->objType != PPLOBJ_ZOM);

  if (!c->shellReturnable) { snprintf(c->errStat.errBuff, LSTR_LENGTH, "The return statement can only be placed inside subroutines."); TBADD2(ERR_SYNTAX, 0); return; }

  c->shellBreakLevel = -1;
  c->shellReturned   =  1;

  if (gotval) pplObjCpy(&c->shellReturnVal, retval, 0, 0, 1);
  else        pplObjNum(&c->shellReturnVal, 0, 0, 0);
  return;
 }

void directive_do(ppl_context *c, parserLine *pl, parserOutput *in, int interactive, int iterDepth)
 {
  int          criterion = 1;
  pplObj      *stk  = in->stk;
  parserLine  *plc  = (parserLine *)stk[PARSE_do_code     ].auxil;
  pplExpr     *cond = (pplExpr    *)stk[PARSE_do_condition].auxil;
  char        *name = (char *)stk[PARSE_do_loopname].auxil;
  int          named= (stk[PARSE_do_loopname].objType == PPLOBJ_STR);
  ppl_context *context = c; // Needed for CAST_TO_BOOL
  int          shellBreakableOld = c->shellBreakable;
  const int    stkLevelOld = c->stackPtr;

  if (named) c->shellLoopName[iterDepth] = name;
  else       c->shellLoopName[iterDepth] = NULL;
  c->shellBreakable = 1;

  do
   {
    int lastOpAssign, dollarAllowed=1;
    pplObj *val;
    ppl_parserExecute(c, plc, interactive, iterDepth+1);
    if (c->errStat.status) { strcpy(c->errStat.errBuff,""); TBADD(ERR_GENERAL,"do loop"); goto cleanup; }
    while (c->stackPtr>stkLevelOld) { STACK_POP; }
    if ((c->shellContinued)&&((c->shellBreakLevel==iterDepth)||(c->shellBreakLevel<0))) { c->shellContinued=0; c->shellBreakLevel=0; continue; }
    if ((c->shellBroken)||(c->shellContinued)||(c->shellReturned)||(c->shellExiting)) break;
    val = ppl_expEval(context, cond, &lastOpAssign, dollarAllowed, iterDepth+1);
    if (c->errStat.status) { strcpy(c->errStat.errBuff,""); TBADD(ERR_GENERAL,"do loop stopping criterion"); goto cleanup; }
    CAST_TO_BOOL(val);
    criterion = (val->real != 0);
    while (c->stackPtr>stkLevelOld) { STACK_POP; }
   }
  while (criterion);

cleanup:
  if ((c->shellBroken)&&((c->shellBreakLevel==iterDepth)||(c->shellBreakLevel<0))) { c->shellBroken=0; c->shellBreakLevel=0; }
  c->shellLoopName[iterDepth] = NULL;
  c->shellBreakable = shellBreakableOld;
  return;
 }

void directive_for(ppl_context *c, parserLine *pl, parserOutput *in, int interactive, int iterDepth)
 {
  pplObj      *stk       = in->stk;
  int          BASICstyle= (stk[PARSE_for_var_name].objType == PPLOBJ_STR);
  char        *varname   = (char *)stk[PARSE_for_var_name].auxil;
  pplObj      *beginVal  = &stk[PARSE_for_start_value];
  pplObj      *endVal    = &stk[PARSE_for_final_value];
  pplObj      *stepVal   = &stk[PARSE_for_step_size], stepTmp;
  int          stepGot   = (stk[PARSE_for_step_size].objType == PPLOBJ_NUM);
  parserLine  *plc       = (parserLine *)stk[PARSE_for_code       ].auxil;
  pplExpr     *begin     = (pplExpr    *)stk[PARSE_for_begin      ].auxil;
  int          beginGot  = (stk[PARSE_for_begin    ].objType == PPLOBJ_EXP);
  pplExpr     *cond      = (pplExpr    *)stk[PARSE_for_criterion  ].auxil;
  int          condGot   = (stk[PARSE_for_criterion].objType == PPLOBJ_EXP);
  pplExpr     *end       = (pplExpr    *)stk[PARSE_for_iterate    ].auxil;
  int          endGot    = (stk[PARSE_for_iterate  ].objType == PPLOBJ_EXP);
  char        *name      = (char *)stk[PARSE_for_loopname].auxil;
  int          named     = (stk[PARSE_for_loopname].objType == PPLOBJ_STR);
  ppl_context *context   = c; // Needed for CAST_TO_BOOL
  int          shellBreakableOld = c->shellBreakable;
  const int    stkLevelOld = c->stackPtr;

  if (named) c->shellLoopName[iterDepth] = name;
  else       c->shellLoopName[iterDepth] = NULL;
  c->shellBreakable = 1;

  if (BASICstyle)
   {
    double iter;
    int backwards = endVal->real > beginVal->real;

    if (!stepGot) // If no step is specified, make it equal to one SI unit
     {
      memcpy(&stepTmp, beginVal, sizeof(pplObj));
      stepTmp.refCount=1;
      stepTmp.amMalloced=0;
      stepTmp.real=1.0;
      stepVal = &stepTmp;
     }
    if (!ppl_unitsDimEqual(beginVal , endVal )) { sprintf(c->errStat.errBuff,"The start and end values of this for loop are dimensionally incompatible: the start value has dimensions of <%s> but the end value has dimensions of <%s>.",ppl_printUnit(c,beginVal,NULL,NULL,0,1,0),ppl_printUnit(c,endVal,NULL,NULL,1,1,0)); TBADD2(ERR_NUMERIC,in->stkCharPos[PARSE_for_final_value]); goto cleanup; }
    if (!ppl_unitsDimEqual(beginVal , stepVal)) { sprintf(c->errStat.errBuff,"The start value and step size of this for loop are dimensionally incompatible: the start value has dimensions of <%s> but the step size has dimensions of <%s>.",ppl_printUnit(c,beginVal,NULL,NULL,0,1,0),ppl_printUnit(c,stepVal,NULL,NULL,1,1,0)); TBADD2(ERR_NUMERIC,in->stkCharPos[PARSE_for_start_value]); goto cleanup; }
    if ( (stepVal->real==0) || ((stepVal->real > 0) != backwards) ) { strcpy(c->errStat.errBuff,"The number of iterations in this for loop is infinite."); TBADD2(ERR_NUMERIC,in->stkCharPos[PARSE_for_start_value]); goto cleanup; }

    for (iter=beginVal->real; iter<=endVal->real; iter+=stepVal->real)
     {
      pplObj *varObj, vartmp;
      ppl_contextGetVarPointer(c, varname, &varObj, &vartmp);
      pplObjNum(varObj, varObj->amMalloced, iter, 0);
      ppl_unitsDimCpy(varObj, beginVal);
      ppl_parserExecute(c, plc, interactive, iterDepth+1);
      ppl_contextRestoreVarPointer(c, varname, &vartmp);
      if (c->errStat.status) { strcpy(c->errStat.errBuff,""); TBADD(ERR_GENERAL,"for loop"); goto cleanup; }
      while (c->stackPtr>stkLevelOld) { STACK_POP; }
      if ((c->shellContinued)&&((c->shellBreakLevel==iterDepth)||(c->shellBreakLevel<0))) { c->shellContinued=0; c->shellBreakLevel=0; continue; }
      if ((c->shellBroken)||(c->shellContinued)||(c->shellReturned)||(c->shellExiting)) break;
     }
   }
  else
   {
    if (beginGot)
     {
      int lastOpAssign, dollarAllowed=1;
      pplObj *val;
      val = ppl_expEval(context, begin, &lastOpAssign, dollarAllowed, iterDepth+1);
      if (c->errStat.status) { strcpy(c->errStat.errBuff,""); TBADD(ERR_GENERAL,"for loop initialisation expression"); goto cleanup; }
      while (c->stackPtr>stkLevelOld) { STACK_POP; }
     }
    while (1)
     {
      if (condGot)
       {
        int lastOpAssign, dollarAllowed=1, criterion=1;
        pplObj *val;
        val = ppl_expEval(context, cond, &lastOpAssign, dollarAllowed, iterDepth+1);
        if (c->errStat.status) { strcpy(c->errStat.errBuff,""); TBADD(ERR_GENERAL,"for loop stopping criterion"); goto cleanup; }
        CAST_TO_BOOL(val);
        criterion = (val->real != 0);
        while (c->stackPtr>stkLevelOld) { STACK_POP; }
        if (!criterion) break;
       }
      ppl_parserExecute(c, plc, interactive, iterDepth+1);
      if (c->errStat.status) { strcpy(c->errStat.errBuff,""); TBADD(ERR_GENERAL,"for loop"); goto cleanup; }
      while (c->stackPtr>stkLevelOld) { STACK_POP; }
      if ((c->shellContinued)&&((c->shellBreakLevel==iterDepth)||(c->shellBreakLevel<0))) { c->shellContinued=0; c->shellBreakLevel=0; continue; }
      if ((c->shellBroken)||(c->shellContinued)||(c->shellReturned)||(c->shellExiting)) break;
      if (endGot)
       {
        int lastOpAssign, dollarAllowed=1;
        pplObj *val;
        val = ppl_expEval(context, end, &lastOpAssign, dollarAllowed, iterDepth+1);
        if (c->errStat.status) { strcpy(c->errStat.errBuff,""); TBADD(ERR_GENERAL,"for loop stopping criterion"); goto cleanup; }
        while (c->stackPtr>stkLevelOld) { STACK_POP; }
       }
     }
   }

cleanup:
  if ((c->shellBroken)&&((c->shellBreakLevel==iterDepth)||(c->shellBreakLevel<0))) { c->shellBroken=0; c->shellBreakLevel=0; }
  c->shellLoopName[iterDepth] = NULL;
  c->shellBreakable = shellBreakableOld;
  return;
 }

void directive_foreach(ppl_context *c, parserLine *pl, parserOutput *in, int interactive, int iterDepth)
 {
  pplObj      *stk     = in->stk;
  pplObj      *iter    = &stk[PARSE_foreach_item_list];
  parserLine  *plc     = (parserLine *)stk[PARSE_foreach_code].auxil;
  char        *varname = (char *)stk[PARSE_foreach_var_name].auxil;
  char        *name    = (char *)stk[PARSE_foreach_loopname].auxil;
  int          named   = (stk[PARSE_foreach_loopname].objType == PPLOBJ_STR);
  int          shellBreakableOld = c->shellBreakable;
  const int    stkLevelOld = c->stackPtr;

  if (named) c->shellLoopName[iterDepth] = name;
  else       c->shellLoopName[iterDepth] = NULL;
  c->shellBreakable = 1;

#define FOREACH_STOP ( c->errStat.status || c->shellBroken || c->shellContinued || c->shellReturned || c->shellExiting )

  if (iter->objType == PPLOBJ_STR) // Iterate over string: treat this as a filename to glob
   {
    int i,j;
    wordexp_t w;
    glob_t    g;
    if (wordexp((char *)iter->auxil, &w, 0) != 0) goto cleanup; // No matches
    for (i=0; i<w.we_wordc && !FOREACH_STOP; i++)
     {
      if (glob(w.we_wordv[i], 0, NULL, &g) != 0) continue;
      for (j=0; j<g.gl_pathc && !FOREACH_STOP ; j++)
       {
        pplObj *varObj, vartmp;
        ppl_contextGetVarPointer(c, varname, &varObj, &vartmp);
        pplObjStr(varObj, varObj->amMalloced, 0, g.gl_pathv[j]);
        ppl_parserExecute(c, plc, interactive, iterDepth+1);
        ppl_contextRestoreVarPointer(c, varname, &vartmp);
        if (c->errStat.status) { strcpy(c->errStat.errBuff,""); TBADD(ERR_GENERAL,"foreach loop"); }
        while (c->stackPtr>stkLevelOld) { STACK_POP; }
        if ((c->shellContinued)&&((c->shellBreakLevel==iterDepth)||(c->shellBreakLevel<0))) { c->shellContinued=0; c->shellBreakLevel=0; continue; }
       }
      globfree(&g);
     }
    wordfree(&w);
   }
  else if (iter->objType == PPLOBJ_VEC) // Iterate over vector
   {
    gsl_vector *vin = ((pplVector *)iter->auxil)->v;
    const int   vinl = vin->size;
    int         i;
    for (i=0; i<vinl; i++)
     {
      pplObj *varObj, vartmp;
      ppl_contextGetVarPointer(c, varname, &varObj, &vartmp);
      pplObjNum(varObj, varObj->amMalloced, gsl_vector_get(vin, i), 0);
      ppl_unitsDimCpy(varObj, iter);
      ppl_parserExecute(c, plc, interactive, iterDepth+1);
      ppl_contextRestoreVarPointer(c, varname, &vartmp);
      if (c->errStat.status) { strcpy(c->errStat.errBuff,""); TBADD(ERR_GENERAL,"foreach loop"); }
      while (c->stackPtr>stkLevelOld) { STACK_POP; }
      if ((c->shellContinued)&&((c->shellBreakLevel==iterDepth)||(c->shellBreakLevel<0))) { c->shellContinued=0; c->shellBreakLevel=0; continue; }
      if ((c->shellBroken)||(c->shellContinued)||(c->shellReturned)||(c->shellExiting)) break;
     }
   }
  else if (iter->objType == PPLOBJ_LIST) // Iterate over list
   {
    list         *lin = (list *)iter->auxil;
    listIterator *li  = ppl_listIterateInit(lin);
    pplObj       *obj;
    while ((obj=(pplObj*)ppl_listIterate(&li))!=NULL)
     {
      pplObj *varObj, vartmp;
      ppl_contextGetVarPointer(c, varname, &varObj, &vartmp);
      pplObjCpy(varObj, obj, 0, varObj->amMalloced, 1);
      ppl_parserExecute(c, plc, interactive, iterDepth+1);
      ppl_contextRestoreVarPointer(c, varname, &vartmp);
      if (c->errStat.status) { strcpy(c->errStat.errBuff,""); TBADD(ERR_GENERAL,"foreach loop"); }
      while (c->stackPtr>stkLevelOld) { STACK_POP; }
      if ((c->shellContinued)&&((c->shellBreakLevel==iterDepth)||(c->shellBreakLevel<0))) { c->shellContinued=0; c->shellBreakLevel=0; continue; }
      if ((c->shellBroken)||(c->shellContinued)||(c->shellReturned)||(c->shellExiting)) break;
     }
   }
  else
   {
    sprintf(c->errStat.errBuff,"Cannot iterate over an object of type <%s>.",pplObjTypeNames[iter->objType]);
    TBADD2(ERR_TYPE,in->stkCharPos[PARSE_foreach_item_list]);
    goto cleanup;
   }

cleanup:
  if ((c->shellBroken)&&((c->shellBreakLevel==iterDepth)||(c->shellBreakLevel<0))) { c->shellBroken=0; c->shellBreakLevel=0; }
  c->shellLoopName[iterDepth] = NULL;
  c->shellBreakable = shellBreakableOld;
  return;
 }

void directive_fordata(ppl_context *c, parserLine *pl, parserOutput *in, int interactive, int iterDepth)
 {
  return;
 }

void directive_if(ppl_context *c, parserLine *pl, parserOutput *in, int interactive, int iterDepth)
 {
  pplObj      *stk     = in->stk;
  parserLine  *pl_if   = (parserLine *)stk[PARSE_if_code     ].auxil;
  parserLine  *pl_else = (parserLine *)stk[PARSE_if_code_else].auxil;
  int          gotelse = (stk[PARSE_if_code_else].objType==PPLOBJ_BYT);
  pplExpr     *cond    = (pplExpr    *)stk[PARSE_if_criterion].auxil;
  ppl_context *context = c; // Needed for CAST_TO_BOOL
  pplObj      *val;
  int          lastOpAssign, dollarAllowed=1, criterion;
  int          pos = PARSE_if_0elifs;
  const int    stkLevelOld = c->stackPtr;

  // Test criterion
  val = ppl_expEval(context, cond, &lastOpAssign, dollarAllowed, iterDepth+1);
  if (c->errStat.status) { strcpy(c->errStat.errBuff,""); TBADD(ERR_GENERAL,"if criterion"); return; }
  CAST_TO_BOOL(val);
  criterion = (val->real != 0);
  while (c->stackPtr>stkLevelOld) { STACK_POP; }
  if (criterion)
   {
    ppl_parserExecute(c, pl_if, interactive, iterDepth+1);
    if (c->errStat.status) { strcpy(c->errStat.errBuff,""); TBADD(ERR_GENERAL,"if statement"); return; }
    while (c->stackPtr>stkLevelOld) { STACK_POP; }
    return;
   }

  // loop over else ifs
  while (stk[pos].objType == PPLOBJ_NUM)
   {
    parserLine  *pl_elif   = NULL;
    pplExpr     *cond_elif = NULL;
    pplObj      *val;
    int          lastOpAssign, dollarAllowed=1, criterion;
    pos       = (int)round(stk[pos].real);
    pl_elif   = (parserLine *)stk[pos+PARSE_if_code_elif_0elifs].auxil;
    cond_elif = (pplExpr    *)stk[pos+PARSE_if_criterion_0elifs].auxil;
    if (pos<=0) break;
    val = ppl_expEval(context, cond_elif, &lastOpAssign, dollarAllowed, iterDepth+1);
    if (c->errStat.status) { strcpy(c->errStat.errBuff,""); TBADD(ERR_GENERAL,"else if criterion"); return; }
    CAST_TO_BOOL(val);
    criterion = (val->real != 0);
    while (c->stackPtr>stkLevelOld) { STACK_POP; }
    if (criterion)
     {
      ppl_parserExecute(c, pl_elif, interactive, iterDepth+1);
      if (c->errStat.status) { strcpy(c->errStat.errBuff,""); TBADD(ERR_GENERAL,"if statement"); return; }
      while (c->stackPtr>stkLevelOld) { STACK_POP; }
      return;
     }
   }

  // else
  if (gotelse)
   {
    ppl_parserExecute(c, pl_else, interactive, iterDepth+1);
    if (c->errStat.status) { strcpy(c->errStat.errBuff,""); TBADD(ERR_GENERAL,"if statement"); return; }
    while (c->stackPtr>stkLevelOld) { STACK_POP; }
    return;
   }
  return;
 }

void directive_subrt(ppl_context *c, parserLine *pl, parserOutput *in, int interactive, int iterDepth)
 {
  pplObj     *stk   = in->stk;
  parserLine *plsub = (parserLine *)stk[PARSE_subroutine_code     ].auxil;
  char       *name  = (char *)stk[PARSE_subroutine_subroutine_name].auxil;
  int         pos, nArgs=0, argListLen=0, i;
  pplFunc    *f     = NULL;
  pplObj     *fnObj, tmpObj;

  // Count number of arguments
  pos = PARSE_subroutine_0argument_list;
  while ((stk[pos].objType == PPLOBJ_NUM) && (stk[pos].real > 0))
   {
    char *name;
    pos = (int)round(stk[pos].real);
    name = (char *)stk[pos+PARSE_subroutine_argument_name_0argument_list].auxil;
    nArgs++;
    argListLen+=strlen(name)+1;
   }
  if (nArgs > FUNC_MAXARGS)
   {
    sprintf(c->errStat.errBuff, "Too many arguments to function; the maximum allowed number is %d.", FUNC_MAXARGS);
    TBADD(ERR_OVERFLOW,0);
    return;
   }

  // Allocate function descriptor
  f = (pplFunc *)malloc(sizeof(pplFunc));
  if (f==NULL) { sprintf(c->errStat.errBuff, "Out of memory."); TBADD(ERR_MEMORY,0); return; }
  f->functionType = PPL_FUNC_SUBROUTINE;
  f->refCount     = 1;
  f->minArgs      = nArgs;
  f->maxArgs      = nArgs;
  f->functionPtr  = (void *)plsub;
  f->argList      = (char *)malloc(argListLen);
  f->min          = NULL;
  f->max          = NULL;
  f->minActive    = NULL;
  f->maxActive    = NULL;
  f->numOnly      = 0;
  f->notNan       = 0;
  f->realOnly     = 0;
  f->dimlessOnly  = 0;
  f->next         = NULL;
  f->LaTeX        = NULL;
  f->description  = NULL;
  f->descriptionShort = NULL;
  if (f->argList==NULL) { sprintf(c->errStat.errBuff, "Out of memory."); TBADD(ERR_MEMORY,0); goto fail; }

  // Put argument names into structure
  pos  = PARSE_subroutine_0argument_list;
  i    = 0;
  while ((stk[pos].objType == PPLOBJ_NUM) && (stk[pos].real > 0))
   {
    char *name;
    pos  = (int)round(stk[pos].real);
    name = (char *)stk[pos+PARSE_subroutine_argument_name_0argument_list].auxil;
    strcpy(f->argList+i, name);
    i+= strlen(name)+1;
   }

  // Look up function or variable we are superseding
  ppl_contextGetVarPointer(c, name, &fnObj, &tmpObj);
  tmpObj.amMalloced=0;
  ppl_garbageObject(&tmpObj);
  pplObjFunc(fnObj,fnObj->amMalloced,1,f);
  __sync_add_and_fetch(&plsub->refCount,1);
  return;

fail:
  if (f!=NULL)
   {
    if (f->argList != NULL) free(f->argList);
    free(f);
   }
  return;
 }

void directive_while(ppl_context *c, parserLine *pl, parserOutput *in, int interactive, int iterDepth)
 {
  int          criterion = 1;
  pplObj      *stk  = in->stk;
  parserLine  *plc  = (parserLine *)stk[PARSE_while_code     ].auxil;
  pplExpr     *cond = (pplExpr    *)stk[PARSE_while_criterion].auxil;
  char        *name = (char *)stk[PARSE_while_loopname].auxil;
  int          named= (stk[PARSE_while_loopname].objType == PPLOBJ_STR);
  ppl_context *context = c; // Needed for CAST_TO_BOOL
  int          shellBreakableOld = c->shellBreakable;
  const int    stkLevelOld = c->stackPtr;

  if (named) c->shellLoopName[iterDepth] = name;
  else       c->shellLoopName[iterDepth] = NULL;
  c->shellBreakable = 1;

  while (1)
   {
    int lastOpAssign, dollarAllowed=1;
    pplObj *val;
    val = ppl_expEval(context, cond, &lastOpAssign, dollarAllowed, iterDepth+1);
    if (c->errStat.status) { strcpy(c->errStat.errBuff,""); TBADD(ERR_GENERAL,"while loop stopping criterion"); goto cleanup; }
    CAST_TO_BOOL(val);
    criterion = (val->real != 0);
    while (c->stackPtr>stkLevelOld) { STACK_POP; }
    if (!criterion) break;
    ppl_parserExecute(c, plc, interactive, iterDepth+1);
    if (c->errStat.status) { strcpy(c->errStat.errBuff,""); TBADD(ERR_GENERAL,"while loop"); goto cleanup; }
    while (c->stackPtr>stkLevelOld) { STACK_POP; }
    if ((c->shellContinued)&&((c->shellBreakLevel==iterDepth)||(c->shellBreakLevel<0))) { c->shellContinued=0; c->shellBreakLevel=0; continue; }
    if ((c->shellBroken)||(c->shellContinued)||(c->shellReturned)||(c->shellExiting)) break;
   }

cleanup:
  if ((c->shellBroken)&&((c->shellBreakLevel==iterDepth)||(c->shellBreakLevel<0))) { c->shellBroken=0; c->shellBreakLevel=0; }
  c->shellLoopName[iterDepth] = NULL;
  c->shellBreakable = shellBreakableOld;
  return;
 }

