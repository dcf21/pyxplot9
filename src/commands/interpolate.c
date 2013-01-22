// interpolate.c
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

#define _FFT_C 1

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <glob.h>
#include <wordexp.h>

#include <gsl/gsl_math.h>

#include "commands/interpolate.h"
#include "commands/interpolate_2d_engine.h"
#include "coreUtils/memAlloc.h"
#include "epsMaker/bmp_bmpread.h"
#include "expressions/expEval.h"
#include "expressions/traceback_fns.h"
#include "parser/cmdList.h"
#include "parser/parser.h"
#include "settings/settingTypes.h"
#include "userspace/context.h"
#include "userspace/contextVarDef.h"
#include "userspace/garbageCollector.h"
#include "userspace/pplObj_fns.h"
#include "userspace/pplObjFunc.h"
#include "userspace/unitsArithmetic.h"
#include "userspace/unitsDisp.h"

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

static double *compareZeroPoint;

static int interpolateSorter(const void *x, const void *y)
 {
  if      (  *(compareZeroPoint + (*((long *)x))) > *(compareZeroPoint + (*((long *)y)))  ) return  1;
  else if (  *(compareZeroPoint + (*((long *)x))) < *(compareZeroPoint + (*((long *)y)))  ) return -1;
  else                                                                                      return  0;
 }

