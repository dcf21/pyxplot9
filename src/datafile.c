// datafile.c
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

#define _DATAFILE_C 1

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <unistd.h>
#include <signal.h>

#include <gsl/gsl_math.h>

#include "coreUtils/errorReport.h"
#include "coreUtils/memAlloc.h"

#include "expressions/dollarOp.h"
#include "expressions/expEval.h"
#include "expressions/traceback_fns.h"

#include "parser/parser.h"

#include "stringTools/asciidouble.h"
#include "stringTools/strConstants.h"

#include "settings/settings.h"
#include "settings/settingTypes.h"

#include "userspace/context.h"
#include "userspace/garbageCollector.h"
#include "userspace/pplObj_fns.h"
#include "userspace/pplObjCmp.h"
#include "userspace/unitsDisp.h"
#include "userspace/unitsArithmetic.h"

#include "children.h"
#include "datafile.h"
#include "pplConstants.h"

dataBlock *ppldata_NewDataBlock(const int Ncolumns_real, const int Ncolumns_obj, const int memContext, const int length)
 {
  dataBlock *output;
  int blockLength = 1 + DATAFILE_DATABLOCK_BYTES / ((sizeof(double)+sizeof(long int))*Ncolumns_real + (sizeof(pplObj)+sizeof(long int))*Ncolumns_obj); // automatic length

  if (length>0) blockLength=length; // overriden when manually specified length > 0

  output = (dataBlock *)ppl_memAlloc_incontext(sizeof(dataBlock), memContext);
  if (output==NULL) return NULL;
  output->data_real      = (double *)       ppl_memAlloc_incontext(blockLength * Ncolumns_real * sizeof(double       ), memContext);
  output->data_obj       = (pplObj *)       ppl_memAlloc_incontext(blockLength * Ncolumns_obj  * sizeof(pplObj       ), memContext);
  output->text           = (char **)        ppl_memAlloc_incontext(blockLength *                 sizeof(char *       ), memContext);
  output->fileLine_real  = (long int *)     ppl_memAlloc_incontext(blockLength * Ncolumns_real * sizeof(long int     ), memContext);
  output->fileLine_obj   = (long int *)     ppl_memAlloc_incontext(blockLength * Ncolumns_obj  * sizeof(long int     ), memContext);
  output->split          = (unsigned char *)ppl_memAlloc_incontext(blockLength *                 sizeof(unsigned char), memContext);
  output->blockLength    = blockLength;
  output->blockPosition  = 0;
  output->next           = NULL;
  output->prev           = NULL;
  if ((output->data_real==NULL)||(output->text==NULL)||(output->fileLine_real==NULL)||(output->fileLine_obj==NULL)||(output->split==NULL)) return NULL;
  return output;
 }

dataTable *ppldata_NewDataTable(const int Ncolumns_real, const int Ncolumns_obj, const int memContext, const int length)
 {
  dataTable *output;
  int i;

  output = (dataTable *)ppl_memAlloc_incontext(sizeof(dataTable), memContext);
  if (output==NULL) return NULL;
  output->Ncolumns_real = Ncolumns_real;
  output->Ncolumns_obj  = Ncolumns_obj;
  output->Nrows         = 0;
  output->memContext    = memContext;
  output->firstEntries  = (pplObj *)ppl_memAlloc_incontext(Ncolumns_real*sizeof(pplObj), memContext);
  if (output->firstEntries==NULL) return NULL;
  for (i=0;i<Ncolumns_real;i++) pplObjNum(output->firstEntries + i,0,0,0);
  output->first         = ppldata_NewDataBlock(Ncolumns_real, Ncolumns_obj, memContext, length);
  output->current       = output->first;
  if (output->first==NULL) return NULL;
  return output;
 }

