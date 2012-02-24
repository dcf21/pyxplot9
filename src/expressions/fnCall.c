// fnCall.c
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
#include "expressions/fnCall.h"
#include "expressions/traceback_fns.h"
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

#define TBADD(et,pos,lt) ppl_tbAdd(context,inExpr->srcLineN,inExpr->srcId,inExpr->srcFname,0,et,pos,lt)

#define STACK_POP \
   { \
    context->stackPtr--; \
    ppl_garbageObject(&context->stack[context->stackPtr]); \
    if (context->stack[context->stackPtr].refCount != 0) { strcpy(context->errStat.errBuff,"Stack forward reference detected."); TBADD(ERR_INTERNAL,0,NULL); goto cleanup; } \
   }

void ppl_fnCall(ppl_context *context, pplExpr *inExpr, int nArgs, int dollarAllowed, int iterDepth)
 {
  pplObj  *out  = &context->stack[context->stackPtr-1-nArgs];
  pplObj  *args = context->stack+context->stackPtr-nArgs;
  pplObj   called;
  pplFunc *fn;
  int      i;
  int      t = out->objType;
  if (context->stackPtr<nArgs+1) { sprintf(context->errStat.errBuff,"Attempt to call function with few items on the stack."); TBADD(ERR_INTERNAL,0,NULL); return; }
  memcpy(&called, out, sizeof(pplObj));
  pplObjNum(out, 0, 0, 0); // Overwrite function object on stack, but don't garbage collect it yet
  out->refCount = 1;
  out->self_this = called.self_this;

  // Attempt to call a module to general a class instance?
  if ((t == PPLOBJ_MOD) || (t == PPLOBJ_USER))
   {
    if (nArgs!=0) { sprintf(context->errStat.errBuff,"Instantiation takes zero arguments; %d supplied.",nArgs); TBADD(ERR_TYPE,0,NULL); goto cleanup; }
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
        else if (nArgs==1) { CAST_TO_NUM(&args[0]); pplObjCpy(out,&args[0],0,0,1); }
        else               { sprintf(context->errStat.errBuff,"The numeric object constructor takes either zero or one arguments; %d supplied.",nArgs); TBADD(ERR_TYPE,0,NULL); }
        goto cleanup;
      case PPLOBJ_STR:
        if      (nArgs==0) { pplObjStr(out,0,0,""); }
        else if (nArgs==1) { if (args[0].objType==PPLOBJ_STR) pplObjCpy(out,&args[0],0,0,1);
                             else { char *outstr=(char*)malloc(65536); pplObjPrint(context,&args[0],NULL,outstr,65536,0,0); pplObjStr(out,0,1,outstr); }
                           }
        else               { sprintf(context->errStat.errBuff,"The string object constructor takes either zero or one arguments; %d supplied.",nArgs); TBADD(ERR_TYPE,0,NULL); }
        goto cleanup;
      case PPLOBJ_BOOL:
        if      (nArgs==0) { pplObjBool(out,0,1); }
        else if (nArgs==1) { CAST_TO_BOOL(&args[0]); pplObjCpy(out,&args[0],0,0,1); }
        else               { sprintf(context->errStat.errBuff,"The boolean object constructor takes either zero or one arguments; %d supplied.",nArgs); TBADD(ERR_TYPE,0,NULL); }
        goto cleanup;
      case PPLOBJ_DATE:
        if      (nArgs==0) { pplObjDate(out,0,(double)time(NULL)); }
        else if (nArgs==1) { if (args[0].objType==PPLOBJ_DATE) pplObjDate(out,0,args[0].real);
                             else { sprintf(context->errStat.errBuff,"The first argument to the date object constructor should be a date; an object of type <%s> was supplied.",pplObjTypeNames[args[0].objType]); TBADD(ERR_TYPE,0,NULL); }
                           }
        else               { sprintf(context->errStat.errBuff,"The date object constructor takes zero or one arguments; %d supplied.",nArgs); TBADD(ERR_TYPE,0,NULL); }
        goto cleanup;
      case PPLOBJ_COL:
        if      (nArgs==0) { pplObjColor(out,0,SW_COLSPACE_RGB,0,0,0,0); }
        else if (nArgs==1) { if (args[0].objType==PPLOBJ_COL) pplObjColor(out,0,round(args[0].exponent[0]),args[0].exponent[8],args[0].exponent[9],args[0].exponent[10],args[0].exponent[11]);
                             else { sprintf(context->errStat.errBuff,"The first argument to the color object constructor should be a color; an object of type <%s> was supplied.",pplObjTypeNames[args[0].objType]); TBADD(ERR_TYPE,0,NULL); }
                           }
        else               { sprintf(context->errStat.errBuff,"The color object constructor takes zero or one arguments; %d supplied.",nArgs); TBADD(ERR_TYPE,0,NULL); }
        goto cleanup;
      case PPLOBJ_DICT:
        if      (nArgs==0) { pplObjDict(out,0,1,NULL); }
        else if (nArgs==1) { if (args[0].objType==PPLOBJ_DICT) pplObjDeepCpy(out,&args[0],0,0,1);
                             else { sprintf(context->errStat.errBuff,"The first argument to the dictionary object constructor should be a dictionary; an object of type <%s> was supplied.",pplObjTypeNames[args[0].objType]); TBADD(ERR_TYPE,0,NULL); }
                           }
        else               { sprintf(context->errStat.errBuff,"The dictionary object constructor takes zero or one arguments; %d supplied.",nArgs); TBADD(ERR_TYPE,0,NULL); }
        goto cleanup;
      case PPLOBJ_MOD:
      case PPLOBJ_USER:
        if      (nArgs==0) { pplObjModule(out,0,1,0); }
        else if (nArgs==1) { if ((args[0].objType==PPLOBJ_MOD)||(args[0].objType==PPLOBJ_USER)) pplObjDeepCpy(out,&args[0],0,0,1);
                             else { sprintf(context->errStat.errBuff,"The first argument to the module/instance object constructor should be a module or instance; an object of type <%s> was supplied.",pplObjTypeNames[args[0].objType]); TBADD(ERR_TYPE,0,NULL); }
                           }
        else               { sprintf(context->errStat.errBuff,"The module/instance object constructor takes zero or one arguments; %d supplied.",nArgs); TBADD(ERR_TYPE,0,NULL); }
        goto cleanup;
      case PPLOBJ_FILE:
        if      (nArgs==0) { pplObjFile(out,0,1,tmpfile(),0); }
        else if (nArgs==1) { if (args[0].objType==PPLOBJ_FILE) pplObjCpy(out,&args[0],0,0,1);
                             else { sprintf(context->errStat.errBuff,"The first argument to the file object constructor should be a file object; an object of type <%s> was supplied.",pplObjTypeNames[args[0].objType]); TBADD(ERR_TYPE,0,NULL); }
                           }
        else               { sprintf(context->errStat.errBuff,"The file object constructor takes zero or one arguments; %d supplied.",nArgs); TBADD(ERR_TYPE,0,NULL); }
        goto cleanup;
      case PPLOBJ_TYPE:
        sprintf(context->errStat.errBuff,"Creation of new data type objects is not permitted."); TBADD(ERR_TYPE,0,NULL); goto cleanup;
      case PPLOBJ_FUNC:
        sprintf(context->errStat.errBuff,"New function objects must be created with the syntax f(x)=... or subroutine f(x) { ... }."); TBADD(ERR_TYPE,0,NULL); goto cleanup;
      case PPLOBJ_EXC:
        if (nArgs==1) { if      (args[0].objType==PPLOBJ_STR) { pplObjCpy(out,&args[0],0,0,1); pplObjException(out,0,1,(char*)out->auxil,ERR_GENERAL); }
                        else if (args[0].objType==PPLOBJ_EXC) { pplObjCpy(out,&args[0],0,0,1); }
                        else { sprintf(context->errStat.errBuff,"The first argument to the exception object constructor should be a string; an object of type <%s> was supplied.",pplObjTypeNames[args[0].objType]); TBADD(ERR_TYPE,0,NULL); }
                      }
        else          { sprintf(context->errStat.errBuff,"The exception object constructor takes one argument; %d supplied.",nArgs); TBADD(ERR_TYPE,0,NULL); }
        goto cleanup;
      case PPLOBJ_NULL:
        if      (nArgs==0) { pplObjNull(out,0); }
        else               { sprintf(context->errStat.errBuff,"The null object constructor takes zero arguments; %d supplied.",nArgs); TBADD(ERR_TYPE,0,NULL); }
        goto cleanup;
      case PPLOBJ_LIST:
        if ((nArgs==1)&&(args[0].objType==PPLOBJ_LIST)) { pplObjDeepCpy(out, &args[0], 0, 0, 1); goto cleanup; }
        if (pplObjList(out,0,1,NULL)==NULL) { sprintf(context->errStat.errBuff,"Out of memory."); TBADD(ERR_MEMORY,0,NULL); goto cleanup; }
        if (nArgs<1) goto cleanup;
        if ((nArgs==1)&&(args[0].objType==PPLOBJ_VEC))
         {
          int i;
          gsl_vector *vec = ((pplVector *)args[0].auxil)->v;
          const int l = vec->size;
          pplObj v;
          list *lo = (list *)out->auxil;
          v.refCount = 1;
          pplObjNum(&v,1,0,0);
          ppl_unitsDimCpy(&v, &args[0]);
          for (i=0; i<l; i++) { v.real = gsl_vector_get(vec,i); ppl_listAppendCpy(lo, &v, sizeof(pplObj)); }
         }
        else
         {
          int i;
          for (i=0; i<nArgs; i++)
           {
            pplObj v;
            list *lo = (list *)out->auxil;
            v.refCount = 1;
            pplObjCpy(&v,&args[i],0,1,1);
            ppl_listAppendCpy(lo, &v, sizeof(pplObj));
           }
         }
        goto cleanup;
      case PPLOBJ_VEC:
        if (nArgs<1) { pplObjVector(out,0,1,1); gsl_vector_set( ((pplVector*)out->auxil)->v,0,0); goto cleanup; }
        if (nArgs==1)
         {
          if (args[0].objType==PPLOBJ_VEC) { pplObjDeepCpy(out, &args[0], 0, 0, 1); goto cleanup; }
          else if (args[0].objType==PPLOBJ_NUM)
           {
            gsl_vector *v;
            long i,len;
            if (!args[0].dimensionless) { sprintf(context->errStat.errBuff,"Specified length of vector should be dimensionless; supplied length has units of <%s>.", ppl_printUnit(context, &args[0], NULL, NULL, 0, 1, 0)); TBADD(ERR_TYPE,0,NULL); goto cleanup; }
            if (args[0].flagComplex) { sprintf(context->errStat.errBuff,"Specified length of vector should be real; supplied length is complex number."); TBADD(ERR_TYPE,0,NULL); goto cleanup; }
            if ((args[0].real<1)||(args[0].real>INT_MAX)) { sprintf(context->errStat.errBuff,"Specified length of vector should be in the range 1<len<%d.",INT_MAX); TBADD(ERR_RANGE,0,NULL); goto cleanup; }
            len = floor(args[0].real);
            if (pplObjVector(out,0,1,len)==NULL) { sprintf(context->errStat.errBuff,"Out of memory."); TBADD(ERR_MEMORY,0,NULL); goto cleanup; }
            v = ((pplVector *)out->auxil)->v;
            for (i=0; i<len; i++) { gsl_vector_set(v,i,0); }
            goto cleanup;
           }
          else if (args[0].objType==PPLOBJ_LIST)
           {
            long i=0;
            list *listin = (list *)args[0].auxil;
            listIterator *li = ppl_listIterateInit(listin);
            const long len = listin->length;
            gsl_vector *v;
            if (len==0) { sprintf(context->errStat.errBuff,"Cannot create a vector of length zero."); TBADD(ERR_MEMORY,0,NULL); goto cleanup; }
            if (pplObjVector(out,0,1,len)==NULL) { sprintf(context->errStat.errBuff,"Out of memory."); TBADD(ERR_MEMORY,0,NULL); goto cleanup; }
            v = ((pplVector *)out->auxil)->v;
            if (len>0)
             {
              pplObj *item = (pplObj*)ppl_listIterate(&li);
              if (item->objType!=PPLOBJ_NUM) { sprintf(context->errStat.errBuff,"Vectors can only hold numeric values. Attempt to add object of type <%s> to vector.", pplObjTypeNames[item->objType]); TBADD(ERR_TYPE,0,NULL); goto cleanup; }
              if (item->flagComplex) { sprintf(context->errStat.errBuff,"Vectors can only hold real numeric values. Attempt to add a complex number."); TBADD(ERR_TYPE,0,NULL); goto cleanup; }
              ppl_unitsDimCpy(out,item);
              gsl_vector_set(v,i,item->real);
             }
            for (i=1; i<len; i++)
             {
              pplObj *item = (pplObj*)ppl_listIterate(&li);
              if (item->objType!=PPLOBJ_NUM) { sprintf(context->errStat.errBuff,"Vectors can only hold numeric values. Attempt to add object of type <%s> to vector.", pplObjTypeNames[item->objType]); TBADD(ERR_TYPE,0,NULL); goto cleanup; }
              if (item->flagComplex) { sprintf(context->errStat.errBuff,"Vectors can only hold real numeric values. Attempt to add a complex number."); TBADD(ERR_TYPE,0,NULL); goto cleanup; }
              if (!ppl_unitsDimEqual(item, out))
               {
                if (out->dimensionless)
                 { sprintf(context->errStat.errBuff, "Attempt to append a number (argument %ld) to a vector with conflicting dimensions: vector is dimensionless, but number has units of <%s>.", i+1, ppl_printUnit(context, item, NULL, NULL, 1, 1, 0) ); }
                else if (item->dimensionless)
                 { sprintf(context->errStat.errBuff, "Attempt to append a number (argument %ld) to a vector with conflicting dimensions: vector has units of <%s>, while number is dimensionless.", i+1, ppl_printUnit(context, out, NULL, NULL, 0, 1, 0) ); }
                else
                 { sprintf(context->errStat.errBuff, "Attempt to append a number (argument %ld) to a vector with conflicting dimensions: vector has units of <%s>, while number has units of <%s>.", i+1, ppl_printUnit(context, out, NULL, NULL, 0, 1, 0), ppl_printUnit(context, item, NULL, NULL, 1, 1, 0) ); }
                TBADD(ERR_UNIT,0,NULL); goto cleanup;
               }
              gsl_vector_set(v,i,item->real);
             }
            goto cleanup;
           }
         }
        // List is to made up from arguments supplied to constructor
         {
          long i=0;
          gsl_vector *v;
          if (pplObjVector(out,0,1,nArgs)==NULL) { sprintf(context->errStat.errBuff,"Out of memory."); TBADD(ERR_MEMORY,0,NULL); goto cleanup; }
          v = ((pplVector *)out->auxil)->v;
          if (nArgs>0)
           {
            pplObj *item = &args[0];
            if (item->objType!=PPLOBJ_NUM) { sprintf(context->errStat.errBuff,"Vectors can only hold numeric values. Attempt to add object of type <%s> to vector.", pplObjTypeNames[item->objType]); TBADD(ERR_TYPE,0,NULL); goto cleanup; }
            if (item->flagComplex) { sprintf(context->errStat.errBuff,"Vectors can only hold real numeric values. Attempt to add a complex number."); TBADD(ERR_TYPE,0,NULL); goto cleanup; }
            ppl_unitsDimCpy(out,item);
            gsl_vector_set(v,i,item->real);
           }
          for (i=1; i<nArgs; i++)
           {
            pplObj *item = &args[i];
            if (item->objType!=PPLOBJ_NUM) { sprintf(context->errStat.errBuff,"Vectors can only hold numeric values. Attempt to add object of type <%s> to vector.", pplObjTypeNames[item->objType]); TBADD(ERR_TYPE,0,NULL); goto cleanup; }
            if (item->flagComplex) { sprintf(context->errStat.errBuff,"Vectors can only hold real numeric values. Attempt to add a complex number."); TBADD(ERR_TYPE,0,NULL); goto cleanup; }
            if (!ppl_unitsDimEqual(item, out))
             {
              if (out->dimensionless)
               { sprintf(context->errStat.errBuff, "Attempt to append a number (argument %ld) to a vector with conflicting dimensions: vector is dimensionless, but number has units of <%s>.", i+1, ppl_printUnit(context, item, NULL, NULL, 1, 1, 0) ); }
              else if (item->dimensionless)
               { sprintf(context->errStat.errBuff, "Attempt to append a number (argument %ld) to a vector with conflicting dimensions: vector has units of <%s>, while number is dimensionless.", i+1, ppl_printUnit(context, out, NULL, NULL, 0, 1, 0) ); }
              else
               { sprintf(context->errStat.errBuff, "Attempt to append a number (argument %ld) to a vector with conflicting dimensions: vector has units of <%s>, while number has units of <%s>.", i+1, ppl_printUnit(context, out, NULL, NULL, 0, 1, 0), ppl_printUnit(context, item, NULL, NULL, 1, 1, 0) ); }
              TBADD(ERR_UNIT,0,NULL); goto cleanup;
             }
            gsl_vector_set(v,i,item->real);
           }
          goto cleanup;
         }
      case PPLOBJ_MAT:
        if (nArgs<1) { pplObjMatrix(out,0,1,1,1); gsl_matrix_set( ((pplMatrix*)out->auxil)->m,0,0,0); goto cleanup; }
        else if (args[0].objType==PPLOBJ_MAT)
         {
          if (nArgs!=1) { sprintf(context->errStat.errBuff,"When initialising a matrix from another matrix, only one argument should be supplied (the source matrix). %d have been provided.",nArgs); TBADD(ERR_TYPE,0,NULL); goto cleanup; }
          pplObjDeepCpy(out, &args[0], 0, 0, 1);
          goto cleanup;
         }
        else if (args[0].objType==PPLOBJ_NUM)
         {
          int s1,s2,i,j;
          gsl_matrix *m;
          if (nArgs!=2) { sprintf(context->errStat.errBuff,"When specifying the size of a matrix, two numerical arguments must be supplied. %d have been provided.",nArgs); TBADD(ERR_TYPE,0,NULL); goto cleanup; }
          if (args[1].objType!=PPLOBJ_NUM) { sprintf(context->errStat.errBuff,"When specifying the size of a matrix, two numerical arguments must be supplied. Second argument has type <%s>.", pplObjTypeNames[args[1].objType]); TBADD(ERR_TYPE,0,NULL); goto cleanup; }
          if (!args[0].dimensionless) { sprintf(context->errStat.errBuff,"When specifying the size of a matrix, both numerical arguments must be dimensionless. First has units of <%s>.", ppl_printUnit(context, &args[0], NULL, NULL, 1, 1, 0) ); TBADD(ERR_TYPE,0,NULL); goto cleanup; }
          if (!args[1].dimensionless) { sprintf(context->errStat.errBuff,"When specifying the size of a matrix, both numerical arguments must be dimensionless. Second has units of <%s>.", ppl_printUnit(context, &args[1], NULL, NULL, 1, 1, 0) ); TBADD(ERR_TYPE,0,NULL); goto cleanup; }
          if ((args[0].flagComplex) || (args[1].flagComplex)) { sprintf(context->errStat.errBuff,"When specifying the size of a matrix, both arguments must be real numbers. Supplied arguments are complex."); TBADD(ERR_TYPE,0,NULL); goto cleanup; }
          s1 = args[0].real ; s2 = args[1].real;
          if ((s1<1)||(s1>INT_MAX)) { sprintf(context->errStat.errBuff,"Specified dimension of vector should be in the range 1<len<%d.",INT_MAX); TBADD(ERR_RANGE,0,NULL); goto cleanup; }
          if ((s2<1)||(s2>INT_MAX)) { sprintf(context->errStat.errBuff,"Specified dimension of vector should be in the range 1<len<%d.",INT_MAX); TBADD(ERR_RANGE,0,NULL); goto cleanup; }
          if (pplObjMatrix(out,0,1,s1,s2)==NULL) { sprintf(context->errStat.errBuff,"Out of memory."); TBADD(ERR_MEMORY,0,NULL); goto cleanup; }
          m = ((pplMatrix *)out->auxil)->m;
          for (i=0; i<s1; i++) for (j=0; j<s2; j++) { gsl_matrix_set(m,i,j,0); }
          goto cleanup;
         }
        else if (args[0].objType==PPLOBJ_VEC)
         {
          long i;
          const long s1 = ((pplVector*)args[0].auxil)->v->size;
          const long s2 = nArgs;
          gsl_matrix *m;
          if (pplObjMatrix(out,0,1,s1,s2)==NULL) { sprintf(context->errStat.errBuff,"Out of memory."); TBADD(ERR_MEMORY,0,NULL); goto cleanup; }
          m = ((pplMatrix *)out->auxil)->m;
          ppl_unitsDimCpy(out,&args[0]);
          for (i=0; i<s2; i++)
           {
            long j;
            gsl_vector *v;
            if (args[i].objType!=PPLOBJ_VEC) { sprintf(context->errStat.errBuff,"When initialising a matrix from a list of vectors, all arguments must be vectors. Supplied argument has type <%s>.", pplObjTypeNames[args[i].objType]); TBADD(ERR_TYPE,0,NULL); goto cleanup; }
            v = ((pplVector*)args[i].auxil)->v;
            if (v->size != s1) { sprintf(context->errStat.errBuff,"When initialising a matrix from a list of vectors, the vectors must have consistent lengths. Supplied vector has length %ld whereas previous arguments have had a length %ld.",(long)v->size,s1); TBADD(ERR_RANGE,0,NULL); goto cleanup; }
            if (!ppl_unitsDimEqual(out, &args[i])) { sprintf(context->errStat.errBuff,"When initialising a matrix from a list of vectors, the vectors must all have the same dimensions. Supplied vectors have units of <%s> and <%s>.", ppl_printUnit(context, out, NULL, NULL, 0, 1, 0), ppl_printUnit(context, &args[i], NULL, NULL, 1, 1, 0) ); TBADD(ERR_UNIT,0,NULL); }
            for (j=0; j<s1; j++) { gsl_matrix_set(m, j, i, gsl_vector_get(v,j) ); }
           }
          goto cleanup;
         }
        else if (args[0].objType==PPLOBJ_LIST)
         {
          list *listin = (list *)args[0].auxil;
          listIterator *li = ppl_listIterateInit(listin);
          const long len = listin->length;
          pplObj *item;
          if (len==0) { sprintf(context->errStat.errBuff,"Cannot create a matrix of dimension zero."); TBADD(ERR_MEMORY,0,NULL); goto cleanup; }
          item = (pplObj*)listin->first->data;
          if (item->objType==PPLOBJ_NUM)
           {
            long i;
            const long s1 = len;
            const long s2 = nArgs;
            gsl_matrix *m;
            for (i=0; i<s2; i++) if (args[i].objType!=PPLOBJ_LIST) { sprintf(context->errStat.errBuff,"When initialising a matrix from a list of lists, all arguments must be lists. Supplied argument has type <%s>.", pplObjTypeNames[args[i].objType]); TBADD(ERR_TYPE,0,NULL); goto cleanup; }
            if (pplObjMatrix(out,0,1,s1,s2)==NULL) { sprintf(context->errStat.errBuff,"Out of memory."); TBADD(ERR_MEMORY,0,NULL); goto cleanup; }
            m = ((pplMatrix *)out->auxil)->m;
            ppl_unitsDimCpy(out,item);
            for (i=0; i<s2; i++)
             {
              long j;
              list *listin = (list *)args[i].auxil;
              listIterator *li = ppl_listIterateInit(listin);
              if (listin->length != s1) { sprintf(context->errStat.errBuff,"When initialising a matrix from a list of lists, the lists must have consistent lengths. Supplied list has length %ld whereas previous lists have had a length %ld.",(long)listin->length,s1); TBADD(ERR_RANGE,0,NULL); goto cleanup; }
              for (j=0; j<s1; j++)
               {
                pplObj *item = (pplObj *)ppl_listIterate(&li);
                if (item->objType!=PPLOBJ_NUM) { sprintf(context->errStat.errBuff,"When initialising a matrix from a list of lists, the elements must all be numerical values; supplied element has type <%s>.", pplObjTypeNames[item->objType]); TBADD(ERR_RANGE,0,NULL); goto cleanup; }
                if (!ppl_unitsDimEqual(out, item)) { sprintf(context->errStat.errBuff,"When initialising a matrix from a list of lists, all of the elements must have the same dimensions. Supplied elements have units of <%s> and <%s>.", ppl_printUnit(context, out, NULL, NULL, 0, 1, 0), ppl_printUnit(context, item, NULL, NULL, 1, 1, 0) ); TBADD(ERR_UNIT,0,NULL); goto cleanup; }
                if (item->flagComplex) { sprintf(context->errStat.errBuff,"Matrices can only hold real numbers; supplied elements are complex."); TBADD(ERR_NUMERIC,0,NULL); goto cleanup; }
                gsl_matrix_set(m, j, i, item->real );
               }
             }
            goto cleanup;
           }
          else if (item->objType==PPLOBJ_VEC)
           {
            long i;
            const long s1 = ((pplVector*)item->auxil)->v->size;
            const long s2 = len;
            gsl_matrix *m;
            if (nArgs!=1) { sprintf(context->errStat.errBuff,"When initialising a matrix from a list of vectors, only one argument should be supplied. %d arguments were supplied.",nArgs); TBADD(ERR_RANGE,0,NULL); goto cleanup; }
            if (pplObjMatrix(out,0,1,s1,s2)==NULL) { sprintf(context->errStat.errBuff,"Out of memory."); TBADD(ERR_MEMORY,0,NULL); goto cleanup; }
            m = ((pplMatrix *)out->auxil)->m;
            ppl_unitsDimCpy(out,item);
            for (i=0; i<s2; i++)
             {
              long j;
              pplObj *item = (pplObj *)ppl_listIterate(&li);
              gsl_vector *v;
              if (item->objType!=PPLOBJ_VEC) { sprintf(context->errStat.errBuff,"When initialising a matrix from a list of vectors, all arguments must be vectors. Supplied argument has type <%s>.", pplObjTypeNames[item->objType]); TBADD(ERR_TYPE,0,NULL); goto cleanup; }
              v = ((pplVector*)item->auxil)->v;
              if (v->size != s1) { sprintf(context->errStat.errBuff,"When initialising a matrix from a list of vectors, the vectors must have consistent lengths. Supplied vector has length %ld whereas previous arguments have had a length %ld.",(long)v->size,s1); TBADD(ERR_RANGE,0,NULL); goto cleanup; }
              if (!ppl_unitsDimEqual(out, item)) { sprintf(context->errStat.errBuff,"When initialising a matrix from a list of vectors, the vectors must all have the same dimensions. Supplied vectors have units of <%s> and <%s>.", ppl_printUnit(context, out, NULL, NULL, 0, 1, 0), ppl_printUnit(context, item, NULL, NULL, 1, 1, 0) ); TBADD(ERR_UNIT,0,NULL); goto cleanup; }
              for (j=0; j<s1; j++) { gsl_matrix_set(m, j, i, gsl_vector_get(v,j) ); }
             }
            goto cleanup;
           }
          else if (item->objType==PPLOBJ_LIST)
           {
            long i;
            const long s1 = ((list*)item->auxil)->length;
            const long s2 = len;
            gsl_matrix *m;
            if (nArgs!=1) { sprintf(context->errStat.errBuff,"When initialising a matrix from a list of lists, only one argument should be supplied. %d arguments were supplied.",nArgs); TBADD(ERR_RANGE,0,NULL); goto cleanup; }
            if (s1==0) { sprintf(context->errStat.errBuff,"Cannot create a matrix of dimension zero."); TBADD(ERR_MEMORY,0,NULL); goto cleanup; }
            if (pplObjMatrix(out,0,1,s1,s2)==NULL) { sprintf(context->errStat.errBuff,"Out of memory."); TBADD(ERR_MEMORY,0,NULL); goto cleanup; }
            m = ((pplMatrix *)out->auxil)->m;
            ppl_unitsDimCpy(out,((pplObj*)((list*)item->auxil)->first->data));
            for (i=0; i<s2; i++)
             {
              long j;
              pplObj *item = (pplObj *)ppl_listIterate(&li);
              if (item->objType!=PPLOBJ_LIST) { sprintf(context->errStat.errBuff,"When initialising a matrix from a list of lists, all arguments must be lists. Supplied argument has type <%s>.", pplObjTypeNames[item->objType]); TBADD(ERR_TYPE,0,NULL); goto cleanup; }
              list *listin2 = (list *)item->auxil;
              listIterator *li2 = ppl_listIterateInit(listin2);
              if (listin2->length != s1) { sprintf(context->errStat.errBuff,"When initialising a matrix from a list of lists, the lists must have consistent lengths. Supplied list has length %ld whereas previous lists have had a length %ld.",(long)listin2->length,s1); TBADD(ERR_RANGE,0,NULL); goto cleanup; }
              for (j=0; j<s1; j++)
               {
                pplObj *item2 = (pplObj *)ppl_listIterate(&li2);
                if (item2->objType!=PPLOBJ_NUM) { sprintf(context->errStat.errBuff,"When initialising a matrix from a list of lists, the elements must all be numerical values; supplied element has type <%s>.", pplObjTypeNames[item2->objType]); TBADD(ERR_RANGE,0,NULL); goto cleanup; }
                if (!ppl_unitsDimEqual(out, item2)) { sprintf(context->errStat.errBuff,"When initialising a matrix from a list of lists, all of the elements must have the same dimensions. Supplied elements have units of <%s> and <%s>.", ppl_printUnit(context, out, NULL, NULL, 0, 1, 0), ppl_printUnit(context, item2, NULL, NULL, 1, 1, 0) ); TBADD(ERR_UNIT,0,NULL); goto cleanup; }
                if (item->flagComplex) { sprintf(context->errStat.errBuff,"Matrices can only hold real numbers; supplied elements are complex."); TBADD(ERR_NUMERIC,0,NULL); goto cleanup; }
                gsl_matrix_set(m, j, i, item2->real );
               }
             }
            goto cleanup;
           }
          else { sprintf(context->errStat.errBuff,"Cannot initialise a matrix from an object of type <%s>.", pplObjTypeNames[item->objType]); TBADD(ERR_TYPE,0,NULL); goto cleanup; }
         }
        else { sprintf(context->errStat.errBuff,"Cannot initialise a matrix from an object of type <%s>.", pplObjTypeNames[args[0].objType]); TBADD(ERR_TYPE,0,NULL); goto cleanup; }
        goto cleanup;
     }
   }

  // Otherwise object being called must be a function object
  if (t != PPLOBJ_FUNC) { sprintf(context->errStat.errBuff,"Object of type <%s> cannot be called as a function.",pplObjTypeNames[t]); TBADD(ERR_TYPE,0,NULL); goto cleanup; }
  fn = (pplFunc *)called.auxil;

  if (fn->functionType==PPL_FUNC_MAGIC)
   {
    if (fn->minArgs==1) // unit()
     {
      int     end, errPos=-1;
      char   *u = (char*)context->stack[context->stackPtr-1].auxil;
      if (nArgs != 1) { sprintf(context->errStat.errBuff,"The unit() function takes exactly one argument; %d supplied.",nArgs); TBADD(ERR_TYPE,0,NULL); goto cleanup; }
      ppl_unitsStringEvaluate(context, u, out, &end, &errPos, context->errStat.errBuff);
      if (errPos>=0) { TBADD(ERR_UNIT,0,NULL); goto cleanup; }
      if (end!=strlen(u)) { sprintf(context->errStat.errBuff,"Unexpected trailing matter after unit string."); TBADD(ERR_UNIT,0,NULL); goto cleanup; }
     }
    else if (fn->minArgs==2) // diff_d()
     {
      pplObj v, *step, *xpos;
      if      (nArgs == 3) step = &v;
      else if (nArgs == 4) step = &args[3];
      else    { sprintf(context->errStat.errBuff,"The diff_d() function takes two or thee arguments; %d supplied.",nArgs-1); TBADD(ERR_TYPE,0,NULL); goto cleanup; }
      if (args[0].objType!=PPLOBJ_STR) { sprintf(context->errStat.errBuff,"Dummy variable not passed to diff_d() as a string"); TBADD(ERR_INTERNAL,0,NULL); goto cleanup; }
      if (args[1].objType!=PPLOBJ_STR) { sprintf(context->errStat.errBuff,"Differentiation expression not passed to diff_d() as a string"); TBADD(ERR_INTERNAL,0,NULL); goto cleanup; }
      if (args[2].objType!=PPLOBJ_NUM) { sprintf(context->errStat.errBuff,"The diff_d() function requires a number as its second argument; supplied argument has type <%s>.",pplObjTypeNames[args[2].objType]); TBADD(ERR_TYPE,0,NULL); goto cleanup; }
      if ((nArgs==4)&&(args[3].objType!=PPLOBJ_NUM)) { sprintf(context->errStat.errBuff,"The diff_d() function requires a number as its third argument; supplied argument has type <%s>.",pplObjTypeNames[args[3].objType]); TBADD(ERR_TYPE,0,NULL); goto cleanup; }
      xpos = &args[2];
      memcpy(&v,xpos,sizeof(pplObj)); v.imag=0; v.real = hypot(xpos->real,xpos->imag)*1e-6; v.flagComplex=0;
      if (v.real<DBL_MIN*1e6) v.real=1e-6;
      ppl_expDifferentiate(context,inExpr,(char*)args[1].auxil,(char*)args[0].auxil,xpos,step,out,dollarAllowed,iterDepth);
      if (context->errStat.status) { goto cleanup; }
     }
    else if (fn->minArgs==3) // int_d()
     {
      if (nArgs != 4) { sprintf(context->errStat.errBuff,"The int_d() function takes two or three arguments; %d supplied.",nArgs-1); TBADD(ERR_TYPE,0,NULL); goto cleanup; }
      if (args[0].objType!=PPLOBJ_STR) { sprintf(context->errStat.errBuff,"Dummy variable not passed to int_d() as a string"); TBADD(ERR_INTERNAL,0,NULL); goto cleanup; }
      if (args[1].objType!=PPLOBJ_STR) { sprintf(context->errStat.errBuff,"Integration expression not passed to diff_d() as a string"); TBADD(ERR_INTERNAL,0,NULL); goto cleanup; }
      if (args[2].objType!=PPLOBJ_NUM) { sprintf(context->errStat.errBuff,"The int_d() function requires a number as its second argument; supplied argument has type <%s>.",pplObjTypeNames[args[2].objType]); TBADD(ERR_TYPE,0,NULL); goto cleanup; }
      if (args[3].objType!=PPLOBJ_NUM) { sprintf(context->errStat.errBuff,"The int_d() function requires a number as its third argument; supplied argument has type <%s>.",pplObjTypeNames[args[3].objType]); TBADD(ERR_TYPE,0,NULL); goto cleanup; }
      ppl_expIntegrate(context,inExpr,(char*)args[1].auxil,(char*)args[0].auxil,&args[2],&args[3],out,dollarAllowed,iterDepth);
      if (context->errStat.status) { goto cleanup; }
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
        if (context->set->term_current.ExplicitErrors == SW_ONOFF_ON) { sprintf(context->errStat.errBuff,"Function takes exactly %d arguments; %d supplied.",fn->maxArgs,nArgs); TBADD(ERR_TYPE,0,NULL); goto cleanup; }
        else                                                          { context->stack[context->stackPtr-1-nArgs].real = GSL_NAN; goto cleanup; }
       }
     }
    else if (nArgs < fn->minArgs) 
     {
      if (context->set->term_current.ExplicitErrors == SW_ONOFF_ON) { sprintf(context->errStat.errBuff,"Function takes a minimum of %d arguments; %d supplied.",fn->minArgs,nArgs); TBADD(ERR_TYPE,0,NULL); goto cleanup; }
      else                                                          { context->stack[context->stackPtr-1-nArgs].real = GSL_NAN; goto cleanup; }
     }
    else if (nArgs > fn->maxArgs) 
     {
      if (context->set->term_current.ExplicitErrors == SW_ONOFF_ON) { sprintf(context->errStat.errBuff,"Function takes a maximum of %d arguments; %d supplied.",fn->maxArgs,nArgs); TBADD(ERR_TYPE,0,NULL); goto cleanup; }
      else                                                          { context->stack[context->stackPtr-1-nArgs].real = GSL_NAN; goto cleanup; }
     }

    if (fn->numOnly)
     for (i=0; i<nArgs; i++) if (args[i].objType!=PPLOBJ_NUM) 
      {
       if (context->set->term_current.ExplicitErrors == SW_ONOFF_ON) { sprintf(context->errStat.errBuff,"Function required numeric arguments; argument %d has type <%s>.",i+1,pplObjTypeNames[args[i].objType]); TBADD(ERR_TYPE,0,NULL); goto cleanup; }
       else                                                          { context->stack[context->stackPtr-1-nArgs].real = GSL_NAN; goto cleanup; }
      }

    if (fn->realOnly)
     for (i=0; i<nArgs; i++) if (args[i].flagComplex) 
      {
       if (context->set->term_current.ExplicitErrors == SW_ONOFF_ON) { sprintf(context->errStat.errBuff,"Function requires real arguments; argument %d is complex.",i+1); TBADD(ERR_TYPE,0,NULL); goto cleanup; }
       else                                                          { context->stack[context->stackPtr-1-nArgs].real = GSL_NAN; goto cleanup; }
      }

    if (fn->dimlessOnly)
     for (i=0; i<nArgs; i++) if (!args[i].dimensionless)
      {
       if (context->set->term_current.ExplicitErrors == SW_ONOFF_ON) { sprintf(context->errStat.errBuff,"Function requires dimensionless arguments; argument %d has dimensions of <%s>.",i+1,ppl_printUnit(context, &args[i], NULL, NULL, 1, 1, 0)); TBADD(ERR_TYPE,0,NULL); goto cleanup; }
       else                                                          { context->stack[context->stackPtr-1-nArgs].real = GSL_NAN; goto cleanup; }
      }

    if (fn->notNan)
     for (i=0; i<nArgs; i++)
      if ((!gsl_finite(args[i].real)) || (!gsl_finite(args[i].imag)))
       {
        if (context->set->term_current.ExplicitErrors == SW_ONOFF_ON) { sprintf(context->errStat.errBuff,"Function requires finite arguments; argument %d is not finite.",i+1); TBADD(ERR_TYPE,0,NULL); goto cleanup; }
        else                                                          { context->stack[context->stackPtr-1-nArgs].real = GSL_NAN; goto cleanup; }
       }

    switch (fn->functionType)
     {
      case PPL_FUNC_SYSTEM:
       {
        int stat=0, errType=-1;
        ((void(*)(ppl_context *, pplObj *, int, int *, int *, char *))fn->functionPtr)(context, args, nArgs, &stat, &errType, context->errStat.errBuff);
        if (stat) { TBADD(errType,0,NULL); goto cleanup; }
        break;
       }
      default:
        { sprintf(context->errStat.errBuff,"Call of unsupported function type."); TBADD(ERR_INTERNAL,0,NULL); goto cleanup; }
     }
   }
cleanup:
cast_fail:
  out->self_this = NULL;
  out->refCount = 1;
  for (i=0; i<nArgs; i++) { STACK_POP; }
  ppl_garbageObject(&called);
  return;
 }