void ppl_directive_interpolate(ppl_context *c, parserLine *pl, parserOutput *in, int interactive, int iterDepth, int mode)
 {
  pplObj        *stk = in->stk;
  dataTable     *data;
  dataBlock     *blk;
  bitmap_data    bmpdata;
  long int       i=0, j, k, ims, jms, sizeX=0, sizeY=0;
  int            contextLocalVec, contextDataTab, errCount=DATAFILE_NERRS;
  unsigned char *bmpchars = NULL;
  double        *xdata=NULL, *ydata=NULL, *zdata=NULL, *MinList=NULL, *MaxList=NULL;
  pplFunc       *funcPtr;
  pplObj         v, firstEntries[3];
  int            NxRequired, NcolRequired, bmp=-1;
  char          *splineTypeName;
  char           filenameOut[FNAME_LENGTH]="";
  parserLine    *spool=NULL, **dataSpool = &spool;
  pplObj         unit  [USING_ITEMS_MAX];
  int            minSet[USING_ITEMS_MAX], maxSet[USING_ITEMS_MAX];
  double         min   [USING_ITEMS_MAX], max   [USING_ITEMS_MAX];
  splineDescriptor      *desc;
  const gsl_interp_type *splineType= NULL;
  gsl_spline            *splineObj = NULL;
  gsl_interp_accel      *accel     = NULL;

  // Read ranges
  {
   int i,pos=PARSE_interpolate2d_0range_list,nr=0;
   const int o1 = PARSE_interpolate2d_min_0range_list;
   const int o2 = PARSE_interpolate2d_max_0range_list;
   for (i=0; i<USING_ITEMS_MAX; i++) minSet[i]=0;
   for (i=0; i<USING_ITEMS_MAX; i++) maxSet[i]=0;
   while (stk[pos].objType == PPLOBJ_NUM)
    {
     pos = (int)round(stk[pos].real);
     if (pos<=0) break;
     if (nr>=2)
      {
       sprintf(c->errStat.errBuff,"Too many ranges specified; a maximum of two are allowed.");
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
         TBADD2(ERR_TYPE,in->stkCharPos[pos+PARSE_interpolate2d_min_0range_list]);
         return;
        }
       if ((minSet[nr])&&(!ppl_unitsDimEqual(&unit[nr],&stk[pos+o2])))
        {
         sprintf(c->errStat.errBuff,"Minimum and maximum limits specified for variable %d have conflicting physical units of <%s> and <%s>.", nr+1, ppl_printUnit(c,&unit[nr],NULL,NULL,0,0,0), ppl_printUnit(c,&stk[pos+o2],NULL,NULL,1,0,0));
         TBADD2(ERR_UNIT,in->stkCharPos[pos+PARSE_interpolate2d_min_0range_list]);
         return;
        }
       unit[nr]=stk[pos+o2];
       max[nr]=unit[nr].real;
       maxSet[nr]=1;
      }
     nr++;
    }
  }

  // Work out number of datapoints and columns we need for each type of interpolation
  if      (mode == INTERP_AKIMA   ) { splineType = gsl_interp_akima;      NcolRequired = 2; NxRequired = 5; }
  else if (mode == INTERP_LINEAR  ) { splineType = gsl_interp_linear;     NcolRequired = 2; NxRequired = 2; }
  else if (mode == INTERP_LOGLIN  ) { splineType = gsl_interp_linear;     NcolRequired = 2; NxRequired = 2; }
  else if (mode == INTERP_POLYN   ) { splineType = gsl_interp_polynomial; NcolRequired = 2; NxRequired = 3; }
  else if (mode == INTERP_SPLINE  ) { splineType = gsl_interp_cspline;    NcolRequired = 2; NxRequired = 3; }
  else if (mode == INTERP_STEPWISE) { splineType = NULL;                  NcolRequired = 2; NxRequired = 1; }
  else if (mode == INTERP_2D      ) {                                     NcolRequired = 3; NxRequired = 1; }
  else if (mode == INTERP_BMPR    ) { bmp = 0;                            NcolRequired = 3; NxRequired = 1; }
  else if (mode == INTERP_BMPG    ) { bmp = 1;                            NcolRequired = 3; NxRequired = 1; }
  else if (mode == INTERP_BMPB    ) { bmp = 2;                            NcolRequired = 3; NxRequired = 1; }
  else                            { sprintf(c->errStat.errBuff,"interpolate command requested to perform unknown type of interpolation."); TBADD2(ERR_INTERNAL,0); return; }

  // Allocate a new memory context for the data file we're about to read
  contextLocalVec= ppl_memAlloc_DescendIntoNewContext();
  contextDataTab = ppl_memAlloc_DescendIntoNewContext();

  // Fetch string name of interpolation type
  if      (mode == INTERP_AKIMA   ) splineTypeName="Akima spline";
  else if (mode == INTERP_LINEAR  ) splineTypeName="Linear";
  else if (mode == INTERP_LOGLIN  ) splineTypeName="Power-law";
  else if (mode == INTERP_POLYN   ) splineTypeName="Polynomial";
  else if (mode == INTERP_SPLINE  ) splineTypeName="Cubic spline";
  else if (mode == INTERP_STEPWISE) splineTypeName="Stepwise";
  else if (mode == INTERP_2D      ) splineTypeName="Two-dimensional";
  else if (mode == INTERP_BMPR    ) splineTypeName="Bitmap (red component)";
  else if (mode == INTERP_BMPG    ) splineTypeName="Bitmap (green component)";
  else if (mode == INTERP_BMPB    ) splineTypeName="Bitmap (blue component)";
  else                            { sprintf(c->errStat.errBuff,"interpolate command requested to perform unknown type of interpolation."); TBADD2(ERR_INTERNAL,0); return; }

  // Read input data
  if (bmp<0)
   {
    int status=0;
    ppldata_fromCmd(c, &data, pl, in, 0, filenameOut, dataSpool, PARSE_TABLE_interpolate2d_, 0, NcolRequired, 0, min, minSet, max, maxSet, unit, 0, &status, c->errStat.errBuff, &errCount, iterDepth);

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
       if (!ppl_unitsDimEqual(&unit[j],data->firstEntries+j)) { sprintf(c->errStat.errBuff, "The minimum and maximum limits specified in range %ld in the fit command have conflicting physical dimensions with the data returned from the data file. The limits have units of <%s>, whilst the data have units of <%s>.", j+1, ppl_printUnit(c,unit+j,NULL,NULL,0,1,0), ppl_printUnit(c,data->firstEntries+j,NULL,NULL,1,1,0)); TBADD2(ERR_NUMERICAL,0); return; }
      }

    // Transfer data from multiple data tables into single vectors
    if ((NcolRequired<3)&&(mode!=INTERP_STEPWISE)) xdata = (double *)ppl_memAlloc_incontext(NcolRequired * (data->Nrows+2) * sizeof(double), contextLocalVec);
    else                                           xdata = (double *)malloc                (NcolRequired * (data->Nrows+2) * sizeof(double));
    if (xdata==NULL) { sprintf(c->errStat.errBuff, "Out of memory."); TBADD2(ERR_MEMORY,0); return; }
    ydata   = xdata +   data->Nrows;
    zdata   = xdata + 2*data->Nrows;
    MinList = xdata + NcolRequired* data->Nrows;
    MaxList = xdata + NcolRequired*(data->Nrows+1);

    // Copy data table into an array of x values, and an array of y values
    blk = data->first; i=0;
    while (blk != NULL)
     {
      k=i;                       for (j=0; j<blk->blockPosition; j++) xdata[i++] = blk->data_real[0 + NcolRequired*j];
      i=k;                       for (j=0; j<blk->blockPosition; j++) ydata[i++] = blk->data_real[1 + NcolRequired*j];
      if (NcolRequired>2) { i=k; for (j=0; j<blk->blockPosition; j++) zdata[i++] = blk->data_real[2 + NcolRequired*j]; }
      blk=blk->next;
     }

    // Check that we have at least minimum number of points to interpolate
    if (i<NxRequired) { sprintf(c->errStat.errBuff,"%s interpolation is only possible on data sets with at least %d member%s.",splineTypeName,NxRequired,(NxRequired>1)?"s":""); TBADD2(ERR_NUMERICAL,0); if (NcolRequired>=3) free(xdata); return; }

    // Fill out minimum and maximum of data
    for (jms=0; jms<NcolRequired; jms++)
     {
      MinList[jms] = MaxList[jms] = 0.0;
      for (ims=0; ims<i; ims++)
       {
        if ((ims==0)||(MinList[jms]>xdata[jms*data->Nrows+ims])) MinList[jms] = xdata[jms*data->Nrows+ims];
        if ((ims==0)||(MaxList[jms]<xdata[jms*data->Nrows+ims])) MaxList[jms] = xdata[jms*data->Nrows+ims];
       }
      if (MaxList[jms]<=MinList[jms]) { double t=MinList[jms]; MinList[jms]=t*0.999;  MaxList[jms]=t*1.001; }
     }

    firstEntries[0] = data->firstEntries[0];
    firstEntries[1] = data->firstEntries[1];
    if (NcolRequired>2) firstEntries[2] = data->firstEntries[2];
   }
  else // Read data from bmp file
   {
    int           i,j;
    long          p;
    unsigned char buff[10];
    int           pos = PARSE_interpolate2d_expression_list;
    FILE         *infile;

    for (i=0; i<2; i++)
     if ( minSet[i] || maxSet[i] )
      { sprintf(c->errStat.errBuff, "Ranges cannot be applied when interpolating bitmap data."); TBADD2(ERR_NUMERICAL,0); return; }

    // Fetch filename of bitmap image
    if ( (stk[pos].objType!=PPLOBJ_NUM) ||
         (pos=(int)round(stk[pos].real) , stk[pos].objType==PPLOBJ_NUM)
       ) { sprintf(c->errStat.errBuff, "A single filename must be supplied when interpolating a bitmap image."); TBADD2(ERR_SYNTAX,0); return; }
    {
     const int stkLevelOld = c->stackPtr;
     int       i;
     pplExpr  *expr  = (pplExpr *)stk[pos+PARSE_interpolate2d_expression_expression_list].auxil;
     pplObj   *first = ppl_expEval(c, expr, &i, 0, iterDepth);
     if ((c->errStat.status) || (first->objType!=PPLOBJ_STR))
      {
       STACK_CLEAN;
       sprintf(c->errStat.errBuff, "A single filename must be supplied when interpolating a bitmap image."); TBADD2(ERR_SYNTAX,0);
       return;
      }
     strcpy(filenameOut, (char *)first->auxil);
     STACK_CLEAN;
    }

    // glob filename
    {
     wordexp_t wordExp;
     glob_t    globData;
     char      escaped[FNAME_LENGTH];
     { int j,k; for (j=k=0; ((filenameOut[j]!='\0')&&(k<FNAME_LENGTH-1)); ) { if (filenameOut[j]==' ') escaped[k++]='\\'; escaped[k++]=filenameOut[j++]; } escaped[k++]='\0'; }
     if ((wordexp(escaped, &wordExp, 0) != 0) || (wordExp.we_wordc <= 0)) { sprintf(c->errStat.errBuff, "Could not open file '%s'.", escaped); TBADD2(ERR_FILE,0); return; }
     if ((glob(wordExp.we_wordv[0], 0, NULL, &globData) != 0) || (globData.gl_pathc <= 0)) { sprintf(c->errStat.errBuff, "Could not open file '%s'.", escaped); TBADD2(ERR_FILE,0); wordfree(&wordExp); return; }
     wordfree(&wordExp);
     snprintf(filenameOut, FNAME_LENGTH, "%s", globData.gl_pathv[0]);
     filenameOut[FNAME_LENGTH-1]='\0';
     globfree(&globData);
    }

    infile = fopen(filenameOut, "r");
    if (infile==NULL) { sprintf(c->errStat.errBuff, "Could not open input file '%s'", filenameOut); TBADD2(ERR_FILE,0); return; }

    for (i=0; i<3; i++) // Use magic to determine file type
     {
      j = fgetc(infile);
      if (j==EOF) { sprintf(c->errStat.errBuff, "Could not read any image data from the input file '%s'", filenameOut); TBADD2(ERR_FILE,0); fclose(infile); return; }
      buff[i] = (unsigned char)j;
     }
    if ((buff[0]!='B')&&(buff[1]!='M')) { sprintf(c->errStat.errBuff, "File '%s' does not appear to be a valid bitmap image.", filenameOut); TBADD2(ERR_FILE,0); fclose(infile); return; }

    ppl_bmp_bmpread(&c->errcontext, infile, &bmpdata);
    if (bmpdata.data == NULL) { sprintf(c->errStat.errBuff, "Reading of bitmap image data failed"); TBADD2(ERR_GENERIC,0); return; }

    pplObjNum(&firstEntries[0],0,0,0); firstEntries[0].refCount=1;
    pplObjNum(&firstEntries[1],0,0,0); firstEntries[1].refCount=1;
    pplObjNum(&firstEntries[2],0,0,0); firstEntries[2].refCount=1;
    sizeX = bmpdata.width;
    sizeY = bmpdata.height;

    bmpchars = (unsigned char *)malloc(sizeX*sizeY);
    if (bmpchars==NULL) { sprintf(c->errStat.errBuff, "Out of memory whilst reading data from input file."); TBADD2(ERR_MEMORY,0); return; }

    if (bmpdata.colour == BMP_COLOUR_RGB) // RGB image
     {
      for (p=0; p<sizeX*sizeY; p++) bmpchars[p] = bmpdata.data[3*p+bmp];
     }
    else // Paletted image
     {
      for (p=0; p<sizeX*sizeY; p++) bmpchars[p] = bmpdata.palette[3*bmpdata.data[p]+bmp];
     }
   }

  // Free original data table which is no longer needed
  ppl_memAlloc_AscendOutOfContext(contextDataTab);

  // 1D case: sort data and make a GSL interpolation object
  if (NcolRequired==2)
   {
    // Check that the firstEntries above have the same units as any supplied ranges
    if      (minSet[0] || maxSet[0])
     {
      if (!ppl_unitsDimEqual(unit+0,firstEntries+0)) { sprintf(c->errStat.errBuff, "The minimum and maximum limits specified in the interpolate command for the x axis have conflicting physical dimensions with the data returned from the data file. The limits have units of <%s>, whilst the data have units of <%s>.", ppl_printUnit(c,unit+0,NULL,NULL,0,1,0), ppl_printUnit(c,firstEntries+0,NULL,NULL,1,1,0)); TBADD2(ERR_UNIT,0); return; }
     }
    if      (minSet[1] || maxSet[1])
     {
      if (!ppl_unitsDimEqual(unit+1,firstEntries+1)) { sprintf(c->errStat.errBuff, "The minimum and maximum limits specified in the interpolate command for the y axis have conflicting physical dimensions with the data returned from the data file. The limits have units of <%s>, whilst the data have units of <%s>.", ppl_printUnit(c,unit+1,NULL,NULL,0,1,0), ppl_printUnit(c,firstEntries+1,NULL,NULL,1,1,0)); TBADD2(ERR_UNIT,0); return; }
     }

    // Free original data table which is no longer needed
    ppl_memAlloc_AscendOutOfContext(contextDataTab);

    // Sort data vectors according to x values
    {
     long int *sortArray = (long int *)ppl_memAlloc(2 * i * sizeof(long int));
     long int *sortArrayI= sortArray + i;
     long int j;
     if (sortArray==NULL) { sprintf(c->errStat.errBuff, "Out of memory."); TBADD2(ERR_MEMORY,0); return; }
     for (j=0; j<i; j++) sortArray[j]=j;

     compareZeroPoint = xdata;
     qsort((void *)sortArray, i, sizeof(long int), interpolateSorter);
     for (j=0; j<i; j++) sortArrayI[sortArray[j]]=j;

     for (j=0; j<i; j++)
      {
       double tmp,tmp2; long int tmp3;
       tmp3            = sortArray[  j ];
       if (tmp3==j) continue;
       tmp             = xdata    [  j ];
       xdata    [  j ] = xdata    [tmp3];
       xdata    [tmp3] = tmp;
       tmp2            = ydata    [  j ];
       ydata    [  j ] = ydata    [tmp3];
       ydata    [tmp3] = tmp2;
       sortArray[sortArrayI[j]] = tmp3;
       sortArrayI[tmp3]         = sortArrayI[j];
      }
    }

    // Filter out repeat values of x
    for (j=k=0; j<i; j++)
     {
      if ( (!gsl_finite(xdata[j])) || (!gsl_finite(ydata[j])) ) continue; // Ignore non-finite datapoints
      if ( (minSet[0]) && (xdata[j]<min[0]) )                   continue; // Ignore out-of-range datapoints
      if ( (maxSet[0]) && (xdata[j]>max[0]) )                   continue; // Ignore out-of-range datapoints
      if ( (minSet[1]) && (ydata[j]<min[1]) )                   continue; // Ignore out-of-range datapoints
      if ( (maxSet[1]) && (ydata[j]>max[1]) )                   continue; // Ignore out-of-range datapoints

      if ((j>0) && (xdata[j]==xdata[j-1])) { COUNTEDERR1; v=firstEntries[0]; v.real=xdata[j]; sprintf(c->errcontext.tempErrStr,"Repeat values for interpolation have been supplied at x=%s.",ppl_unitsNumericDisplay(c, &v, 0, 0, 0)); ppl_warning(&c->errcontext, ERR_GENERIC, NULL); COUNTEDERR2; continue; }

      if (mode == INTERP_LOGLIN)
       {
        if ((xdata[j]<=0.0) || (ydata[j]<=0.0)) { COUNTEDERR1; v=firstEntries[0]; v.real=xdata[j]; sprintf(c->errcontext.tempErrStr,"Negative or zero values are not allowed in power-law interpolation; negative values supplied at x=%s will be ignored.",ppl_unitsNumericDisplay(c, &v, 0, 0, 0)); ppl_warning(&c->errcontext, ERR_NUMERICAL, NULL); COUNTEDERR2; continue; }
        xdata[k]=log(xdata[j]);
        ydata[k]=log(ydata[j]);
        if ( (!gsl_finite(xdata[k])) || (!gsl_finite(ydata[k])) ) continue;
       } else {
        xdata[k]=xdata[j];
        ydata[k]=ydata[j];
       }
      k++;
     }

    // Check that we have at least minimum number of points to interpolate
    if (k<NxRequired) { sprintf(c->errStat.errBuff,"%s interpolation is only possible on data sets with members at at least %d distinct values of x.",splineTypeName,NxRequired); TBADD2(ERR_NUMERICAL,0); if (NcolRequired>=3) free(xdata); return; }

    // Create GSL interpolation object
    if (splineType!=NULL)
     {
      int status=0;
      splineObj = gsl_spline_alloc(splineType, k);
      accel     = gsl_interp_accel_alloc();
      if (splineObj==NULL) { sprintf(c->errStat.errBuff,"Failed to make interpolation object."); TBADD2(ERR_INTERNAL,0); return; }
      if (accel    ==NULL) { sprintf(c->errStat.errBuff,"Failed to make GSL interpolation accelerator."); TBADD2(ERR_MEMORY,0); return; }
      status    = gsl_spline_init(splineObj, xdata, ydata, k);
      if (status) { sprintf(c->errStat.errBuff,"Error whilst creating interpolation object: %s", gsl_strerror(status)); TBADD2(ERR_INTERNAL,0); return; }
     }
    else
     {
      splineObj = (gsl_spline *)xdata;
      sizeX     = k;
      sizeY     = i;
     }
   }
  else if (bmp<0) // 2D interpolation
   {
    splineObj = (gsl_spline *)xdata;
    sizeX     = i;
   }
  else // bitmap interpolation
   {
    splineObj = (gsl_spline *)bmpchars;
   }

  // Generate a function descriptor for this spline
  desc              = (splineDescriptor *)malloc(sizeof(splineDescriptor));
  if (desc == NULL) { sprintf(c->errStat.errBuff, "Out of memory whilst adding interpolation object to function dictionary."); TBADD2(ERR_MEMORY,0); return; }
  desc->unitX       = firstEntries[0];
  desc->unitY       = firstEntries[1];
  desc->unitZ       = firstEntries[2];
  desc->sizeX       = sizeX;
  desc->sizeY       = sizeY;
  desc->logInterp   = (mode == INTERP_LOGLIN);
  desc->splineObj   = splineObj;
  desc->accelerator = accel;
  desc->filename    = (char *)malloc(strlen(filenameOut)+1);
  if (desc->filename == NULL) { sprintf(c->errStat.errBuff, "Out of memory whilst adding interpolation object to function dictionary."); TBADD2(ERR_MEMORY,0); return; }
  strcpy(desc->filename, filenameOut);
  desc->splineType  = splineTypeName;

  // Make a new function descriptor
  funcPtr = (pplFunc *)malloc(sizeof(pplFunc));
  if (funcPtr == NULL) { sprintf(c->errStat.errBuff, "Out of memory whilst adding interpolation object to function dictionary."); TBADD2(ERR_MEMORY,0); return; }
  if      (NcolRequired==2) funcPtr->functionType = PPL_FUNC_SPLINE;
  else if (bmp<0          ) funcPtr->functionType = PPL_FUNC_INTERP2D;
  else                      funcPtr->functionType = PPL_FUNC_BMPDATA;
  funcPtr->refCount        = 1;
  funcPtr->minArgs         = NcolRequired-1;
  funcPtr->maxArgs         = NcolRequired-1;
  funcPtr->notNan          = 1;
  funcPtr->realOnly        = 1;
  funcPtr->numOnly         = 1;
  funcPtr->dimlessOnly     = 0;
  funcPtr->needSelfThis    = 0;
  funcPtr->functionPtr     = (void *)desc;
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

   ppl_contextVarHierLookup(c, pl->srcLineN, pl->srcId, pl->srcFname, pl->linetxt, stk, in->stkCharPos, &obj, PARSE_interpolate2d_varnames, PARSE_interpolate2d_varname_varnames);
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

