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
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <limits.h>
#include <time.h>

#include <gsl/gsl_math.h>

#include "coreUtils/dict.h"
#include "coreUtils/errorReport.h"
#include "coreUtils/list.h"
#include "expressions/expEval.h"
#include "expressions/expEvalCalculus.h"
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

void ppl_fnCall(ppl_context *context, int nArgs, int charpos, int dollarAllowed, int iterDepth, int *status, int *errPos, int *errType, char *errText)
 {
  pplObj  *out  = &context->stack[context->stackPtr-1-nArgs];
  pplObj  *args = context->stack+context->stackPtr-nArgs;
  pplObj   called;
  pplFunc *fn;
  int      i;
  int      t = out->objType;
  if (context->stackPtr<nArgs+1) { *status=1; *errPos=charpos; *errType=ERR_INTERNAL; sprintf(errText,"Attempt to call function with few items on the stack."); return; }
  memcpy(&called, out, sizeof(pplObj));
  pplObjNum(out, 0, 0, 0); // Overwrite function object on stack, but don't garbage collect it yet
  out->self_this = called.self_this;

  // Attempt to call a module to general a class instance?
  if ((t == PPLOBJ_MOD) || (t == PPLOBJ_USER))
   {
    if (nArgs!=0) { *status=1; *errPos=charpos; *errType=ERR_TYPE; sprintf(errText,"Instantiation takes zero arguments; %d supplied.",nArgs); goto cleanup; }
    pplObjUser(out,0,1,&called);
    goto cleanup;
   }

  // Attempt to call a type object as a constructor?
  else if (t == PPLOBJ_TYPE)
   {
    int id = ((pplType *)(out->auxil))->id;
    switch (id)
     {
      case PPLOBJ_NUM:
        if      (nArgs==0) { pplObjNum(out,0,0,0); }
        else if (nArgs==1) { CAST_TO_NUM(&args[0]); pplObjCpy(out,&args[0],0,1); }
        else               { *status=1; *errPos=charpos; *errType=ERR_TYPE; sprintf(errText,"The numeric object constructor takes either zero or one arguments; %d supplied.",nArgs); }
        goto cleanup;
      case PPLOBJ_STR:
        if      (nArgs==0) { pplObjStr(out,0,0,""); }
        else if (nArgs==1) { if (args[0].objType==PPLOBJ_STR) pplObjCpy(out,&args[0],0,1);
                             else { char *outstr=(char*)malloc(65536); pplObjPrint(context,&args[0],NULL,outstr,65536,0,0); pplObjStr(out,0,1,outstr); }
                           }
        else               { *status=1; *errPos=charpos; *errType=ERR_TYPE; sprintf(errText,"The string object constructor takes either zero or one arguments; %d supplied.",nArgs); }
        goto cleanup;
      case PPLOBJ_BOOL:
        if      (nArgs==0) { pplObjBool(out,0,1); }
        else if (nArgs==1) { CAST_TO_BOOL(&args[0]); pplObjCpy(out,&args[0],0,1); }
        else               { *status=1; *errPos=charpos; *errType=ERR_TYPE; sprintf(errText,"The boolean object constructor takes either zero or one arguments; %d supplied.",nArgs); }
        goto cleanup;
      case PPLOBJ_DATE:
        if      (nArgs==0) { pplObjDate(out,0,(double)time(NULL)); }
        else if (nArgs==1) { if (args[0].objType==PPLOBJ_DATE) pplObjDate(out,0,args[0].real);
                             else { *status=1; *errPos=charpos; *errType=ERR_TYPE; sprintf(errText,"The first argument to the date object constructor should be a date; an object of type <%s> was supplied.",pplObjTypeNames[args[0].objType]); }
                           }
        else               { *status=1; *errPos=charpos; *errType=ERR_TYPE; sprintf(errText,"The date object constructor takes zero or one arguments; %d supplied.",nArgs); }
        goto cleanup;
      case PPLOBJ_COL:
        if      (nArgs==0) { pplObjColor(out,0,SW_COLSPACE_RGB,0,0,0,0); }
        else if (nArgs==1) { if (args[0].objType==PPLOBJ_COL) pplObjColor(out,0,round(args[0].exponent[0]),args[0].exponent[8],args[0].exponent[9],args[0].exponent[10],args[0].exponent[11]);
                             else { *status=1; *errPos=charpos; *errType=ERR_TYPE; sprintf(errText,"The first argument to the color object constructor should be a color; an object of type <%s> was supplied.",pplObjTypeNames[args[0].objType]); }
                           }
        else               { *status=1; *errPos=charpos; *errType=ERR_TYPE; sprintf(errText,"The color object constructor takes zero or one arguments; %d supplied.",nArgs); }
        goto cleanup;
      case PPLOBJ_DICT:
        if      (nArgs==0) { pplObjDict(out,0,1,NULL); }
        else if (nArgs==1) { if (args[0].objType==PPLOBJ_DICT) pplObjDeepCpy(out,&args[0],0,0,1);
                             else { *status=1; *errPos=charpos; *errType=ERR_TYPE; sprintf(errText,"The first argument to the dictionary object constructor should be a dictionary; an object of type <%s> was supplied.",pplObjTypeNames[args[0].objType]); }
                           }
        else               { *status=1; *errPos=charpos; *errType=ERR_TYPE; sprintf(errText,"The dictionary object constructor takes zero or one arguments; %d supplied.",nArgs); }
        goto cleanup;
      case PPLOBJ_MOD:
      case PPLOBJ_USER:
        if      (nArgs==0) { pplObjModule(out,0,1,0); }
        else if (nArgs==1) { if ((args[0].objType==PPLOBJ_MOD)||(args[0].objType==PPLOBJ_USER)) pplObjDeepCpy(out,&args[0],0,0,1);
                             else { *status=1; *errPos=charpos; *errType=ERR_TYPE; sprintf(errText,"The first argument to the module/instance object constructor should be a module or instance; an object of type <%s> was supplied.",pplObjTypeNames[args[0].objType]); }
                           }
        else               { *status=1; *errPos=charpos; *errType=ERR_TYPE; sprintf(errText,"The module/instance object constructor takes zero or one arguments; %d supplied.",nArgs); }
        goto cleanup;
      case PPLOBJ_FILE:
        if      (nArgs==0) { pplObjFile(out,0,1,tmpfile(),0); }
        else if (nArgs==1) { if (args[0].objType==PPLOBJ_FILE) pplObjCpy(out,&args[0],0,1);
                             else { *status=1; *errPos=charpos; *errType=ERR_TYPE; sprintf(errText,"The first argument to the file object constructor should be a file object; an object of type <%s> was supplied.",pplObjTypeNames[args[0].objType]); }
                           }
        else               { *status=1; *errPos=charpos; *errType=ERR_TYPE; sprintf(errText,"The file object constructor takes zero or one arguments; %d supplied.",nArgs); }
        goto cleanup;
      case PPLOBJ_TYPE:
        *status=1; *errPos=charpos; *errType=ERR_TYPE; sprintf(errText,"Creation of new data type objects is not permitted."); goto cleanup;
      case PPLOBJ_FUNC:
        *status=1; *errPos=charpos; *errType=ERR_TYPE; sprintf(errText,"New function objects must be created with the syntax f(x)=... or subroutine f(x) { ... }."); goto cleanup;
      case PPLOBJ_EXC:
        if (nArgs==1) { if      (args[0].objType==PPLOBJ_STR) pplObjException(out,0,1,(char*)args[0].auxil);
                        else if (args[0].objType==PPLOBJ_EXC) pplObjCpy(out,&args[0],0,1);
                        else { *status=1; *errPos=charpos; *errType=ERR_TYPE; sprintf(errText,"The first argument to the exception object constructor should be a string; an object of type <%s> was supplied.",pplObjTypeNames[args[0].objType]); }
                      }
        else          { *status=1; *errPos=charpos; *errType=ERR_TYPE; sprintf(errText,"The exception object constructor takes one argument; %d supplied.",nArgs); }
        goto cleanup;
      case PPLOBJ_NULL:
        if      (nArgs==0) { pplObjNull(out,0); }
        else               { *status=1; *errPos=charpos; *errType=ERR_TYPE; sprintf(errText,"The null object constructor takes zero arguments; %d supplied.",nArgs); }
        goto cleanup;
     }
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
      if (nArgs != 1) { *status=1; *errPos=charpos; *errType=ERR_TYPE; sprintf(errText,"The unit() function takes exactly one argument; %d supplied.",nArgs); goto cleanup; }
      ppl_unitsStringEvaluate(context, u, out, &end, errPos, errText);
      if (*errPos>=0) { *errType=ERR_UNIT; goto cleanup; }
      if (end!=strlen(u)) { *errType=ERR_UNIT; *errPos=charpos; sprintf(errText,"Unexpected trailing matter after unit string."); goto cleanup; }
     }
    else if (fn->minArgs==2) // diff_d()
     {
      pplObj v, *step, *xpos;
      if      (nArgs == 3) step = &v;
      else if (nArgs == 4) step = &args[3];
      else    { *status=1; *errPos=charpos; *errType=ERR_TYPE; sprintf(errText,"The diff_d() function takes two or thee arguments; %d supplied.",nArgs-1); goto cleanup; }
      if (args[0].objType!=PPLOBJ_STR) { *status=1; *errPos=charpos; *errType=ERR_INTERNAL; strcpy(errText,"Dummy variable not passed to diff_d() as a string"); goto cleanup; }
      if (args[1].objType!=PPLOBJ_STR) { *status=1; *errPos=charpos; *errType=ERR_INTERNAL; strcpy(errText,"Differentiation expression not passed to diff_d() as a string"); goto cleanup; }
      if (args[2].objType!=PPLOBJ_NUM) { *status=1; *errPos=charpos; *errType=ERR_TYPE; sprintf(errText,"The diff_d() function requires a number as its second argument; supplied argument has type <%s>.",pplObjTypeNames[args[2].objType]); goto cleanup; }
      if ((nArgs==4)&&(args[3].objType!=PPLOBJ_NUM)) { *status=1; *errPos=charpos; *errType=ERR_TYPE; sprintf(errText,"The diff_d() function requires a number as its third argument; supplied argument has type <%s>.",pplObjTypeNames[args[3].objType]); goto cleanup; }
      xpos = &args[2];
      memcpy(&v,xpos,sizeof(pplObj)); v.imag=0; v.real = hypot(xpos->real,xpos->imag)*1e-6; v.flagComplex=0;
      if (v.real<DBL_MIN*1e6) v.real=1e-6;
      ppl_expDifferentiate(context,(char*)args[1].auxil,(char*)args[0].auxil,xpos,step,out,dollarAllowed,errPos,errType,errText,iterDepth);
      if (*errPos>=0) { *status=1; *errPos=charpos; goto cleanup; }
     }
    else if (fn->minArgs==3) // int_d()
     {
      if (nArgs != 4) { *status=1; *errPos=charpos; *errType=ERR_TYPE; sprintf(errText,"The int_d() function takes two or thee arguments; %d supplied.",nArgs-1); goto cleanup; }
      if (args[0].objType!=PPLOBJ_STR) { *status=1; *errPos=charpos; *errType=ERR_INTERNAL; strcpy(errText,"Dummy variable not passed to int_d() as a string"); goto cleanup; }
      if (args[1].objType!=PPLOBJ_STR) { *status=1; *errPos=charpos; *errType=ERR_INTERNAL; strcpy(errText,"Integration expression not passed to diff_d() as a string"); goto cleanup; }
      if (args[2].objType!=PPLOBJ_NUM) { *status=1; *errPos=charpos; *errType=ERR_TYPE; sprintf(errText,"The int_d() function requires a number as its second argument; supplied argument has type <%s>.",pplObjTypeNames[args[2].objType]); goto cleanup; }
      if (args[3].objType!=PPLOBJ_NUM) { *status=1; *errPos=charpos; *errType=ERR_TYPE; sprintf(errText,"The int_d() function requires a number as its third argument; supplied argument has type <%s>.",pplObjTypeNames[args[3].objType]); goto cleanup; }
      ppl_expIntegrate(context,(char*)args[1].auxil,(char*)args[0].auxil,&args[2],&args[3],out,dollarAllowed,errPos,errType,errText,iterDepth);
      if (*errPos>=0) { *status=1; *errPos=charpos; goto cleanup; }
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
  out->self_this = NULL;
  ppl_garbageObject(&called);
  for (i=0; i<nArgs; i++) { context->stackPtr--; ppl_garbageObject(&context->stack[context->stackPtr]); }
  return;
cast_fail:
  *status=1; *errPos=charpos;
  goto cleanup;
 }

