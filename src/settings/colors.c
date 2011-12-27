// colors.c
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
#include <stdio.h>
#include <string.h>
#include <limits.h>
#include <unistd.h>

#include <gsl/gsl_math.h>

#include "coreUtils/dict.h"
#include "coreUtils/errorReport.h"
#include "mathsTools/dcfmath.h"
#include "settings/epsColors.h"
#include "settings/settingTypes.h"
#include "stringTools/asciidouble.h"
#include "stringTools/strConstants.h"
#include "userspace/context.h"
#include "userspace/pplObj.h"
#include "userspace/pplObjUnits.h"
#include "userspace/unitsDisp.h"
#include "pplConstants.h"

#define COLMALLOC(X) (tmp = malloc(X)); if (tmp==NULL) { ppl_error(&c->errcontext, ERR_MEMORY, -1, -1,"Out of memory"); *outcol1S=*outcol2S=*outcol3S=*outcol4S=NULL; return 1; }

int ppl_colorFromDict  (ppl_context *c, dict *in, char *prefix, int *outcol, int *outcolspace,
                        double *outcol1, double *outcol2, double *outcol3, double *outcol4,
                        char **outcolS, char **outcol1S, char **outcol2S, char **outcol3S, char **outcol4S,
                        unsigned char *USEcol, unsigned char *USEcol1234, int *errpos, unsigned char malloced)
 {
  char   *tempstr, *tempstre, DictName[32];
  int     cindex, i, j, palette_index;
  void   *tmp;
  pplObj  valobj;
  char   *tempstrR, *tempstrG, *tempstrB, *tempstrH, *tempstrS, *tempstrC, *tempstrM, *tempstrY, *tempstrK;
  pplObj *tempvalR, *tempvalG, *tempvalB, *tempvalH, *tempvalS, *tempvalC, *tempvalM, *tempvalY, *tempvalK;

  sprintf(DictName, "%scolour" , prefix);      tempstr = (char   *)ppl_dictLookup(in,DictName);
  sprintf(DictName, "%scolourR", prefix);      tempvalR= (pplObj *)ppl_dictLookup(in,DictName);
  sprintf(DictName, "%scolourG", prefix);      tempvalG= (pplObj *)ppl_dictLookup(in,DictName);
  sprintf(DictName, "%scolourB", prefix);      tempvalB= (pplObj *)ppl_dictLookup(in,DictName);
  sprintf(DictName, "%scolourH", prefix);      tempvalH= (pplObj *)ppl_dictLookup(in,DictName);
  sprintf(DictName, "%scolourS", prefix);      tempvalS= (pplObj *)ppl_dictLookup(in,DictName);
  sprintf(DictName, "%scolourC", prefix);      tempvalC= (pplObj *)ppl_dictLookup(in,DictName);
  sprintf(DictName, "%scolourM", prefix);      tempvalM= (pplObj *)ppl_dictLookup(in,DictName);
  sprintf(DictName, "%scolourY", prefix);      tempvalY= (pplObj *)ppl_dictLookup(in,DictName);
  sprintf(DictName, "%scolourK", prefix);      tempvalK= (pplObj *)ppl_dictLookup(in,DictName);
  sprintf(DictName, "%scolourRexpr", prefix);  tempstrR= (char   *)ppl_dictLookup(in,DictName);
  sprintf(DictName, "%scolourGexpr", prefix);  tempstrG= (char   *)ppl_dictLookup(in,DictName);
  sprintf(DictName, "%scolourBexpr", prefix);  tempstrB= (char   *)ppl_dictLookup(in,DictName);
  sprintf(DictName, "%scolourHexpr", prefix);  tempstrH= (char   *)ppl_dictLookup(in,DictName);
  sprintf(DictName, "%scolourSexpr", prefix);  tempstrS= (char   *)ppl_dictLookup(in,DictName);
  sprintf(DictName, "%scolourCexpr", prefix);  tempstrC= (char   *)ppl_dictLookup(in,DictName);
  sprintf(DictName, "%scolourMexpr", prefix);  tempstrM= (char   *)ppl_dictLookup(in,DictName);
  sprintf(DictName, "%scolourYexpr", prefix);  tempstrY= (char   *)ppl_dictLookup(in,DictName);
  sprintf(DictName, "%scolourKexpr", prefix);  tempstrK= (char   *)ppl_dictLookup(in,DictName);

  // Decide whether colour expression contains $columns and needs postponing
  tempstre=NULL;
  if ((tempstr!=NULL) && (outcolS!=NULL))
   for (j=0; tempstr[j]!='\0'; j++)
    if (tempstr[j]=='$') { tempstre=tempstr; tempstr=NULL; break; }

  if (tempstr != NULL) // Colour is specified by name or by palette index
   {
    ppl_strStrip(tempstr,tempstr);
    i = ppl_fetchSettingByName(&c->errcontext, tempstr, SW_COLOR_INT, SW_COLOR_STR);
    if (i >= 0)
     {
      cindex = i;
      *outcol1 = *outcol2 = *outcol3 = *outcol4 = 0;
     }
    else
     {
      j = strlen(tempstr);
      *errpos = -1;
      // ppl_EvaluateAlgebra(tempstr, &valobj, 0, &j, 0, errpos, c->errcontext.tempErrStr, 0);
      if (*errpos>=0) { ppl_error(&c->errcontext, ERR_GENERAL, -1, -1, NULL); return 1; }
      if (!valobj.dimensionless) { sprintf(c->errcontext.tempErrStr, "Colour indices should be dimensionless quantities; the specified quantity has units of <%s>.", ppl_printUnit(c, &valobj, NULL, NULL, 1, 1, 0)); ppl_error(&c->errcontext, ERR_GENERAL, -1, -1, NULL); return 1; }
      if ((valobj.real <= INT_MIN+5) || (valobj.real >= INT_MAX-5)) { sprintf(c->errcontext.tempErrStr, "Colour indices should be in the range %d to %d.", INT_MIN, INT_MAX); ppl_error(&c->errcontext, ERR_GENERAL, -1, -1, NULL); return 1; }
      for (j=1; j<PALETTE_LENGTH; j++) if (c->set->palette_current[j]==-1) break;
      palette_index = ((int)valobj.real-1)%j;
      while (palette_index < 0) palette_index+=j;
      cindex       = c->set->palette_current [palette_index];
      *outcolspace = c->set->paletteS_current[palette_index];
      *outcol1     = c->set->palette1_current[palette_index];
      *outcol2     = c->set->palette2_current[palette_index];
      *outcol3     = c->set->palette3_current[palette_index];
      *outcol4     = c->set->palette4_current[palette_index];
     }
    *outcol  = cindex;
    if (malloced && (outcolS !=NULL) && (*outcolS !=NULL)) free(*outcolS );
    if (malloced && (outcol1S!=NULL) && (*outcol1S!=NULL)) free(*outcol1S);
    if (malloced && (outcol2S!=NULL) && (*outcol2S!=NULL)) free(*outcol2S);
    if (malloced && (outcol3S!=NULL) && (*outcol3S!=NULL)) free(*outcol3S);
    if (malloced && (outcol4S!=NULL) && (*outcol4S!=NULL)) free(*outcol4S);
    if (outcolS !=NULL) *outcolS =NULL;
    if (outcol1S!=NULL) *outcol1S=NULL;
    if (outcol2S!=NULL) *outcol2S=NULL;
    if (outcol3S!=NULL) *outcol3S=NULL;
    if (outcol4S!=NULL) *outcol4S=NULL;
    if (USEcol    !=NULL) *USEcol     = (cindex> 0);
    if (USEcol1234!=NULL) *USEcol1234 = (cindex==0);
   }
  else if (tempstre!=NULL)
   {
    if (USEcol    !=NULL) *USEcol     = 0;
    if (USEcol1234!=NULL) *USEcol1234 = 0;
    if (outcolS   ==NULL) { ppl_error(&c->errcontext, ERR_INTERNAL, -1, -1, "Received colour expressions, but have not received strings to put them into."); return 1; }
    if (malloced)
     {
      if (*outcolS !=NULL) free(*outcolS );
      if (*outcol1S!=NULL) free(*outcol1S);
      if (*outcol2S!=NULL) free(*outcol2S);
      if (*outcol3S!=NULL) free(*outcol3S);
      if (*outcol4S!=NULL) free(*outcol4S);
      *outcol1S = *outcol2S = *outcol3S = *outcol4S = NULL;
      *outcolS  = (char *)COLMALLOC(strlen(tempstre)+1); strcpy(*outcolS , tempstre);
     }
    else
     {
      *outcol1S = *outcol2S = *outcol3S = *outcol4S = NULL;
      *outcolS  = tempstre;
     }
   }
  else if ((tempvalR!=NULL) || (tempvalH!=NULL) || (tempvalC!=NULL)) // Colour is specified by RGB/HSB/CMYK components
   {

#define CHECK_REAL_DIMLESS(X) \
   { \
    if (!X->dimensionless) { sprintf(c->errcontext.tempErrStr, "Colour components should be dimensionless quantities; the specified quantity has units of <%s>.", ppl_printUnit(c, X, NULL, NULL, 1, 1, 0)); ppl_error(&c->errcontext, ERR_GENERAL, -1, -1, NULL); return 1; }\
    if (X->imag>1e-6) { sprintf(c->errcontext.tempErrStr, "Colour components should be real numbers; the specified quantity is complex."); ppl_error(&c->errcontext, ERR_GENERAL, -1, -1, NULL); return 1; }\
    if (!gsl_finite(X->real)) { sprintf(c->errcontext.tempErrStr, "Supplied colour components is not a finite number."); ppl_error(&c->errcontext, ERR_GENERAL, -1, -1, NULL); return 1; }\
   }

#define MAP01(X) (X->real <= 0.0) ? 0.0 : ((X->real >= 1.0) ? 1.0 : X->real); /* Make sure that colour component is in the range 0-1 */

    *outcol  = 0;
    *outcol4 = 0.0;
    if (tempvalR!=NULL)
     {
      *outcolspace = SW_COLSPACE_RGB;
      CHECK_REAL_DIMLESS(tempvalR); *outcol1 = MAP01(tempvalR);
      CHECK_REAL_DIMLESS(tempvalG); *outcol2 = MAP01(tempvalG);
      CHECK_REAL_DIMLESS(tempvalB); *outcol3 = MAP01(tempvalB);
     }
    else if (tempvalH!=NULL)
     {
      *outcolspace = SW_COLSPACE_HSB;
      CHECK_REAL_DIMLESS(tempvalH); *outcol1 = MAP01(tempvalH);
      CHECK_REAL_DIMLESS(tempvalS); *outcol2 = MAP01(tempvalS);
      CHECK_REAL_DIMLESS(tempvalB); *outcol3 = MAP01(tempvalB);
     }
    else
     {
      *outcolspace = SW_COLSPACE_CMYK;
      CHECK_REAL_DIMLESS(tempvalC); *outcol1 = MAP01(tempvalC);
      CHECK_REAL_DIMLESS(tempvalM); *outcol2 = MAP01(tempvalM);
      CHECK_REAL_DIMLESS(tempvalY); *outcol3 = MAP01(tempvalY);
      CHECK_REAL_DIMLESS(tempvalK); *outcol4 = MAP01(tempvalK);
     }
    if (USEcol    !=NULL) *USEcol     = 0;
    if (USEcol1234!=NULL) *USEcol1234 = 1;
    if (malloced && (outcolS !=NULL) && (*outcolS !=NULL)) free(*outcolS );
    if (malloced && (outcol1S!=NULL) && (*outcol1S!=NULL)) free(*outcol1S);
    if (malloced && (outcol2S!=NULL) && (*outcol2S!=NULL)) free(*outcol2S);
    if (malloced && (outcol3S!=NULL) && (*outcol3S!=NULL)) free(*outcol3S);
    if (malloced && (outcol4S!=NULL) && (*outcol4S!=NULL)) free(*outcol4S);
    if (outcolS !=NULL) *outcolS =NULL;
    if (outcol1S!=NULL) *outcol1S=NULL;
    if (outcol2S!=NULL) *outcol2S=NULL;
    if (outcol3S!=NULL) *outcol3S=NULL;
    if (outcol4S!=NULL) *outcol4S=NULL;
   } else if ((tempstrR!=NULL) || (tempstrH!=NULL) || (tempstrC!=NULL)) { // Colour is specified by RGB/HSB/CMYK expressions
    if (USEcol    !=NULL) *USEcol     = 0;
    if (USEcol1234!=NULL) *USEcol1234 = 0;
    if (outcol1S  ==NULL) { ppl_error(&c->errcontext, ERR_INTERNAL, -1, -1, "Received colour expressions, but have not received strings to put them into."); return 1; }
    if (malloced)
     {
      if (*outcolS !=NULL) free(*outcolS );
      if (*outcol1S!=NULL) free(*outcol1S);
      if (*outcol2S!=NULL) free(*outcol2S);
      if (*outcol3S!=NULL) free(*outcol3S);
      if (*outcol4S!=NULL) free(*outcol4S);
      *outcolS = *outcol4S = NULL;
      if (tempstrR!=NULL)
       {
        *outcolspace = SW_COLSPACE_RGB;
        *outcol1S = (char *)COLMALLOC(strlen(tempstrR)+1); strcpy(*outcol1S , tempstrR);
        *outcol2S = (char *)COLMALLOC(strlen(tempstrG)+1); strcpy(*outcol2S , tempstrG);
        *outcol3S = (char *)COLMALLOC(strlen(tempstrB)+1); strcpy(*outcol3S , tempstrB);
       }
      else if (tempstrH!=NULL)
       {
        *outcolspace = SW_COLSPACE_HSB;
        *outcol1S = (char *)COLMALLOC(strlen(tempstrH)+1); strcpy(*outcol1S , tempstrH);
        *outcol2S = (char *)COLMALLOC(strlen(tempstrS)+1); strcpy(*outcol2S , tempstrS);
        *outcol3S = (char *)COLMALLOC(strlen(tempstrB)+1); strcpy(*outcol3S , tempstrB);
       }
      else
       {
        *outcolspace = SW_COLSPACE_CMYK;
        *outcol1S = (char *)COLMALLOC(strlen(tempstrC)+1); strcpy(*outcol1S , tempstrC);
        *outcol2S = (char *)COLMALLOC(strlen(tempstrM)+1); strcpy(*outcol2S , tempstrM);
        *outcol3S = (char *)COLMALLOC(strlen(tempstrY)+1); strcpy(*outcol3S , tempstrY);
        *outcol4S = (char *)COLMALLOC(strlen(tempstrK)+1); strcpy(*outcol4S , tempstrK);
       }
     }
    else
     {
      *outcolS = *outcol4S = NULL;
      if (tempstrR!=NULL)
       {
        *outcolspace = SW_COLSPACE_RGB;
        *outcol1S = tempstrR;
        *outcol2S = tempstrG;
        *outcol3S = tempstrB;
       }
      else if (tempstrH!=NULL)
       {
        *outcolspace = SW_COLSPACE_HSB;
        *outcol1S = tempstrH;
        *outcol2S = tempstrS;
        *outcol3S = tempstrB;
       }
      else
       {
        *outcolspace = SW_COLSPACE_CMYK;
        *outcol1S = tempstrC;
        *outcol2S = tempstrM;
        *outcol3S = tempstrY;
        *outcol4S = tempstrK;
       }
     }
   }
  return 0;
 }

