// fft.c
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

#define _FFT_C 1

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

#include <gsl/gsl_math.h>

#ifdef HAVE_FFTW3
#include <fftw3.h>
#else
#include <fftw.h>
#endif

#include "commands/fft.h"
#include "coreUtils/memAlloc.h"
#include "expressions/expCompile_fns.h"
#include "expressions/expEval.h"
#include "expressions/traceback_fns.h"
#include "parser/cmdList.h"
#include "parser/parser.h"
#include "settings/settings.h"
#include "settings/settingTypes.h"
#include "stringTools/asciidouble.h"
#include "userspace/context.h"
#include "userspace/contextVarDef.h"
#include "userspace/garbageCollector.h"
#include "userspace/pplObj_fns.h"
#include "userspace/pplObjFunc.h"
#include "userspace/unitsArithmetic.h"
#include "userspace/unitsDisp.h"

#include "datafile.h"

#define STACK_POP \
   { \
    c->stackPtr--; \
    ppl_garbageObject(&c->stack[c->stackPtr]); \
    if (c->stack[c->stackPtr].refCount != 0) { sprintf(c->errcontext.tempErrStr,"Stack forward reference detected."); TBADD2(ERR_INTERNAL,0); return; } \
   }

#define STACK_CLEAN    while (c->stackPtr>stkLevelOld) { STACK_POP; }

#define TBADD2(et,pos) ppl_tbAdd(c,pl->srcLineN,pl->srcId,pl->srcFname,0,et,pos,pl->linetxt,"")

#define COUNTEDERR1 if (errCount >0) { errCount--;
#define COUNTEDERR2 if (errCount==0) { sprintf(c->errcontext.tempErrStr, "%s: Too many errors: no more errors will be shown.",filenameOut); ppl_warning(&c->errcontext,ERR_STACKED,NULL); } }

// Window functions for FFTs
static void fftwindow_rectangular (pplObj *x, int Ndim, int *Npos, int *Nstep) { }
static void fftwindow_hamming     (pplObj *x, int Ndim, int *Npos, int *Nstep) { double y=1.0; int i; for (i=0; i<Ndim; i++) { y*=0.54-0.46*cos(2*M_PI*((double)Npos[i])/((double)(Nstep[i]-1))); } x->real*=y; x->imag*=y; if (x->imag==0.0) { x->imag=0.0; x->flagComplex=0; } }
static void fftwindow_hann        (pplObj *x, int Ndim, int *Npos, int *Nstep) { double y=1.0; int i; for (i=0; i<Ndim; i++) { y*=0.5*(1.0-cos(2*M_PI*((double)Npos[i])/((double)(Nstep[i]-1)))); } x->real*=y; x->imag*=y; if (x->imag==0.0) { x->imag=0.0; x->flagComplex=0; } }
static void fftwindow_cosine      (pplObj *x, int Ndim, int *Npos, int *Nstep) { double y=1.0; int i; for (i=0; i<Ndim; i++) { y*=sin(M_PI*((double)Npos[i])/((double)(Nstep[i]-1))); } x->real*=y; x->imag*=y; if (x->imag==0.0) { x->imag=0.0; x->flagComplex=0; } }
static void fftwindow_lanczos     (pplObj *x, int Ndim, int *Npos, int *Nstep) { double y=1.0,z; int i; for (i=0; i<Ndim; i++) { z=2*M_PI*((double)Npos[i])/((double)(Nstep[i]-1))-M_PI; y*=sin(z)/z; } x->real*=y; x->imag*=y; if (x->imag==0.0) { x->imag=0.0; x->flagComplex=0; } }
static void fftwindow_bartlett    (pplObj *x, int Ndim, int *Npos, int *Nstep) { double y=1.0; int i; for (i=0; i<Ndim; i++) { y*=(2.0/(Nstep[i]-1))*((Nstep[i]-1)/2.0-fabs(Npos[i]-(Nstep[i]-1)/2.0)); } x->real*=y; x->imag*=y; if (x->imag==0.0) { x->imag=0.0; x->flagComplex=0; } }
static void fftwindow_triangular  (pplObj *x, int Ndim, int *Npos, int *Nstep) { double y=1.0; int i; for (i=0; i<Ndim; i++) { y*=(2.0/(Nstep[i]  ))*((Nstep[i]  )/2.0-fabs(Npos[i]-(Nstep[i]-1)/2.0)); } x->real*=y; x->imag*=y; if (x->imag==0.0) { x->imag=0.0; x->flagComplex=0; } }
static void fftwindow_gauss       (pplObj *x, int Ndim, int *Npos, int *Nstep) { double y=1.0; const double sigma=0.5; int i; for (i=0; i<Ndim; i++) { y*=exp(-0.5*pow((Npos[i]-((Nstep[i]-1)/2.0))/(sigma*(Nstep[i]-1)/2.0),2.0)); } x->real*=y; x->imag*=y; if (x->imag==0.0) { x->imag=0.0; x->flagComplex=0; } }
static void fftwindow_bartletthann(pplObj *x, int Ndim, int *Npos, int *Nstep) { double y=1.0; int i; for (i=0; i<Ndim; i++) { y*=0.62 - 0.48*(((double)Npos[i])/((double)(Nstep[i]-1))-0.5) - 0.38*cos(2*M_PI*((double)Npos[i])/((double)(Nstep[i]-1))); } x->real*=y; x->imag*=y; if (x->imag==0.0) { x->imag=0.0; x->flagComplex=0; } }
static void fftwindow_blackman    (pplObj *x, int Ndim, int *Npos, int *Nstep) { double y=1.0; const double alpha=0.16; int i; for (i=0; i<Ndim; i++) { y*=(1.0-alpha)/2.0 - 0.5*cos(2*M_PI*((double)Npos[i])/((double)(Nstep[i]-1))) + alpha/2.0*cos(4*M_PI*((double)Npos[i])/((double)(Nstep[i]-1))); } x->real*=y; x->imag*=y; if (x->imag==0.0) { x->imag=0.0; x->flagComplex=0; } }