rawDataBlock *ppldata_NewRawDataBlock(const int memContext)
 {
  rawDataBlock *output;
  int blockLength = 1 + DATAFILE_DATABLOCK_BYTES / (sizeof(char *)+sizeof(long int));

  output = (rawDataBlock *)ppl_memAlloc_incontext(sizeof(rawDataBlock), memContext);
  if (output==NULL) return NULL;
  output->text          = (char **)   ppl_memAlloc_incontext(blockLength * sizeof(char *       ), memContext);
  output->fileLine      = (long int *)ppl_memAlloc_incontext(blockLength * sizeof(long int     ), memContext);
  output->blockLength   = blockLength;
  output->blockPosition = 0;
  output->next          = NULL;
  output->prev          = NULL;
  if ((output->text==NULL)||(output->fileLine==NULL)) return NULL;
  return output;
 }

rawDataTable *ppldata_NewRawDataTable(const int memContext)
 {
  rawDataTable *output;

  output = (rawDataTable *)ppl_memAlloc_incontext(sizeof(rawDataTable), memContext);
  if (output==NULL) return NULL;
  output->Nrows      = 0;
  output->memContext = memContext;
  output->first      = ppldata_NewRawDataBlock(memContext);
  output->current    = output->first;
  if (output->first==NULL) return NULL;
  return output;
 }

int ppldata_DataTable_AddRow(dataTable *i)
 {
  if (i==NULL) return 1;
  if (i->current==NULL) return 1;
  i->Nrows++;
  if (i->current->blockPosition < (i->current->blockLength-1)) { i->current->blockPosition++; return 0; }
  i->current->next          = ppldata_NewDataBlock(i->Ncolumns_real, i->Ncolumns_obj, i->memContext, -1);
  if (i->current==NULL) return 1;
  i->current->blockPosition = i->current->blockLength;
  i->current->next->prev    = i->current;
  i->current                = i->current->next;
  return 0;
 }

int ppldata_RawDataTable_AddRow(rawDataTable *i)
 {
  if (i==NULL) return 1;
  if (i->current==NULL) return 1;
  i->Nrows++;
  if (i->current->blockPosition < (i->current->blockLength-1)) { i->current->blockPosition++; return 0; }
  i->current->next          = ppldata_NewRawDataBlock(i->memContext);
  if (i->current==NULL) return 1;
  i->current->blockPosition = i->current->blockLength;
  i->current->next->prev    = i->current;
  i->current                = i->current->next;
  if (i->current==NULL) return 1;
  return 0;
 }

void ppldata_DataTable_List(ppl_context *c, dataTable *i)
 {
  dataBlock *blk;
  pplObj v;
  int j,k,Ncolumns;

  if (i==NULL) { printf("<NULL data table>\n"); return; }
  printf("Table size: %d x %ld\n", i->Ncolumns_real, i->Nrows);
  printf("Memory context: %d\n", i->memContext);
  blk      = i->first;
  Ncolumns = i->Ncolumns_real;
  while (blk != NULL)
   {
    for (j=0; j<blk->blockPosition; j++)
     {
      if (blk->split[j]) printf("\n\n");
      for (k=0; k<Ncolumns; k++)
       {
        v             = i->firstEntries[k];
        v.real        = blk->data_real[j*Ncolumns + k];
        v.imag        = 0.0;
        v.flagComplex = 0;
        printf("%15s [line %6ld]  ",ppl_unitsNumericDisplay(c, &v, 0, 0, 0), blk->fileLine_real[j*Ncolumns + k] );
       }
      if (blk->text[j] != NULL) printf("Label: <%s>",blk->text[j]);
      printf("\n");
     }
    blk=blk->next;
   }
 }