void ppl_spline_evaluate(ppl_context *c, char *FuncName, splineDescriptor *desc, pplObj *in, pplObj *out, int *status, char *errout)
 {
  double dblin, dblout;

  if (!ppl_unitsDimEqual(in, &desc->unitX))
   {
    if (c->set->term_current.ExplicitErrors == SW_ONOFF_ON) { *status=1; sprintf(errout, "The %s(x) function expects an argument with dimensions of <%s>, but has instead received an argument with dimensions of <%s>.", FuncName, ppl_printUnit(c, &desc->unitX, NULL, NULL, 0, 1, 0), ppl_printUnit(c, in, NULL, NULL, 1, 1, 0)); }
    else { pplObjNum(out,0,0,0); out->real = GSL_NAN; out->imag = 0; }
    return;
   }
  if (in->flagComplex)
   {
    if (c->set->term_current.ExplicitErrors == SW_ONOFF_ON) { *status=1; sprintf(errout, "The %s(x) function expects a real argument, but the supplied argument has an imaginary component.", FuncName); }
    else { pplObjNum(out,0,0,0); out->real = GSL_NAN; out->imag = 0; }
    return;
   }

  // If loglinear interpolation, log input value
  dblin = in->real;
  if (desc->logInterp) dblin = log(dblin);

  if (desc->splineType[1]!='t') // Not stepwise interpolation
   {
    *status = gsl_spline_eval_e(desc->splineObj, dblin, desc->accelerator, &dblout);
   }
  else // Stepwise interpolation
   {
    long          i, pos, ss, len=desc->sizeX, xystep=desc->sizeY, Nsteps = (long)ceil(log(desc->sizeX)/log(2));
    double       *data = (double *)desc->splineObj;
    for (pos=i=0; i<Nsteps; i++)
     {
      ss = 1<<(Nsteps-1-i);
      if (pos+ss>=len) continue;
      if (data[pos+ss]<=dblin) pos+=ss;
     }
    if      (data[pos]>dblin)                               dblout=data[pos  +xystep]; // Off left end
    else if (pos==len-1)                                    dblout=data[pos  +xystep]; // Off right end
    else if (fabs(dblin-data[pos])<fabs(dblin-data[pos+1])) dblout=data[pos  +xystep];
    else                                                    dblout=data[pos+1+xystep];
   }

  // Catch interpolation failure
  if (*status!=0)
   {
    if (c->set->term_current.ExplicitErrors == SW_ONOFF_ON) { *status=1; sprintf(errout, "Error whilst evaluating the %s(x) function: %s", FuncName, gsl_strerror(*status)); }
    else { *status=0; pplObjNum(out,0,0,0); out->real = GSL_NAN; out->imag = 0; }
    return;
   }

  // If loglinear interpolation, unlog output value
  if (desc->logInterp) dblout = exp(dblout);

  if (!gsl_finite(dblout))
   {
    if (c->set->term_current.ExplicitErrors == SW_ONOFF_ON) { *status=1; sprintf(errout, "Error whilst evaluating the %s(x) function: result was not a finite number.", FuncName); }
    else { pplObjNum(out,0,0,0); out->real = GSL_NAN; out->imag = 0; }
    return;
   }

  // Return output
  pplObjNum(out,0,0,0);
  out->real = dblout;
  ppl_unitsDimCpy(out, &desc->unitY);
  return;
 }