// Main entry point for the FFT command
void ppl_directive_fft(ppl_context *c, parserLine *pl, parserOutput *in, int interactive, int iterDepth)
 {
  pplObj        *stk = in->stk;
  dataTable     *data;
  dataBlock     *blk;
  long int       i=0, k;
  int            contextLocalVec, contextDataTab, errCount=DATAFILE_NERRS;
  int            Ndims=0, inverse, Nsamples, Nsteps[USING_ITEMS_MAX];
  pplFunc       *funcPtr;
  pplObj         firstEntry;
  char           filenameOut[FNAME_LENGTH]="";
  pplObj         unit  [USING_ITEMS_MAX];
  int            minSet[USING_ITEMS_MAX], maxSet[USING_ITEMS_MAX];
  double         min   [USING_ITEMS_MAX], max   [USING_ITEMS_MAX], step[USING_ITEMS_MAX];
  fftw_complex  *datagrid;
  FFTDescriptor *output;
  void (*WindowType)(pplObj *,int,int *,int *);

  #ifdef HAVE_FFTW3
  fftw_plan     fftwplan; // FFTW 3.x
  #else
  fftwnd_plan   fftwplan; // FFTW 2.x
  #endif

  // Check whether we are doing a forward or a backward FFT
  if (strcmp((char *)stk[PARSE_ifft_directive].auxil,"ifft")==0) inverse=1;
  else                                                           inverse=0;

  // Read ranges
  {
   int i,pos=PARSE_ifft_range_list,nr=0;
   const int o1 = PARSE_ifft_min_range_list;
   const int o2 = PARSE_ifft_max_range_list;
   const int o3 = PARSE_ifft_step_range_list;
   for (i=0; i<USING_ITEMS_MAX; i++) minSet[i]=0;
   for (i=0; i<USING_ITEMS_MAX; i++) maxSet[i]=0;
   while (stk[pos].objType == PPLOBJ_NUM)
    {
     pos = (int)round(stk[pos].real);
     if (pos<=0) break;
     if (nr>=USING_ITEMS_MAX)
      {
       sprintf(c->errStat.errBuff,"Too many ranges specified; a maximum of %d are allowed.",USING_ITEMS_MAX);
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
         sprintf(c->errStat.errBuff,"Minimum and maximum limits specified for variable %d have conflicting types of <%s> and <%s>.", i+1, pplObjTypeNames[unit[nr].objType], pplObjTypeNames[stk[pos+o2].objType]);
         TBADD2(ERR_TYPE,in->stkCharPos[pos+PARSE_ifft_min_range_list]);
         return;
        }
       if ((minSet[nr])&&(!ppl_unitsDimEqual(&unit[nr],&stk[pos+o2])))
        {
         sprintf(c->errStat.errBuff,"Minimum and maximum limits specified for variable %d have conflicting physical units of <%s> and <%s>.", i+1, ppl_printUnit(c,&unit[nr],NULL,NULL,0,0,0), ppl_printUnit(c,&stk[pos+o2],NULL,NULL,1,0,0));
         TBADD2(ERR_UNIT,in->stkCharPos[pos+PARSE_ifft_min_range_list]);
         return;
        }
       unit[nr]=stk[pos+o2];
       max[nr]=unit[nr].real;
       maxSet[nr]=1;
      }
     if ((stk[pos+o3].objType==PPLOBJ_NUM)||(stk[pos+o3].objType==PPLOBJ_DATE)||(stk[pos+o3].objType==PPLOBJ_BOOL))
      {
       int tempDbl;
       if (unit[nr].objType!=stk[pos+o3].objType)
        {
         sprintf(c->errStat.errBuff,"Minimum and maximum limits specified for variable %d have conflicting types of <%s> and <%s>.", i+1, pplObjTypeNames[unit[nr].objType], pplObjTypeNames[stk[pos+o3].objType]);
         TBADD2(ERR_TYPE,in->stkCharPos[pos+PARSE_ifft_step_range_list]);
         return;
        }
       if (!ppl_unitsDimEqual(&unit[nr],&stk[pos+o3]))
        {
         sprintf(c->errStat.errBuff,"Minimum and maximum limits specified for variable %d have conflicting physical units of <%s> and <%s>.", i+1, ppl_printUnit(c,&unit[nr],NULL,NULL,0,0,0), ppl_printUnit(c,&stk[pos+o3],NULL,NULL,1,0,0));
         TBADD2(ERR_UNIT,in->stkCharPos[pos+PARSE_ifft_step_range_list]);
         return;
        }
       step[nr]= stk[pos+o3].real;
       tempDbl = 1.0 + floor((max[nr]-min[nr]) / step[nr] + 0.5); // Add one because this is a fencepost problem
       if (tempDbl<2.0) { sprintf(c->errStat.errBuff, "The number of samples produced by the range and step size specified for dimension %d to the fft command is fewer than two; a single data sample cannot be FFTed.", nr+1); TBADD2(ERR_RANGE,in->stkCharPos[pos+PARSE_ifft_step_range_list]); return; }
       if ((!gsl_finite(tempDbl)||(tempDbl>1e8))) { sprintf(c->errStat.errBuff, "The number of samples produced by the range and step size specified for dimension %d to the fft command is in excess of 1e8; PyXPlot is not the right tool to do this FFT in.", nr+1); TBADD2(ERR_RANGE,in->stkCharPos[pos+PARSE_ifft_step_range_list]); return; }
       Nsteps[nr] = (int)tempDbl;
      }
     else
      {
       Nsteps[nr] = 100; // shouldn't be here
       step  [nr] = floor((max[nr]-min[nr]) / (Nsteps[nr]-1));
      }
     nr++;
    }
   Ndims=nr;
  }

  // Work out total size of FFT data grid
  {
  double tempDbl = 1.0;
  for (i=0; i<Ndims; i++) tempDbl *= Nsteps[i];
  if (tempDbl > 1e8) { sprintf(c->errStat.errBuff, "The total number of samples in the requested %d-dimensional FFT is in excess of 1e8; PyXPlot is not the right tool to do this FFT in.", Ndims); TBADD2(ERR_NUMERIC,0); return; }
  Nsamples = (int)tempDbl;
  }

  // Allocate a new memory context for the data file we're about to read
  contextLocalVec= ppl_memAlloc_DescendIntoNewContext();
  contextDataTab = ppl_memAlloc_DescendIntoNewContext();

  // Work out what type of window we're going to use
  {
  int   gotWindow = (stk[PARSE_ifft_window].objType==PPLOBJ_STR);
  char *cptr      = (char *)stk[PARSE_ifft_window].auxil;
  WindowType = fftwindow_rectangular;
  if (gotWindow)
   {
    if      (strcmp(cptr,"hamming"     )!=0) WindowType = fftwindow_hamming;
    else if (strcmp(cptr,"hann"        )!=0) WindowType = fftwindow_hann;
    else if (strcmp(cptr,"cosine"      )!=0) WindowType = fftwindow_cosine;
    else if (strcmp(cptr,"lanczos"     )!=0) WindowType = fftwindow_lanczos;
    else if (strcmp(cptr,"bartlett"    )!=0) WindowType = fftwindow_bartlett;
    else if (strcmp(cptr,"triangular"  )!=0) WindowType = fftwindow_triangular;
    else if (strcmp(cptr,"gauss"       )!=0) WindowType = fftwindow_gauss;
    else if (strcmp(cptr,"bartletthann")!=0) WindowType = fftwindow_bartletthann;
    else if (strcmp(cptr,"blackman"    )!=0) WindowType = fftwindow_blackman;
   }
  }

  // Read input data
  if (stk[PARSE_ifft_filename].objType==PPLOBJ_STR)
   {
    const int NcolRequired=Ndims+2;
    int status=0, j;
    ppldata_fromCmd(c, &data, pl, in, 0, filenameOut, PARSE_TABLE_ifft_, 0, NcolRequired, 0, min, minSet, max, maxSet, unit, 0, &status, c->errcontext.tempErrStr, &errCount, iterDepth);

    // Exit on error
    if ((status)||(data==NULL))
     {
      TBADD2(ERR_GENERAL,0);
      ppl_memAlloc_AscendOutOfContext(contextLocalVec);
      return;
     }

    // Check that the firstEntries above have the same units as any supplied ranges
    for (j=0; j<Ndims; j++)
     if (minSet[j] || maxSet[j])
      {
       if (!ppl_unitsDimEqual(&unit[j],data->firstEntries+j)) { sprintf(c->errStat.errBuff, "The minimum and maximum limits specified in range %d in the fit command have conflicting physical dimensions with the data returned from the data file. The limits have units of <%s>, whilst the data have units of <%s>.", j+1, ppl_printUnit(c,unit+j,NULL,NULL,0,1,0), ppl_printUnit(c,data->firstEntries+j,NULL,NULL,1,1,0)); TBADD2(ERR_NUMERIC,0); return; }
      }

    // Read unit of f(x) in final two columns of data table (real and imaginary components of f(x)
    firstEntry = data->firstEntries[Ndims];
    firstEntry.real=1.0; firstEntry.imag=0.0; firstEntry.flagComplex=0;
    if (!ppl_unitsDimEqual(&data->firstEntries[Ndims], &data->firstEntries[Ndims+1])) { sprintf(c->errStat.errBuff, "Data in columns %d and %d of the data table supplied to the fft command have conflicting units of <%s> and <%s> respectively. These represent the real and imaginary components of an input sample, and must have the same units.", Ndims+1, Ndims+2, ppl_printUnit(c,&data->firstEntries[Ndims],NULL,NULL,0,1,0), ppl_printUnit(c,&data->firstEntries[Ndims+1],NULL,NULL,1,1,0)); TBADD2(ERR_NUMERIC,0); return; }


    // Check that we have at least two datapoints
    if (data->Nrows < 2)
     {
      sprintf(c->errStat.errBuff, "FFT construction is only possible on data sets with members at at least two values of x.");
      TBADD2(ERR_NUMERIC,0); return;
     }

    // Allocate workspace in which to do FFT
    datagrid = (fftw_complex *)fftw_malloc(Nsamples * sizeof(fftw_complex));
    if (datagrid == NULL) { sprintf(c->errStat.errBuff, "Out of memory."); TBADD2(ERR_MEMORY,0); return; }

    // Loop through data table
    blk = data->first; j=0;
    for (i=0; i<Nsamples; i++)
     {
      int l, Npos[USING_ITEMS_MAX];
      double pos[USING_ITEMS_MAX];
      if ((blk==NULL)||(j==blk->blockPosition)) { sprintf(c->errStat.errBuff, "Premature end to data table supplied to the fft command. To perform a "); k=strlen(c->errStat.errBuff); for (l=0;l<Ndims;l++) { sprintf(c->errStat.errBuff+k, "%dx", Nsteps[l]); k+=strlen(c->errStat.errBuff+k); } k-=(Ndims>0); sprintf(c->errStat.errBuff+k, " Fourier transform, need a grid of %d samples. Only received %ld samples.", Nsamples, i); TBADD2(ERR_FILE,0); return; }

      // Work out what position we're expecting this data point to represent
      for (k=i, l=Ndims-1; l>=0; l--) { Npos[l] = (k % Nsteps[l]); pos[l] = min[l]+step[l]*Npos[l]; k /= Nsteps[l]; }

      // Check that first Ndims columns indeed represent this point
      for (k=0; k<Ndims; k++)
       if (!ppl_dblEqual(blk->data_real[k + (Ndims+2)*j] , pos[k]))
        {
         int m;
         sprintf(c->errStat.errBuff, "Data supplied to fft command must be on a regular rectangular grid and in row-major ordering. Row %ld should represent a data point at position (", i+1);
         m=strlen(c->errStat.errBuff);
         for (l=0; l<Ndims; l++) { pplObj x=unit[l]; x.real=pos[l]; sprintf(c->errStat.errBuff+m,"%s,",ppl_unitsNumericDisplay(c,&x,0,1,-1)); m+=strlen(c->errStat.errBuff+m); }
         m-=(Ndims>0);
         sprintf(c->errStat.errBuff+m, "). In fact, it contained a data point at position ("); m+=strlen(c->errStat.errBuff+m);
         for (l=0; l<Ndims; l++) { pplObj x=unit[l]; x.real=blk->data_real[l + (Ndims+2)*j]; sprintf(c->errStat.errBuff+m,"%s,",ppl_unitsNumericDisplay(c,&x,0,1,-1)); m+=strlen(c->errStat.errBuff+m); }
         m-=(Ndims>0);
         sprintf(c->errStat.errBuff+m, ")."); j=strlen(c->errStat.errBuff);
         TBADD2(ERR_NUMERIC,0);
         return;
        }

      {
      pplObj x; x.refCount=1;
      pplObjNum(&x,0,0,0);
      x.real = blk->data_real[(Ndims+0) + (Ndims+2)*j];
      x.imag = blk->data_real[(Ndims+1) + (Ndims+2)*j];
      if (x.imag==0) { x.flagComplex=0; x.imag=0.0; } else { x.flagComplex=1; }
      (*WindowType)(&x, Ndims, Npos, Nsteps); // Apply window function to data
      #ifdef HAVE_FFTW3
      datagrid[i][0] = x.real; datagrid[i][1] = x.imag;
      #else
      datagrid[i].re = x.real; datagrid[i].im = x.imag;
      #endif
      }
      j++;
      if (j==blk->blockPosition) { j=0; blk=blk->next; }
     }
   }
  else // Fetch data from a function
   {
    int   i, fnlen;
    int   pos = PARSE_ifft_fnnames;
    char *scratchpad = (char *)ppl_memAlloc(LSTR_LENGTH);

    // Allocate workspace in which to do FFT
    datagrid = (fftw_complex *)fftw_malloc(Nsamples * sizeof(fftw_complex));
    if (datagrid == NULL) { sprintf(c->errStat.errBuff, "Out of memory."); TBADD2(ERR_MEMORY,0); return; }

    // Print function name
    fnlen=0;
    while (stk[pos].objType == PPLOBJ_NUM)
     {
      pos = (int)round(stk[pos].real);
      if (pos<=0) break;
      if (fnlen!=0) scratchpad[fnlen++]='.';
      sprintf(scratchpad+fnlen,"%s",(char *)stk[pos+PARSE_ifft_fnname_fnnames].auxil);
     }
    scratchpad[fnlen++]='(';

    for (i=0; i<Nsamples; i++)
     {
      const int stkLevelOld = c->stackPtr;
      int       j = fnlen;
      int       k = i, l, explen1, explen2, errPos=-1, errType=-1, lastOpAssign;
      int       Npos[USING_ITEMS_MAX];
      double    pos[USING_ITEMS_MAX];
      pplExpr  *e;
      pplObj   *out;
      for (l=Ndims-1; l>=0; l--) { Npos[l] = (k % Nsteps[l]); pos[l] = min[l]+step[l]*Npos[l]; k /= Nsteps[l]; }
      for (l=0; l<Ndims; l++) { pplObj x=unit[l]; x.real=pos[l]; sprintf(scratchpad+j,"%s,",ppl_unitsNumericDisplay(c,&x,0,1,20)); j+=strlen(scratchpad+j); }
      sprintf(scratchpad+j-(Ndims>0),")");

      explen1 = explen2 = j-(Ndims>0)+1;
      ppl_error_setstreaminfo(&c->errcontext, 1, "fitting expression");
      ppl_expCompile(c,pl->srcLineN,pl->srcId,pl->srcFname,scratchpad,&explen2,1,1,1,&e,&errPos,&errType,c->errStat.errBuff);
      if (c->errStat.status || (errPos>=0)) { fftw_free(datagrid); return; }
      if (explen2<explen1)                  { fftw_free(datagrid); strcpy(c->errStat.errBuff, "Unexpected trailing matter at the end of expression."); pplExpr_free(e); TBADD2(ERR_SYNTAX,0); return; }
      out = ppl_expEval(c, e, &lastOpAssign, 1, 1);
      pplExpr_free(e);
      if (c->errStat.status) { fftw_free(datagrid); return; }
      if (out->objType!=PPLOBJ_NUM) { fftw_free(datagrid); sprintf(c->errStat.errBuff, "The supplied function to fit produces a value which is not a number but has type <%s>.", pplObjTypeNames[out->objType]); TBADD2(ERR_TYPE,0); STACK_CLEAN; return; }
      if ((!gsl_finite(out->real))||(!gsl_finite(out->imag)))
       {
        int j=0;
        sprintf(c->errStat.errBuff+j, "Could not evaluate input function at position "); j+=strlen(c->errStat.errBuff+j);
        strncpy(c->errStat.errBuff+j, scratchpad, fnlen); j+=fnlen;
        for (l=0; l<Ndims; l++) { pplObj x=unit[l]; x.real=pos[l]; sprintf(c->errStat.errBuff+j,"%s,",ppl_unitsNumericDisplay(c,&x,0,1,-1)); j+=strlen(c->errStat.errBuff+j); }
        sprintf(c->errStat.errBuff+j, ")");
        TBADD2(ERR_NUMERIC,0);
        STACK_CLEAN; return;
       }

      if (i==0) { firstEntry=*out; firstEntry.real=1.0; firstEntry.imag=0.0; firstEntry.flagComplex=0; }
      else if (!ppl_unitsDimEqual(out, &firstEntry)) { sprintf(c->errStat.errBuff, "The supplied function to FFT does not produce values with consistent units; has produced values with units of <%s> and of <%s>.", ppl_printUnit(c,&firstEntry,NULL,NULL,0,1,0), ppl_printUnit(c,out,NULL,NULL,1,1,0)); TBADD2(ERR_NUMERIC,0); STACK_CLEAN; return; }
      (*WindowType)(out, Ndims, Npos, Nsteps); // Apply window function to data
      #ifdef HAVE_FFTW3
      datagrid[i][0] = out->real; datagrid[i][1] = out->imag;
      #else
      datagrid[i].re = out->real; datagrid[i].im = out->imag;
      #endif
      STACK_CLEAN;
     }
   }

  // We're finished... can now free DataTable
  ppl_memAlloc_AscendOutOfContext(contextLocalVec);

  // FFT data
  #ifdef HAVE_FFTW3
  fftwplan = fftw_plan_dft(Ndims, Nsteps, datagrid, datagrid, inverse ? FFTW_BACKWARD : FFTW_FORWARD, FFTW_ESTIMATE); // FFTW 3.x
  fftw_execute(fftwplan);
  fftw_destroy_plan(fftwplan);
  #else
  fftwplan = fftwnd_create_plan(Ndims, Nsteps, inverse ? FFTW_BACKWARD : FFTW_FORWARD, FFTW_ESTIMATE);                // FFTW 2.x
  fftwnd_one(fftwplan, datagrid, datagrid);
  fftwnd_destroy_plan(fftwplan);
  #endif

  // Make FFTDescriptor data structure
  output = (FFTDescriptor *)malloc(sizeof(FFTDescriptor));
  if (output == NULL) { sprintf(c->errStat.errBuff, "Out of memory."); TBADD2(ERR_MEMORY,0); fftw_free(datagrid); return; }
  output->Ndims    = Ndims;
  for (i=0; i<Ndims; i++) { output->XSize[i] = Nsteps[i]; }
  for (i=0; i<Ndims; i++) { pplObjNum(&output->range[i],0,max[i]-min[i],0); output->range[i].refCount=1; ppl_unitsDimCpy(&output->range[i],&unit[i]); }
  for (i=0; i<Ndims; i++) { pplObjNum(&output->invRange[i],0,0,0); ppl_unitsDimInverse(&output->invRange[i], &output->range[i]); }
  output->datagrid = datagrid;

  // Apply normalisation to data and phase-shift it t put zero in the right place
  {
  int    Npos[USING_ITEMS_MAX];
  double norm = 1.0;
  for (i=0; i<Ndims; i++) norm *= output->range[i].real / Nsteps[i];
  for (i=0; i<Ndims; i++) Npos[i] = - min[i] * Nsteps[i] / (max[i] - min[i]); // Position of zero
  for (i=0; i<Nsamples; i++)
   {
    int l;
    double pos,angle,normR,normI,normR2,normI2;
    normR = norm;
    normI = 0.0;
    k=i;
    for (l=Ndims-1; l>=0; l--)
     {
      pos = (k % Nsteps[l]);
      k /= Nsteps[l];
      angle = (inverse?-1:1)*2*M_PI*pos*Npos[l]/Nsteps[l];
      normR2 = normR * cos(angle) - normI * sin(angle);
      normI2 = normR * sin(angle) + normI * cos(angle);
      normR = normR2;
      normI = normI2;
     }
    #ifdef HAVE_FFTW3
    datagrid[i][0] = datagrid[i][0] * normR - datagrid[i][1] * normI;
    datagrid[i][1] = datagrid[i][0] * normI + datagrid[i][1] * normR;
    #else
    datagrid[i].re = datagrid[i].re * normR - datagrid[i].im * normI;
    datagrid[i].im = datagrid[i].re * normI + datagrid[i].im * normR;
    #endif
   }
  }

  // Make output unit
  {
  int status=0, et=0;
  output->outputUnit = firstEntry; // Output of an FFT has units of fn being FFTed, multiplied by the units of all of the arguments being FFTed
  for (i=0; i<Ndims; i++) { ppl_uaMul(c, &output->outputUnit, &output->range[i], &output->outputUnit, &status, &et, c->errStat.errBuff); if (status>=0) break; }
  if (status) { TBADD2(ERR_INTERNAL,0); fftw_free(datagrid); free(output); return; }
  output->outputUnit.real = output->outputUnit.imag = 0.0;
  output->outputUnit.flagComplex = 0; // Output unit has zero magnitude
  }

  // Make a new function descriptor
  funcPtr = (pplFunc *)malloc(sizeof(pplFunc));
  if (funcPtr == NULL) { sprintf(c->errStat.errBuff, "Out of memory whilst adding FFT object to function dictionary."); TBADD2(ERR_MEMORY,0); return; }
  funcPtr->functionType    = PPL_FUNC_FFT;
  funcPtr->refCount        = 1;
  funcPtr->minArgs         = Ndims;
  funcPtr->maxArgs         = Ndims;
  funcPtr->notNan          = 1;
  funcPtr->realOnly        = 0;
  funcPtr->dimlessOnly     = 0;
  funcPtr->functionPtr     = (void *)output;
  funcPtr->argList         = NULL;
  funcPtr->min             = funcPtr->max       = NULL;
  funcPtr->minActive       = funcPtr->maxActive = NULL;
  funcPtr->next            = NULL;
  funcPtr->description = funcPtr->LaTeX = funcPtr->descriptionShort = NULL;

  // Supersede any previous function descriptor
  {
   int     om, rc;
   pplObj  val;
   pplObj *obj=NULL;

   ppl_contextVarHierLookup(c, pl->srcLineN, pl->srcId, pl->srcFname, pl->linetxt, stk, in->stkCharPos, &obj, PARSE_ifft_varnames, PARSE_ifft_varname_varnames);
   if ((c->errStat.status) || (obj==NULL)) return;
   if (obj->objType==PPLOBJ_GLOB) { sprintf(c->errStat.errBuff,"Variable declared global in global namespace."); TBADD2(ERR_NAMESPACE,0); return; }

   om = obj->amMalloced;
   rc = obj->refCount;
   obj->amMalloced = 0;
   obj->refCount = 1;
   ppl_garbageObject(obj);
   val.refCount=1;
   pplObjFunc(&val,0,1,funcPtr);
   pplObjCpy(obj, &val, 0, om, 1);
   obj->refCount = rc;
  }

  // Free copies of data vectors defined within our local context
  ppl_memAlloc_AscendOutOfContext(contextDataTab);
  return;
 }