// on error, returns NULL with error message in errout.
FILE *ppldata_LaunchCoProcess(ppl_context *c, char *filename, char *errout)
 {
  FILE         *infile;
  dictIterator *dictIter;
  char         *filter, *filterArgs, **argList;
  int           i,j,k;
  sigset_t      sigs;

  sigemptyset(&sigs);
  sigaddset(&sigs,SIGCHLD);

  // Implement the magic filename '' to refer to the last used filename
  if (filename[0]=='\0') filename = c->dollarStat.lastFilename;
  else                   { strncpy(c->dollarStat.lastFilename, filename, FNAME_LENGTH); c->dollarStat.lastFilename[FNAME_LENGTH-1]='\0'; }

  // Check whether we have a specified coprocessor to work on this filetype
  dictIter = ppl_dictIterateInit(c->set->filters);
  while (dictIter != NULL)
   {
    if (ppl_strWildcardTest(filename, dictIter->key))
     {
      filter = (char *)((pplObj *)dictIter->data)->auxil;
      if (DEBUG) { sprintf(c->errcontext.tempErrStr, "Using input filter '%s'.", filter); ppl_log(&c->errcontext, NULL); }
      filterArgs = (char  *)ppl_memAlloc(strlen(filter)+1);
      argList    = (char **)ppl_memAlloc((strlen(filter)/2+1)*sizeof(char *));
      if ((filterArgs==NULL)||(argList==NULL)) { sprintf(errout,"Out of memory."); if (DEBUG) ppl_log(&c->errcontext, errout); return NULL; };
      strcpy(filterArgs, filter);
      for (i=j=k=0; filterArgs[i]!='\0'; i++)
       {
        if      ((k==0) && (filterArgs[i]> ' ')) { k=1; argList[j++] = filterArgs+i; }
        else if ((k==1) && (filterArgs[i]<=' ')) { k=0; filterArgs[i] = '\0'; }
       }
      argList[j++] = filename;
      argList[j++] = NULL;
      pplcsp_forkInputFilter(c, argList, &i); // Fork process for input filter, and runned piped output through the standard IO library using fdopen()
      sigprocmask(SIG_UNBLOCK, &sigs, NULL);
      if ((infile = fdopen(i, "r")) == NULL) { sprintf(errout,"Could not open connection to input filter '%s'.",argList[0]); if (DEBUG) ppl_log(&c->errcontext, errout); return NULL; };
      return infile;
     }
    ppl_dictIterate(&dictIter, NULL);
   }

  // If not, then we just open the file and return a file-handle to it
  if ((infile = fopen(filename, "r")) == NULL) { sprintf(errout,"Could not open input file '%s'.",filename); if (DEBUG) ppl_log(&c->errcontext, errout); return NULL; };
  return infile;
 }

void ppldata_UsingConvert(ppl_context *c, pplExpr *input, char **columns_str, pplObj *columns_val, int Ncols, char *filename, long file_linenumber, long *file_linenumbers, long linenumber_count, long block_count, long index_number, int usingRowCol, char **colHeads, int NcolHeads, pplObj *colUnits, int NcolUnits, int *status, char *errtext, int iterDepth)
 {
  double    dbl;
  int       i=0;
  const int l=strlen(input->ascii);
  int       namedColumn=0;

  *status=0;
  ppl_dollarOp_config(c, columns_str, columns_val, Ncols, filename, file_linenumber, file_linenumbers, linenumber_count, block_count, index_number, usingRowCol, input->ascii, colHeads, NcolHeads, colUnits, NcolUnits);

  if ((!ppl_validFloat(input->ascii,&i))&&(i==l)) // using 1:2 -- number is column number -- "1" means "the contents of column 1", i.e. "$1".
   {
    dbl = ppl_getFloat(input->ascii,NULL);
    if ((dbl>-4)&&(dbl<MAX_DATACOLS)) namedColumn=1;
   }
  else
   {
    for (i=0;i<NcolHeads;i++)
     if (strcmp(colHeads[i],input->ascii)==0)
      {
       dbl=i+1.01;
       namedColumn=1;
       break;
      }
    } // using ColumnName

  c->dollarStat.warntxt[0]='\0';
  if (namedColumn) // Clean up these cases
   {
    ppl_dollarOp_fetchColByNum(c, input, 0, (int)round(dbl));
   }
  else
   {
    int lOp=0;
    ppl_expEval(c, input, &lOp, 1, iterDepth+1);
   }
  if ((c->errStat.status) || (c->dollarStat.warntxt[0]!='\0'))
   {
    int   errp = c->errStat.errPosExpr;
    char *errt = NULL;
    if (errp<0) errp=0;
    if      (c->dollarStat.warntxt[0]!='\0') errt=c->dollarStat.warntxt;
    else if (c->errStat.errMsgExpr[0]!='\0') errt=c->errStat.errMsgExpr;
    else if (c->errStat.errMsgCmd [0]!='\0') errt=c->errStat.errMsgCmd;
    else                                      errt="Fail occured.";
    sprintf(errtext, "%s:%ld: Could not evaluate expression <%s>. The error, encountered at character position %d, was: '%s'", filename, file_linenumber, input->ascii, errp, errt);
    ppl_tbClear(c);
    *status=1;
   }

  ppl_dollarOp_deconfig(c); // Disable dollar operator
  return;
 }

