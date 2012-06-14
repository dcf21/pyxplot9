// eqnsolve.c
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

#define _EQNSOLVE_C 1

#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <string.h>

#include <gsl/gsl_errno.h>
#include <gsl/gsl_multimin.h>

#include "commands/eqnsolve.h"
#include "coreUtils/errorReport.h"
#include "expressions/expCompile.h"
#include "expressions/expEval.h"
#include "expressions/traceback_fns.h"
#include "mathsTools/dcfmath.h"
#include "parser/cmdList.h"
#include "parser/parser.h"
#include "settings/settingTypes.h"
#include "stringTools/asciidouble.h"
#include "userspace/context.h"
#include "userspace/contextVarDef.h"
#include "userspace/garbageCollector.h"
#include "userspace/pplObj_fns.h"
#include "userspace/pplObjPrint.h"
#include "userspace/pplObjUnits.h"
#include "userspace/unitsDisp.h"
#include "userspace/unitsArithmetic.h"
#include "stringTools/strConstants.h"

// Functions which we use to optimise in a log-log type space to help when
// solutions are many orders of magnitude different from initial guess
double ppl_optimise_RealToLog(double in, int iter, double *norm)
 {
  double a=-2;

  if ((iter==1) || (iter >= 4)) { *norm = ppl_max(1e-200, fabs(in)); return in/(*norm); }
  if (iter >=3) a = -500;

  if (in >= exp(a)) return log(in);
  else              return 2*a - log(2*exp(a)-in);
 }

double ppl_optimise_LogToReal(double in, int iter, double *norm)
 {
  double a=-2;

  if ((iter==1) || (iter >=4)) return in*(*norm);
  if (iter >=3) a = -500;

  if (in >= a) return exp(in);
  else         return 2*exp(a)-exp(2*a-in);
 }

// Structure used to communicate between top-level optimiser, and the
// evaluation slave called by GSL
typedef struct MMComm {
 ppl_context  *context;
 char         *inLine;
 int           iterDepth;
 int           expr1pos  [EQNSOLVE_MAXDIMS];
 int           expr2pos  [EQNSOLVE_MAXDIMS];
 pplExpr      *expr1     [EQNSOLVE_MAXDIMS];
 pplExpr      *expr2     [EQNSOLVE_MAXDIMS];
 char         *fitvarname[EQNSOLVE_MAXDIMS]; // Name of nth fit variable
 pplObj       *fitvar    [EQNSOLVE_MAXDIMS];
 double        norm      [EQNSOLVE_MAXDIMS];
 int           Nfitvars , Nexprs , GoneNaN, iter2;
 double        sign;
 pplObj        first  [EQNSOLVE_MAXDIMS];
 unsigned char isFirst[EQNSOLVE_MAXDIMS];
 double        WorstScore;
 int           warnPos,warnExprId,warnExprNo; // One final algebra error is allowed to produce a warning
 char          warntext[LSTR_LENGTH];
 } MMComm;

#define WORSTSCORE_INIT -(DBL_MAX/1e10)

static char *printParameterValues(MMComm *data, char *output)
 {
  int i=0, j=0;

  if (data->Nfitvars>1) { sprintf(output+j, "( "); j+=strlen(output+j); }
  for (i=0; i<data->Nfitvars; i++)
   {
    sprintf(output+j, "%s=", data->fitvarname[i]);
    j+=strlen(output+j);
    pplObjPrint(data->context,data->fitvar[i],data->fitvarname[i],output+j,LSTR_LENGTH-j,0,1);
    j+=strlen(output+j);
    sprintf(output+j, "; ");
    j+=strlen(output+j);
   }
  if (j>2)
   {
    if (data->Nfitvars>1) strcpy(output+j-2, " )");
    else                  strcpy(output+j-2, ""  );
   }
  else                    strcpy(output    , "()");

  return output;
 }

