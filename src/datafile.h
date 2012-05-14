// datafile.h
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

#ifndef _PPL_DATAFILE_H
#define _PPL_DATAFILE_H 1

#define DATAFILE_CONTINUOUS     24001
#define DATAFILE_DISCONTINUOUS  24002

#define DATAFILE_ROW 24101
#define DATAFILE_COL 24102

#define USING_ITEMS_MAX 32

#define MAX_DATACOLS 16384

#define DATAFILE_NERRS 10

// The approximate number of bytes we seek to put in each data block

#define DATAFILE_DATABLOCK_BYTES 524288

#include "coreUtils/list.h"
#include "parser/parser.h"
#include "userspace/context.h"
#include "userspace/pplObj.h"

// rawDataTabls structure, used for storing raw text from datafiles prior to rotation when "with rows" is used

typedef struct rawDataBlock {
  char               **text;      // Array of BlockLength x [string data from datafile]
  long int            *fileLine;  // For each string above... store the line number in the data file that it came from
  long int             blockLength;
  long int             blockPosition; // Where have we filled up to?
  struct rawDataBlock *next;
  struct rawDataBlock *prev;
 } rawDataBlock;

typedef struct rawDataTable {
  long int             Nrows;
  int                  memContext;
  struct rawDataBlock *first;
  struct rawDataBlock *current;
 } rawDataTable;


// DataTable structure, used for returning tables of VALUEs from ppl_datafile.c

typedef struct dataBlock {
  double           *data_real;     // Array of Ncolumns x array of length BlockLength
  pplObj           *data_obj;
  char            **text;          // Array of BlockLength x string labels for datapoints
  long int         *fileLine_real; // For each double above... store the line number in the data file that it came from
  long int         *fileLine_obj;
  unsigned char    *split;         // Array of length BlockLength; TRUE if we should break data before this next datapoint
  long int          blockLength;
  long int          blockPosition; // Where have we filled up to?
  struct dataBlock *next;
  struct dataBlock *prev;
 } dataBlock;

typedef struct dataTable {
  int               Ncolumns_real;
  int               Ncolumns_obj;
  long int          Nrows;
  int               memContext;
  pplObj           *firstEntries; // Array of size Ncolumns; store units for data in each column here
  struct dataBlock *first;
  struct dataBlock *current;
 } dataTable;

// Functions in ppl_datafile.c

dataBlock    *ppldata_NewDataBlock       (const int Ncolumns_real, const int Ncolumns_obj, const int memContext, const int length);
dataTable    *ppldata_NewDataTable       (const int Ncolumns_real, const int Ncolumns_obj, const int memContext, const int length);
rawDataBlock *ppldata_NewRawDataBlock    (const int memContext);
rawDataTable *ppldata_NewRawDataTable    (const int memContext);
int           ppldata_DataTable_AddRow   (dataTable *i);
int           ppldata_RawDataTable_AddRow(rawDataTable *i);
void          ppldata_DataTable_List     (ppl_context *c, dataTable *i);
FILE         *ppldata_LaunchCoProcess    (ppl_context *c, char *filename, int wildcardMatchNumber, char *filenameOut, char *errout);
void          ppldata_UsingConvert       (ppl_context *c, pplExpr *input, char **columns_str, pplObj *columns_val, int Ncols, char *filename, long file_linenumber, long *file_linenumbers, long linenumber_count, long block_count, long index_number, int usingRowCol, char **colHeads, int NcolHeads, pplObj *colUnits, int NcolUnits, int *status, char *errtext, int iterDepth);
void          ppldata_ApplyUsingList     (ppl_context *c, dataTable *out, pplExpr **usingExprs, pplExpr *labelExpr, pplExpr *selectExpr, int continuity, int *discontinuity, char **columns_str, pplObj *columns_val, int Ncols, char *filename, long file_linenumber, long *file_linenumbers, long linenumber_count, long block_count, long index_number, int usingRowCol, char **colHeads, int NcolHeads, pplObj *colUnits, int NcolUnits, int *status, char *errtext, int *errCount, int iterDepth);
void          ppldata_RotateRawData      (ppl_context *c, rawDataTable **in, dataTable *out, pplExpr **usingExprs, pplExpr *labelExpr, pplExpr *selectExpr, int continuity, char *filename, long block_count, long index_number, char **colHeads, int NcolHeads, pplObj *colUnits, int NcolUnits, int *status, char *errtext, int *errCount, int iterDepth);
dataTable    *ppldata_sort(ppl_context *c, dataTable *in, int sortCol, int ignoreContinuity);

void          ppldata_fromFile           (ppl_context *c, dataTable **out, char *filename, int wildcardMatchNumber, char *filenameOut, parserLine *dataSpool, int indexNo, pplExpr **usingExprs, int autoUsingExprs, int Ncols, int NusingObjs, pplExpr *labelExpr, pplExpr *selectExpr, pplExpr *sortBy, int usingRowCol, long *everyList, int continuity, int persistent, int *status, char *errtext, int *errCount, int iterDepth);
void          ppldata_fromFuncs          (ppl_context *c, dataTable **out, pplExpr **fnlist, int fnlist_len, double *rasterX, int rasterXlen, int parametric, pplObj *unitX, double *rasterY, int rasterYlen, pplObj *unitY, pplExpr **usingExprs, int autoUsingExprs, int Ncols, int NusingObjs, pplExpr *labelExpr, pplExpr *selectExpr, pplExpr *sortBy, int continuity, int *status, char *errtext, int *errCount, int iterDepth);
void          ppldata_fromVectors        (ppl_context *c, dataTable **out, pplObj *objList, int objListLen, pplExpr **usingExprs, int autoUsingExprs, int Ncols, int NusingObjs, pplExpr *labelExpr, pplExpr *selectExpr, pplExpr *sortBy, int continuity, int *status, char *errtext, int *errCount, int iterDepth);
void          ppldata_fromCmd            (ppl_context *c, dataTable **out, parserLine *pl, parserOutput *in, int wildcardMatchNumber, char *filenameOut, const int *ptab, const int stkbase, int Ncols, int NusingObjs, double *min, int *minSet, double *max, int *maxSet, pplObj *unitRange, int persistent, int *status, char *errtext, int *errCount, int iterDepth);

#endif