#define COUNTERR_BEGIN if (*errCount> 0) { (*errCount)--;
#define COUNTERR_END   if (*errCount==0) { sprintf(c->errcontext.tempErrStr, "%s:%ld: Too many errors: no more errors will be shown.",filename,file_linenumber); \
                       ppl_warning(&c->errcontext,ERR_STACKED,NULL); } }

#define TBADD(et)      ppl_tbAdd(c,ex->srcLineN,ex->srcId,ex->srcFname,0,et,0,ex->ascii,"")

#define STACK_POP \
   { \
    c->stackPtr--; \
    ppl_garbageObject(&c->stack[c->stackPtr]); \
    if (c->stack[c->stackPtr].refCount != 0) { strcpy(errtext,"Stack forward reference detected."); ppl_warning(&c->errcontext,ERR_STACKED,errtext); FAIL; } \
   }

#define STACK_CLEAN    while (c->stackPtr>stkLevelOld) { STACK_POP; }

#define FAIL { COUNTERR_BEGIN; ppl_warning(&c->errcontext, ERR_STACKED, errtext); COUNTERR_END; *status = 1; *discontinuity = 1; if ((*labOut)!=NULL) free(*labOut); *labOut=NULL; if (DEBUG) ppl_log(&c->errcontext, errtext); return; }

