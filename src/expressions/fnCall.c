// fnCall.c
//
// The code in this file is part of PyXPlot
// <http://www.pyxplot.org.uk>
//
// Copyright (C) 2006-2012 Dominic Ford <coders@pyxplot.org.uk>
//               2008-2011 Ross Church
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

#define _FNCALL_C 1

#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <limits.h>

#include <gsl/gsl_math.h>

#include "coreUtils/dict.h"
#include "coreUtils/errorReport.h"
#include "coreUtils/list.h"
#include "expressions/expEval.h"
#include "settings/settingTypes.h"
#include "stringTools/asciidouble.h"
#include "userspace/context.h"
#include "userspace/garbageCollector.h"
#include "userspace/pplObj_fns.h"
#include "userspace/pplObjFunc.h"
#include "userspace/pplObjPrint.h"
#include "userspace/unitsArithmetic.h"
#include "userspace/unitsDisp.h"

#include "pplConstants.h"

void ppl_fnCall(ppl_context *context, int nArgs, int charpos, int IterDepth, int *status, int *errPos, int *errType, char *errText)
 {
  pplObj  *out  = &context->stack[context->stackPtr-1-nArgs];
  pplObj  *args = context->stack+context->stackPtr-nArgs;
  pplObj   called;
  pplFunc *fn;
  int      i;
  int      t = out->objType;
  if (context->stackPtr<nArgs+1) { *status=1; *errPos=charpos; *errType=ERR_INTERNAL; sprintf(errText,"Attempt to call function with few items on the stack."); return; }
  memcpy(&called, out, sizeof(pplObj));
  pplObjNum(&context->stack[context->stackPtr-1-nArgs] , 0 , 0.0 , 0.0); // Overwrite function object on stack, but don't garbage collect it yet

  // Attempt to call a module to general a class instance?
  if ((t == PPLOBJ_MOD) || (t == PPLOBJ_USER))
   {

   }

  // Attempt to call a type object as a constructor?
  else if (t == PPLOBJ_TYPE)
   {

   }

  // Otherwise object being called must be a function object
  if (t != PPLOBJ_FUNC) { *status=1; *errPos=charpos; *errType=ERR_TYPE; sprintf(errText,"Object of type <%s> cannot be called as a function.",pplObjTypeNames[t]); goto cleanup; }
  fn = (pplFunc *)called.auxil;

  if (fn->functionType==PPL_FUNC_MAGIC)
   {
    if (fn->minArgs==1) // unit()
     {
      int     end;
      char   *u = (char*)context->stack[context->stackPtr-1].auxil;
      pplObj *o = &context->stack[context->stackPtr-1-nArgs];
      if (nArgs != 1) { *status=1; *errPos=charpos; *errType=ERR_TYPE; sprintf(errText,"The unit() function takes exactly one argument; %d supplied.",nArgs); goto cleanup; }
      ppl_unitsStringEvaluate(context, u, o, &end, errPos, errText);
      if (*errPos>=0) { *errType=ERR_UNIT; goto cleanup; }
      if (end!=strlen(u)) { *errType=ERR_UNIT; *errPos=charpos; sprintf(errText,"Unexpected trailing matter after unit string."); goto cleanup; }
     }
    else if (fn->minArgs==2) // diff_d()
     {
     }
    else if (fn->minArgs==3) // int_d()
     {
     }
    else if (fn->minArgs==4) // texify()
     {
     }
   }
  else
   {
    if (fn->minArgs == fn->maxArgs)
     {
      if (nArgs != fn->maxArgs) 
       {
        if (context->set->term_current.ExplicitErrors == SW_ONOFF_ON) { *status=1; *errPos=charpos; *errType=ERR_TYPE; sprintf(errText,"Function takes exactly %d arguments; %d supplied.",fn->maxArgs,nArgs); goto cleanup; }
        else                                                          { context->stack[context->stackPtr-1-nArgs].real = GSL_NAN; goto cleanup; }
       }
     }
    else if (nArgs < fn->minArgs) 
     {
      if (context->set->term_current.ExplicitErrors == SW_ONOFF_ON) { *status=1; *errPos=charpos; *errType=ERR_TYPE; sprintf(errText,"Function takes a minimum of %d arguments; %d supplied.",fn->minArgs,nArgs); goto cleanup; }
      else                                                          { context->stack[context->stackPtr-1-nArgs].real = GSL_NAN; goto cleanup; }
     }
    else if (nArgs > fn->maxArgs) 
     {
      if (context->set->term_current.ExplicitErrors == SW_ONOFF_ON) { *status=1; *errPos=charpos; *errType=ERR_TYPE; sprintf(errText,"Function takes a maximum of %d arguments; %d supplied.",fn->maxArgs,nArgs); goto cleanup; }
      else                                                          { context->stack[context->stackPtr-1-nArgs].real = GSL_NAN; goto cleanup; }
     }

    if (fn->numOnly)
     for (i=0; i<nArgs; i++) if (args[i].objType!=PPLOBJ_NUM) 
      {
       if (context->set->term_current.ExplicitErrors == SW_ONOFF_ON) { *status=1; *errPos=charpos; *errType=ERR_TYPE; sprintf(errText,"Function required numeric arguments; argument %d has type <%s>.",i+1,pplObjTypeNames[args[i].objType]); goto cleanup; }
       else                                                          { context->stack[context->stackPtr-1-nArgs].real = GSL_NAN; goto cleanup; }
      }

    if (fn->realOnly)
     for (i=0; i<nArgs; i++) if (args[i].flagComplex) 
      {
       if (context->set->term_current.ExplicitErrors == SW_ONOFF_ON) { *status=1; *errPos=charpos; *errType=ERR_TYPE; sprintf(errText,"Function requires real arguments; argument %d is complex.",i+1); goto cleanup; }
       else                                                          { context->stack[context->stackPtr-1-nArgs].real = GSL_NAN; goto cleanup; }
      }

    if (fn->dimlessOnly)
     for (i=0; i<nArgs; i++) if (!args[i].dimensionless)
      {
       if (context->set->term_current.ExplicitErrors == SW_ONOFF_ON) { *status=1; *errPos=charpos; *errType=ERR_TYPE; sprintf(errText,"Function requires dimensionless arguments; argument %d has dimensions of <%s>.",i+1,ppl_printUnit(context, &args[i], NULL, NULL, 1, 1, 0)); goto cleanup; }
       else                                                          { context->stack[context->stackPtr-1-nArgs].real = GSL_NAN; goto cleanup; }
      }

    if (fn->notNan)
     for (i=0; i<nArgs; i++)
      if ((!gsl_finite(args[i].real)) || (!gsl_finite(args[i].imag)))
       {
        if (context->set->term_current.ExplicitErrors == SW_ONOFF_ON) { *status=1; *errPos=charpos; *errType=ERR_TYPE; sprintf(errText,"Function requires finite arguments; argument %d is not finite.",i+1); goto cleanup; }
        else                                                          { context->stack[context->stackPtr-1-nArgs].real = GSL_NAN; goto cleanup; }
       }

    switch (fn->functionType)
     {
      case PPL_FUNC_SYSTEM:
       {
        int stat=0;
        ((void(*)(ppl_context *, pplObj *, int, int *, int *, char *))fn->functionPtr)(context, args, nArgs, &stat, errType, errText);
        if (stat) { *status=1; *errPos=charpos; goto cleanup; }
        break;
       }
      default:
        { *status=1; *errPos=charpos; *errType=ERR_INTERNAL; sprintf(errText,"Call of unsupported function type."); goto cleanup; }
     }
   }
cleanup:
  ppl_garbageObject(&called);
  for (i=0; i<nArgs; i++) { context->stackPtr--; ppl_garbageObject(&context->stack[context->stackPtr]); }
  return;
 }

