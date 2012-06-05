// histogram.c
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

#define _HISTOGRAM_C 1

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

#include <gsl/gsl_math.h>

#include "commands/histogram.h"
#include "coreUtils/memAlloc.h"
#include "expressions/traceback_fns.h"
#include "parser/cmdList.h"
#include "parser/parser.h"
#include "settings/settings.h"
#include "settings/settingTypes.h"
#include "userspace/context.h"
#include "userspace/contextVarDef.h"
#include "userspace/garbageCollector.h"
#include "userspace/pplObj_fns.h"
#include "userspace/pplObjFunc.h"
#include "userspace/unitsArithmetic.h"
#include "userspace/unitsDisp.h"

#include "datafile.h"

#define TBADD2(et,pos) ppl_tbAdd(c,pl->srcLineN,pl->srcId,pl->srcFname,0,et,pos,pl->linetxt,"")

#define COUNTEDERR1 if (errCount >0) { errCount--;
#define COUNTEDERR2 if (errCount==0) { sprintf(c->errcontext.tempErrStr, "%s: Too many errors: no more errors will be shown.",filenameOut); ppl_warning(&c->errcontext,ERR_STACKED,NULL); } }

static int hcompare(const void *x, const void *y)
 {
  if      (*((const double *)x) > *((const double *)y)) return  1.0;
  else if (*((const double *)x) < *((const double *)y)) return -1.0;
  else                                                  return  0.0;
 }

