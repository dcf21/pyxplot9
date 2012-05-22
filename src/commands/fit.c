// fit.c
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

#define _FIT_C 1

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <gsl/gsl_deriv.h>
#include <gsl/gsl_errno.h>
#include <gsl/gsl_linalg.h>
#include <gsl/gsl_math.h>
#include <gsl/gsl_matrix.h>
#include <gsl/gsl_multimin.h>
#include <gsl/gsl_permutation.h>
#include <gsl/gsl_vector.h>

#include "commands/fit.h"
#include "coreUtils/errorReport.h"
#include "coreUtils/memAlloc.h"
#include "expressions/expCompile_fns.h"
#include "expressions/traceback_fns.h"
#include "expressions/expEval.h"
#include "parser/cmdList.h"
#include "parser/parser.h"
#include "settings/settings.h"
#include "settings/settingTypes.h"
#include "stringTools/asciidouble.h"
#include "stringTools/strConstants.h"
#include "userspace/context.h"
#include "userspace/contextVarDef.h"
#include "userspace/garbageCollector.h"
#include "userspace/pplObj_fns.h"
#include "userspace/pplObjFunc.h"
#include "userspace/unitsArithmetic.h"
#include "userspace/unitsDisp.h"

#include "datafile.h"

// Structure used for passing data around
typedef struct fitComm {
 ppl_context      *c;
 parserLine       *pl;
 int               NArgs; // The number of arguments taken by the function that we're fitting
 int               NExpect; // The total number of columns we're reading from datafile; equals NArgs, plus the target value for f(), plus possibly the error on each target
 int               NFitVars; // The number of variables listed after via ....
 int               NParams; // The total number of free parameters in the fitting problems; either equals NFitVars, or twice this if complex arithmetic is enabled
 long int          NDataPoints; // The number of data points read from the supplied datafile
 pplObj          **varObj; // Pointers to the values of the variables which we're fitting the values of in the user's variable space
 const gsl_vector *paramVals; // Trial parameter values to be tried by fitResidual in this iteration
 const gsl_vector *bestFitParamVals; // The best fit parameter values (only set after first round of minimisation)
 gsl_vector       *paramValsHessian; // Internal variable used by GetHessian() to vary parameter values when differentiating
 pplObj           *firstVals; // The first values found in each column of the supplied datafile. These determine the physical units associated with each column.
 double           *dataTable; // Two-dimensional table of the data read from the datafile.
 unsigned char     flagYErrorBars; // If true, the user has specified errorbars for each target value. If false, we have no idea of uncertainty.
 char             *scratchPad, *errtext, *functionName; // String workspaces
 unsigned char     goneNaN; // Used by the minimiser to keep track of when the function being minimised has returned NAN.
 double            sigmaData; // The assumed errorbar (uniform for all datapoints) on the supplied target values if errorbars are not supplied. We fit this.
 int               diff1    , diff2; // The numbers of the free parameters currently being differentiated inside GetHessian()
 double            diff1step, diff2step; // The step size which GetHessian() recommends using for each of the parameters being differentiated
} fitComm;

#define STACK_POP \
   { \
    c->stackPtr--; \
    ppl_garbageObject(&c->stack[c->stackPtr]); \
    if (c->stack[c->stackPtr].refCount != 0) { strcpy(errText,"Stack forward reference detected."); return GSL_NAN; } \
   }

#define STACK_CLEAN    while (c->stackPtr>stkLevelOld) { STACK_POP; }

#define TBADD2(et,pos) ppl_tbAdd(c,pl->srcLineN,pl->srcId,pl->srcFname,0,et,pos,pl->linetxt,"")

// Routine for printing a GSL matrix in pythonesque format
static char *matrixPrint(ppl_context *c, const gsl_matrix *m, const size_t size, char *out)
 {
  size_t i,j,p=0;
  strcpy(out+p, "[ [");
  p+=strlen(out+p);
  for (i=0;i<size;i++)
   {
    for (j=0;j<size;j++) { sprintf(out+p,"%s,",ppl_numericDisplay(gsl_matrix_get(m,i,j),c->numdispBuff[0],c->set->term_current.SignificantFigures,0)); p+=strlen(out+p); }
    if (size>0) p--; // Delete final comma
    strcpy(out+p, "] , ["); p+=strlen(out+p); // New row
   }
  if (size>0) p-=3; // Delete final comma and open bracket
  strcpy(out+p, "]");
  return out;
 }

