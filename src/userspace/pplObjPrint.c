// pplObjPrint.c
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

#include <stdlib.h>
#include <math.h>
#include <string.h>

#include "coreUtils/errorReport.h"
#include "coreUtils/list.h"
#include "coreUtils/dict.h"

#include "settings/settingTypes.h"
#include "stringTools/asciidouble.h"

#include "userspace/context.h"
#include "userspace/pplObj.h"
#include "userspace/pplObjFunc.h"
#include "userspace/pplObjPrint.h"
#include "userspace/unitsDisp.h"

void pplObjPrint(ppl_context *c, pplObj *o, char *oname, char *out, int outlen, int internal, int modIterDepth)
 {
  int t;
  int NSigFigs = c->set->term_current.SignificantFigures;
  int typeable = c->set->term_current.NumDisplay;

  if (o==NULL) { strcpy(out,"<null>"); return; }
  t = o->objType;
  switch (t)
   {
    case PPLOBJ_NUM:
     strncpy(out, ppl_unitsNumericDisplay(c, o, 0, internal?SW_DISPLAY_T:typeable, NSigFigs), outlen);
     break;
    case PPLOBJ_STR:
     if (!internal) strncpy(out, o->auxil, outlen);
     else           ppl_strEscapify(o->auxil, out);
     break;
    case PPLOBJ_BOOL:
     if (o->real==0) strncpy(out, "false", outlen);
     else            strncpy(out, "true" , outlen);
     break;
    case PPLOBJ_DATE:
     strncpy(out, "<date>", outlen);
     break;
    case PPLOBJ_COL:
     {
      int i=0,n;
      int ct=floor(o->exponent[0]+0.01);
      if      (ct == SW_COLSPACE_CMYK) { strncpy(out+i, "cmyk(", outlen-i); n=4; }
      else if (ct == SW_COLSPACE_RGB)  { strncpy(out+i, "rgb(" , outlen-i); n=3; }
      else if (ct == SW_COLSPACE_HSB)  { strncpy(out+i, "hsb(" , outlen-i); n=3; }
      else                             { ppl_warning(&c->errcontext, ERR_INTERNAL, "Unknown colour space in pplObjPrint."); strncpy(out+i, "ERR(", outlen-i); n=0; }
      i+=strlen(out+i);
      if (n>=1) {               strcpy(out+i,ppl_numericDisplay(o->exponent[ 8], c->numdispBuff[0], NSigFigs, (typeable==SW_DISPLAY_L))); i+=strlen(out+i); }
      if (n>=2) { out[i++]=','; strcpy(out+i,ppl_numericDisplay(o->exponent[ 9], c->numdispBuff[0], NSigFigs, (typeable==SW_DISPLAY_L))); i+=strlen(out+i); }
      if (n>=3) { out[i++]=','; strcpy(out+i,ppl_numericDisplay(o->exponent[10], c->numdispBuff[0], NSigFigs, (typeable==SW_DISPLAY_L))); i+=strlen(out+i); }
      if (n>=4) { out[i++]=','; strcpy(out+i,ppl_numericDisplay(o->exponent[11], c->numdispBuff[0], NSigFigs, (typeable==SW_DISPLAY_L))); i+=strlen(out+i); }
      strncpy(out+i, ")", outlen-i);
      break;
     }
    case PPLOBJ_DICT:
     {
      int           i=0, first=1, prevMod=0;
      dictIterator *iter = ppl_dictIterateInit((dict *)o->auxil);
      char         *key;
      pplObj       *item;
      out[i++]='{';
      while ((item = (pplObj *)ppl_dictIterate(&iter, &key))!=NULL)
       {
        if (!first) { out[i++]=','; if (prevMod) { out[i++]='\n'; } out[i++]=' '; }
        ppl_strEscapify(key, out+i);
        i+=strlen(out+i);
        sprintf(out+i,":");
        i+=strlen(out+i);
        pplObjPrint(c, item, NULL, out+i, outlen-i, 1, 0);
        i+=strlen(out+i);
        if (i>outlen-100) { strcpy(out+i,", ..."); i+=strlen(out+i); break; }
        first=0;
        prevMod = item->objType = PPLOBJ_MOD;
       }
      strcpy(out+i,"}");
      break;
     }
    case PPLOBJ_USER:
    case PPLOBJ_MOD:
     {
      int           i=0, k;
      const int     expand = (modIterDepth==0);
      dictIterator *iter = ppl_dictIterateInit((dict *)o->auxil);
      char         *key;
      pplObj       *item;
      sprintf(out+i, (t==PPLOBJ_MOD)?"module {":"user-defined type {");
      i+=strlen(out+i);
      if (!expand) strcpy(out+i," ... }");
      else
       {
        out[i++]='\n';
        while ((item = (pplObj *)ppl_dictIterate(&iter, &key))!=NULL)
         {
          int j=i;
          if ((item->objType==PPLOBJ_GLOB) || (item->objType==PPLOBJ_ZOM)) continue; // Hide globals and zombies
          for (k=0; k<=modIterDepth; k++) { out[i++]=' '; out[i++]=' '; }
          strcpy(out+i, key);
          i+=strlen(out+i);
          while (i<j+16) out[i++]=' ';
          sprintf(out+i,": ");
          i+=strlen(out+i);
          pplObjPrint(c, item, key, out+i, outlen-i, 1, modIterDepth+1);
          i+=strlen(out+i);
          out[i++]='\n';
          if (i>outlen-100) { strcpy(out+i,"  ...\n"); i+=strlen(out+i); break; }
         }
        out[i++]=' ';
        for (k=0; k<modIterDepth; k++) { out[i++]=' '; out[i++]=' '; }
        strcpy(out+i,"}");
       }
      break;
     }
    case PPLOBJ_LIST:
     {
      int           i=0, first=1, prevMod=0;
      listIterator *iter = ppl_listIterateInit((list *)o->auxil);
      pplObj       *item;
      out[i++]='[';
      while ((item = (pplObj *)ppl_listIterate(&iter))!=NULL)
       {
        if (!first) { out[i++]=','; if (prevMod) { out[i++]='\n'; } out[i++]=' '; }
        pplObjPrint(c, item, NULL, out+i, outlen-i, 1, 0);
        i+=strlen(out+i);
        if (i>outlen-100) { strcpy(out+i,", ..."); i+=strlen(out+i); break; }
        first=0;
        prevMod = item->objType = PPLOBJ_MOD;
       }
      strcpy(out+i,"]");
      break;
     }
    case PPLOBJ_VEC:
     {
      int         i=0, j;
      gsl_vector *v = ((pplVector *)(o->auxil))->v;
      out[i++]='[';
      for(j=0; j<v->size; j++)
       {
        if (j>0) { out[i++]=','; out[i++]=' '; }
        strcpy(out+i,ppl_numericDisplay(v->data[j], c->numdispBuff[0], NSigFigs, (typeable==SW_DISPLAY_L)));
        i+=strlen(out+i);
        if (i>outlen-100) { strcpy(out+i,", ..."); i+=strlen(out+i); break; }
       }
      strcpy(out+i,"]");
      break;
     }
    case PPLOBJ_MAT:
     {
      int         i=0, j, k;
      gsl_matrix *m = ((pplMatrix *)(o->auxil))->m;
      if (typeable==SW_DISPLAY_T)
       {
        out[i++]='[';
        for(j=0; j<m->size1; j++)
         {
          if (j>0) { out[i++]=','; out[i++]=' '; }
          out[i++]='[';
          for(k=0; k<m->size2; k++)
           {
            if (k>0) { out[i++]=','; out[i++]=' '; }
            strcpy(out+i,ppl_numericDisplay(m->data[j*m->tda+k], c->numdispBuff[0], NSigFigs, (typeable==SW_DISPLAY_L)));
            i+=strlen(out+i);
            if (i>outlen-100) { strcpy(out+i,", ..."); i+=strlen(out+i); break; }
           }
          strcpy(out+i,"]"); i+=strlen(out+i);
          if (i>outlen-100) { strcpy(out+i,", ..."); i+=strlen(out+i); break; }
         }
        strcpy(out+i,"]"); i+=strlen(out+i);
       }
      else
       {
        for(j=0; j<m->size1; j++)
         {
          if (j>0) { out[i++]=','; out[i++]=' '; }
          strcpy(out+i,"( "); i+=strlen(out+i);
          for(k=0; k<m->size2; k++)
           {
            if (k>0) { out[i++]=','; out[i++]=' '; }
            strcpy(out+i,ppl_numericDisplay(m->data[j*m->tda+k], c->numdispBuff[0], NSigFigs, (typeable==SW_DISPLAY_L)));
            i+=strlen(out+i);
            if (i>outlen-100) { strcpy(out+i,", ..."); i+=strlen(out+i); break; }
           }
          strcpy(out+i,")\n"); i+=strlen(out+i);
          if (i>outlen-100) { strcpy(out+i,"...\n"); i+=strlen(out+i); break; }
         }
       }
      break;
     }
    case PPLOBJ_FILE:
     {
      strcpy(out, "<file handle>");
      break;
     }
    case PPLOBJ_FUNC:
     {
      const char    *fnname = (oname==NULL)?"function":oname;
      const pplFunc *f      = (pplFunc *)o->auxil;
      int            first  = 1, i=0;
      int            indent = 18*(modIterDepth>0) + 2*modIterDepth;
      for ( ; f!=NULL ; f=f->next , first=0)
       {
        const int t = f->functionType;
        if (!first) { int j; out[i++]='\n'; for (j=0; j<indent; j++) out[i++]=' '; }
        switch (t)
         {
          case PPL_FUNC_SYSTEM:
            strcpy(out+i, f->description);
            i+=strlen(out+i);
            break;
          case PPL_FUNC_USERDEF:
           {
            int j=-1, k, l, m;
            for (k=0; k<f->maxArgs; k++) if (f->minActive[k] || f->maxActive[k]) j=k;
            sprintf(out+i,"%s(",fnname); i+=strlen(out+i);
            for (l=0, m=0; l<f->maxArgs; l++, m++)
             {
              for ( ; f->argList[m]!='\0'; m++) out[i++] = f->argList[m];
              out[i++] = ',';
             }
            if (f->maxArgs>0) i--; // Remove final comma from list of arguments
            out[i++] = ')';
            for (k=0; k<=j; k++)
             {
              out[i++] = '[';
              if (f->minActive[k]) { sprintf(out+i,"%s", ppl_unitsNumericDisplay(c, f->min+k, 0, 0, 0)); i+=strlen(out+i); }
              out[i++] = ':';
              if (f->maxActive[k]) { sprintf(out+i,"%s", ppl_unitsNumericDisplay(c, f->max+k, 0, 0, 0)); i+=strlen(out+i); }
              out[i++] = ']';
             }
            sprintf(out+i,"=%s",(char *)f->description); i+=strlen(out+i);
            break;
           }
          case PPL_FUNC_SPLINE:
          case PPL_FUNC_INTERP2D:
          case PPL_FUNC_BMPDATA:
           {
            break;
           }
          case PPL_FUNC_HISTOGRAM:
           {
            sprintf(out+i,"%s(x)= [histogram of data from the file '%s']\n", fnname, ((HistogramDescriptor *)f->functionPtr)->filename );
            i+=strlen(out+i);
            break;
           }
          case PPL_FUNC_FFT:
           {
            sprintf(out+i,"%s(x)= [%d-dimensional fft]\n", fnname, ((FFTDescriptor *)f->functionPtr)->Ndims );
            i+=strlen(out+i);
            break;
           }
          case PPL_FUNC_SUBROUTINE:
           {
            int l,m;
            SubroutineDescriptor *SDiter = (SubroutineDescriptor *)f->functionPtr;
            sprintf(out+i,"%s(", fnname); i+=strlen(out+i);
            for (l=0, m=0; l<SDiter->nArgs; l++, m++)
             {
              for ( ; SDiter->argList[m]!='\0'; m++) out[i++] = SDiter->argList[m];
              out[i++] = ',';
             }
            if (SDiter->nArgs>0) i--; // Remove final comma from list of arguments
            sprintf(out+i,") = [subroutine]\n");
            i+=strlen(out+i);
            break;
           }
          default:
            strcpy(out, "<unknown function type>");
            break;
         }
       }
      break;
     }
    case PPLOBJ_TYPE:
     {
      int i=0;
      strcpy(out, "<data type"); i+=strlen(out+i);
      if (((pplType *)(o->auxil))->id>=0)
       {
        strcpy(out+i, ": "); i+=strlen(out+i);
        strcpy(out+i, pplObjTypeNames[((pplType *)(o->auxil))->id]); i+=strlen(out+i);
       }
      strcpy(out+i, ">");
      break;
     }
    case PPLOBJ_NULL:
     {
      strcpy(out, "null");
      break;
     }
    case PPLOBJ_EXC:
     {
      strcpy(out, "<exception>");
      break;
     }
    case PPLOBJ_GLOB:
     {
      strcpy(out, "<internal: global marker>");
      break;
     }
    case PPLOBJ_ZOM:
     {
      strcpy(out, "<internal: zombie>");
      break;
     }
    default:
     strcpy(out, "<unknown object type>");
     break;
   }
  out[outlen-1]='\0';
  return;
 }

