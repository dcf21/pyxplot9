// pplObj.c
//
// The code in this file is part of PyXPlot
// <http://www.pyxplot.org.uk>
//
// Copyright (C) 2006-2012 Dominic Ford <coders@pyxplot.org.uk>
//               2008-2011 Ross Church
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

#define _PPLOBJ_C 1

#include <stdlib.h>
#include <string.h>

#include "coreUtils/errorReport.h"
#include "coreUtils/memAlloc.h"
#include "coreUtils/list.h"
#include "coreUtils/dict.h"

#include "expressions/pplObj.h"
#include "expressions/pplObjUnits.h"

char *pplObjTypeNames[] = {"number","string","boolean","date","color","dictionary","list","vector","matrix","file handle","function","type","null","exception",NULL};

void *pplObjZero(pplObj *in, unsigned char amMalloced)
 {
  int i;
  in->real = in->imag = 0.0;
  in->dimensionless = 1;
  in->modified = in->FlagComplex = in->TempType = 0;
  in->objType = PPLOBJ_NUM;
  in->auxil = in->objCustomType = NULL;
  in->auxilMalloced = 0;
  in->auxilLen = 0;
  in->amMalloced = amMalloced;
  for (i=0; i<UNITS_MAX_BASEUNITS; i++) in->exponent[i]=0;
  return in;
 }

void *pplObjNullStr(pplObj *in, unsigned char amMalloced)
 {
  int i;
  in->real = in->imag = 0.0;
  in->dimensionless = 1;
  in->modified = in->FlagComplex = in->TempType = 0;
  in->objType = PPLOBJ_STR;
  in->auxil = "";
  in->auxilMalloced = 0;
  in->auxilLen = 0;
  in->objCustomType = NULL;
  in->amMalloced = amMalloced;
  for (i=0; i<UNITS_MAX_BASEUNITS; i++) in->exponent[i]=0;
  return in;
 }

pplObj *pplObjCpy(pplObj *in, unsigned char useMalloc)
 {
  pplObj *out=NULL;

  if (in==NULL) return NULL;

  if (useMalloc) out = (pplObj *)malloc  (sizeof(pplObj));
  else           out = (pplObj *)ppl_memAlloc(sizeof(pplObj));
  if (out==NULL) return out;

  memcpy(out, in, sizeof(pplObj));
  out->amMalloced = useMalloc;

  switch(in->objType)
   {
    case PPLOBJ_STR:
    case PPLOBJ_EXC: // copy string value
      if (useMalloc) out->auxil = (void *)malloc  (in->auxilLen);
      else           out->auxil = (void *)ppl_memAlloc(in->auxilLen);
      if (out->auxil==NULL) { if (useMalloc) free(out); return NULL; }
      memcpy(out->auxil, in->auxil, in->auxilLen);
      out->auxilMalloced = useMalloc;
      break;
    case PPLOBJ_FILE: // copy pointer
      ((pplFile *)(out->auxil))->iNodeCount++; 
      break;
    case PPLOBJ_FUNC: // copy pointer
      ((pplFunc *)(out->auxil))->iNodeCount++;
      break;
    case PPLOBJ_TYPE: // copy pointer
      ((pplType *)(out->auxil))->iNodeCount++;
      break;
    case PPLOBJ_LIST: // copy pointer
      ((list *)(out->auxil))->iNodeCount++;
      break;
    case PPLOBJ_DICT:
    case PPLOBJ_USER: // copy pointer
      ((dict *)(out->auxil))->iNodeCount++;
      break;
   }
  return out;
 }

