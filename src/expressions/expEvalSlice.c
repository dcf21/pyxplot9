// expEvalSlice.c
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

#include <stdlib.h>
#include <string.h>
#include <limits.h>

#include <gsl/gsl_math.h>

#include "expressions/expEvalSlice.h"
#include "userspace/context.h"
#include "userspace/garbageCollector.h"
#include "userspace/pplObj_fns.h"
#include "userspace/unitsDisp.h"

#include "pplConstants.h"

void ppl_sliceItem (ppl_context *context, int getPtr, int *status, int *errType, char *errText)
 {
  const int nArgs=1;
  pplObj  *out  = &context->stack[context->stackPtr-1-nArgs];
  pplObj  *args = context->stack+context->stackPtr-nArgs;
  pplObj   called;
  int      i;
  int      t = out->objType;
  if (context->stackPtr<nArgs+1) { *status=1; *errType=ERR_INTERNAL; sprintf(errText,"Attempt to slice object with few items on the stack."); return; }
  memcpy(&called, out, sizeof(pplObj));
  pplObjNum(out, 0, 0, 0); // Overwrite function object on stack, but don't garbage collect it yet

  if (args->objType!=PPLOBJ_NUM) { *errType=ERR_TYPE; sprintf(errText,"Item numbers when slicing must be numerical values; supplied limit has type <%s>.",pplObjTypeNames[args->objType]); goto fail; }
  if (!args->dimensionless) { *errType=ERR_NUMERIC; sprintf(errText,"Item numbers when slicing must be dimensionless numbers; supplied limit has units of <%s>.", ppl_printUnit(context, args, NULL, NULL, 0, 1, 0) ); goto fail; }
  if (args->flagComplex) { *errType=ERR_NUMERIC; sprintf(errText,"Item numbers when slicing must be real numbers; supplied limit is complex."); goto fail; }
  if ( (!gsl_finite(args->real)) || (args->real<INT_MIN) || (args->real>INT_MAX) ) { *errType=ERR_NUMERIC; sprintf(errText,"Item numbers when slicing must be in the range %d to %d.", INT_MIN, INT_MAX); goto fail; }

  switch (t)
   {
    case PPLOBJ_STR:
     {
      const char *in  = (char *)called.auxil;
      const int   inl = strlen(in);
      char *outstr;
      int p = args[0].real;
      if (p<0) p+=inl;
      if ((p<0)||(p>inl)) { *status=1; *errType=ERR_RANGE; sprintf(errText,"String index out of range."); goto fail; }
      if ((outstr = (char *)malloc(2))==NULL) { *status=1; *errType=ERR_MEMORY; sprintf(errText,"Out of memory."); goto fail; }
      outstr[0] = in[p];
      outstr[1] = '\0';
      pplObjStr(out, 0, 1, outstr);
      break;
     }
    default:
      *status=1; *errType=ERR_TYPE;
      sprintf(errText,"Attempt to slice an object of type <%s>, which cannot be sliced.",pplObjTypeNames[t]);
      goto fail;
   }

cleanup:
  ppl_garbageObject(&called);
  for (i=0; i<nArgs; i++) { context->stackPtr--; ppl_garbageObject(&context->stack[context->stackPtr]); }
  return;
fail:
  *status=1;
  goto cleanup;
 }

void ppl_sliceRange(ppl_context *context, int minset, int min, int maxset, int max, int *status, int *errType, char *errText)
 {
  const int nArgs = minset + maxset;
  pplObj  *out  = &context->stack[context->stackPtr-1-nArgs];
  pplObj   called;
  int      i;
  int      t = out->objType;
  if (context->stackPtr<nArgs+1) { *status=1; *errType=ERR_INTERNAL; sprintf(errText,"Attempt to slice object with few items on the stack."); return; }
  memcpy(&called, out, sizeof(pplObj));
  pplObjNum(out, 0, 0, 0); // Overwrite function object on stack, but don't garbage collect it yet

  switch (t)
   {
    case PPLOBJ_STR:
     {
      const char *in  = (char *)called.auxil;
      const int   inl = strlen(in);
      int   outlen;
      char *outstr;
      if (!minset)    min =0;
      else if (min<0) min+=inl;
      if (!maxset)    max =inl;
      else if (max<0) max+=inl;
      if ((min<0)||(min>inl)||(max<0)||(max>inl)) { *status=1; *errType=ERR_RANGE; sprintf(errText,"String index out of range."); goto fail; }
      outlen = max-min;
      if (outlen<0) outlen=0;
      if ((outstr = (char *)malloc(outlen+1))==NULL) { *status=1; *errType=ERR_MEMORY; sprintf(errText,"Out of memory."); goto fail; }
      strncpy(outstr,in+min,outlen);
      outstr[outlen]='\0';
      pplObjStr(out, 0, 1, outstr);
      break;
     }
    default:
      *status=1; *errType=ERR_TYPE;
      sprintf(errText,"Attempt to slice an object of type <%s>, which cannot be sliced.",pplObjTypeNames[t]);
      goto fail;
   }

cleanup:
  ppl_garbageObject(&called);
  for (i=0; i<nArgs; i++) { context->stackPtr--; ppl_garbageObject(&context->stack[context->stackPtr]); }
  return;
fail:
  *status=1;
  goto cleanup;
 }