void ppl_directive_histogram(ppl_context *c, parserLine *pl, parserOutput *in, int interactive, int iterDepth)
 {
  pplObj        *stk = in->stk;
  dataTable     *data;
  dataBlock     *blk;
  long int       i=0, j, k;
  int            contextLocalVec, contextDataTab, errCount=DATAFILE_NERRS;
  int            gotBinList, logAxis;
  pplFunc       *funcPtr;
  pplObj         v, firstEntry;
  char           filenameOut[FNAME_LENGTH]="";
  pplObj         unit  [USING_ITEMS_MAX];
  int            minSet[USING_ITEMS_MAX], maxSet[USING_ITEMS_MAX];
  double         min   [USING_ITEMS_MAX], max   [USING_ITEMS_MAX];
  double        *xdata;
  parserLine    *spool=NULL, **dataSpool = &spool;
  histogramDescriptor *output;

  // Read ranges
  {
   int i,pos=PARSE_histogram_0range_list,nr=0;
   const int o1 = PARSE_histogram_min_0range_list;
   const int o2 = PARSE_histogram_max_0range_list;
   for (i=0; i<USING_ITEMS_MAX; i++) minSet[i]=0;
   for (i=0; i<USING_ITEMS_MAX; i++) maxSet[i]=0;
   while (stk[pos].objType == PPLOBJ_NUM)
    {
     pos = (int)round(stk[pos].real);
     if (pos<=0) break;
     if (nr>=1)
      {
       sprintf(c->errStat.errBuff,"Too many ranges specified; only one is allowed.");
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
         TBADD2(ERR_TYPE,in->stkCharPos[pos+PARSE_histogram_min_0range_list]);
         return;
        }
       if ((minSet[nr])&&(!ppl_unitsDimEqual(&unit[nr],&stk[pos+o2])))
        {
         sprintf(c->errStat.errBuff,"Minimum and maximum limits specified for variable %d have conflicting physical units of <%s> and <%s>.", nr+1, ppl_printUnit(c,&unit[nr],NULL,NULL,0,0,0), ppl_printUnit(c,&stk[pos+o2],NULL,NULL,1,0,0));
         TBADD2(ERR_UNIT,in->stkCharPos[pos+PARSE_histogram_min_0range_list]);
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

  // Read input data
   {
    const int NcolRequired=1;
    int status=0, j;
    ppldata_fromCmd(c, &data, pl, in, 0, filenameOut, dataSpool, PARSE_TABLE_histogram_, 0, NcolRequired, 0, min, minSet, max, maxSet, unit, 0, &status, c->errStat.errBuff, &errCount, iterDepth);

    // Exit on error
    if ((status)||(data==NULL))
     {
      TBADD2(ERR_GENERIC,0);
      ppl_memAlloc_AscendOutOfContext(contextLocalVec);
      return;
     }

    // Check that the firstEntries above have the same units as any supplied ranges
    for (j=0; j<NcolRequired; j++)
     if (minSet[j] || maxSet[j])
      {
       if (!ppl_unitsDimEqual(&unit[j],data->firstEntries+j)) { sprintf(c->errStat.errBuff, "The minimum and maximum limits specified in range %d in the fit command have conflicting physical dimensions with the data returned from the data file. The limits have units of <%s>, whilst the data have units of <%s>.", j+1, ppl_printUnit(c,unit+j,NULL,NULL,0,1,0), ppl_printUnit(c,data->firstEntries+j,NULL,NULL,1,1,0)); TBADD2(ERR_NUMERICAL,0); return; }
      }

    // Check that we have at least three datapoints
    if (data->Nrows < 3)
     {
      sprintf(c->errStat.errBuff, "Histogram construction is only possible on data sets with members at at least three values of x.");
      TBADD2(ERR_NUMERICAL,0); return;
     }

    firstEntry = data->firstEntries[0];
   }

  // Transfer data from multiple data tables into a single vector
  xdata = (double *)ppl_memAlloc_incontext(data->Nrows * sizeof(double), contextLocalVec);

  if (xdata==NULL) { sprintf(c->errStat.errBuff, "Out of memory."); TBADD2(ERR_MEMORY,0); return; }

  // Copy data table into an array of x values
  blk = data->first; i=0;
  while (blk != NULL)
   {
    for (j=0; j<blk->blockPosition; j++) xdata[i++] = blk->data_real[j];
    blk=blk->next;
   }

  // Free original data table which is no longer needed
  ppl_memAlloc_AscendOutOfContext(contextDataTab);

  // Sort data vector
  qsort((void *)xdata, i, sizeof(double), hcompare);

  // Work out whether we are constructing histogram in linear space or log space
  gotBinList = (stk[PARSE_histogram_bin_list].objType==PPLOBJ_NUM);
  logAxis = ((!gotBinList) && (c->set->XAxes[1].log == SW_BOOL_TRUE)); // Read from axis x1

  // Filter out any values of x which are out-of-range
  for (j=k=0; j<i; j++)
   {
    if ( !gsl_finite(xdata[j])            ) continue; // Ignore non-finite datapoints
    if ( (minSet[0]) && (xdata[j]<min[0]) ) continue; // Ignore out-of-range datapoints
    if ( (maxSet[0]) && (xdata[j]>max[0]) ) continue; // Ignore out-of-range datapoints

    if (logAxis)
     {
      if (xdata[j]<=0.0) { COUNTEDERR1; v=firstEntry; v.real=xdata[j]; sprintf(c->errcontext.tempErrStr,"Negative or zero values are not allowed in the construction of histograms in log space; value of x=%s will be ignored.",ppl_unitsNumericDisplay(c,&v, 0, 0, 0)); ppl_warning(&c->errcontext,ERR_NUMERICAL,NULL); COUNTEDERR2; continue; }
      xdata[k]=log(xdata[j]); // If we're constructing histogram in log space, log data now
      if (!gsl_finite(xdata[k])) continue;
     } else {
      xdata[k]=xdata[j];
     }
    k++;
   }

  // Check that we have at least three points to interpolate
  if (k<3) { sprintf(c->errStat.errBuff, "Histogram construction is only possible on data sets with members at at least three values of x."); TBADD2(ERR_NUMERICAL,0); return; }

  // Make histogramDescriptor data structure
  output = (histogramDescriptor *)malloc(sizeof(histogramDescriptor));
  if (output == NULL) { sprintf(c->errStat.errBuff, "Out of memory."); TBADD2(ERR_MEMORY,0); return; }
  output->unit     = firstEntry;
  output->log      = logAxis;
  output->filename = (char *)malloc(strlen(filenameOut)+1);
  if (output->filename == NULL) { sprintf(c->errStat.errBuff, "Out of memory."); TBADD2(ERR_MEMORY,0); free(output); return; }
  strcpy(output->filename , filenameOut);

  // Now need to work out what bins we're going to use
  if (gotBinList)
   {
    int binListLen=0, j;
    int pos=PARSE_histogram_bin_list;
    while (stk[pos].objType == PPLOBJ_NUM)
     {
      pos = (int)round(stk[pos].real);
      if (pos<=0) break;
      binListLen++;
     }
    output->bins  = (double *)malloc(binListLen*sizeof(double));
    if (output->bins == NULL) { sprintf(c->errStat.errBuff, "Out of memory."); TBADD2(ERR_MEMORY,0); free(output); free(output->filename); return; }
    j=0;
    pos=PARSE_histogram_bin_list;
    while (stk[pos].objType == PPLOBJ_NUM)
     {
      pplObj *val;
      pos = (int)round(stk[pos].real);
      if (pos<=0) break;
      val = &stk[pos+PARSE_histogram_x_bin_list];
      if (!ppl_unitsDimEqual(&firstEntry,val)) { sprintf(c->errcontext.tempErrStr, "The supplied bin boundary at x=%s has conflicting physical dimensions with the data supplied, which has units of <%s>. Ignoring this bin boundary.", ppl_printUnit(c,val,NULL,NULL,0,1,0), ppl_printUnit(c,&firstEntry,NULL,NULL,1,1,0)); ppl_warning(&c->errcontext,ERR_NUMERICAL,NULL); }
      else { output->bins[j++] = val->real; }
     }
    binListLen = j; // some of the bins may have been rejected
    output->Nbins = binListLen;
    qsort((void *)output->bins, binListLen, sizeof(double), hcompare); // Make sure that bins are in ascending order
   }
  else
   {
    pplObj *BinWidth, *BinOrigin, tempValObj;
    int     BinOriginSet;
    double  BinOriginDbl=0, BinWidthDbl, xbinmin, xbinmax;
    pplObj *inbinWidth  = &stk[PARSE_histogram_binwidth];
    pplObj *inbinOrigin = &stk[PARSE_histogram_binorigin];
    xbinmin = xdata[0];
    xbinmax = xdata[k-1];
    if (minSet[0]) xbinmin = logAxis ? log(min[0]) : min[0];
    if (maxSet[0]) xbinmax = logAxis ? log(max[0]) : max[0];
    if (xbinmax < xbinmin) { double temp=xbinmax; xbinmax=xbinmin; xbinmin=temp; }
    if (DEBUG) { sprintf(c->errcontext.tempErrStr, "Prior to application of BinOrigin, histogram command using range [%e:%e]",xbinmin,xbinmax); ppl_log(&c->errcontext,NULL); }

    if (inbinWidth->objType == PPLOBJ_NUM)
     { BinWidth = inbinWidth; }
    else if (c->set->term_current.BinWidthAuto)
     {
      BinWidth               = &tempValObj;
      tempValObj             = firstEntry;
      tempValObj.flagComplex = 0;
      tempValObj.imag        = 0.0;
      tempValObj.real        = (xbinmax-xbinmin)/100;
      if (logAxis) tempValObj.real = exp(tempValObj.real);
     }
    else
     { BinWidth  = &(c->set->term_current.BinWidth); }

    if (inbinOrigin->objType == PPLOBJ_NUM) { BinOrigin = inbinOrigin;                       BinOriginSet = 1;                                   }
    else                                    { BinOrigin = &(c->set->term_current.BinOrigin); BinOriginSet = !c->set->term_current.BinOriginAuto; }

    if ((!logAxis) && (!ppl_unitsDimEqual(&firstEntry,BinWidth))) { sprintf(c->errStat.errBuff,"The bin width supplied to the histogram command has conflicting physical dimensions with the data supplied. The former has units of <%s>, whilst the latter has units of <%s>.", ppl_printUnit(c,BinWidth,NULL,NULL,0,1,0), ppl_printUnit(c,&firstEntry,NULL,NULL,1,1,0)); TBADD2(ERR_NUMERICAL,0); free(output); free(output->filename); return; }
    if ((logAxis) && (BinWidth->dimensionless==0)) { sprintf(c->errStat.errBuff, "For logarithmically spaced bins, the multiplicative spacing between bins must be dimensionless. The supplied spacing has units of <%s>.", ppl_printUnit(c,BinWidth,NULL,NULL,0,1,0)); TBADD2(ERR_NUMERICAL,0); free(output); free(output->filename); return; }
    if ((BinOriginSet) && (!ppl_unitsDimEqual(&firstEntry,BinOrigin))) { sprintf(c->errStat.errBuff, "The bin origin supplied to the histogram command has conflicting physical dimensions with the data supplied. The former has units of <%s>, whilst the latter has units of <%s>.", ppl_printUnit(c,BinOrigin,NULL,NULL,0,1,0), ppl_printUnit(c,&firstEntry,NULL,NULL,1,1,0)); TBADD2(ERR_NUMERICAL,0); free(output); free(output->filename); return; }
    if ((logAxis) && (BinWidth->real <= 1.0)) { sprintf(c->errStat.errBuff, "For logarithmically spaced bins, the multiplicative spacing between bins must be greater than 1.0. Value supplied was %s.", ppl_unitsNumericDisplay(c,BinWidth,0,0,0)); TBADD2(ERR_NUMERICAL,0); free(output); free(output->filename); return; }
    if (BinWidth->real <= 0.0) { sprintf(c->errStat.errBuff, "The bin width supplied to the histogram command must be greater than zero. Value supplied was %s.", ppl_unitsNumericDisplay(c,BinWidth,0,0,0)); TBADD2(ERR_NUMERICAL,0); free(output); free(output->filename); return; }
    if ((logAxis) && (BinOriginSet) && (BinOrigin->real <= 0.0)) { sprintf(c->errStat.errBuff, "For logarithmically spaced bins, the specified bin origin must be greater than zero. Value supplied was %s.", ppl_unitsNumericDisplay(c,BinOrigin,0,0,0)); TBADD2(ERR_NUMERICAL,0); free(output); free(output->filename); return; }
    if ((logAxis) && (minSet[0]) && (min[0] <= 0.0)) { unit[0].real=min[0]; sprintf(c->errStat.errBuff, "For logarithmically spaced bins, the specified minimum must be greater than zero. Value supplied was %s.", ppl_unitsNumericDisplay(c,&unit[0],0,0,0)); TBADD2(ERR_NUMERICAL,0); free(output); free(output->filename); return; }
    if ((logAxis) && (maxSet[0]) && (max[0] <= 0.0)) { unit[0].real=max[0]; sprintf(c->errStat.errBuff, "For logarithmically spaced bins, the specified maximum must be greater than zero. Value supplied was %s.", ppl_unitsNumericDisplay(c,&unit[0],0,0,0)); TBADD2(ERR_NUMERICAL,0); free(output); free(output->filename); return; }
    if ((minSet[0]||maxSet[0])&&(!ppl_unitsDimEqual(&firstEntry,&unit[0]))) { sprintf(c->errStat.errBuff, "The range supplied to the histogram command has conflicting physical dimensions with the data supplied. The former has units of <%s>, whilst the latter has units of <%s>.", ppl_printUnit(c,&unit[0],NULL,NULL,0,1,0), ppl_printUnit(c,&firstEntry,NULL,NULL,1,1,0)); TBADD2(ERR_NUMERICAL,0); free(output); free(output->filename); return; }
    if (!gsl_finite(BinWidth->real)) { sprintf(c->errStat.errBuff, "The bin width specified to the histogram command is not a finite number."); TBADD2(ERR_NUMERICAL,0); free(output); free(output->filename); return; }
    if (BinOriginSet && !gsl_finite(BinOrigin->real)) { sprintf(c->errStat.errBuff, "The bin origin specified to the histogram command is not a finite number."); TBADD2(ERR_NUMERICAL,0); free(output); free(output->filename); return; }

    if (logAxis) { if (BinOriginSet) BinOriginDbl = log(BinOrigin->real); BinWidthDbl = log(BinWidth->real); }
    else         { if (BinOriginSet) BinOriginDbl =     BinOrigin->real ; BinWidthDbl =     BinWidth->real ; }

    if (DEBUG) { sprintf(c->errcontext.tempErrStr, "Histogram command using a bin width of %e and a bin origin of %e.", BinWidthDbl, BinOriginDbl); ppl_log(&c->errcontext,NULL); }

    // Generate a series of bins to use based on supplied BinWidth and BinOrigin
    if (BinOriginSet) BinOriginDbl = BinOriginDbl - BinWidthDbl * floor(BinOriginDbl / BinWidthDbl);
    else              BinOriginDbl = 0.0;
    xbinmin = floor((xbinmin-BinOriginDbl)/BinWidthDbl)*BinWidthDbl + BinOriginDbl;
    xbinmax = ceil ((xbinmax-BinOriginDbl)/BinWidthDbl)*BinWidthDbl + BinOriginDbl;
    if (DEBUG) { sprintf(c->errcontext.tempErrStr, "After application of BinOrigin, histogram command using range [%e:%e]",xbinmin,xbinmax); ppl_log(&c->errcontext,NULL); }

    if (((xbinmax-xbinmin)/BinWidthDbl + 1.0001) > 1e7) { sprintf(c->errStat.errBuff, "The supplied value of BinWidth produces a binning scheme with more than 1e7 bins. This is probably not sensible."); TBADD2(ERR_GENERIC,0); return; }
    output->Nbins = (long)((xbinmax-xbinmin)/BinWidthDbl + 1.0001);
    if (DEBUG) { sprintf(c->errcontext.tempErrStr, "Histogram command using %ld bins",output->Nbins); ppl_log(&c->errcontext,NULL); }
    output->bins = (double *)malloc(output->Nbins*sizeof(double));
    if (output->bins == NULL) { sprintf(c->errStat.errBuff, "Out of memory."); TBADD2(ERR_MEMORY,0); return; }

    for (j=0; j<output->Nbins; j++) output->bins[j] = xbinmin + j*BinWidthDbl;
   }

  // Allocate vector for storing histogram values
  output->binvals = (double *)malloc(output->Nbins * sizeof(double));
  if (output->binvals == NULL) { sprintf(c->errStat.errBuff, "Out of memory."); TBADD2(ERR_MEMORY,0); free(output->bins); return; }
  for (i=0; i<output->Nbins; i++) output->binvals[i]=0.0;

  // Count number of data values in each histogram bin
  i=0;
  for (j=0; j<k; j++) // Loop over all of the data points we have read in
   {
    while ((i < output->Nbins) && (xdata[j] > output->bins[i])) i++;
    if (i>=output->Nbins) break; // We gone off the top of the last bin
    if (i==0) continue; // We've not yet arrived in the bottom bin
    output->binvals[i-1] += 1.0;
   }

  // If this is a logarithmic set of bins, unlog bin boundaries now
  if (logAxis) for (i=0; i<output->Nbins; i++) output->bins[i] = exp(output->bins[i]);

  // Divide each bin's number of counts by its width
  for (i=0; i<output->Nbins-1; i++) output->binvals[i] /= output->bins[i+1] - output->bins[i] + 1e-200;

  // Debugging lines
  if (DEBUG)
   for (i=0; i<output->Nbins-1; i++)
    {
     sprintf(c->errcontext.tempErrStr, "Bin %ld [%e:%e] --> %e", i+1, output->bins[i], output->bins[i+1], output->binvals[i]);
     ppl_log(&c->errcontext,NULL);
    }

  // Make a new function descriptor
  funcPtr = (pplFunc *)malloc(sizeof(pplFunc));
  if (funcPtr == NULL) { sprintf(c->errStat.errBuff, "Out of memory whilst adding histogram object to function dictionary."); TBADD2(ERR_MEMORY,0); return; }
  funcPtr->functionType    = PPL_FUNC_HISTOGRAM;
  funcPtr->refCount        = 1;
  funcPtr->minArgs         = 1;
  funcPtr->maxArgs         = 1;
  funcPtr->notNan          = 1;
  funcPtr->realOnly        = 1;
  funcPtr->numOnly         = 1;
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

   ppl_contextVarHierLookup(c, pl->srcLineN, pl->srcId, pl->srcFname, pl->linetxt, stk, in->stkCharPos, &obj, PARSE_histogram_varnames, PARSE_histogram_varname_varnames);
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
  ppl_memAlloc_AscendOutOfContext(contextLocalVec);
  return;
 }

void ppl_histogram_evaluate(ppl_context *c, char *FuncName, histogramDescriptor *desc, pplObj *in, pplObj *out, int *status, char *errout)
 {
  long   i, Nsteps, len, ss, pos;
  double dblin;

  if (!ppl_unitsDimEqual(in, &desc->unit))
   {
    if (c->set->term_current.ExplicitErrors == SW_ONOFF_ON) { sprintf(errout, "The %s(x) function expects an argument with dimensions of <%s>, but has instead received an argument with dimensions of <%s>.", FuncName, ppl_printUnit(c, &desc->unit, NULL, NULL, 0, 1, 0), ppl_printUnit(c, in, NULL, NULL, 1, 1, 0)); }
    else { pplObjNum(out,0,0,0); out->real = GSL_NAN; out->imag = 0; }
    *status=1;
    return;
   }
  if (in->flagComplex)
   {
    if (c->set->term_current.ExplicitErrors == SW_ONOFF_ON) { sprintf(errout, "The %s(x) function expects a real argument, but the supplied argument has an imaginary component.", FuncName); }
    else { pplObjNum(out,0,0,0); out->real = GSL_NAN; out->imag = 0; }
    *status=1;
    return;
   }

  dblin = in->real;

  *out = desc->unit;
  out->imag = 0.0;
  out->real = 0.0;
  out->flagComplex = 0;
  for (i=0; i<UNITS_MAX_BASEUNITS; i++) if (out->exponent[i]!=0.0) out->exponent[i]*=-1; // Output has units of 'per x'
  if ((desc->Nbins<1) || (dblin<desc->bins[0]) || (dblin>desc->bins[desc->Nbins-1])) return; // Query is outside range of histogram

  len    = desc->Nbins;
  Nsteps = (long)ceil(log(len)/log(2));
  for (pos=i=0; i<Nsteps; i++)
   {
    ss = 1<<(Nsteps-1-i);
    if (pos+ss>=len) continue;
    if (desc->bins[pos+ss]<=dblin) pos+=ss;
   }
  if      (desc->bins[pos]>dblin) return; // Off left end
  else if (pos==len-1)            return; // Off right end

  out->real = desc->binvals[pos];
  return;
 }