void ppldata_ApplyUsingList(ppl_context *c, dataTable *out, pplExpr **usingExprs, pplExpr *labelExpr, pplExpr *selectExpr, int continuity, int *discontinuity, char **columns_str, pplObj *columns_val, int Ncols, char *filename, long file_linenumber, long *file_linenumbers, long linenumber_count, long block_count, long index_number, int usingRowCol, char **colHeads, int NcolHeads, pplObj *colUnits, int NcolUnits, int *status, char *errtext, int *errCount, int iterDepth)
 {
  ppl_context *context     = c;
  const int    stkLevelOld = c->stackPtr;
  const int    nUsing      = out->Ncolumns_real + out->Ncolumns_obj;
  char       **labOut      = &out->current->text[out->current->blockPosition];
  const int    memContext  = out->memContext;
  pplObj      *stkObj      = &c->stack[c->stackPtr-1];
  int          i;

  *status = 0;
  *labOut = NULL;

  // select criterion
  if (selectExpr != NULL)
   {
    int      fail = 0;
    pplExpr *ex   = selectExpr;
    ppldata_UsingConvert(c, ex, columns_str, columns_val, Ncols, filename, file_linenumber, file_linenumbers, linenumber_count, block_count, index_number, usingRowCol, colHeads, NcolHeads, colUnits, NcolUnits, &fail, errtext, iterDepth);
    if (fail) { FAIL; }
    CAST_TO_BOOL(stkObj);
    if (stkObj->real == 0) { STACK_CLEAN; *discontinuity=(continuity==DATAFILE_DISCONTINUOUS); *status=0; return; }
    STACK_CLEAN;
   }

  // label
  if (labelExpr != NULL)
   {
    int      fail   = 0;
    char    *labIn  = NULL;
    pplExpr *ex     = labelExpr;
    ppldata_UsingConvert(c, ex, columns_str, columns_val, Ncols, filename, file_linenumber, file_linenumbers, linenumber_count, block_count, index_number, usingRowCol, colHeads, NcolHeads, colUnits, NcolUnits, &fail, errtext, iterDepth);
    if (fail) { FAIL; }
    if (stkObj->objType!=PPLOBJ_STR)
     {
      sprintf(errtext, "%s:%ld: Label expression '%s' did not evaluate to a string, but to type <%s>.", filename, file_linenumber, ex->ascii, pplObjTypeNames[stkObj->objType]);
      FAIL;
     }
    labIn   = (char *)stkObj->auxil;
    *labOut = (char *)ppl_memAlloc_incontext(strlen(labIn)+1, memContext);
    if (*labOut!=NULL) strcpy(*labOut, labIn);
    STACK_CLEAN;
   }

  // using expressions
  for (i=0; i<nUsing; i++)
   {
    int        fail   = 0;
    const int  outObj = i>=out->Ncolumns_real;
    const int  idx    = outObj ? (i-out->Ncolumns_real) : i;
    pplExpr   *ex     = usingExprs[i];
    ppldata_UsingConvert(c, ex, columns_str, columns_val, Ncols, filename, file_linenumber, file_linenumbers, linenumber_count, block_count, index_number, usingRowCol, colHeads, NcolHeads, colUnits, NcolUnits, &fail, errtext, iterDepth);
    if (fail) { FAIL; }
    if (!outObj)
     {
      if ((stkObj->objType!=PPLOBJ_NUM)&&(stkObj->objType!=PPLOBJ_BOOL)&&(stkObj->objType!=PPLOBJ_DATE))
       {
        sprintf(errtext, "%s:%ld: Data item calculated from expression '%s' was not a number, but had type <%s>.", filename, file_linenumber, ex->ascii, pplObjTypeNames[stkObj->objType]);
        FAIL;
       }
      if ((stkObj->objType==PPLOBJ_NUM) && (stkObj->flagComplex))
       {
        sprintf(errtext, "%s:%ld: Data item calculated from expression '%s' was a complex number.", filename, file_linenumber, ex->ascii);
        FAIL;
       }
      if (out->Nrows==0)
       {
        out->firstEntries[i] = *stkObj;
        out->firstEntries[i].refCount = 1; // no memory leak here, as all allowed types have no auxilary data
        out->firstEntries[i].amMalloced = 0;
       }
      else
       {
        if (out->firstEntries[i].objType != stkObj->objType)
         {
          sprintf(errtext, "%s:%ld: Data item calculated from expression '%s' has inconsistent types, including <%s> and <%s>.", filename, file_linenumber, ex->ascii, pplObjTypeNames[out->firstEntries[i].objType], pplObjTypeNames[stkObj->objType]);
          FAIL;
         }
        if ( (out->firstEntries[i].objType==PPLOBJ_NUM) && (!ppl_unitsDimEqual(&out->firstEntries[i], stkObj)) )
         {
          sprintf(errtext, "%s:%ld: Data item calculated from expression '%s' has inconsistent physical units, including <%s> and <%s>.", filename, file_linenumber, ex->ascii, ppl_printUnit(c, &out->firstEntries[i], NULL, NULL, 0, 1, 0), ppl_printUnit(c, stkObj, NULL, NULL, 1, 1, 0));
          FAIL;
         }
       }
      out->current->data_real[idx + out->current->blockPosition * out->Ncolumns_real] = stkObj->real;
     }
    else
     {
      pplObj *oo = &out->current->data_obj[idx + out->current->blockPosition * out->Ncolumns_obj];
      oo->refCount = 1;
      pplObjCpy(oo,stkObj,0,0,1);
     }
   }

  out->current->split[out->current->blockPosition] = *discontinuity;
  i = ppldata_DataTable_AddRow(out);
  if (i) { sprintf(errtext, "%s:%ld: Out of memory storing data table.", filename, file_linenumber); *errCount=-1; FAIL; }
  *discontinuity = 0;
  return;
 }

