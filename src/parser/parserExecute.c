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
#include <math.h>
#include <string.h>

#include <gsl/gsl_math.h>

#include "coreUtils/errorReport.h"

#include "expressions/expEval.h"
#include "expressions/traceback_fns.h"

#include "settings/settingTypes.h"

#include "stringTools/asciidouble.h"
#include "stringTools/strConstants.h"

#include "parser/parser.h"

#include "userspace/context.h"
#include "userspace/garbageCollector.h"
#include "userspace/pplObj_fns.h"
#include "userspace/pplObjPrint.h"

#include "pplConstants.h"

#define TBADD(et,pos) ppl_tbAdd(c,in->srcLineN,in->srcId,in->srcFname,0,et,pos,in->linetxt,NULL)

void ppl_parserLinePrint(ppl_context *c, parserLine *in)
 {
  char *t = c->errcontext.tempErrStr;
  parserAtom *a = in->firstAtom;

  sprintf(t,"Parser line -- stack length %d\nOriginal line was: %s",in->stackLen,in->linetxt);
  ppl_report(&c->errcontext, NULL);
  if (in->containsMacros) ppl_report(&c->errcontext, "Contains macros.");

  for (a = in->firstAtom; a!=NULL; a=a->next)
   {
    if (a->literal!=NULL)
     {
      int i=0;
      sprintf(t+i,"Set %4d to literal of type %2d -- ",a->stackOutPos,a->literal->objType);
      i=strlen(t+i);
      pplObjPrint(c,a->literal,NULL,t+i,LSTR_LENGTH-i,0,0);
      i+=strlen(t+i);
      sprintf(t+i,".");
     }
    else
     {
      int i=0;
      sprintf(t+i,"Set %4d to expression -- %s.",a->stackOutPos,a->expr->ascii);
     }
    ppl_report(&c->errcontext, NULL);
   }

  ppl_report(&c->errcontext, "---------------");
  return;
 }

void ppl_parserOutFree(parserOutput *in)
 {
  int i;
  if (in==NULL) return;
  for (i=0; i<in->stackLen; i++) ppl_garbageObject(&in->stk[i]);
  free(in->stk);
  free(in);
  return;
 }

#define STACK_POP \
   { \
    c->stackPtr--; \
    if (c->stack[c->stackPtr].objType!=PPLOBJ_NUM) /* optimisation: Don't waste time garbage collecting numbers */ \
     { \
      ppl_garbageObject(&c->stack[c->stackPtr]); \
      if (c->stack[c->stackPtr].refCount != 0) { strcpy(c->errStat.errBuff,"Stack forward reference detected."); TBADD(ERR_INTERNAL,0); return; } \
    } \
   } \

