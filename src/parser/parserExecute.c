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

#include "pplConstants.h"

#define TBADD(et,pos,lt) ppl_tbAdd(c,in->srcLineN,in->srcId,in->srcFname,0,et,pos,lt)

void ppl_parserOutFree(parserOutput *in)
 {
  int i;
  if (in==NULL) return;
  for (i=0; i<in->stackLen; i++) ppl_garbageObject(&in->stk[i]);
  free(in->stk);
  free(in);
  return;
 }

void ppl_parserExecute(ppl_context *c, parserLine *in, int iterDepth)
 {
  int           i;
  parserOutput *out  = NULL;
  parserAtom   *item = in->firstAtom;
  pplObj       *stk  = NULL;
  char         *eB   = c->errStat.errBuff;

  // Check that recursion depth has not been exceeded
  if (iterDepth > MAX_RECURSION_DEPTH) { strcpy(eB,"Maximum recursion depth exceeded."); TBADD(ERR_OVERFLOW,0,in->linetxt); return; }

  while (in != NULL)
   {
    // If line contains macros, need to recompile it now
    if (in->containsMacros)
     {
      int stat=0;
      parserStatus *ps = NULL;
      parserLine   *pl = NULL;
      ppl_parserStatInit(&ps,&pl);
      if (ps==NULL) { strcpy(eB,"Out of memory."); TBADD(ERR_MEMORY,0,in->linetxt); return; }
      stat = ppl_parserCompile(c, ps, in->srcLineN, in->srcId, in->srcFname, in->linetxt, 1, iterDepth+1);
      if (stat || c->errStat.status) break;
      ppl_parserExecute(c, pl, iterDepth+1);
      ppl_parserStatFree(&ps);
      in = in->next;
      continue;
     }

    // Initialise output stack
    out = (parserOutput *)malloc(sizeof(parserOutput));
    if (out==NULL) { strcpy(eB,"Out of memory."); TBADD(ERR_MEMORY,0,in->linetxt); return; }
    stk = (pplObj *)malloc(in->stackLen*sizeof(pplObj));
    if (stk==NULL) { free(out); strcpy(eB,"Out of memory."); TBADD(ERR_MEMORY,0,in->linetxt); return; }
    for (i=0; i<in->stackLen; i++) { stk[i].refCount=1; pplObjZom(&stk[i],0); }
    out->stk = stk;
    out->stackLen = in->stackLen;

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
        if (c->errStat.status) { ppl_tbWasInSubstring(c, item->linePos, in->linetxt); break; } // On error, stop

        // Check type of pplObj produced by expression, and complain if the wrong type
        strcpy(eB+k,"Expression evaluates to the wrong type: needed"); k+=strlen(eB+k);
        for (j=0; ((j<PARSER_TYPE_OPTIONS)&&(item->options[j]!='\0')&&(!passed)); j++)
         {
          ppl_context *context = c; // Needed for CAST_TO_BOOL
          if (j!=0) { strcpy(eB+k,", or"); k+=strlen(eB+k); }
          switch(item->options[j])
           {
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
              if (val->objType != PPLOBJ_COL) break;
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
              if (val->objType != PPLOBJ_VEC)
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
                                           pplObjNum(val      ,0,x,0);
                                           pplObjNum(&stk[i+1],0,y,0);
                if (item->options[j]=='P') pplObjNum(&stk[i+2],0,z,0);
               }
              else if (val->objType != PPLOBJ_LIST)
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
                                           pplObjNum(val      ,0,x[0],0);
                                           pplObjNum(&stk[i+1],0,x[1],0);
                if (item->options[j]=='P') pplObjNum(&stk[i+2],0,x[2],0);
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
          TBADD(ERR_TYPE, item->linePos, in->linetxt);
          break;
         }

        // Copy value produced by atom
        pplObjCpy(&stk[item->stackOutPos], val, 0, 0, 1);
       }
      item = item->next;
     }

    // If no error, pass command to shell
    if (!c->errStat.status)
     {
      ppl_parserShell(c, out, iterDepth);
     }

    // Free output stack
    ppl_parserOutFree(out);

    in = in->next;
   }
  return;
 }