static double MultiMinSlave(const gsl_vector *x, void *params)
 {
  pplObj        output1, output2, *tmp;
  int           i, lastOpAssign;
  unsigned char squareroot=0;
  double        accumulator=0.0, output;
  MMComm       *data = (MMComm *)params;
  ppl_context  *c    = data->context;

  if (c->errStat.status) return GSL_NAN; // We've previously had a serious error... so don't do any more work

  if (c->set->term_current.ComplexNumbers == SW_ONOFF_OFF)
   {
    for (i=0; i<data->Nfitvars; i++)
      data->fitvar[i]->real = ppl_optimise_LogToReal( gsl_vector_get(x, i) , data->iter2, &data->norm[i] );
   }
  else
   {
    for (i=0; i<data->Nfitvars; i++)
     {
      data->fitvar[i]->real        = ppl_optimise_LogToReal( gsl_vector_get(x,2*i  ) , data->iter2, &data->norm[2*i  ] );
      data->fitvar[i]->imag        = ppl_optimise_LogToReal( gsl_vector_get(x,2*i+1) , data->iter2, &data->norm[2*i+1] );
      data->fitvar[i]->flagComplex = !ppl_dblEqual(data->fitvar[i]->imag, 0);
      if (!data->fitvar[i]->flagComplex) data->fitvar[i]->imag=0.0; // Enforce that real numbers have positive zero imaginary components
     }
   }

  for (i=0; i<data->Nexprs; i++)
   {
    tmp = ppl_expEval(c, data->expr1[i], &lastOpAssign, 0, data->iterDepth+1);

    // If a numerical error happened; ignore it for now, but return NAN
    if (c->errStat.status) { data->warnExprId=i; data->warnExprNo=1; data->warnPos=c->errStat.errPosExpr; sprintf(data->warntext, "An algebraic error was encountered at %s: %s", printParameterValues(data,c->errcontext.tempErrStr), c->errStat.errMsgExpr); ppl_tbClear(c); return GSL_NAN; }

    // Check that expression evaluates a number
    if ((tmp==NULL) || (tmp->objType!=PPLOBJ_NUM))
     {
      strcpy(c->errStat.errBuff, "Supplied expression does not evaluate to a numeric quantity.");
      ppl_tbAdd(c,data->expr1[i]->srcLineN,data->expr1[i]->srcId,data->expr1[i]->srcFname,0,ERR_TYPE,data->expr1pos[i],data->inLine,"");
      ppl_garbageObject(&c->stack[--c->stackPtr]); // trash and pop tmp from stack
      return GSL_NAN;
     }
    memcpy(&output1,tmp,sizeof(pplObj));
    ppl_garbageObject(&c->stack[--c->stackPtr]); // trash and pop tmp from stack

    if (data->expr2[i] != NULL)
     {
      tmp = ppl_expEval(c, data->expr2[i], &lastOpAssign, 0, data->iterDepth+1);

      // If a numerical error happened; ignore it for now, but return NAN
      if (c->errStat.status) { data->warnExprId=i; data->warnExprNo=2; data->warnPos=c->errStat.errPosExpr; sprintf(data->warntext, "An algebraic error was encountered at %s: %s", printParameterValues(data,c->errcontext.tempErrStr), c->errStat.errMsgExpr); ppl_tbClear(c); return GSL_NAN; }

      // Check that expression evaluates a number
      if ((tmp==NULL) || (tmp->objType!=PPLOBJ_NUM))
       {
        strcpy(c->errStat.errBuff, "Supplied expression does not evaluate to a numeric quantity.");
        ppl_tbAdd(c,data->expr2[i]->srcLineN,data->expr2[i]->srcId,data->expr2[i]->srcFname,0,ERR_TYPE,data->expr2pos[i],data->inLine,"");
        ppl_garbageObject(&c->stack[--c->stackPtr]); // trash and pop tmp from stack
        return GSL_NAN;
       }
      memcpy(&output2,tmp,sizeof(pplObj));
      ppl_garbageObject(&c->stack[--c->stackPtr]); // trash and pop tmp from stack

      if (!ppl_unitsDimEqual(&output1, &output2))
       {
        sprintf(c->errStat.errBuff, "The two sides of the equation which is being solved are not dimensionally compatible. The left side has dimensions of <%s> while the right side has dimensions of <%s>.",ppl_printUnit(c,&output1, NULL, NULL, 0, 1, 0),ppl_printUnit(c,&output2, NULL, NULL, 1, 1, 0));
        ppl_tbAdd(c,data->expr1[i]->srcLineN,data->expr1[i]->srcId,data->expr1[i]->srcFname,0,ERR_UNIT,data->expr1pos[i],data->inLine,"");
        return GSL_NAN;
       }

#define TWINLOG(X) ((X>1e-200) ? log(X) : (2*log(1e-200) - log(2e-200-X)))
      accumulator += pow(TWINLOG(output1.real) - TWINLOG(output2.real) , 2); // Minimise sum of square deviations of many equations
      accumulator += pow(TWINLOG(output1.imag) - TWINLOG(output2.imag) , 2);
      squareroot = 1;
     } else {
      if (data->isFirst[i])
       {
        memcpy(&data->first[i], &output1, sizeof(pplObj));
        data->isFirst[i] = 0;
       } else {
        if (!ppl_unitsDimEqual(&data->first[i],&output1))
         {
          sprintf(c->errStat.errBuff, "The function being minimised or maximised does not have consistent units.");
          ppl_tbAdd(c,data->expr1[i]->srcLineN,data->expr1[i]->srcId,data->expr1[i]->srcFname,0,ERR_UNIT,data->expr1pos[i],data->inLine,"");
          return GSL_NAN;
         }
       }
      accumulator += output1.real; // ... or just the raw values in minimise and maximise commands
     }
   }
  if (squareroot) accumulator = sqrt(accumulator);
  output = accumulator * data->sign;
  if ((!gsl_finite(output)) && (data->WorstScore > WORSTSCORE_INIT)) { data->WorstScore *= 1.1; return data->WorstScore; }
  if (!gsl_finite(output)) { data->GoneNaN=1; }
  else                     { if (data->WorstScore < output) data->WorstScore = output; }
  return output;
 }