void ppldata_RotateRawData(ppl_context *c, rawDataTable **in, dataTable *out, pplExpr **usingExprs, pplExpr *labelExpr, pplExpr *selectExpr, int continuity, char *filename, long block_count, long index_number, char **colHeads, int NcolHeads, pplObj *colUnits, int NcolUnits, int *status, char *errtext, int *errCount, int iterDepth)
 {
  int           i;
  long          linenumber_count = 0;
  int           discontinuity = 1;
  int           contextRaw = (*in )->memContext;
  unsigned char hadspace[MAX_DATACOLS];
  unsigned char hadcomma[MAX_DATACOLS];

  for (i=0; i<MAX_DATACOLS; i++) hadspace[i] = 1;
  for (i=0; i<MAX_DATACOLS; i++) hadcomma[i] = 0;

  while (1)
   {
    int           gotData=0, Ncols=0;
    rawDataBlock *blk = (*in)->first;
    char         *rowData[MAX_DATACOLS];
    long          file_linenumber[MAX_DATACOLS];

    while ((blk!=NULL)&&(Ncols<MAX_DATACOLS))
     {
      char *s = blk->text[i];
      file_linenumber[0]=-1;
      for (i=0; ((i<blk->blockPosition)&&(Ncols<MAX_DATACOLS)); i++)
       {
        if (s==NULL)
         {
          rowData[Ncols]=" ";
          file_linenumber[Ncols]=-1;
          Ncols++;
         }
        else
         {
          int caught=0, p=0;
          for ( ; ((!caught) && (s[p]!='\0')) ; p++)
           {
            if      (s[p]<=' ') { hadspace[Ncols] = 1; }
            else if (s[p]==',') { p++; while ((s[p]!='\0')&&(s[p]<=' ')) { p++; } caught=1; hadspace[Ncols] = hadcomma[Ncols] = 1; }
            else                { if (hadspace[Ncols] && !hadcomma[Ncols]) { caught=1; } hadspace[Ncols] = hadcomma[Ncols] = 0; }
           }

          if (!caught)
           {
            rowData[Ncols]=" ";
            file_linenumber[Ncols]=-1;
            Ncols++;
           }
          else
           {
            rowData        [Ncols] = blk->text    [i];
            file_linenumber[Ncols] = blk->fileLine[i];
            Ncols++;
            gotData = 1;
           }
         }
       }
      blk = blk->next;
     }
    if (!gotData) break;

    ppldata_ApplyUsingList(c, out, usingExprs, labelExpr, selectExpr, continuity, &discontinuity, rowData, NULL, Ncols, filename, file_linenumber[0], file_linenumber, linenumber_count, block_count, index_number, DATAFILE_ROW, colHeads, NcolHeads, colUnits, NcolUnits, status, errtext, errCount, iterDepth);
    linenumber_count++;
   }

  ppl_memAlloc_Free(contextRaw);
  *in = ppldata_NewRawDataTable(contextRaw);
  return;
 }

// Routines for sorting data tables

typedef struct ppldata_sorter
 {
  double       *data_real;
  pplObj       *data_obj;
  char         *text;
  long         *fileLine_real;
  long         *fileLine_obj;
  unsigned char split;
 } ppldata_sorter;

static int ppldata_sort_sortCol;

