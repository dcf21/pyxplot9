// funcset.c
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <unistd.h>

#include "stringTools/asciidouble.h"
#include "stringTools/strConstants.h"

#include "coreUtils/dict.h"
#include "coreUtils/errorReport.h"

#include "expressions/traceback_fns.h"

#include "parser/cmdList.h"
#include "parser/parser.h"

#include "userspace/context.h"
#include "userspace/contextVarDef.h"
#include "userspace/garbageCollector.h"
#include "userspace/pplObj.h"
#include "userspace/pplObj_fns.h"
#include "userspace/pplObjFunc_fns.h"
#include "userspace/unitsArithmetic.h"
#include "userspace/unitsDisp.h"

#include "pplConstants.h"

#define TBADD(et,charpos) ppl_tbAdd(c,pl->srcLineN,pl->srcId,pl->srcFname,0,et,charpos,pl->linetxt,"")

void directive_funcset(ppl_context *c, parserLine *pl, parserOutput *in, int interactive)
 {
  pplObj  *stk = in->stk;
  pplFunc *f   = NULL;
  int      i, j, nArgs=0, pos, posR, argListLen=0;
  int      needToStoreNan=0, nullDefn=0;
  char    *fnName = (char *)stk[PARSE_func_set_function_name].auxil;
  pplObj  *fnObj, tmpObj;

  // Count number of arguments
  pos = PARSE_func_set_0argument_list;
  while ((stk[pos].objType == PPLOBJ_NUM) && (stk[pos].real > 0))
   {
    char *name;
    pos = (int)round(stk[pos].real);
    name = (char *)stk[pos+PARSE_func_set_argument_name_0argument_list].auxil;
    nArgs++;
    argListLen+=strlen(name)+1;
   }
  if (nArgs > FUNC_MAXARGS)
   {
    sprintf(c->errStat.errBuff, "Too many arguments to function; the maximum allowed number is %d.", FUNC_MAXARGS);
    TBADD(ERR_OVERFLOW,0);
    return;
   }

  // Are we defining or undefining this function?
  nullDefn = (stk[PARSE_func_set_definition].objType != PPLOBJ_EXP);

  // Allocate function descriptor
  f = (pplFunc *)malloc(sizeof(pplFunc));
  if (f==NULL) { sprintf(c->errStat.errBuff, "Out of memory."); TBADD(ERR_MEMORY,0); return; }
  f->functionType = PPL_FUNC_USERDEF;
  f->refCount     = 1;
  f->minArgs      = nArgs;
  f->maxArgs      = nArgs;
  f->functionPtr  = nullDefn ? NULL : pplExpr_cpy((pplExpr *)stk[PARSE_func_set_definition].auxil);
  f->argList      = (char    *)malloc(argListLen);
  f->min          = (pplObj  *)malloc(nArgs*sizeof(pplObj));
  f->max          = (pplObj  *)malloc(nArgs*sizeof(pplObj));
  f->minActive    = (unsigned char *)malloc(nArgs);
  f->maxActive    = (unsigned char *)malloc(nArgs);
  f->numOnly      = 0;
  f->notNan       = 0;
  f->realOnly     = 0;
  f->dimlessOnly  = 0;
  f->next         = NULL;
  f->LaTeX        = NULL;
  f->description  = NULL;
  f->descriptionShort = NULL;
  if ((f->argList==NULL)||((!nullDefn)&&(f->functionPtr==NULL))||(f->min==NULL)||(f->max==NULL)||(f->minActive==NULL)||(f->maxActive==NULL)) { sprintf(c->errStat.errBuff, "Out of memory."); TBADD(ERR_MEMORY,0); goto fail; }

  // Put ranges and argument names into structures
  pos  = PARSE_func_set_0argument_list;
  posR = PARSE_func_set_0range_list;
  i    = 0;
  j    = 0;
  while ((stk[pos].objType == PPLOBJ_NUM) && (stk[pos].real > 0))
   {
    char *name; 
    pos  = (int)round(stk[pos].real);
    name = (char *)stk[pos+PARSE_func_set_argument_name_0argument_list].auxil;
    strcpy(f->argList+i, name);
    i+= strlen(name)+1;
    if ((stk[posR].objType == PPLOBJ_NUM) && (stk[posR].real > 0))
     {
      pplObj *min, *max;
      posR = (int)round(stk[posR].real);
      min  = &stk[posR+PARSE_func_set_min_0range_list];
      max  = &stk[posR+PARSE_func_set_max_0range_list];

      // Check that the specified minimum is not complex
      if (min->objType != PPLOBJ_NUM)
       {
        pplObjZom(f->min+j,0);
        f->minActive[j]=0;
       }
      else
       {
        if (min->flagComplex) { sprintf(c->errStat.errBuff, "Where ranges are specified for function arguments, these must be real numbers. Ranges may not be imposed upon complex arguments to functions."); TBADD(ERR_NUMERIC,in->stkCharPos[posR+PARSE_func_set_min_0range_list]); goto fail; }
        pplObjCpy(f->min+j,min,0,0,1);
        f->minActive[j]=1;
       }

      // Check that specified maximum is not complex, and does not conflict with units of minimum
      if (max->objType != PPLOBJ_NUM)
       {
        pplObjZom(f->max+j,0);
        f->maxActive[j]=0;
       }
      else
       {
        if (max->flagComplex) { sprintf(c->errStat.errBuff, "Where ranges are specified for function arguments, these must be real numbers. Ranges may not be imposed upon complex arguments to functions."); TBADD(ERR_NUMERIC,in->stkCharPos[posR+PARSE_func_set_max_0range_list]); goto fail; }
        if (f->minActive[j] && !ppl_unitsDimEqual(min,max)) { sprintf(c->errStat.errBuff, "The minimum and maximum values specified for argument number %d are dimensionally incompatible: the minimum has dimensions of <%s>, while the maximum has dimensions of <%s>.",j+1,ppl_printUnit(c,min,NULL,NULL,0,1,0),ppl_printUnit(c,min,NULL,NULL,1,1,0)); TBADD(ERR_NUMERIC,in->stkCharPos[posR+PARSE_func_set_max_0range_list]); goto fail; }
        pplObjCpy(f->max+j,max,0,0,1);
        f->maxActive[j]=1;
       }

      // If maximum and minimum are the wrong way around, swap them
      if (f->minActive[j] && f->maxActive[j] && (min->real > max->real))
       {
        double tmp = f->max[j].real;
        f->max[j].real = f->min[j].real;
        f->min[j].real = tmp;
       }
     }
    else
     {
      f->minActive[j] = f->maxActive[j] = 0;
     }
    j++;
   }

  // Look up function or variable we are superseding
  ppl_contextGetVarPointer(c, fnName, &fnObj, &tmpObj);

  // If not a function, we delete it and supersede it
  if ( (tmpObj.objType!=PPLOBJ_FUNC) || (((pplFunc*)tmpObj.auxil)->functionType!=PPL_FUNC_USERDEF) || (((pplFunc*)tmpObj.auxil)->minArgs!=nArgs) )
   goto supersede;

  // If old function has dimensionally incompatible limits with us, we cannot splice with it; supersede it
  {
   int k;
   pplFunc *fIter;
   for (fIter = (pplFunc*)tmpObj.auxil ; fIter!=NULL ; fIter=fIter->next)
    for (k=0; k<nArgs; k++)
     if ( ((f->minActive[k])&&(fIter->minActive[k])&&(!ppl_unitsDimEqual(f->min+k , fIter->min+k))) ||
          ((f->minActive[k])&&(fIter->maxActive[k])&&(!ppl_unitsDimEqual(f->min+k , fIter->max+k))) ||
          ((f->maxActive[k])&&(fIter->minActive[k])&&(!ppl_unitsDimEqual(f->max+k , fIter->min+k))) ||
          ((f->maxActive[k])&&(fIter->maxActive[k])&&(!ppl_unitsDimEqual(f->max+k , fIter->max+k)))    )
      goto supersede;
  }

  // If old function has ranges which extend outside our range, we should not supersede it
  {
   pplFunc **fIterPtr = (pplFunc **)&tmpObj.auxil;
   pplFunc  *fIter    = *fIterPtr;
   for ( ; fIter!=NULL ; )
    {
     int k, Nsupersede=0, Noverlap=0, Nmiss=0, lastOverlapType=0, lastOverlapK=0;
     for (k=0; k<nArgs; k++) // Looping over all dimensions, count how new ranges miss/overlap/supersede old ranges
      {
       if      ( ((!f->minActive[k])|| (( fIter->minActive[k])&&(fIter->min[k].real>=f->min[k].real))) &&
                 ((!f->maxActive[k])|| (( fIter->maxActive[k])&&(fIter->max[k].real<=f->max[k].real)))    ) Nsupersede++; // New min/max range completely encompasses old
       else if ( (( f->minActive[k])&&  ( fIter->maxActive[k])&&(fIter->max[k].real<=f->min[k].real) ) ||
                 (( f->maxActive[k])&&  ( fIter->minActive[k])&&(fIter->min[k].real>=f->max[k].real) )    ) Nmiss++; // New min/max range completely outside old
       else if (  ( f->minActive[k])&& ((!fIter->minActive[k])||(fIter->min[k].real< f->min[k].real)) &&
                  ( f->maxActive[k])&& ((!fIter->maxActive[k])||(fIter->max[k].real> f->max[k].real))     )  { Noverlap++; lastOverlapType = 2; lastOverlapK = k; } // New range in middle of old
       else if (  ( f->minActive[k])&&((( fIter->maxActive[k])&&(fIter->max[k].real> f->min[k].real)  &&
                                       ((!f    ->maxActive[k])||(fIter->max[k].real<=f->max[k].real)))||
                                       ((!fIter->maxActive[k])&&(!f->maxActive[k]                  ))    )) { Noverlap++; lastOverlapType = 3; lastOverlapK = k; } // New range goes off top of old
       else if (  ( f->maxActive[k])&&((( fIter->minActive[k])&&(fIter->min[k].real< f->max[k].real)  &&
                                       ((!f    ->minActive[k])||(fIter->min[k].real>=f->min[k].real)))||
                                       ((!fIter->minActive[k])&&(!f->minActive[k]                  ))    )) { Noverlap++; lastOverlapType = 1; lastOverlapK = k; } // New range goes off bottom of old
       else  ppl_fatal(&c->errcontext,__FILE__,__LINE__,"Could not work out how the ranges of two functions overlap");
      }

     if ((Nmiss==0) && (Noverlap==0)) // Remove entry from linked list; we supersede its range on all axes
      {
       *fIterPtr = fIter->next;
       pplObjFuncDestroy(fIter);
       fIter = *fIterPtr;
       continue;
      }
     else if ((Nmiss==0) && (Noverlap==1)) // we should reduce the range of the function we overlap with
      {
       if      (lastOverlapType == 1) // Bring lower limit of old definition up above maximum for this new definition
        { fIter->min[lastOverlapK] = f->max[lastOverlapK]; fIter->minActive[lastOverlapK] = 1; }
       else if (lastOverlapType == 3) // Bring upper limit of old definition down below minimum for this new definition
        { fIter->max[lastOverlapK] = f->min[lastOverlapK]; fIter->maxActive[lastOverlapK] = 1; }
       else // Old definition is cut in two by the new definition; duplicate it.
        {
         pplFunc *dup=pplObjFuncCpy(fIter);
         fIter->max[lastOverlapK]        = f->min[lastOverlapK];
         fIter->maxActive[lastOverlapK]  = 1;
         dup  ->min[lastOverlapK]        = f->max[lastOverlapK];
         dup  ->minActive[lastOverlapK]  = 1;
         dup  ->next                     = fIter->next;
         fIter->next                     = dup;
        }
      }
     else if (nullDefn && (Nmiss==0) && (Noverlap>1)) // We are undefining the function, but overlap is complicated. Best we can do is store undefinition.
      {
       needToStoreNan = 1;
      }

     // Move along linked list
     fIterPtr = &fIter->next;
     fIter    = fIter->next;
    }
  }

  // If definition is null, only insert it if it has complicated overlap
  if (nullDefn && !needToStoreNan)
   {
    if (tmpObj.auxil==NULL) pplObjZom(&tmpObj,1);
    ppl_contextRestoreVarPointer(c, fnName, &tmpObj);
    return;
   }

  // Add new function definition to chain of other definitions
  f->next      = (pplFunc *)tmpObj.auxil;
  tmpObj.auxil = (void *)f;
  ppl_contextRestoreVarPointer(c, fnName, &tmpObj);
  return;

supersede:
  tmpObj.amMalloced=0;
  ppl_garbageObject(&tmpObj);
  pplObjFunc(fnObj,fnObj->amMalloced,1,f);
  //pplObjZom(&stk[PARSE_func_set_definition],0); // Don't free the expression we've stored as a variable
  return;

fail:
  if (f!=NULL)
   {
    if (f->argList     != NULL) free(f->argList);
    if (f->min         != NULL) free(f->min);
    if (f->max         != NULL) free(f->max);
    if (f->minActive   != NULL) free(f->minActive);
    if (f->maxActive   != NULL) free(f->maxActive);
    if (f->functionPtr != NULL) pplExpr_free((pplExpr *)f->functionPtr);
    free(f);
   }
  return;
 }