// Low-level routine for working out the mismatch between function and data for a given set of free parameter values
static double fitResidual(fitComm *p)
 {
  ppl_context *c = p->c;
  char        *errText = p->errtext;
  const int    stkLevelOld = c->stackPtr;
  int          i, k, errPos=-1, errType=0, lastOpAssign, explen;
  long int     j;
  double       accumulator, residual;
  pplObj      *out;
  pplExpr     *e=NULL;

  // Set free parameter values
  for (i=0; i<p->NFitVars; i++)
   if (c->set->term_current.ComplexNumbers == SW_ONOFF_OFF)   p->varObj[i]->real = gsl_vector_get(p->paramVals,   i  ); // This is setting variables in the user's variable space
   else                                                     { p->varObj[i]->real = gsl_vector_get(p->paramVals, 2*i  );
                                                              p->varObj[i]->imag = gsl_vector_get(p->paramVals, 2*i+1);
                                                            }

  accumulator = 0.0; // Add up sum of square residuals

  for (j=0; j<p->NDataPoints; j++) // Loop over all of the data points in the file that we're fitting
   {
    i=0;
    sprintf(p->scratchPad+i, "%s(", p->functionName); i+=strlen(p->scratchPad+i);
    for (k=0; k<p->NArgs; k++)
     {
      pplObj x = p->firstVals[k];
      x.real = p->dataTable[j*p->NExpect+k];
      x.imag = 0.0;
      x.flagComplex = 0;
      sprintf(p->scratchPad+i, "%s,", ppl_unitsNumericDisplay(c,&x, 0, 1, 20));
      i+=strlen(p->scratchPad+i);
     }
    if (p->NArgs>0) i--; // Remove final comma from list of arguments
    sprintf(p->scratchPad+i, ")");
    explen = i+1;
    ppl_error_setstreaminfo(&c->errcontext, 1, "fitting expression");
    ppl_expCompile(c,p->pl->srcLineN,p->pl->srcId,p->pl->srcFname,p->scratchPad,&explen,1,1,1,&e,&errPos,&errType,errText);
    if (c->errStat.status) { errPos=1; strcpy(errText, c->errStat.errMsgExpr); ppl_tbClear(c); return GSL_NAN; }
    if (errPos>=0)         { return GSL_NAN; }
    if (explen<i+1)        { strcpy(errText, "Unexpected trailing matter at the end of expression."); pplExpr_free(e); return GSL_NAN; }
    out = ppl_expEval(c, e, &lastOpAssign, 1, 1);
    pplExpr_free(e);
    if (c->errStat.status) { errPos=1; strcpy(errText, c->errStat.errMsgExpr); ppl_tbClear(c); STACK_CLEAN; return GSL_NAN; }
    if (out->objType!=PPLOBJ_NUM) { sprintf(errText, "The supplied function to fit produces a value which is not a number but has type <%s>.", pplObjTypeNames[out->objType]); STACK_CLEAN; return GSL_NAN; }
    if (!ppl_unitsDimEqual(out, p->firstVals+p->NArgs)) { sprintf(errText, "The supplied function to fit produces a value which is dimensionally incompatible with its target value. The function produces a result with dimensions of <%s>, while its target value has dimensions of <%s>.", ppl_printUnit(c,out,NULL,NULL,0,1,0), ppl_printUnit(c,p->firstVals+p->NArgs,NULL,NULL,1,1,0)); STACK_CLEAN; return GSL_NAN; }
    residual = pow(out->real - p->dataTable[j*p->NExpect+k] , 2) + pow(out->imag , 2); // Calculate squared deviation of function result from desired result
    if (p->flagYErrorBars) residual /= 2 * pow(p->dataTable[j*p->NExpect+k+1]    , 2); // Divide square residual by 2 sigma squared.
    else                   residual /= 2 * pow(p->sigmaData                      , 2);
    accumulator += residual; // ... and sum
   }

  STACK_CLEAN;
  return accumulator;
 }

// Slave routines called by the differentiation operation when working out the Hessian matrix

static double GetHessian_diff2(double x, void *p_void)
 {
  double tmp, output;
  fitComm *p = (fitComm *)p_void;
  tmp = gsl_vector_get(p->paramValsHessian, p->diff2); // Replace old parameter with x
  gsl_vector_set(p->paramValsHessian, p->diff2, x);
  output = fitResidual(p); // Evaluate residual
  gsl_vector_set(p->paramValsHessian, p->diff2, tmp); // Restore old parameter value
  return output;
 }

static double GetHessian_diff1(double x, void *p_void)
 {
  double        tmp, output, output_error;
  gsl_function  fn;
  fitComm      *p = (fitComm *)p_void;

  fn.function = &GetHessian_diff2;
  fn.params   = p_void;

  tmp = gsl_vector_get(p->paramValsHessian, p->diff1); // Replace old parameter with x
  gsl_vector_set(p->paramValsHessian, p->diff1, x);
  gsl_deriv_central(&fn, gsl_vector_get(p->paramValsHessian, p->diff2), p->diff2step, &output, &output_error); // Differentiate residual a second time
  gsl_vector_set(p->paramValsHessian, p->diff1, tmp); // Restore old parameter value
  return output;
 }

// Routine for working out the Hessian matrix
static gsl_matrix *GetHessian(fitComm *p)
 {
  gsl_matrix   *out;
  int           i,j;
  double        output, output_error;
  gsl_function  fn;

  // Allocate a vector for passing our position in free-parameter space to fitResidual()
  p->paramValsHessian = gsl_vector_alloc(p->NParams);
  for (i=0; i<p->NParams; i++) gsl_vector_set(p->paramValsHessian, i, gsl_vector_get(p->bestFitParamVals, i));
  p->paramVals = p->paramValsHessian;

  out = gsl_matrix_alloc(p->NParams, p->NParams); // Allocate memory for Hessian matrix
  if (out==NULL) return NULL;

  fn.function = &GetHessian_diff1;
  fn.params   = (void *)p;

  for (i=0; i<p->NParams; i++) for (j=0; j<p->NParams; j++) // Loop over elements of Hessian matrix
   {
    p->diff1 = i; p->diff1step = ((output = gsl_vector_get(p->paramVals,i)*1e-6) < 1e-100) ? 1e-6 : output;
    p->diff2 = j; p->diff2step = ((output = gsl_vector_get(p->paramVals,j)*1e-6) < 1e-100) ? 1e-6 : output;
    gsl_deriv_central(&fn, gsl_vector_get(p->paramVals, p->diff1), p->diff1step, &output, &output_error);
    gsl_matrix_set(out,i,j,-output); // Minus sign here since fitResidual returns the negative of log(P)
   }

  gsl_vector_free(p->paramValsHessian);
  p->paramValsHessian = NULL;
  return out;
 }