static void multiMinIterate(MMComm *commlink, parserLine *pl)
 {
  ppl_context                        *c = commlink->context;
  size_t                              iter = 0,iter2 = 0;
  int                                 i, Nparams, status=0;
  double                              size=0,sizelast=0,sizelast2=0;
  const gsl_multimin_fminimizer_type *T = gsl_multimin_fminimizer_nmsimplex; // We don't use nmsimplex2 here because it was new in gsl 1.12
  gsl_multimin_fminimizer            *s;
  gsl_vector                         *x, *ss;
  gsl_multimin_function               fn;

  Nparams = commlink->Nfitvars * ((c->set->term_current.ComplexNumbers == SW_ONOFF_OFF) ? 1:2);

  fn.n = Nparams;
  fn.f = &MultiMinSlave;
  fn.params = (void *)commlink;

  x  = gsl_vector_alloc( Nparams );
  ss = gsl_vector_alloc( Nparams );

  iter2=0;
  do
   {
    iter2++;
    commlink->iter2 = iter2;
    sizelast2 = size;

    // Transfer starting guess values from fitting variables into gsl_vector x
    if (c->set->term_current.ComplexNumbers == SW_ONOFF_OFF)
     {
      for (i=0; i<commlink->Nfitvars; i++)
        gsl_vector_set(x,  i  ,ppl_optimise_RealToLog(commlink->fitvar[i]->real , iter2, &commlink->norm[  i  ]));
     }
    else
     {
      for (i=0; i<commlink->Nfitvars; i++)
       {
        gsl_vector_set(x,2*i  ,ppl_optimise_RealToLog(commlink->fitvar[i]->real , iter2, &commlink->norm[2*i  ]));
        gsl_vector_set(x,2*i+1,ppl_optimise_RealToLog(commlink->fitvar[i]->imag , iter2, &commlink->norm[2*i+1]));
       }
     }

    for (i=0; i<Nparams; i++)
     {
      if (fabs(gsl_vector_get(x,i))>1e-6) gsl_vector_set(ss, i, 0.05 * gsl_vector_get(x,i));
      else                                gsl_vector_set(ss, i, 0.05                      ); // Avoid having a stepsize of zero
     }

    s = gsl_multimin_fminimizer_alloc (T, fn.n);
    gsl_multimin_fminimizer_set (s, &fn, x, ss);

    // If initial value we are giving the minimiser produces an algebraic error, it's not worth continuing
    MultiMinSlave(x,(void *)commlink);
    if (c->errStat.status) return;
    if (commlink->warnPos>=0)
     {
      int i = commlink->warnExprId;
      int j = commlink->warnExprNo;
      strcpy(c->errStat.errBuff, commlink->warntext);
      if (j==1) ppl_tbAdd(c,commlink->expr1[i]->srcLineN,commlink->expr1[i]->srcId,commlink->expr1[i]->srcFname,1,ERR_NUMERICAL,commlink->expr1pos[i] + commlink->warnPos,commlink->inLine,"");
      else      ppl_tbAdd(c,commlink->expr2[i]->srcLineN,commlink->expr2[i]->srcId,commlink->expr2[i]->srcFname,1,ERR_NUMERICAL,commlink->expr2pos[i] + commlink->warnPos,commlink->inLine,"");
      return;
     }

    iter                 = 0;
    commlink->GoneNaN    = 0;
    commlink->WorstScore = WORSTSCORE_INIT;
    do
     {
      iter++;
      // When you're minimising over many parameters simultaneously sometimes nothing happens for a long time
      for (i=0; i<2+Nparams*2; i++) { status = gsl_multimin_fminimizer_iterate(s); if (status) break; }
      if (status || c->errStat.status) break;
      sizelast = size;
      size     = gsl_multimin_fminimizer_minimum(s);
     }
    while ((iter < 10) || ((size < sizelast) && (iter < 50))); // Iterate 10 times, and then see whether size carries on getting smaller

    // Transfer best-fit values from s->x into fitting variables
    if (c->set->term_current.ComplexNumbers == SW_ONOFF_OFF)
     {
      for (i=0; i<commlink->Nfitvars; i++)
        commlink->fitvar[i]->real = ppl_optimise_LogToReal(gsl_vector_get(s->x,  i  ) , iter2, &commlink->norm[  i  ]);
     }
    else
     {
      for (i=0; i<commlink->Nfitvars; i++)
       {
        commlink->fitvar[i]->real = ppl_optimise_LogToReal(gsl_vector_get(s->x,2*i  ) , iter2, &commlink->norm[2*i  ]);
        commlink->fitvar[i]->imag = ppl_optimise_LogToReal(gsl_vector_get(s->x,2*i+1) , iter2, &commlink->norm[2*i+1]);
       }
     }

    gsl_multimin_fminimizer_free(s);
   }
  while ((iter2 < 4) || ((commlink->GoneNaN==0) && (!status) && (size < sizelast2) && (iter2 < 20))); // Iterate 2 times, and then see whether size carries on getting smaller

  if (iter2>=20) status=1;
  if (status)
   {
    sprintf(c->errStat.errBuff, "Failed to converge. GSL returned error: %s", gsl_strerror(status));
    ppl_tbAdd(c,pl->srcLineN,pl->srcId,pl->srcFname,0,ERR_UNIT,0,pl->linetxt,"");
    return;
   }
  gsl_vector_free(x);
  gsl_vector_free(ss);
  return;
 }