void ppl_interp2d_evaluate(ppl_context *c, const char *FuncName, splineDescriptor *desc, const pplObj *in1, const pplObj *in2, const unsigned char bmp, pplObj *out, int *status, char *errout)
 {
  double dblin1, dblin2, dblout;

  if (!ppl_unitsDimEqual(in1, &desc->unitX))
   {
    if (c->set->term_current.ExplicitErrors == SW_ONOFF_ON) { *status=1; sprintf(errout, "The %s(x,y) function expects its first argument to have dimensions of <%s>, but has instead received an argument with dimensions of <%s>.", FuncName, ppl_printUnit(c, &desc->unitX, NULL, NULL, 0, 1, 0), ppl_printUnit(c, in1, NULL, NULL, 1, 1, 0)); }
    else { pplObjNum(out,0,0,0); out->real = GSL_NAN; out->imag = 0; }
    return;
   }
  if (in1->flagComplex)
   {
    if (c->set->term_current.ExplicitErrors == SW_ONOFF_ON) { *status=1; sprintf(errout, "The %s(x,y) function expects real arguments, but first supplied argument has an imaginary component.", FuncName); }
    else { pplObjNum(out,0,0,0); out->real = GSL_NAN; out->imag = 0; }
    return;
   }

  if (!ppl_unitsDimEqual(in2, &desc->unitY))
   {
    if (c->set->term_current.ExplicitErrors == SW_ONOFF_ON) { *status=1; sprintf(errout, "The %s(x,y) function expects its second argument to have dimensions of <%s>, but has instead received an argument with dimensions of <%s>.", FuncName, ppl_printUnit(c, &desc->unitY, NULL, NULL, 0, 1, 0), ppl_printUnit(c, in2, NULL, NULL, 1, 1, 0)); }
    else { pplObjNum(out,0,0,0); out->real = GSL_NAN; out->imag = 0; }
    return;
   }
  if (in2->flagComplex)
   {
    if (c->set->term_current.ExplicitErrors == SW_ONOFF_ON) { *status=1; sprintf(errout, "The %s(x,y) function expects real arguments, but second supplied argument has an imaginary component.", FuncName); }
    else { pplObjNum(out,0,0,0); out->real = GSL_NAN; out->imag = 0; }
    return;
   }

  dblin1 = in1->real;
  dblin2 = in2->real;

  if (!bmp)
   {
    ppl_interp2d_eval(c, &dblout, &c->set->graph_current, (double *)desc->splineObj, desc->sizeX, 2, 3, dblin1, dblin2);
   } else {
    int x = floor(dblin1);
    int y = floor(dblin2);
    if ((x<0) || (x>=desc->sizeX) || (y<0) || (y>=desc->sizeY)) dblout = GSL_NAN;
    else                                                        dblout = ((double)(((unsigned char *)desc->splineObj)[x+y*desc->sizeX]))/255.0;
   }

  if (!gsl_finite(dblout))
   {
    if (c->set->term_current.ExplicitErrors == SW_ONOFF_ON) { *status=1; sprintf(errout, "Error whilst evaluating the %s(x,y) function: result was not a finite number.", FuncName); }
    else { pplObjNum(out,0,0,0); out->real = GSL_NAN; out->imag = 0; }
    return;
   }

  // Return output
  pplObjNum(out,0,0,0);
  out->real = dblout;
  ppl_unitsDimCpy(out, &desc->unitZ);
  return;
 }

