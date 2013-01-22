// datafile_rasters.c
//
// The code in this file is part of Pyxplot
// <http://www.pyxplot.org.uk>
//
// Copyright (C) 2006-2013 Dominic Ford <coders@pyxplot.org.uk>
//               2008-2013 Ross Church
//
// $Id$
//
// Pyxplot is free software; you can redistribute it and/or modify it under the
// terms of the GNU General Public License as published by the Free Software
// Foundation; either version 2 of the License, or (at your option) any later
// version.
//
// You should have received a copy of the GNU General Public License along with
// Pyxplot; if not, write to the Free Software Foundation, Inc., 51 Franklin
// Street, Fifth Floor, Boston, MA  02110-1301, USA

// ----------------------------------------------------------------------------

#define _DATAFILE_RASTERS_C 1

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <math.h>

#include "coreUtils/errorReport.h"
#include "coreUtils/memAlloc.h"
#include "coreUtils/list.h"
#include "coreUtils/dict.h"
#include "parser/parser.h"
#include "settings/settings.h"
#include "settings/settingTypes.h"
#include "stringTools/asciidouble.h"
#include "stringTools/strConstants.h"
#include "userspace/context.h"
#include "userspace/contextVarDef.h"
#include "userspace/pplObj.h"
#include "userspace/pplObjFunc.h"
#include "userspace/unitsArithmetic.h"

void ppldata_fromFuncs_checkSpecialRaster(ppl_context *C, pplExpr **fnlist, int fnlist_len, char *dummyVar, double *min, double *max, pplObj *unit, double **raster, int *rasterLen)
 {
  int      i, j, pos, commaPos=0, bracketLevel=0, containsOperator=0, containsDummy=0, NWords=0, outputContext, contextRough=-1;
  int      dummyArgNo=0;
  char     newWord=1, fail=0;
  char     c;
  char    *buf = C->errcontext.tempErrStr;
  char    *currentFnName="", *dummyInFunction=NULL;
  list    *bracketStack, *bracketCommaPos;
  pplObj  *funcObjPtr=NULL;
  pplFunc *funcPtr=NULL;

  // Make temporary rough workspace
  outputContext   = ppl_memAlloc_GetMemContext();
  contextRough    = ppl_memAlloc_DescendIntoNewContext();
  bracketStack    = ppl_listInit(0);
  bracketCommaPos = ppl_listInit(0);

  for (i=0; i<fnlist_len; i++)
   {
    j=pos=0;
    while (1)
     {
      c = fnlist[i]->ascii[j++];
      if ((isalnum(c)) || (c=='_')) { newWord=0; buf[pos++] = c; continue; }
      if ((!newWord) && (pos>0)) NWords++;
      if (!newWord) buf[pos++] = '\0';
      newWord=1;
      pos=0;
      if ((c<=' ') && (c!='\0')) continue;
      if (strcmp(buf, dummyVar)==0)
       {
        containsDummy=1;
        if (containsOperator || (NWords>1)) fail=1;
        if ((dummyInFunction != NULL) && (currentFnName != NULL))
         {
          if ((strcmp(dummyInFunction, currentFnName)!=0) || (dummyArgNo != commaPos)) fail=1;
         }
        if (currentFnName==NULL) fail=1;
        else
         {
          dummyInFunction = currentFnName;
          dummyArgNo      = commaPos;
         }
       }
      if (c=='(')
       {
        if (containsDummy) fail=1;
        ppl_listAppendCpy(bracketCommaPos, &commaPos, sizeof(int));
        ppl_listAppendCpy(bracketStack, buf, strlen(buf)+1);
        bracketLevel++;
        currentFnName = (char *)ppl_listLast(bracketStack);
        containsOperator = 0;
        containsDummy = 0;
        NWords = 0;
        buf[0]='\0';
        continue;
       }
      buf[0]='\0';
      if (c==')')
       {
        if (bracketLevel>0)
         {
          ppl_listPop(bracketStack);
          currentFnName = (char *)ppl_listLast(bracketStack);
          commaPos = *(int *)ppl_listPop(bracketCommaPos);
          containsOperator = 1;
          containsDummy = 0;
          NWords = 10;
          bracketLevel--;
         }
        continue;
       }
      if (c==',')
       {
        commaPos++;
        containsOperator = 0;
        containsDummy = 0;
        NWords = 0;
        continue;
       }
      if (c=='\0') break;
      containsOperator=1;
      if (containsDummy) fail=1;
     }
   }

  if (fail || (dummyInFunction==NULL) || (dummyInFunction[0]=='\0')) goto CLEANUP;

  // Look up function which dummy variable is an argument to
  ppl_contextVarLookup(C, dummyInFunction, &funcObjPtr, 0);
  if ( (funcObjPtr==NULL) || (funcObjPtr->objType!=PPLOBJ_FUNC) ) goto CLEANUP;
  funcPtr = (pplFunc *)funcObjPtr->auxil;
  if (funcPtr==NULL) goto CLEANUP;
  if ((funcPtr->functionType!=PPL_FUNC_HISTOGRAM) && (funcPtr->functionType!=PPL_FUNC_FFT)) goto CLEANUP;
  if (dummyArgNo >= funcPtr->minArgs) goto CLEANUP;

  // Make custom raster
  if (funcPtr->functionType == PPL_FUNC_HISTOGRAM)
   {
    histogramDescriptor *desc = (histogramDescriptor *)funcPtr->functionPtr;
    double              *outputRaster;
    int                  rasterCount=0;
    long                 NInput = desc->Nbins-1;
    if (NInput<1) NInput=1;

    outputRaster = (double *)ppl_memAlloc_incontext(NInput*sizeof(double), outputContext);
    if (outputRaster == NULL) goto CLEANUP;

    for (i=0; i<(desc->Nbins-1); i++)
     {
      double midpoint = desc->log?sqrt(desc->bins[i]*desc->bins[i+1])
                                 :   ((desc->bins[i]+desc->bins[i+1])/2);
      if ( ((min==NULL)||(*min<=midpoint)) && ((max==NULL)||(*max>=midpoint)) )
       { outputRaster[ rasterCount++ ] = midpoint; }
     }

    *raster    = outputRaster;
    *rasterLen = rasterCount;
   }
  else if (funcPtr->functionType == PPL_FUNC_FFT)
   {
    FFTDescriptor *desc = (FFTDescriptor *)funcPtr->functionPtr;
    double        *outputRaster;
    int            rasterCount=0;

    if (dummyArgNo >= desc->Ndims) goto CLEANUP;
    outputRaster = (double *)ppl_memAlloc_incontext(desc->XSize[dummyArgNo]*sizeof(double), outputContext);
    if (outputRaster == NULL) goto CLEANUP;

    for (i=0; i<desc->XSize[dummyArgNo]; i++)
     {
      double x = (i-desc->XSize[dummyArgNo]/2.0) / desc->range[dummyArgNo].real;
      if ( ((min==NULL)||(*min<=x)) && ((max==NULL)||(*max>=x)) ) { outputRaster[ rasterCount++ ] = x; }
     }

    *raster    = outputRaster;
    *rasterLen = rasterCount;
   }

CLEANUP:
  // Delete rough workspace
  if (contextRough>0) ppl_memAlloc_AscendOutOfContext(contextRough);
  return;
 }