static void minOrMax(ppl_context *c, parserLine *pl, parserOutput *in, int interactive, int iterDepth, double sign)
 {
  pplObj       *stk = in->stk, *pos;
  MMComm        commlink;
  char         *varName;
  pplObj        dummyTemp;
  int           i;

  commlink.context   = c;
  commlink.inLine    = pl->linetxt;
  commlink.iterDepth = iterDepth;
  commlink.Nfitvars  = 0;
  commlink.Nexprs    = 1;

  // Fetch the names of the variables which we are fitting
  pos = &stk[PARSE_minimise_fit_variables];
  while ((pos->objType == PPLOBJ_NUM) && (pos->real > 0))
   {
    pos = &stk[ (int)round(pos->real) ];
    varName = (char *)(pos + PARSE_minimise_fit_variable_fit_variables)->auxil;
    ppl_contextGetVarPointer(c, varName, &commlink.fitvar[commlink.Nfitvars], &dummyTemp);
    if (dummyTemp.objType==PPLOBJ_NUM)
     {
      ppl_unitsDimCpy(commlink.fitvar[commlink.Nfitvars], &dummyTemp);
      commlink.fitvar[commlink.Nfitvars]->real        = dummyTemp.real;
      commlink.fitvar[commlink.Nfitvars]->imag        = dummyTemp.imag;
      commlink.fitvar[commlink.Nfitvars]->flagComplex = dummyTemp.flagComplex;
     }
    dummyTemp.amMalloced=0;
    ppl_garbageObject(&dummyTemp);
    commlink.fitvarname[ commlink.Nfitvars ] = varName;
    commlink.Nfitvars += 1;
    if (commlink.Nfitvars >= EQNSOLVE_MAXDIMS)
     {
      sprintf(c->errStat.errBuff, "Too many via variables; the maximum allowed number is %d.", EQNSOLVE_MAXDIMS);
      ppl_tbAdd(c,pl->srcLineN,pl->srcId,pl->srcFname,0,ERR_OVERFLOW,0,pl->linetxt,"");
      return;
     }
   }

  commlink.expr1[0]   = (pplExpr *)stk[PARSE_minimise_expression].auxil;
  commlink.expr1pos[0]= in->stkCharPos[PARSE_minimise_expression];
  commlink.expr2[0]   = NULL;
  commlink.sign       = sign;
  commlink.isFirst[0] = 1;
  commlink.warnPos    =-1;
  commlink.GoneNaN    = 0;
  commlink.WorstScore = WORSTSCORE_INIT;
  pplObjNum(&commlink.first[0],0,0,0);

  multiMinIterate(&commlink, pl);

  if ((c->errStat.status) || (commlink.GoneNaN==1))
   {
    for (i=0; i<commlink.Nfitvars; i++) { commlink.fitvar[i]->real=GSL_NAN; commlink.fitvar[i]->imag=0; commlink.fitvar[i]->flagComplex=0; } // We didn't produce a sensible answer
    return;
   }

  if (commlink.warnPos >= 0) ppl_warning(&c->errcontext, ERR_NUMERICAL, commlink.warntext);
  return;
 }

