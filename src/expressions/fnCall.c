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
#include "userspace/pplObj.h"
#include "userspace/pplObjFunc.h"
#include "userspace/pplObjPrint.h"
#include "userspace/unitsArithmetic.h"
#include "userspace/unitsDisp.h"

#include "pplConstants.h"

void ppl_fnCall(ppl_context *context, int nArgs, int charpos, int IterDepth, int *status, int *errPos, int *errType, char *errText)
 {
  int i;
  pplFunc *fn;
  pplObj  *args = context->stack+context->stackPtr-nArgs;
  if (context->stackPtr<nArgs+1) { *status=1; *errPos=charpos; *errType=ERR_INTERNAL; sprintf(errText,"Attempt to call function with few items on the stack."); return; }
  if (context->stack[context->stackPtr-1-nArgs].objType != PPLOBJ_FUNC) { *status=1; *errPos=charpos; *errType=ERR_TYPE; sprintf(errText,"Object of type <%s> cannot be called as a function.",pplObjTypeNames[context->stack[context->stackPtr-1-nArgs].objType]); return; }
  fn = (pplFunc *)context->stack[context->stackPtr-1-nArgs].auxil;
  ppl_garbageObject(&context->stack[context->stackPtr-1-nArgs]);
  pplObjZero(&context->stack[context->stackPtr-1-nArgs] , 0);

  if (fn->minArgs == fn->maxArgs)
   {
    if (nArgs != fn->maxArgs) 
     {
      if (context->set->term_current.ExplicitErrors == SW_ONOFF_ON) { *status=1; *errPos=charpos; *errType=ERR_TYPE; sprintf(errText,"Function takes exactly %d arguments; %d supplied.",fn->maxArgs,nArgs); return; }
      else                                                          { context->stack[context->stackPtr-1-nArgs].real = GSL_NAN; return; }
     }
   }
  else if (nArgs < fn->minArgs) 
   {
    if (context->set->term_current.ExplicitErrors == SW_ONOFF_ON) { *status=1; *errPos=charpos; *errType=ERR_TYPE; sprintf(errText,"Function takes a minimum of %d arguments; %d supplied.",fn->minArgs,nArgs); return; }
    else                                                          { context->stack[context->stackPtr-1-nArgs].real = GSL_NAN; return; }
   }
  else if (nArgs > fn->maxArgs) 
   {
    if (context->set->term_current.ExplicitErrors == SW_ONOFF_ON) { *status=1; *errPos=charpos; *errType=ERR_TYPE; sprintf(errText,"Function takes a maximum of %d arguments; %d supplied.",fn->maxArgs,nArgs); return; }
    else                                                          { context->stack[context->stackPtr-1-nArgs].real = GSL_NAN; return; }
   }

  if (fn->numOnly)
   for (i=0; i<nArgs; i++) if (args[i].objType!=PPLOBJ_NUM) 
    {
     if (context->set->term_current.ExplicitErrors == SW_ONOFF_ON) { *status=1; *errPos=charpos; *errType=ERR_TYPE; sprintf(errText,"Function required numeric arguments; argument %d has type <%s>.",i+1,pplObjTypeNames[args[i].objType]); return; }
     else                                                          { context->stack[context->stackPtr-1-nArgs].real = GSL_NAN; return; }
    }

  if (fn->realOnly)
   for (i=0; i<nArgs; i++) if (args[i].flagComplex) 
    {
     if (context->set->term_current.ExplicitErrors == SW_ONOFF_ON) { *status=1; *errPos=charpos; *errType=ERR_TYPE; sprintf(errText,"Function requires real arguments; argument %d is complex.",i+1); return; }
     else                                                          { context->stack[context->stackPtr-1-nArgs].real = GSL_NAN; return; }
    }


  if (fn->dimlessOnly)
   for (i=0; i<nArgs; i++) if (!args[i].dimensionless)
    {
     if (context->set->term_current.ExplicitErrors == SW_ONOFF_ON) { *status=1; *errPos=charpos; *errType=ERR_TYPE; sprintf(errText,"Function requires dimensionless arguments; argument %d has dimensions of <%s>.",i+1,ppl_printUnit(context, &args[i], NULL, NULL, 1, 1, 0)); return; }
     else                                                          { context->stack[context->stackPtr-1-nArgs].real = GSL_NAN; return; }
    }

  if (fn->notNan)
   for (i=0; i<nArgs; i++)
    if ((!gsl_finite(args[i].real)) || (!gsl_finite(args[i].imag)))
     {
      if (context->set->term_current.ExplicitErrors == SW_ONOFF_ON) { *status=1; *errPos=charpos; *errType=ERR_TYPE; sprintf(errText,"Function requires finite arguments; argument %d is not finite.",i+1); return; }
      else                                                          { context->stack[context->stackPtr-1-nArgs].real = GSL_NAN; return; }
     }

  switch (fn->functionType)
   {
    case PPL_FUNC_SYSTEM:
     {
      int stat=0, i;
      ((void(*)(ppl_context *, pplObj *, int, int *, int *, char *))fn->functionPtr)(context, args, nArgs, &stat, errType, errText);
      if (stat) { *status=1; *errPos=charpos; return; }
      for (i=0; i<nArgs; i++) { context->stackPtr--; ppl_garbageObject(&context->stack[context->stackPtr]); }
      break;
     }
    default:
      { *status=1; *errPos=charpos; *errType=ERR_INTERNAL; sprintf(errText,"Call of unsupported function type."); return; }
   }
  return;
 }