static int ppldata_sort_cmp(const void *xv, const void *yv)
 {
  ppldata_sorter *x = (ppldata_sorter *)xv;
  ppldata_sorter *y = (ppldata_sorter *)yv;
  return pplObjCmpQuiet( (void *)&x->data_obj[ppldata_sort_sortCol] ,
                         (void *)&y->data_obj[ppldata_sort_sortCol]
                       );
 }

dataTable *ppldata_sort(ppl_context *c, dataTable *in, int sortCol, int ignoreContinuity)
 {
  int               i,io,Nc,Nd,Nr;
  long              ji,jo;
  dataBlock        *blk;
  ppldata_sorter *sorter;
  dataTable        *output;

  if (in==NULL) return NULL;

  Nc = in->Ncolumns_obj;
  Nd = in->Ncolumns_real;
  Nr = in->Nrows;
  sorter = (ppldata_sorter *)malloc(Nr * sizeof(ppldata_sorter));
  if (sorter==NULL) return NULL;

  // Transfer data from DataTable into a DataTable_sorter array which is in a format qsort() can sort
  blk = in->first;
  jo=0;
  while (blk != NULL)
   {
    long ji;
    for (ji=0; ji<blk->blockPosition; ji++, jo++)
     {
      sorter[jo].data_real     = &(blk->data_real    [Nd*ji]);
      sorter[jo].data_obj      = &(blk->data_obj     [Nc*ji]);
      sorter[jo].text          =   blk->text         [   ji];
      sorter[jo].fileLine_real = &(blk->fileLine_real[Nc*ji]);
      sorter[jo].fileLine_obj  = &(blk->fileLine_obj [Nc*ji]);
      if (ignoreContinuity) sorter[jo].split = 0;
      else                  sorter[jo].split = blk->split[ji];
     }
    blk=blk->next;
   }

  // Sort the DataTable_sorter array
  ppldata_sort_sortCol = sortCol - in->Ncolumns_real;
  for (i=0,io=0; io<=jo; io++)
   if ((io==jo) || sorter[io].split)
    {
     qsort((void *)(sorter+i), io-i, sizeof(ppldata_sorter), ppldata_sort_cmp);
     sorter[i].split = (i!=0);
     for (ji=i+1; ji<io; ji++) sorter[ji].split = 0;
     i=io;
    }

  // Copy sorted data into a new dataTable, removing the sort column
  output = ppldata_NewDataTable(Nc, Nd-1, in->memContext, Nr);
  if (output!=NULL)
   {
    const int sc = sortCol - in->Ncolumns_real;
    int i, io; long jo;
                            for (i=0,io=0; i<Nc; i++)            output->firstEntries        [io++]             = in->firstEntries[i];
    for (jo=0; jo<Nr; jo++) for (i=0,io=0; i<Nc; i++)            output->first->data_real    [(io++)+ Nc   *jo] = sorter[jo].data_real[i];
    for (jo=0; jo<Nr; jo++) for (i=0,io=0; i<Nd; i++) if (i!=sc) output->first->data_obj     [(io++)+(Nd-1)*jo] = sorter[jo].data_obj [i];
    for (jo=0; jo<Nr; jo++)                                      output->first->text         [              jo] = sorter[jo].text;
    for (jo=0; jo<Nr; jo++) for (i=0,io=0; i<Nc; i++)            output->first->fileLine_real[(io++)+ Nc   *jo] = sorter[jo].fileLine_real[i];
    for (jo=0; jo<Nr; jo++) for (i=0,io=0; i<Nd; i++) if (i!=sc) output->first->fileLine_obj [(io++)+(Nd-1)*jo] = sorter[jo].fileLine_obj [i];
    for (jo=0; jo<Nr; jo++)                                      output->first->split        [              jo] = sorter[jo].split;
    output->Nrows = Nr;
    output->first->blockPosition = Nr;
   }
  free(sorter);
  return output;
 }

