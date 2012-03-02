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
#include "userspace/pplObj.h"

// rawDataTabls structure, used for storing raw text from datafiles prior to rotation when "with rows" is used

typedef struct rawDataBlock {
  char               **text;      // Array of BlockLength x [string data from datafile]
  long int            *FileLine;  // For each string above... store the line number in the data file that it came from
  long int             BlockLength;
  long int             BlockPosition; // Where have we filled up to?
  struct rawDataBlock *next;
  struct rawDataBlock *prev;
 } rawDataBlock;

typedef struct rawDataTable {
  long int             Nrows;
  int                  MemoryContext;
  struct rawDataBlock *first;
  struct rawDataBlock *current;
 } rawDataTable;


// DataTable structure, used for returning tables of VALUEs from ppl_datafile.c

typedef union UnionDblStr {
  double d;
  char  *s;
 } UnionDblStr;

typedef struct dataBlock {
  UnionDblStr      *data_real; // Array of Ncolumns x array of length BlockLength
  char            **text;      // Array of BlockLength x string labels for datapoints
  long int         *FileLine;  // For each double above... store the line number in the data file that it came from
  unsigned char    *split;     // Array of length BlockLength; TRUE if we should break data before this next datapoint
  long int          BlockLength;
  long int          BlockPosition; // Where have we filled up to?
  struct dataBlock *next;
  struct dataBlock *prev;
 } dataBlock;

typedef struct dataTable {
  int               Ncolumns;
  long int          Nrows;
  int               MemoryContext;
  pplObj           *FirstEntries; // Array of size Ncolumns; store units for data in each column here
  struct dataBlock *first;
  struct dataBlock *current;
 } dataTable;

// Functions in ppl_datafile.c

dataTable *ppldata_NewDataTable(const int Ncolumns, const int MemoryContext, const int Length);

void __inline__ ppldata_UsingConvert_FetchColumnByNumber(double ColumnNo, pplObj *output, const int NumericOut, const unsigned char MallocOut, int *status, char *errtext);
void __inline__ ppldata_UsingConvert_FetchColumnByName(char *ColumnName, pplObj *output, const int NumericOut, const unsigned char MallocOut, int *status, char *errtext);

void ppldata_read(dataTable **output, int *status, char *errout, char *filename, int index, int UsingRowCol, list *UsingList, unsigned char AutoUsingList, list *EveryList, char *LabelStr, int Ncolumns, char *SelectCriterion, int continuity, char *SortBy, int SortByContinuity, unsigned char persistent, int *ErrCounter);

void ppldata_FromFunctions(double *OrdinateRaster, unsigned char FlagParametric, int RasterLen, pplObj *RasterUnits, double *OrdinateYRaster, int RasterYLen, pplObj *RasterYUnits, dataTable **output, int *status, char *errout, char **fnlist, int fnlist_len, list *UsingList, unsigned char AutoUsingList, char *LabelStr, int Ncolumns, char *SelectCriterion, int continuity, char *SortBy, int SortByContinuity, int *ErrCounter);

dataTable *ppldata_tabSort(dataTable *in, int SortColumn, int IgnoreContinuity);

#endif