// Function which is called whenever an FFT function is evaluated, to extract value out of data grid
void ppl_fft_evaluate(ppl_context *c, char *FuncName, FFTDescriptor *desc, pplObj *in, pplObj *out, int *status, char *errout)
 {
  int i, j;
  double tempDbl;

  *out = desc->outputUnit;

  // Issue user with a warning if complex arithmetic is not enabled
  if (c->set->term_current.ComplexNumbers != SW_ONOFF_ON) ppl_warning(&c->errcontext, ERR_NUMERIC, "Attempt to evaluate a Fourier transform function whilst complex arithmetic is disabled. Fourier transforms are almost invariably complex and so this is unlikely to work.");

  // Check dimensions of input arguments and ensure that they are all real
  for (i=0; i<desc->Ndims; i++)
   {
    if (!ppl_unitsDimEqual(in+i, &desc->invRange[i]))
     {
      if (c->set->term_current.ExplicitErrors == SW_ONOFF_ON) { sprintf(errout, "The %s(x) function expects argument %d to have dimensions of <%s>, but has instead received an argument with dimensions of <%s>.", FuncName, i+1, ppl_printUnit(c, &desc->invRange[i], NULL, NULL, 0, 1, 0), ppl_printUnit(c, in+i, NULL, NULL, 1, 1, 0)); }
      else { pplObjNum(out,0,0,0); out->real = GSL_NAN; out->imag = 0; }
      *status=1;
      return;
     }
    if ((in+i)->flagComplex)
     {
      if (c->set->term_current.ExplicitErrors == SW_ONOFF_ON) { sprintf(errout, "The %s(x) function expects argument %d to be real, but the supplied argument has an imaginary component.", FuncName, i+1); }
      else { pplObjNum(out,0,0,0); out->real = GSL_NAN; out->imag = 0; }
      *status=1;
      return;
     }
   }

  // Work out closest datapoint in FFT datagrid to the one we want
  j=0;
  for (i=0; i<desc->Ndims; i++)
   {
    tempDbl = floor((in+i)->real * desc->range[i].real + 0.5);
    if      ((tempDbl >= 0.0) && (tempDbl <= desc->XSize[i]/2)) { } // Positive frequencies stored in lower half of array
    else if ((tempDbl <  0.0) && (tempDbl >=-desc->XSize[i]/2)) { tempDbl += desc->XSize[i]; } // Negative frequencies stored in upper half of array
    else                                                        { return; } // Query out of range; return zero with appropriate output unit
    j *= desc->XSize[i];
    j += (int)tempDbl;
   }

  // Write output value to out
  #ifdef HAVE_FFTW3
  out->real = desc->datagrid[j][0];
  if (desc->datagrid[j][1] == 0.0) { out->flagComplex = 0; out->imag = 0.0;                  }
  else                             { out->flagComplex = 1; out->imag = desc->datagrid[j][1]; }
  #else
  out->real = desc->datagrid[j].re;
  if (desc->datagrid[j].im == 0.0) { out->flagComplex = 0; out->imag = 0.0;                  }
  else                             { out->flagComplex = 1; out->imag = desc->datagrid[j].im; }
  #endif
  return;
 }

