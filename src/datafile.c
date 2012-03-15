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
#include <unistd.h>
#include <signal.h>

#include "coreUtils/errorReport.h"
#include "coreUtils/memAlloc.h"

#include "expressions/traceback_fns.h"

#include "parser/parser.h"

#include "stringTools/asciidouble.h"
#include "stringTools/strConstants.h"

#include "settings/settings.h"
#include "settings/settingTypes.h"

#include "userspace/context.h"
#include "userspace/pplObj_fns.h"

#include "datafile.h"
#include "pplConstants.h"

dataBlock *ppldata_NewDataBlock(const int Ncolumns, const int MemoryContext, const int Length)
 {
  dataBlock *output;
  int BlockLength = 1 + DATAFILE_DATABLOCK_BYTES / (sizeof(double) + sizeof(long int)) / Ncolumns; // automatic length

  if (Length>0) BlockLength=Length; // overriden when manually specified Length > 0

  output = (dataBlock *)ppl_memAlloc_incontext(sizeof(dataBlock), MemoryContext);
  if (output==NULL) return NULL;
  output->data_real  = (UnionDblStr *)  ppl_memAlloc_incontext(BlockLength * Ncolumns * sizeof(UnionDblStr  ), MemoryContext);
  output->text       = (char **)        ppl_memAlloc_incontext(BlockLength *            sizeof(char *       ), MemoryContext);
  output->FileLine   = (long int *)     ppl_memAlloc_incontext(BlockLength * Ncolumns * sizeof(long int     ), MemoryContext);
  output->split      = (unsigned char *)ppl_memAlloc_incontext(BlockLength *            sizeof(unsigned char), MemoryContext);
  output->BlockLength   = BlockLength;
  output->BlockPosition = 0;
  output->next          = NULL;
  output->prev          = NULL;
  if ((output->data_real==NULL)||(output->text==NULL)||(output->FileLine==NULL)||(output->split==NULL)) return NULL;
  return output;
 }

dataTable *ppldata_NewDataTable(const int Ncolumns, const int MemoryContext, const int Length)
 {
  dataTable *output;
  int i;

  output = (dataTable *)ppl_memAlloc_incontext(sizeof(dataTable), MemoryContext);
  if (output==NULL) return NULL;
  output->Ncolumns      = Ncolumns;
  output->Nrows         = 0;
  output->MemoryContext = MemoryContext;
  output->FirstEntries  = (pplObj *)ppl_memAlloc_incontext(Ncolumns*sizeof(pplObj), MemoryContext);
  if (output->FirstEntries==NULL) return NULL;
  for (i=0;i<Ncolumns;i++) pplObjNum(output->FirstEntries + i,0,0,0);
  output->first         = ppldata_NewDataBlock(Ncolumns, MemoryContext, Length);
  output->current       = output->first;
  if (output->first==NULL) return NULL;
  return output;
 }