// Slave routines called by minimisers

static double ResidualMinimiserSlave(const gsl_vector *x, void *p_void)
 {
  fitComm *p = (fitComm *)p_void;
  p->paramVals = x;
  return fitResidual(p);
 }

static double FitsigmaData(const gsl_vector *x, void *p_void)
 {
  double term1, term2, term3;
  int    sgn;
  gsl_matrix *hessian;
  gsl_permutation *perm;

  fitComm *p = (fitComm *)p_void;
  p->sigmaData = gsl_vector_get(x,0);
  p->paramVals = p->bestFitParamVals;
  term1 = fitResidual(p); // Likelihood for the best-fit set of parameters, without the Gaussian normalisation factor
  term2 = -(p->NDataPoints * log(1.0 / sqrt(2*M_PI) / p->sigmaData)); // Gaussian normalisation factor

  // term3 is the Occam Factor, which equals the determinant of -H
  hessian = GetHessian(p);
  gsl_matrix_scale(hessian, -1.0); // Want the determinant of -H
  // Generate the LU decomposition of the Hessian matrix
  perm = gsl_permutation_alloc(p->NParams);
  gsl_linalg_LU_decomp(hessian,perm,&sgn); // Hessian matrix is overwritten here, but we don't need it again
  // Calculate the determinant of the Hessian matrix
  term3 = gsl_linalg_LU_lndet(hessian) * 0.5; // -log(1/sqrt(det))
  gsl_matrix_free(hessian);
  gsl_permutation_free(perm);

  return term1+term2+term3; // We return the negative of the log-likelihood, which GSL then minimises by varying the assume errorbar size
 }

// Top-level routine for managing the GSL minimiser

static int FitMinimiseIterate(fitComm *commlink, double(*slave)(const gsl_vector *, void *), unsigned char FittingsigmaData)
 {
  ppl_context                        *c = commlink->c;
  size_t                              iter = 0,iter2 = 0;
  int                                 i, status=0, NParams;
  double                              size=0,sizelast=0,sizelast2=0;
  const gsl_multimin_fminimizer_type *T = gsl_multimin_fminimizer_nmsimplex; // We don't use nmsimplex2 here because it was new in gsl 1.12
  gsl_multimin_fminimizer            *s;
  gsl_vector                         *x, *ss;
  gsl_multimin_function               fn;

  if (!FittingsigmaData) NParams = commlink->NParams;
  else                   NParams = 1;
  x  = gsl_vector_alloc( NParams );
  ss = gsl_vector_alloc( NParams );

  fn.n = NParams;
  fn.f = slave;
  fn.params = (void *)commlink;

  iter2=0;
  do
   {
    iter2++;
    sizelast2 = size;

    if (!FittingsigmaData)
     {
      if (c->set->term_current.ComplexNumbers == SW_ONOFF_OFF)for(i=0;i<commlink->NFitVars;i++) gsl_vector_set(x,  i,commlink->varObj[i]->real);
      else                                                    for(i=0;i<commlink->NFitVars;i++){gsl_vector_set(x,2*i,commlink->varObj[i]->real); gsl_vector_set(x,2*i+1,commlink->varObj[i]->imag); }
     } else {
      gsl_vector_set(x,0,commlink->sigmaData);
     }

    for (i=0; i<NParams; i++)
     {
      if (fabs(gsl_vector_get(x,i))>1e-6) gsl_vector_set(ss, i, 0.1 * gsl_vector_get(x,i));
      else                                gsl_vector_set(ss, i, 0.1                      ); // Avoid having a stepsize of zero
     }

    s = gsl_multimin_fminimizer_alloc (T, fn.n);
    gsl_multimin_fminimizer_set (s, &fn, x, ss);

    // If initial value we are giving the minimiser produces an algebraic error, it's not worth continuing
    (*slave)(x,(void *)commlink);
    if (commlink->errtext[0]!='\0') { gsl_vector_free(x); gsl_vector_free(ss); gsl_multimin_fminimizer_free(s); return 1; }

    iter                 = 0;
    commlink->goneNaN    = 0;
    do
     {
      iter++;
      for (i=0; i<2+NParams*2; i++) // When you're minimising over many parameters simultaneously sometimes nothing happens for a long time
       {
        status = gsl_multimin_fminimizer_iterate(s);
        if (status) break;
       }
      if (status) break;
      sizelast = size;
      size     = gsl_multimin_fminimizer_minimum(s);
     }
    while ((iter < 10) || ((size < sizelast) && (iter < 50))); // Iterate 10 times, and then see whether size carries on getting smaller

    gsl_multimin_fminimizer_free(s);
   }
  while ((iter2 < 3) || ((commlink->goneNaN==0) && (!status) && (size < sizelast2) && (iter2 < 20))); // Iterate 2 times, and then see whether size carries on getting smaller

  if (iter2>=20) status=1;

  if (status) { sprintf(commlink->errtext, "Failed to converge. GSL returned error: %s", gsl_strerror(status)); gsl_vector_free(x); gsl_vector_free(ss); return 1; }
  sizelast = (*slave)(x,(void *)commlink); // Calling minimiser slave now sets all fitting variables to desired values

  gsl_vector_free(x);
  gsl_vector_free(ss);
  return 0;
 }

