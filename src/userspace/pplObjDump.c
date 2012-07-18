// pplObjPrint.c
//
// The code in this file is part of Pyxplot
// <http://www.pyxplot.org.uk>
//
// Copyright (C) 2006-2012 Dominic Ford <coders@pyxplot.org.uk>
//               2008-2012 Ross Church
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

#include <stdlib.h>
#include <math.h>
#include <string.h>

#include <gsl/gsl_matrix.h>
#include <gsl/gsl_vector.h>
#define GSL_RANGE_CHECK_OFF 1

#include "coreUtils/errorReport.h"
#include "coreUtils/list.h"
#include "coreUtils/dict.h"

#include "settings/settingTypes.h"
#include "stringTools/asciidouble.h"

#include "userspace/calendars.h"
#include "userspace/context.h"
#include "userspace/pplObj.h"
#include "userspace/pplObjFunc.h"
#include "userspace/pplObjPrint.h"
#include "userspace/unitsDisp.h"

void pplObjDump(ppl_context *c, pplObj *o, FILE *output)
 {
  char *buffer = c->errcontext.tempErrStr;
  int t;
  const int NSigFigs = 17;

  if (cancellationFlag) return;
  if (o==NULL) { fprintf(output, "types.null()"); return; }
  t = o->objType;
  switch (t)
   {
    case PPLOBJ_NUM:
     fprintf(output, "%s", ppl_unitsNumericDisplay(c, o, 0, SW_DISPLAY_T, NSigFigs));
     break;
    case PPLOBJ_STR:
     ppl_strEscapify(o->auxil, buffer);
     fprintf(output, "%s", buffer);
     break;
    case PPLOBJ_BOOL:
     if (o->real==0) fprintf(output, "false");
     else            fprintf(output, "true" );
     break;
    case PPLOBJ_DATE:
      {
       int status=0;
       int year, month, day, hour, minute;
       double second;
       ppl_fromUnixTime(c, o->real, &year, &month, &day, &hour, &minute, &second, &status, c->errcontext.tempErrStr);
       fprintf(output,"time.fromCalendar(%d,%d,%d,%d,%d,%.12f)",year,month,day,hour,minute,second);
      }
     break;
    case PPLOBJ_COL:
     {
      int n;
      int ct=floor(o->exponent[0]+0.01);
      if      (ct == SW_COLSPACE_CMYK) { fprintf(output, "cmyk("); n=4; }
      else if (ct == SW_COLSPACE_RGB)  { fprintf(output, "rgb(" ); n=3; }
      else if (ct == SW_COLSPACE_HSB)  { fprintf(output, "hsb(" ); n=3; }
      else                             { ppl_warning(&c->errcontext, ERR_INTERNAL, "Unknown color space in pplObjPrint."); fprintf(output, "ERR("); n=0; }

      if (n>=1) {                       fprintf(output,"%s",ppl_numericDisplay(o->exponent[ 8], c->numdispBuff[0], NSigFigs, 0));  }
      if (n>=2) { fprintf(output, ","); fprintf(output,"%s",ppl_numericDisplay(o->exponent[ 9], c->numdispBuff[0], NSigFigs, 0));  }
      if (n>=3) { fprintf(output, ","); fprintf(output,"%s",ppl_numericDisplay(o->exponent[10], c->numdispBuff[0], NSigFigs, 0));  }
      if (n>=4) { fprintf(output, ","); fprintf(output,"%s",ppl_numericDisplay(o->exponent[11], c->numdispBuff[0], NSigFigs, 0));  }
      fprintf(output, ")");
      break;
     }
    case PPLOBJ_DICT:
     {
      int           first=1;
      dictIterator *iter = ppl_dictIterateInit((dict *)o->auxil);
      char         *key;
      pplObj       *item;

      fprintf(output,"{");
      while ((item = (pplObj *)ppl_dictIterate(&iter, &key))!=NULL)
       {
        if (cancellationFlag) break;
        if ((item->objType==PPLOBJ_GLOB) || (item->objType==PPLOBJ_ZOM)) continue; // Hide globals and zombies
        if (!first) fprintf(output,",");
        ppl_strEscapify(key, buffer);
        fprintf(output,"%s:",buffer);
        pplObjDump(c, item, output);
        first=0;
       }
      fprintf(output,"}");
      break;
     }
    case PPLOBJ_USER:
    case PPLOBJ_MOD:
     {
      int           first=1;
      dictIterator *iter = ppl_dictIterateInit((dict *)o->auxil);
      char         *key;
      pplObj       *item;
      fprintf(output,"module({");

      while ((item = (pplObj *)ppl_dictIterate(&iter, &key))!=NULL)
       {
        if (cancellationFlag) break;
        if ((item->objType==PPLOBJ_GLOB) || (item->objType==PPLOBJ_ZOM)) continue; // Hide globals and zombies
        if (!first) fprintf(output,",");
        ppl_strEscapify(key, buffer);
        fprintf(output,"%s:",buffer);
        pplObjDump(c, item, output);
        first=0;
       }
      fprintf(output,"})");
      break;
     }
    case PPLOBJ_LIST:
     {
      int           first=1;
      listIterator *iter = ppl_listIterateInit((list *)o->auxil);
      pplObj       *item;
      fprintf(output,"[");
      while ((item = (pplObj *)ppl_listIterate(&iter))!=NULL)
       {
        if (cancellationFlag) break;
        if (!first) fprintf(output,",");
        pplObjDump(c, item, output);
        first=0;
       }
      fprintf(output,"]");
      break;
     }
    case PPLOBJ_VEC:
     {
      int         j=0;
      double      real,imag;
      char       *unit;
      o->real=1; o->imag=0; o->flagComplex=0;
      unit = ppl_printUnit(c,o,&real,&imag,1,1,SW_DISPLAY_T);
      if (o->dimensionless) real=1;
      gsl_vector *v = ((pplVector *)(o->auxil))->v;
      fprintf(output,"vector([");

      for(j=0; j<v->size; j++)
       {
        if (cancellationFlag) break;
        if (j>0) fprintf(output,",");
        fprintf(output,"%s",ppl_numericDisplay(gsl_vector_get(v,j)*real, c->numdispBuff[0], NSigFigs, 0));
       }
      fprintf(output,"])");

      if (!o->dimensionless) fprintf(output, "%s", unit);
      break;
     }
    case PPLOBJ_MAT:
     {
      int         j=0, k=0;
      double      real,imag;
      char       *unit;
      o->real=1; o->imag=0; o->flagComplex=0;
      unit = ppl_printUnit(c,o,&real,&imag,1,1,SW_DISPLAY_T);
      if (o->dimensionless) real=1;
      gsl_matrix *m = ((pplMatrix *)(o->auxil))->m;
       {
        fprintf(output,"matrix([");

        for(j=0; j<m->size1; j++)
         {
          if (cancellationFlag) break;
          if (j>0) fprintf(output,",");
          fprintf(output,"[");
          for(k=0; k<m->size2; k++)
           {
            if (cancellationFlag) break;
            if (k>0) fprintf(output,",");
            fprintf(output,"%s",ppl_numericDisplay(gsl_matrix_get(m,j,k)*real, c->numdispBuff[0], NSigFigs, 0));
           }
          fprintf(output,"]");
         }
        fprintf(output,"])");
        if (!o->dimensionless) fprintf(output, "%s", unit);
       }
      break;
     }
    case PPLOBJ_EXC:
     {
      fprintf(output, "types.exception(");
      ppl_strEscapify(o->auxil, buffer);
      fprintf(output, "%s", buffer);
      fprintf(output, ")");
      break;
     }
    case PPLOBJ_FILE:
    case PPLOBJ_FUNC:
    case PPLOBJ_TYPE:
    case PPLOBJ_NULL:
    case PPLOBJ_GLOB:
    case PPLOBJ_ZOM:
    case PPLOBJ_EXP:
    case PPLOBJ_BYT:
    default:
      fprintf(output, "types.null()");
      break;
   }
  return;
 }