void ppl_parserExecute(ppl_context *c, parserLine *in, int interactive, int iterDepth)
 {
  int           i;
  parserOutput *out  = NULL;
  pplObj       *stk  = NULL;
  char         *eB   = c->errStat.errBuff;
  int          *stkCharPos = NULL;

  // Check that recursion depth has not been exceeded
  if (iterDepth > MAX_RECURSION_DEPTH) { strcpy(eB,"Maximum recursion depth exceeded."); TBADD(ERR_OVERFLOW,0); return; }

  // If at bottom iteration depth, clean up stack now if there is any left-over junk
  if (iterDepth==0) while (c->stackPtr>0) { STACK_POP; }

  while ((in!=NULL)&&(!c->errStat.status)&&(!c->shellExiting)&&(!c->shellBroken)&&(!c->shellContinued)&&(!c->shellReturned))
   {
    parserAtom *item = in->firstAtom;

    //ppl_parserLinePrint(c,in);
    
    // If line contains macros, need to recompile it now
    if (in->containsMacros)
     {
      int stat=0;
      parserStatus *ps = NULL;
      parserLine   *pl = NULL;
      ppl_parserStatInit(&ps,&pl);
      if (ps==NULL) { strcpy(eB,"Out of memory."); TBADD(ERR_MEMORY,0); return; }
      stat = ppl_parserCompile(c, ps, in->srcLineN, in->srcId, in->srcFname, in->linetxt, 1, iterDepth+1);
      if (stat || c->errStat.status) break;
      ppl_parserExecute(c, pl, interactive, iterDepth+1);
      ppl_parserStatFree(&ps);
      in = in->next;
      continue;
     }

    // Initialise output stack
    out = (parserOutput *)malloc(sizeof(parserOutput));
    if (out==NULL) { strcpy(eB,"Out of memory."); TBADD(ERR_MEMORY,0); return; }
    stk = (pplObj *)malloc(in->stackLen*sizeof(pplObj));
    if (stk==NULL) { free(out); strcpy(eB,"Out of memory."); TBADD(ERR_MEMORY,0); return; }
    for (i=0; i<in->stackLen; i++) { stk[i].refCount=1; pplObjZom(&stk[i],0); }
    stkCharPos = (int *)malloc(in->stackLen*sizeof(int));
    if (stkCharPos==NULL) { free(out); free(stk); strcpy(eB,"Out of memory."); TBADD(ERR_MEMORY,0); return; }
    for (i=0; i<in->stackLen; i++) { stkCharPos[i]=-1; }
    out->stk        = stk;
    out->stkCharPos = stkCharPos;
    out->stackLen   = in->stackLen;

    // Loop over atoms
    while (item != NULL)
     {
      if (item->literal!=NULL)
       {
        // Atom is a literal: simply copy it
        pplObjCpy(&stk[item->stackOutPos],item->literal,0,0,1);
       }
      else
       {
        // Atom is an expression: evaluate it
        int lastOpAssign=0, passed=0, j=0, k=0, p=0;
        pplObj *val = ppl_expEval(c, item->expr, &lastOpAssign, 1, iterDepth+1);
        if (c->errStat.status)
         {
          if (item->options[1]=='E') // If expression has produced an error, and its second option is type E, store it for per-point evaluation
           {
            ppl_tbClear(c);
            pplObjExpression(&stk[item->stackOutPos],0,(void *)item->expr);
            item->expr->refCount++;
            goto gotTypeCexpression;
           }
          else
           {
            strcpy(c->errStat.errBuff,"");
            TBADD(ERR_GENERAL,item->linePos);
            break; // On error, stop
           }
         }

        // Check type of pplObj produced by expression, and complain if the wrong type
        strcpy(eB+k,"Expression evaluates to the wrong type: needed"); k+=strlen(eB+k);
        for (j=0; ((j<PARSER_TYPE_OPTIONS)&&(item->options[j]!='\0')&&(!passed)); j++)
         {
          ppl_context *context = c; // Needed for CAST_TO_BOOL
          if (j!=0) { strcpy(eB+k,", or"); k+=strlen(eB+k); }
          switch(item->options[j])
           {
            case 'a':
              strcpy(eB+k," an axis number"); k+=strlen(eB+k);
              if (val->objType != PPLOBJ_NUM) break;
              if (val->flagComplex) break;
              if (!val->dimensionless) break;
              if ( (val->real <= 0) || (val->real >= MAX_AXES) ) break;
              val->exponent[0] = (double)(item->options[++j]-'0');
              passed=1;
              break;
            case 'A':
              strcpy(eB+k," an angle"); k+=strlen(eB+k);
              if (val->objType != PPLOBJ_NUM) break;
              if (val->flagComplex) break;
              if (val->dimensionless) { val->real *= M_PI/180; passed=1; break; }
              for (p=0; p<UNITS_MAX_BASEUNITS; p++) if (val->exponent[p] != UNIT_ANGLE) { p=-1; break; }
              passed=(p>0);
              break;
            case 'b':
              CAST_TO_BOOL(val);
              passed=1;
              break;
            case 'c':
              strcpy(eB+k," a color"); k+=strlen(eB+k);
              if ( (val->objType!=PPLOBJ_COL) &&
                  ((val->objType!=PPLOBJ_NUM)||(!val->dimensionless)||(val->flagComplex)||(val->real<INT_MIN)||(val->real>INT_MAX))
                 )  break;
              passed=1;
              break;
            case 'd':
              strcpy(eB+k," an integer"); k+=strlen(eB+k);
              if (val->objType != PPLOBJ_NUM) break;
              if (val->flagComplex) break;
              if (!val->dimensionless) break;
              if ( (val->real < INT_MIN) || (val->real > INT_MAX) ) break;
              passed=1;
              break;
            case 'D':
              strcpy(eB+k," a length"); k+=strlen(eB+k);
              if (val->objType != PPLOBJ_NUM) break;
              if (val->flagComplex) break;
              if (val->dimensionless) { val->real *= 0.01; passed=1; break; }
              for (p=0; p<UNITS_MAX_BASEUNITS; p++) if (val->exponent[p] != UNIT_LENGTH) { p=-1; break; }
              passed=(p>0);
              break;
            case 'E':
              passed=0;
              break;
            case 'f':
              strcpy(eB+k," a number"); k+=strlen(eB+k);
              if (val->objType != PPLOBJ_NUM) break;
              if (val->flagComplex) break;
              if (!val->dimensionless) break;
              passed=1;
              break;
            case 'o':
              passed=1;
              break;
            case 'p':
            case 'P':
              strcpy(eB+k," a position vector"); k+=strlen(eB+k);
              if (val->objType == PPLOBJ_VEC)
               {
                double      m=1,x=0,y=0,z=0;
                gsl_vector *v=((pplVector *)val->auxil)->v;
                int         l=v->size;
                if ( (l<2) || (l>((item->options[j]=='P')?3:2)) ) break; // wrong size
                if (val->dimensionless) m=0.01;
                else
                 {
                  for (p=0; p<UNITS_MAX_BASEUNITS; p++) if (val->exponent[p] != UNIT_LENGTH) { p=-1; break; }
                  if (p<0) break; // vector has wrong units
                 }
                if (l>0) x = gsl_vector_get(v,0) * m;
                if (l>1) y = gsl_vector_get(v,1) * m;
                if (l>2) z = gsl_vector_get(v,2) * m;
                ppl_garbageObject(val);
                                           pplObjNum(val                      ,0,x,0);
                                           pplObjNum(&stk[item->stackOutPos+1],0,y,0);
                if (item->options[j]=='P') pplObjNum(&stk[item->stackOutPos+2],0,z,0);
                passed=1;
               }
              else if (val->objType == PPLOBJ_LIST)
               {
                double  x[3]={0,0,0};
                list   *li=(list *)val->auxil;
                int     l=li->length, i;
                if ( (l<2) || (l>((item->options[j]=='P')?3:2)) ) break; // wrong size
                for (i=0; i<l; i++)
                 {
                  double  m=1;
                  pplObj *xo = ppl_listGetItem(li,i);
                  if (xo->objType != PPLOBJ_NUM) break;
                  if (xo->flagComplex) break;
                  if (xo->dimensionless) m=0.01;
                  else
                   {
                    for (p=0; p<UNITS_MAX_BASEUNITS; p++) if (val->exponent[p] != UNIT_LENGTH) { p=-1; break; }
                    if (p<0) break; // vector has wrong units
                   }
                  x[i] = xo->real * m;
                 }
                ppl_garbageObject(val);
                                           pplObjNum(val                      ,0,x[0],0);
                                           pplObjNum(&stk[item->stackOutPos+1],0,x[1],0);
                if (item->options[j]=='P') pplObjNum(&stk[item->stackOutPos+2],0,x[2],0);
                passed=1;
               }
              break; // fail
            case 'q':
              strcpy(eB+k," a string"); k+=strlen(eB+k);
              if (val->objType != PPLOBJ_STR) break;
              passed=1;
              break;
            case 'u':
              strcpy(eB+k," a physical quantity"); k+=strlen(eB+k);
              if (val->objType != PPLOBJ_NUM) break;
              passed=1;
              break;
            default:
              ppl_fatal(&c->errcontext,__FILE__,__LINE__,"Illegal option encountered.");
           }
         }

        // If expression has produced a value of the wrong type, throw an error
        if (!passed)
         {
          strcpy(eB+k,".");
          TBADD(ERR_TYPE, item->linePos);
          break;
         }

        // Copy value produced by atom
        pplObjCpy(&stk[item->stackOutPos], val, 0, 0, 1);
       }
gotTypeCexpression:
      stkCharPos[item->stackOutPos] = item->linePos;
      item = item->next;
     }

    // If no error, pass command to shell
    if (!c->errStat.status)
     {
      ppl_parserShell(c, in, out, interactive, iterDepth);
     }

    // Free output stack
    ppl_parserOutFree(out);

    in = in->next;
   }
  return;
 }