// Main entry point for the implementation of the fit command
void ppl_directive_fit(ppl_context *c, parserLine *pl, parserOutput *in, int interactive, int iterDepth)
 {
  pplObj     *stk=in->stk;
  int         status=0, Nargs, Nvars, NExpect, Nusing;
  char       *fnName;
  long int    i, j, k, NDataPoints;
  int         contextLocalVec, contextDataTab, errCount=DATAFILE_NERRS;
  char       *fitVars[USING_ITEMS_MAX];
  pplObj      unit  [USING_ITEMS_MAX];
  int         minSet[USING_ITEMS_MAX], maxSet[USING_ITEMS_MAX];
  double      min   [USING_ITEMS_MAX], max   [USING_ITEMS_MAX];
  pplObj      firstVals[USING_ITEMS_MAX], *varObj[USING_ITEMS_MAX];
  dataTable  *data;
  dataBlock  *blk;
  parserLine *spool=NULL, **dataSpool = &spool;
  double     *localDataTable, val;
  gsl_vector *bestFitParamVals;
  gsl_matrix *hessian, *hessian_lu, *hessian_inv;
  int         sgn;
  double      stdDev[2*USING_ITEMS_MAX], tmp1, tmp2, tmp3;
  fitComm     dataComm; // Structure which we fill with variables we need to pass to the minimiser
  pplFunc    *funcDef;
  pplObj     *funcObj;
  gsl_permutation *perm;

  // Get list of fitting variables
  {
   int pos = PARSE_fit_fit_variables;
   Nvars=0;
   while (stk[pos].objType == PPLOBJ_NUM)
    {
     pos = (int)round(stk[pos].real);
     if (pos<=0) break;
     if (Nvars>=USING_ITEMS_MAX-4)
      {
       sprintf(c->errStat.errBuff,"Too many fitting variables; a maximum of %d may be specified.",USING_ITEMS_MAX-4);
       TBADD2(ERR_SYNTAX,in->stkCharPos[pos+PARSE_fit_fit_variable_fit_variables]);
       return;
      }
     fitVars[Nvars] = (char *)stk[pos+PARSE_fit_fit_variable_fit_variables].auxil;
     Nvars++;
    }
   if (Nvars<1)
    {
     sprintf(c->errStat.errBuff,"Too few fitting variables; a minimum of one must be specified.");
     TBADD2(ERR_SYNTAX,0);
     return;
    }
  }

  // Get pointers to fitting variables
  {
   int i;
   for (i=0; i<Nvars; i++)
    {
     pplObj dummyTemp;
     ppl_contextGetVarPointer(c, fitVars[i], &varObj[i], &dummyTemp);
     if (dummyTemp.objType==PPLOBJ_NUM) { ppl_unitsDimCpy(varObj[i],&dummyTemp); }
     else                               { pplObjNum(varObj[i],0,0,0); }
     dummyTemp.amMalloced=0;
     ppl_garbageObject(&dummyTemp);
    }
  }

  // Get name of function to fit
  fnName = (char *)stk[PARSE_fit_fit_function].auxil;
  ppl_contextVarLookup(c, fnName, &funcObj, 0);
  if (funcObj==NULL) { sprintf(c->errStat.errBuff,"No such function as '%s()'.",fnName); TBADD2(ERR_NAMESPACE, in->stkCharPos[PARSE_fit_fit_function]); return; }
  if (funcObj->objType!=PPLOBJ_FUNC) { sprintf(c->errStat.errBuff,"Object '%s' is not a function, but has type <%s>.", fnName, pplObjTypeNames[funcObj->objType]); TBADD2(ERR_TYPE, in->stkCharPos[PARSE_fit_fit_function]); return; }
  funcDef = (pplFunc *)funcObj->auxil;
  if (funcDef->functionType==PPL_FUNC_MAGIC) { sprintf(c->errStat.errBuff,"Function %s() needs wrapping in a user-defined function before it can be fit.", fnName); TBADD2(ERR_TYPE, in->stkCharPos[PARSE_fit_fit_function]); return; }
  Nargs = funcDef->minArgs;

  // Count length of using list
  {
   int       hadNonNullUsingItem = 0;
   int       pos = PARSE_fit_using_list;
   const int o   = PARSE_fit_using_item_using_list;

   Nusing=0;
   while (stk[pos].objType == PPLOBJ_NUM)
    {
     pos = (int)round(stk[pos].real);
     if (pos<=0) break;
     if (Nusing>=USING_ITEMS_MAX) { sprintf(c->errStat.errBuff, "Too many using items; maximum of %d are allowed.", USING_ITEMS_MAX); TBADD2(ERR_RANGE, in->stkCharPos[pos+o]); return; }
     Nusing++;
    }
   if ((!hadNonNullUsingItem) && (Nusing==1)) Nusing=0; // If we've only had one using item, and it was blank, this is a parser abberation
  }

  // Work out how many columns of data we're going to read
  if   (Nusing != Nargs+2) NExpect = Nargs+1;
  else                     NExpect = Nargs+2; // Only expect weights to go with fitting data if using list has exactly the right length

  // Read ranges
  {
   int i,pos=PARSE_fit_0range_list,nr=0;
   const int o1 = PARSE_fit_min_0range_list;
   const int o2 = PARSE_fit_max_0range_list;
   for (i=0; i<USING_ITEMS_MAX; i++) minSet[i]=0;
   for (i=0; i<USING_ITEMS_MAX; i++) maxSet[i]=0;
   while (stk[pos].objType == PPLOBJ_NUM)
    {
     pos = (int)round(stk[pos].real);
     if (pos<=0) break;
     if (nr>=USING_ITEMS_MAX)
      {
       sprintf(c->errStat.errBuff,"Too many ranges specified; a maximum of %d are allowed.", USING_ITEMS_MAX);
       TBADD2(ERR_SYNTAX,0);
       return;
      }
     if ((stk[pos+o1].objType==PPLOBJ_NUM)||(stk[pos+o1].objType==PPLOBJ_DATE)||(stk[pos+o1].objType==PPLOBJ_BOOL))
      {
       unit[nr]=stk[pos+o1];
       min[nr]=unit[nr].real;
       minSet[nr]=1;
      }
     if ((stk[pos+o2].objType==PPLOBJ_NUM)||(stk[pos+o2].objType==PPLOBJ_DATE)||(stk[pos+o2].objType==PPLOBJ_BOOL))
      {
       if ((minSet[nr])&&(unit[nr].objType!=stk[pos+o2].objType))
        {
         sprintf(c->errStat.errBuff,"Minimum and maximum limits specified for variable %d have conflicting types of <%s> and <%s>.", nr+1, pplObjTypeNames[unit[nr].objType], pplObjTypeNames[stk[pos+o2].objType]);
         TBADD2(ERR_TYPE,in->stkCharPos[pos+PARSE_fit_min_0range_list]);
         return;
        }
       if ((minSet[nr])&&(!ppl_unitsDimEqual(&unit[nr],&stk[pos+o2])))
        {
         sprintf(c->errStat.errBuff,"Minimum and maximum limits specified for variable %d have conflicting physical units of <%s> and <%s>.", nr+1, ppl_printUnit(c,&unit[nr],NULL,NULL,0,0,0), ppl_printUnit(c,&stk[pos+o2],NULL,NULL,1,0,0));
         TBADD2(ERR_UNIT,in->stkCharPos[pos+PARSE_fit_min_0range_list]);
         return;
        }
       unit[nr]=stk[pos+o2];
       max[nr]=unit[nr].real;
       maxSet[nr]=1;
      }
     nr++;
    }
  }

  // Allocate a new memory context for the data file we're about to read
  contextLocalVec= ppl_memAlloc_DescendIntoNewContext();
  contextDataTab = ppl_memAlloc_DescendIntoNewContext();

  // Read data from file
  ppldata_fromCmd(c, &data, pl, in, 0, NULL, dataSpool, PARSE_TABLE_fit_, 0, NExpect, 0, min, minSet, max, maxSet, unit, 0, &status, c->errStat.errBuff, &errCount, iterDepth);

  // Exit on error
  if ((status)||(data==NULL))
   {
    TBADD2(ERR_GENERAL,0);
    ppl_memAlloc_AscendOutOfContext(contextLocalVec);
    return;
   }

  // Check that the firstEntries above have the same units as any supplied ranges
  for (j=0; j<NExpect; j++)
   if (minSet[j] || maxSet[j])
    {
     if (!ppl_unitsDimEqual(&unit[j],data->firstEntries+j)) { sprintf(c->errStat.errBuff, "The minimum and maximum limits specified in range %ld in the fit command have conflicting physical dimensions with the data returned from the data file. The limits have units of <%s>, whilst the data have units of <%s>.", j+1, ppl_printUnit(c,unit+j,NULL,NULL,0,1,0), ppl_printUnit(c,data->firstEntries+j,NULL,NULL,1,1,0)); TBADD2(ERR_NUMERIC,0); return; }
    }

  // Work out how many data points we have within the specified ranges
  NDataPoints = 0;
  blk = data->first;
  while (blk != NULL)
   {
    int j;
    for (j=0; j<blk->blockPosition; j++)
     {
      int inRange=1, k;
      for (k=0; k<NExpect; k++)
       {
        val = blk->data_real[k + NExpect*j];
        if ( (minSet[k]&&(val<min[k])) || (maxSet[k]&&(val>max[k])) ) { inRange=0; break; } // Check that value is within range
       }
      if (inRange) NDataPoints++;
     }
    blk=blk->next;
   }

  // Copy data into a new table and apply the specified ranges to it
  localDataTable = (double *)ppl_memAlloc_incontext(NDataPoints * NExpect * sizeof(double), contextLocalVec);
  if (localDataTable==NULL) { sprintf(c->errStat.errBuff, "Out of memory."); TBADD2(ERR_MEMORY,0); return; }
  i=0;
  blk = data->first;
  while (blk != NULL)
   {
    int j;
    for (j=0; j<blk->blockPosition; j++)
     {
      int inRange=1, k;
      for (k=0; k<NExpect; k++)
       {
        val = blk->data_real[k + NExpect*j];
        if ( (minSet[k]&&(val<min[k])) || (maxSet[k]&&(val>max[k])) ) { inRange=0; break; } // Check that value is within range
        localDataTable[i * NExpect + k] = val;
       }
      if (inRange) i++;
     }
    blk=blk->next;
   }

  // Make a copy of the physical units of all of the columns of data
  for (j=0; j<NExpect; j++) firstVals[j] = data->firstEntries[j];

  // Free original data table which is no longer needed
  ppl_memAlloc_AscendOutOfContext(contextDataTab);
  data = NULL;

  // Populate dataComm
  dataComm.c            = c;
  dataComm.pl           = pl;
  dataComm.functionName = fnName;
  dataComm.NFitVars     = Nvars;
  dataComm.NArgs        = Nargs;
  dataComm.NExpect      = NExpect;
  dataComm.NParams      = dataComm.NFitVars * ((c->set->term_current.ComplexNumbers == SW_ONOFF_OFF) ? 1 : 2);
  dataComm.NDataPoints  = NDataPoints;
  dataComm.varObj       = varObj;
  dataComm.paramVals    = NULL;
  dataComm.bestFitParamVals = NULL;
  dataComm.firstVals    = firstVals;
  dataComm.dataTable    = localDataTable;
  dataComm.flagYErrorBars = (NExpect == Nargs+2);
  dataComm.sigmaData    = 1.0;
  dataComm.scratchPad   = (char *)ppl_memAlloc_incontext(LSTR_LENGTH, contextLocalVec);
  dataComm.errtext      = (char *)ppl_memAlloc_incontext(LSTR_LENGTH, contextLocalVec); // functionName was already set above
  if ((dataComm.scratchPad==NULL)||(dataComm.errtext==NULL)) { sprintf(c->errStat.errBuff, "Out of memory."); TBADD2(ERR_MEMORY,0); return; }
  dataComm.errtext[0]   = '\0';

  // Set up a minimiser
  status = FitMinimiseIterate(&dataComm, &ResidualMinimiserSlave, 0);
  if (status) { sprintf(c->errStat.errBuff, "%s", dataComm.errtext); TBADD2(ERR_NUMERIC,0); return; }

  // Display the results of the minimiser
  ppl_report(&c->errcontext,"\n# Best fit parameters were:\n# -------------------------\n");
  for (j=0; j<dataComm.NFitVars; j++)
   {
    if (c->set->term_current.NumDisplay != SW_DISPLAY_L) sprintf(c->errcontext.tempErrStr,  "%s = %s", fitVars[j], ppl_unitsNumericDisplay(c,varObj[j],0,0,0)  );
    else                                                 sprintf(c->errcontext.tempErrStr, "$%s = %s", fitVars[j], ppl_unitsNumericDisplay(c,varObj[j],0,0,0)+1);
    ppl_report(&c->errcontext,NULL);
   }

  // If 'withouterrors' is specified, stop now
  if (stk[PARSE_fit_withouterrors].objType==PPLOBJ_STR)
   {
    ppl_memAlloc_AscendOutOfContext(contextLocalVec);
    return;
   }

  // Store best-fit position
  bestFitParamVals = gsl_vector_alloc(dataComm.NParams);
  for (i=0; i<dataComm.NFitVars; i++)
   if (c->set->term_current.ComplexNumbers == SW_ONOFF_OFF)   gsl_vector_set(bestFitParamVals,   i  , varObj[i]->real);
   else                                                     { gsl_vector_set(bestFitParamVals, 2*i  , varObj[i]->real);
                                                              gsl_vector_set(bestFitParamVals, 2*i+1, varObj[i]->imag);
                                                            }
  dataComm.bestFitParamVals = bestFitParamVals;

  // Estimate the size of the errorbars on the supplied data if no errorbars were supplied (this doesn't affect best fit position, but does affect error estimates)
  if (!dataComm.flagYErrorBars)
   {
    sprintf(c->errcontext.tempErrStr, "\n# Estimating the size of the error bars on supplied data.\n# This may take a while.\n# The fit command can be made to run very substantially faster if the 'withouterrors' option is set.");
    ppl_report(&c->errcontext,NULL);
    status = FitMinimiseIterate(&dataComm, &FitsigmaData, 1);
    if (status) { sprintf(c->errStat.errBuff, "%s", dataComm.errtext); TBADD2(ERR_NUMERIC,0); gsl_vector_free(bestFitParamVals); return; }
    firstVals[Nargs].real = dataComm.sigmaData;
    firstVals[Nargs].imag = 0.0;
    firstVals[Nargs].flagComplex = 0;
    sprintf(c->errcontext.tempErrStr, "\n# Estimate of error bars on supplied data, based on their fit to model function = %s", ppl_unitsNumericDisplay(c,firstVals+Nargs,0,0,0)); ppl_report(&c->errcontext,NULL);
   }

  // Calculate and print the Hessian matrix
  hessian = GetHessian(&dataComm);
  matrixPrint(c, hessian, dataComm.NParams, dataComm.scratchPad);
  sprintf(c->errcontext.tempErrStr, "\n# Hessian matrix of log-probability distribution:\n# -----------------------------------------------\n\nhessian = %s", dataComm.scratchPad);
  ppl_report(&c->errcontext,NULL);

  // Set variables in user's variable space to best-fit values
  for (i=0; i<dataComm.NFitVars; i++)
   if (c->set->term_current.ComplexNumbers == SW_ONOFF_OFF)   varObj[i]->real = gsl_vector_get(dataComm.bestFitParamVals,   i  );
   else                                                     { varObj[i]->real = gsl_vector_get(dataComm.bestFitParamVals, 2*i  );
                                                              varObj[i]->imag = gsl_vector_get(dataComm.bestFitParamVals, 2*i+1);
                                                            }

  // Calculate and print the covariance matrix
  hessian_lu  = gsl_matrix_alloc(dataComm.NParams, dataComm.NParams);
  hessian_inv = gsl_matrix_alloc(dataComm.NParams, dataComm.NParams);
  gsl_matrix_memcpy(hessian_lu, hessian);
  gsl_matrix_scale(hessian_lu, -1.0); // Want the inverse of -H
  perm = gsl_permutation_alloc(dataComm.NParams);
  gsl_linalg_LU_decomp(hessian_lu,perm,&sgn);
  gsl_linalg_LU_invert(hessian_lu,perm,hessian_inv);
  matrixPrint(c, hessian_inv, dataComm.NParams, dataComm.scratchPad);
  sprintf(c->errcontext.tempErrStr, "\n# Covariance matrix of probability distribution:\n# ----------------------------------------------\n\ncovariance = %s", dataComm.scratchPad);
  ppl_report(&c->errcontext,NULL);

  // Calculate the standard deviation of each parameter
  for (i=0; i<dataComm.NParams; i++)
   {
    if ((gsl_matrix_get(hessian_inv, i, i) <= 0.0) || (!gsl_finite(gsl_matrix_get(hessian_inv, i, i))))
     {
      sprintf(c->errcontext.tempErrStr, "One of the calculated variances for the fitted parameters is negative. This strongly suggests that the fitting process has failed.");
      ppl_warning(&c->errcontext, ERR_NUMERIC, NULL);
      stdDev[i] = 1e-100;
     } else {
      stdDev[i] = sqrt( gsl_matrix_get(hessian_inv, i, i) );
     }
   }

  // Calculate the correlation matrix
  for (i=0; i<dataComm.NParams; i++) for (j=0; j<dataComm.NParams; j++)
   gsl_matrix_set(hessian_inv, i, j, gsl_matrix_get(hessian_inv, i, j) / stdDev[i] / stdDev[j]);
  matrixPrint(c, hessian_inv, dataComm.NParams, dataComm.scratchPad);
  sprintf(c->errcontext.tempErrStr, "\n# Correlation matrix of probability distribution:\n# ----------------------------------------------\n\ncorrelation = %s", dataComm.scratchPad);
  ppl_report(&c->errcontext,NULL);

  // Print a list of standard deviations
  ppl_report(&c->errcontext,"\n# Uncertainties in best-fit parameters are:\n# -----------------------------------------\n");
  for (i=0; i<dataComm.NFitVars; i++)
   {
    pplObj dummyTemp = *(varObj[i]);
    if (c->set->term_current.ComplexNumbers == SW_ONOFF_OFF)
     {
      dummyTemp.real = stdDev[i] ; dummyTemp.imag = 0.0; dummyTemp.flagComplex = 0; // Apply appropriate unit to standard deviation, which is currently just a double
      if (c->set->term_current.NumDisplay != SW_DISPLAY_L)
       {
        sprintf(dataComm.scratchPad, "sigma_%s", fitVars[i]);
        sprintf(c->errcontext.tempErrStr, "%22s = %s", dataComm.scratchPad, ppl_unitsNumericDisplay(c,&dummyTemp,0,0,0));
       } else {
        sprintf(dataComm.scratchPad, "$\\sigma_\\textrm{");
        j = strlen(dataComm.scratchPad);
        for (k=0; fitVars[i][k]!='\0'; k++) { if (fitVars[i][k]=='_') dataComm.scratchPad[j++]='\\'; dataComm.scratchPad[j++]=fitVars[i][k]; }
        dataComm.scratchPad[j++]='\0';
        sprintf(c->errcontext.tempErrStr, "%33s} = %s", dataComm.scratchPad, ppl_unitsNumericDisplay(c,&dummyTemp,0,0,0)+1);
       }
      ppl_report(&c->errcontext,NULL);
     }
    else
     {
      if (c->set->term_current.NumDisplay != SW_DISPLAY_L)
       {
        dummyTemp.real = stdDev[2*i  ] ; dummyTemp.imag = 0.0; dummyTemp.flagComplex = 0;
        sprintf(dataComm.scratchPad, "sigma_%s_real", fitVars[i]);
        sprintf(c->errcontext.tempErrStr, "%27s = %s", dataComm.scratchPad, ppl_unitsNumericDisplay(c,&dummyTemp,0,0,0));
        ppl_report(&c->errcontext,NULL);
        dummyTemp.real = stdDev[2*i+1] ; dummyTemp.imag = 0.0; dummyTemp.flagComplex = 0;
        sprintf(dataComm.scratchPad, "sigma_%s_imag", fitVars[i]);
        sprintf(c->errcontext.tempErrStr, "%27s = %s", dataComm.scratchPad, ppl_unitsNumericDisplay(c,&dummyTemp,0,0,0));
        ppl_report(&c->errcontext,NULL);
       }
      else
       {
        dummyTemp.real = stdDev[2*i  ] ; dummyTemp.imag = 0.0; dummyTemp.flagComplex = 0;
        sprintf(dataComm.scratchPad, "$\\sigma_\\textrm{");
        j = strlen(dataComm.scratchPad);
        for (k=0; fitVars[i][k]!='\0'; k++) { if (fitVars[i][k]=='_') dataComm.scratchPad[j++]='\\'; dataComm.scratchPad[j++]=fitVars[i][k]; }
        dataComm.scratchPad[j++]='\0';
        sprintf(c->errcontext.tempErrStr, "%38s,real} = %s", dataComm.scratchPad, ppl_unitsNumericDisplay(c,&dummyTemp,0,0,0)+1);
        ppl_report(&c->errcontext,NULL);
        dummyTemp.real = stdDev[2*i+1] ; dummyTemp.imag = 0.0; dummyTemp.flagComplex = 0;
        sprintf(c->errcontext.tempErrStr, "%38s,imag} = %s", dataComm.scratchPad, ppl_unitsNumericDisplay(c,&dummyTemp,0,0,0)+1);
        ppl_report(&c->errcontext,NULL);
       }
     }
   }

  // Print summary information
  ppl_report(&c->errcontext,"\n# Summary:\n# --------\n");
  for (i=0; i<dataComm.NFitVars; i++)
   {
    pplObj dummyTemp;
    char *cptr = ppl_printUnit(c, varObj[i], &tmp1, &tmp2, 0, 1, 0); // Work out what unit the best-fit value is best displayed in
    if      (fabs(varObj[i]->real)>1e-200) tmp3 = tmp1 / varObj[i]->real; // Set tmp3 to be the multiplicative size of this unit relative to its SI counterpart
    else if (fabs(varObj[i]->imag)>1e-200) tmp3 = tmp2 / varObj[i]->imag; // Can't do this if magnitude of best-fit value is zero, though...
    else
     {
      dummyTemp = *(varObj[i]);
      dummyTemp.real = 1.0; dummyTemp.imag = 0.0; dummyTemp.flagComplex = 0; // If best-fit value is zero, use the unit we would use for unity instead.
      cptr = ppl_printUnit(c, &dummyTemp, &tmp1, &tmp2, 0, 1, 0);
      tmp3 = tmp1;
     }
    pplObjNum(&dummyTemp,0,tmp1,tmp2); dummyTemp.refCount=1;
    if (c->set->term_current.ComplexNumbers == SW_ONOFF_OFF)
     {
      if (c->set->term_current.NumDisplay != SW_DISPLAY_L)
       sprintf(c->errcontext.tempErrStr, "%16s = (%s +/- %s) %s", fitVars[i], ppl_unitsNumericDisplay(c,&dummyTemp,1,0,0), ppl_numericDisplay(stdDev[i]*tmp3,c->numdispBuff[0],c->set->term_current.SignificantFigures,0), cptr);
      else
       {
        dataComm.scratchPad[0]='$';
        for (j=1,k=0; fitVars[i][k]!='\0'; k++) { if (fitVars[i][k]=='_') dataComm.scratchPad[j++]='\\'; dataComm.scratchPad[j++]=fitVars[i][k]; }
        dataComm.scratchPad[j++]='\0';
        sprintf(c->errcontext.tempErrStr, "%17s = (%s", dataComm.scratchPad, ppl_unitsNumericDisplay(c,&dummyTemp,1,0,0)+1);
        j=strlen(c->errcontext.tempErrStr)-1; // Remove final $
        sprintf(c->errcontext.tempErrStr+j, "\\pm%s)%s$", ppl_numericDisplay(stdDev[i]*tmp3,c->numdispBuff[0],c->set->term_current.SignificantFigures,0), cptr);
       }
     }
    else
     {
      if      (c->set->term_current.NumDisplay == SW_DISPLAY_T)
       sprintf(c->errcontext.tempErrStr, "%16s = (%s +/- %s +/- %s*sqrt(-1))%s", fitVars[i], ppl_unitsNumericDisplay(c,&dummyTemp,1,0,0), ppl_numericDisplay(stdDev[2*i]*tmp3,c->numdispBuff[0],c->set->term_current.SignificantFigures,0), ppl_numericDisplay(stdDev[2*i+1]*tmp3,c->numdispBuff[2],c->set->term_current.SignificantFigures,0), cptr);
      else if (c->set->term_current.NumDisplay == SW_DISPLAY_L)
       {
        dataComm.scratchPad[0]='$';
        for (j=1,k=0; fitVars[i][k]!='\0'; k++) { if (fitVars[i][k]=='_') dataComm.scratchPad[j++]='\\'; dataComm.scratchPad[j++]=fitVars[i][k]; }
        dataComm.scratchPad[j++]='\0';
        sprintf(c->errcontext.tempErrStr, "%17s = (%s", dataComm.scratchPad, ppl_unitsNumericDisplay(c,&dummyTemp,1,0,0)+1);
        j=strlen(c->errcontext.tempErrStr)-1; // Remove final $
        sprintf(c->errcontext.tempErrStr+j, "\\pm %s\\pm %si)%s$", ppl_numericDisplay(stdDev[2*i]*tmp3,c->numdispBuff[0],c->set->term_current.SignificantFigures,0), ppl_numericDisplay(stdDev[2*i+1]*tmp3,c->numdispBuff[2],c->set->term_current.SignificantFigures,0), cptr);
       }
      else
       sprintf(c->errcontext.tempErrStr, "%16s = (%s +/- %s +/- %si) %s", fitVars[i], ppl_unitsNumericDisplay(c,&dummyTemp,1,0,0), ppl_numericDisplay(stdDev[2*i]*tmp3,c->numdispBuff[0],c->set->term_current.SignificantFigures,0), ppl_numericDisplay(stdDev[2*i+1]*tmp3,c->numdispBuff[2],c->set->term_current.SignificantFigures,0), cptr);
     }
    ppl_report(&c->errcontext,NULL);
   }

  // We're finished... can now free dataTable
  gsl_vector_free(bestFitParamVals);
  gsl_matrix_free(hessian_inv);
  gsl_matrix_free(hessian_lu);
  gsl_matrix_free(hessian);
  gsl_permutation_free(perm);
  ppl_memAlloc_AscendOutOfContext(contextLocalVec);
  return;
 }