void ppl_directive_solve(ppl_context *c, parserLine *pl, parserOutput *in, int interactive, int iterDepth)
 {
  pplObj       *stk = in->stk;
  int           pos;
  MMComm        commlink;
  char         *varName;
  pplObj        dummyTemp;
  int           i;

  commlink.context   = c;
  commlink.inLine    = pl->linetxt;
  commlink.iterDepth = iterDepth;
  commlink.Nfitvars  = 0;
  commlink.Nexprs    = 0;

  // Fetch the names of the variables which we are fitting
  pos = PARSE_solve_fit_variables;
  while ((stk[pos].objType == PPLOBJ_NUM) && (stk[pos].real > 0))
   {
    pos = (int)round(stk[pos].real);
    varName = (char *)stk[pos+PARSE_solve_fit_variable_fit_variables].auxil;
    ppl_contextGetVarPointer(c, varName, &commlink.fitvar[commlink.Nfitvars], &dummyTemp);
    if (dummyTemp.objType==PPLOBJ_NUM)
     {
      ppl_unitsDimCpy(commlink.fitvar[commlink.Nfitvars], &dummyTemp);
      commlink.fitvar[commlink.Nfitvars]->real        = dummyTemp.real;
      commlink.fitvar[commlink.Nfitvars]->imag        = dummyTemp.imag;
      commlink.fitvar[commlink.Nfitvars]->flagComplex = dummyTemp.flagComplex;
     }
    dummyTemp.amMalloced=0;
    ppl_garbageObject(&dummyTemp);
    commlink.fitvarname[ commlink.Nfitvars ] = varName;
    commlink.Nfitvars += 1;
    if (commlink.Nfitvars >= EQNSOLVE_MAXDIMS)
     {
      sprintf(c->errStat.errBuff, "Too many via variables; the maximum allowed number is %d.", EQNSOLVE_MAXDIMS);
      ppl_tbAdd(c,pl->srcLineN,pl->srcId,pl->srcFname,0,ERR_OVERFLOW,in->stkCharPos[pos+PARSE_solve_fit_variable_fit_variables],pl->linetxt,"");
      return;
     }
   }


  // Fetch a list of the equations to solve
  pos = PARSE_solve_expressions;
  while ((stk[pos].objType == PPLOBJ_NUM) && (stk[pos].real > 0))
   {
    pos = (int)round(stk[pos].real);
    commlink.expr1   [commlink.Nexprs] = (pplExpr *)stk[pos+PARSE_solve_left_expression_expressions].auxil;
    commlink.expr1pos[commlink.Nexprs] = in->stkCharPos[pos+PARSE_solve_left_expression_expressions];
    commlink.expr2   [commlink.Nexprs] = (pplExpr *)stk[pos+PARSE_solve_right_expression_expressions].auxil;
    commlink.expr2pos[commlink.Nexprs] = in->stkCharPos[pos+PARSE_solve_right_expression_expressions];
    commlink.Nexprs += 1;
    if (commlink.Nexprs >= EQNSOLVE_MAXDIMS)
     {
      sprintf(c->errStat.errBuff, "Too many simultaneous equations to solve; the maximum allowed number is %d.", EQNSOLVE_MAXDIMS);
      ppl_tbAdd(c,pl->srcLineN,pl->srcId,pl->srcFname,0,ERR_OVERFLOW,in->stkCharPos[pos+PARSE_solve_left_expression_expressions],pl->linetxt,"");
      return;
     }
   }

 if (commlink.Nexprs < 1)
  {
   sprintf(c->errStat.errBuff, "No equations supplied to solve.");
   ppl_tbAdd(c,pl->srcLineN,pl->srcId,pl->srcFname,0,ERR_NUMERICAL,0,pl->linetxt,"");
   return;
  }

  commlink.sign       = 1.0;
  commlink.warnPos    =-1;
  commlink.GoneNaN    = 0;
  commlink.WorstScore = WORSTSCORE_INIT;
  for (i=0; i<commlink.Nexprs; i++) { commlink.isFirst[i]=1; pplObjNum(&commlink.first[i],0,0,0); }

  multiMinIterate(&commlink, pl);

  if ((c->errStat.status) || (commlink.GoneNaN==1))
   {
    for (i=0; i<commlink.Nfitvars; i++) { commlink.fitvar[i]->real=GSL_NAN; commlink.fitvar[i]->imag=0; commlink.fitvar[i]->flagComplex=0; } // We didn't produce a sensible answer
    return;
   }

  if (commlink.warnPos >= 0) ppl_warning(&c->errcontext, ERR_NUMERICAL, commlink.warntext);
  return;
 }

void ppl_directive_minimise(ppl_context *c, parserLine *pl, parserOutput *in, int interactive, int iterDepth)
 {
  minOrMax(c, pl, in, interactive, iterDepth,  1.0);
 }

void ppl_directive_maximise(ppl_context *c, parserLine *pl, parserOutput *in, int interactive, int iterDepth)
 {
  minOrMax(c, pl, in, interactive, iterDepth, -1.0);
 }

