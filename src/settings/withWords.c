// withWords.c
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

#define _PPL_SETTINGS_C 1

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "stringTools/asciidouble.h"
#include "stringTools/strConstants.h"

#include "settings/epsColours.h"

#include "pplConstants.h"
#include "coreUtils/errorReport.h"
#include "settings/settings.h"
#include "settings/settingTypes.h"

#include "userspace/context.h"

// -----------------------------------------------
// ROUTINES FOR MANIPULATING WITH_WORDS STRUCTURES
// -----------------------------------------------

void ppl_withWordsZero(ppl_context *context, withWords *a, const unsigned char malloced)
 {
  a->colour = a->fillcolour = a->linespoints = a->linetype = a->pointtype = a->style = a->Col1234Space = a->FillCol1234Space = 0;
  a->linewidth = a->pointlinewidth = a->pointsize = 1.0;
  a->colour1 = a->colour2 = a->colour3 = a->colour4 = a->fillcolour1 = a->fillcolour2 = a->fillcolour3 = a->fillcolour4 = 0.0;
  a->STRlinetype = a->STRlinewidth = a->STRpointlinewidth = a->STRpointsize = a->STRpointtype = NULL;
  a->STRcolour = a->STRcolour1 = a->STRcolour2 = a->STRcolour3 = a->STRcolour4 = a->STRfillcolour = a->STRfillcolour1 = a->STRfillcolour2 = a->STRfillcolour3 = a->STRfillcolour4 = NULL;
  a->USEcolour = a->USEfillcolour = a->USElinespoints = a->USElinetype = a->USElinewidth = a->USEpointlinewidth = a->USEpointsize = a->USEpointtype = a->USEstyle = a->USEcolour1234 = a->USEfillcolour1234 = 0;
  a->AUTOcolour = a->AUTOlinetype = a->AUTOpointtype = 0;
  a->malloced = malloced;
  return;
 }

#define XWWMALLOC(X) (tmp = malloc(X)); if (tmp==NULL) { ppl_error(&context->errcontext,ERR_MEMORY, -1, -1,"Out of memory"); ppl_withWordsZero(context,out,1); return; }

int ppl_withWordsCmp_zero(ppl_context *context, const withWords *a)
 {
  if (a->STRlinetype != NULL) return 0;
  if (a->STRlinewidth != NULL) return 0;
  if (a->STRpointlinewidth != NULL) return 0;
  if (a->STRpointsize != NULL) return 0;
  if (a->STRpointtype != NULL) return 0;
  if (a->STRcolour1 != NULL) return 0;
  if (a->STRcolour2 != NULL) return 0;
  if (a->STRcolour3 != NULL) return 0;
  if (a->STRcolour4 != NULL) return 0;
  if (a->STRfillcolour1 != NULL) return 0;
  if (a->STRfillcolour2 != NULL) return 0;
  if (a->STRfillcolour3 != NULL) return 0;
  if (a->STRfillcolour4 !=NULL) return 0;
  if (a->USEcolour) return 0;
  if (a->USEfillcolour) return 0;
  if (a->USElinespoints) return 0;
  if (a->USElinetype) return 0;
  if (a->USElinewidth) return 0;
  if (a->USEpointlinewidth) return 0;
  if (a->USEpointsize) return 0;
  if (a->USEpointtype) return 0;
  if (a->USEstyle) return 0;
  if (a->USEcolour1234) return 0;
  if (a->USEfillcolour1234) return 0;
  return 1;
 }

int ppl_withWordsCmp(ppl_context *context, const withWords *a, const withWords *b)
 {
  // Check that the range of items which are defined in both structures are the same
  if ((a->STRcolour1       ==NULL) != (b->STRcolour1       ==NULL)                                                                             ) return 0;
  if ((a->STRcolour4       ==NULL) != (b->STRcolour4       ==NULL)                                                                             ) return 0;
  if ((a->STRcolour1       ==NULL)                                 &&                            (a->USEcolour1234     != b->USEcolour1234    )) return 0;
  if ((a->STRcolour1       ==NULL)                                 && ( a->USEcolour1234    ) && (a->Col1234Space      != b->Col1234Space     )) return 0;
  if ((a->STRcolour1       ==NULL)                                 && (!a->USEcolour1234    ) && (a->USEcolour         != b->USEcolour        )) return 0;
  if ((a->STRfillcolour1   ==NULL) != (b->STRfillcolour1   ==NULL)                                                                             ) return 0;
  if ((a->STRfillcolour4   ==NULL) != (b->STRfillcolour4   ==NULL)                                                                             ) return 0;
  if ((a->STRfillcolour1   ==NULL)                                 &&                            (a->USEfillcolour1234 != b->USEfillcolour1234)) return 0;
  if ((a->STRfillcolour1   ==NULL)                                 && ( a->USEfillcolour1234) && (a->FillCol1234Space  != b->FillCol1234Space )) return 0;
  if ((a->STRfillcolour1   ==NULL)                                 && (!a->USEfillcolour1234) && (a->USEfillcolour     != b->USEfillcolour    )) return 0;
  if (                                                                                           (a->USElinespoints    != b->USElinespoints   )) return 0;
  if ((a->STRlinetype      ==NULL) != (b->STRlinetype      ==NULL)                                                                             ) return 0;
  if ((a->STRlinetype      ==NULL)                                                            && (a->USElinetype       != b->USElinetype      )) return 0;
  if ((a->STRlinewidth     ==NULL) != (b->STRlinewidth     ==NULL)                                                                             ) return 0;
  if ((a->STRlinewidth     ==NULL)                                                            && (a->USElinewidth      != b->USElinewidth     )) return 0;
  if ((a->STRpointlinewidth==NULL) != (b->STRpointlinewidth==NULL)                                                                             ) return 0;
  if ((a->STRpointlinewidth==NULL)                                                            && (a->USEpointlinewidth != b->USEpointlinewidth)) return 0;
  if ((a->STRpointsize     ==NULL) != (b->STRpointsize     ==NULL)                                                                             ) return 0;
  if ((a->STRpointsize     ==NULL)                                                            && (a->USEpointsize      != b->USEpointsize     )) return 0;
  if ((a->STRpointtype     ==NULL) != (b->STRpointtype     ==NULL)                                                                             ) return 0;
  if ((a->STRpointtype     ==NULL)                                                            && (a->USEpointtype      != b->USEpointtype     )) return 0;
  if (                                                                                           (a->USEstyle          != b->USEstyle         )) return 0;

  // Check that the actual values are the same in both structures
  if (a->STRcolour1 != NULL)
   {
    if ((strcmp(a->STRcolour1,b->STRcolour1)!=0)||(strcmp(a->STRcolour2,b->STRcolour2)!=0)||(strcmp(a->STRcolour3,b->STRcolour3)!=0)) return 0;
    if (a->Col1234Space!=b->Col1234Space) return 0;
    if ((a->STRcolour4 != NULL) && ((strcmp(a->STRcolour4,b->STRcolour4)!=0))) return 0;
   }
  else if ((a->USEcolour1234          ) && ((a->colour1!=b->colour1)||(a->colour2!=b->colour2)||(a->colour3!=b->colour3)||(a->colour4!=b->colour4)||(a->Col1234Space!=b->Col1234Space))) return 0;
  else if ((a->USEcolour              ) && ((a->colour !=b->colour ))) return 0;
  if (a->STRfillcolour1 != NULL)
   {
    if ((strcmp(a->STRfillcolour1,b->STRfillcolour1)!=0)||(strcmp(a->STRfillcolour2,b->STRfillcolour2)!=0)||(strcmp(a->STRfillcolour3,b->STRfillcolour3)!=0)) return 0;
    if (a->FillCol1234Space!=b->FillCol1234Space) return 0;
    if ((a->STRfillcolour4 != NULL) && ((strcmp(a->STRfillcolour4,b->STRfillcolour4)!=0))) return 0;
   }
  else if ((a->USEfillcolour1234      ) && ((a->fillcolour1!=b->fillcolour1)||(a->fillcolour2!=b->fillcolour2)||(a->fillcolour3!=b->fillcolour3)||(a->fillcolour4!=b->fillcolour4)||(a->FillCol1234Space!=b->FillCol1234Space))) return 0;
  else if ((a->USEfillcolour          ) && ((a->fillcolour    !=b->fillcolour    ))) return 0;
  if      ((a->USElinespoints         ) && ((a->linespoints   !=b->linespoints   ))) return 0;
  if      ((a->STRlinetype      !=NULL) && ((strcmp(a->STRlinetype      ,b->STRlinetype      )!=0))) return 0;
  else if ((a->USElinetype            ) && ((a->linetype      !=b->linetype      ))) return 0;
  if      ((a->STRlinewidth     !=NULL) && ((strcmp(a->STRlinewidth     ,b->STRlinewidth     )!=0))) return 0;
  else if ((a->USElinewidth           ) && ((a->linewidth     !=b->linewidth     ))) return 0;
  if      ((a->STRpointlinewidth!=NULL) && ((strcmp(a->STRpointlinewidth,b->STRpointlinewidth)!=0))) return 0;
  else if ((a->USEpointlinewidth      ) && ((a->pointlinewidth!=b->pointlinewidth))) return 0;
  if      ((a->STRpointsize     !=NULL) && ((strcmp(a->STRpointsize     ,b->STRpointsize     )!=0))) return 0;
  else if ((a->USEpointsize           ) && ((a->pointsize     !=b->pointsize     ))) return 0;
  if      ((a->STRpointtype     !=NULL) && ((strcmp(a->STRpointtype     ,b->STRpointtype     )!=0))) return 0;
  else if ((a->USEpointtype           ) && ((a->pointtype     !=b->pointtype     ))) return 0;
  if      ((a->USEstyle               ) && ((a->style         !=b->style         ))) return 0;

  return 1; // We have not found any differences
 }

// a has highest priority; e has lowest priority
void ppl_withWordsMerge(ppl_context *context, withWords *out, const withWords *a, const withWords *b, const withWords *c, const withWords *d, const withWords *e, const unsigned char ExpandStyles)
 {
  int i;
  const withWords *InputArray[25] = {a,b,c,d,e};
  unsigned char BlockStyleSubstitution[25] = {0,0,0,0,0};
  const withWords *x;

  ppl_withWordsZero(context,out,0);

  for (i=4; i>=0; i--)
   {
    if (i>24) { ppl_error(&context->errcontext,ERR_GENERAL, -1, -1, "Iteration depth exceeded whilst substituting plot styles. Infinite plot style loop suspected."); return; } // i can reach 24 when recursion happens
    x = InputArray[i];
    if (x == NULL) continue;
    if ((x->USEstyle) && (!BlockStyleSubstitution[i])) // Substitute for numbered plot styles
     {
      if (ExpandStyles)
       {
        BlockStyleSubstitution[i  ] = 1; // Only do this once, to avoid infinite loop
        BlockStyleSubstitution[i+1] = 0; // Allow recursive substitutions
        InputArray[i+1] = &(context->set->plot_styles[ x->style ]);
        i+=2; // Recurse
        continue;
       } else {
        out->style = x->style; out->USEstyle = 1;
       }
     }
    if (x->USEcolour1234          ) { out->colour1 = x->colour1; out->colour2 = x->colour2; out->colour3 = x->colour3; out->colour4 = x->colour4; out->Col1234Space = x->Col1234Space; out->USEcolour1234 = 1; out->USEcolour = 0; out->STRcolour1 = out->STRcolour2 = out->STRcolour3 = out->STRcolour4 = NULL; out->AUTOcolour = x->AUTOcolour; }
    if (x->USEcolour              ) { out->colour = x->colour; out->USEcolour = 1; out->USEcolour1234 = 0; out->STRcolour1 = out->STRcolour2 = out->STRcolour3 = out->STRcolour4 = NULL; out->AUTOcolour = x->AUTOcolour; }
    if (x->STRcolour        !=NULL) { out->STRcolour = x->STRcolour; }
    if (x->STRcolour1       !=NULL) { out->STRcolour1 = x->STRcolour1; out->STRcolour2 = x->STRcolour2; out->STRcolour3 = x->STRcolour3; out->STRcolour4 = x->STRcolour4; out->Col1234Space = x->Col1234Space; }
    if (x->STRfillcolour1   !=NULL) { out->STRfillcolour1 = x->STRfillcolour1; out->STRfillcolour2 = x->STRfillcolour2; out->STRfillcolour3 = x->STRfillcolour3; out->STRfillcolour4 = x->STRfillcolour4; out->FillCol1234Space = x->FillCol1234Space; out->USEfillcolour1234 = 0; out->USEfillcolour = 0; }
    if (x->USEfillcolour1234      ) { out->fillcolour1 = x->fillcolour1; out->fillcolour2 = x->fillcolour2; out->fillcolour3 = x->fillcolour3; out->fillcolour4 = x->fillcolour4; out->FillCol1234Space = x->FillCol1234Space; out->USEfillcolour1234 = 1; out->USEfillcolour = 0; out->STRfillcolour1 = out->STRfillcolour2 = out->STRfillcolour3 = out->STRfillcolour4 = NULL; }
    if (x->USEfillcolour          ) { out->fillcolour = x->fillcolour; out->USEfillcolour = 1; out->USEfillcolour1234 = 0; out->STRfillcolour1 = out->STRfillcolour2 = out->STRfillcolour3 = out->STRfillcolour4 = NULL; }
    if (x->USElinespoints         ) { out->linespoints = x->linespoints; out->USElinespoints = 1; }
    if (x->USElinetype            ) { out->linetype = x->linetype; out->USElinetype = 1; out->STRlinetype = NULL; out->AUTOlinetype = x->AUTOlinetype; }
    if (x->STRlinetype      !=NULL) { out->STRlinetype = x->STRlinetype; }
    if (x->STRlinewidth     !=NULL) { out->STRlinewidth = x->STRlinewidth; out->USElinewidth = 0; }
    if (x->USElinewidth           ) { out->linewidth = x->linewidth; out->USElinewidth = 1; out->STRlinewidth = NULL; }
    if (x->STRpointlinewidth!=NULL) { out->STRpointlinewidth = x->STRpointlinewidth; out->USEpointlinewidth = 0; }
    if (x->USEpointlinewidth      ) { out->pointlinewidth = x->pointlinewidth; out->USEpointlinewidth = 1; out->STRpointlinewidth = NULL; }
    if (x->STRpointsize     !=NULL) { out->STRpointsize = x->STRpointsize; }
    if (x->USEpointsize           ) { out->pointsize = x->pointsize; out->USEpointsize = 1; out->STRpointsize = NULL; }
    if (x->USEpointtype           ) { out->pointtype = x->pointtype; out->USEpointtype = 1; out->STRpointtype = NULL; out->AUTOpointtype = x->AUTOpointtype; }
    if (x->STRpointtype     !=NULL) { out->STRpointtype = x->STRpointtype; }
   }
  return;
 }

#define NUMDISP(X) ppl_numericDisplay(X,0,context->set->term_current.SignificantFigures,(context->set->term_current.NumDisplay==SW_DISPLAY_L))

#define S_RGB(X,Y) (char *)ppl_numericDisplay(X,Y,context->set->term_current.SignificantFigures,(context->set->term_current.NumDisplay==SW_DISPLAY_L))

void ppl_withWordsPrint(ppl_context *context, const withWords *defn, char *out)
 {
  int i=0;
  if      (defn->USElinespoints)          { sprintf(out+i, "%s "            , *(char **)FetchSettingName(&context->errcontext,defn->linespoints, SW_STYLE_INT , (void *)SW_STYLE_STR , sizeof(char *))); i += strlen(out+i); }
  if      (defn->STRcolour1!=NULL)
   {
    if      (defn->Col1234Space==SW_COLSPACE_RGB ) sprintf(out+i, "colour rgb%s:%s:%s "     , defn->STRcolour1, defn->STRcolour2, defn->STRcolour3);
    else if (defn->Col1234Space==SW_COLSPACE_HSB ) sprintf(out+i, "colour hsb%s:%s:%s "     , defn->STRcolour1, defn->STRcolour2, defn->STRcolour3);
    else if (defn->Col1234Space==SW_COLSPACE_CMYK) sprintf(out+i, "colour cmyk%s:%s:%s:%s "  , defn->STRcolour1, defn->STRcolour2, defn->STRcolour3, defn->STRcolour4);
    i += strlen(out+i);
   }
  else if (defn->USEcolour1234)
   {
    if      (defn->Col1234Space==SW_COLSPACE_RGB ) sprintf(out+i, "colour rgb%s:%s:%s "     , S_RGB(defn->colour1,0), S_RGB(defn->colour2,1), S_RGB(defn->colour3,2));
    else if (defn->Col1234Space==SW_COLSPACE_HSB ) sprintf(out+i, "colour hsb%s:%s:%s "     , S_RGB(defn->colour1,0), S_RGB(defn->colour2,1), S_RGB(defn->colour3,2));
    else if (defn->Col1234Space==SW_COLSPACE_CMYK) sprintf(out+i, "colour cmyk%s:%s:%s:%s " , S_RGB(defn->colour1,0), S_RGB(defn->colour2,1), S_RGB(defn->colour3,2), S_RGB(defn->colour4,3));
    i += strlen(out+i);
   }
  else if (defn->USEcolour)               { sprintf(out+i, "colour %s "     , *(char **)FetchSettingName(&context->errcontext,defn->colour     , SW_COLOUR_INT, (void *)SW_COLOUR_STR, sizeof(char *))); i += strlen(out+i); }
  if      (defn->STRfillcolour1!=NULL)
   {
    if      (defn->Col1234Space==SW_COLSPACE_RGB ) sprintf(out+i, "fillcolour rgb%s:%s:%s " , defn->STRfillcolour1, defn->STRfillcolour2, defn->STRfillcolour3);
    else if (defn->Col1234Space==SW_COLSPACE_HSB ) sprintf(out+i, "fillcolour hsb%s:%s:%s " , defn->STRfillcolour1, defn->STRfillcolour2, defn->STRfillcolour3);
    else if (defn->Col1234Space==SW_COLSPACE_CMYK) sprintf(out+i, "fillcolour cmyk%s:%s:%s:%s " , defn->STRfillcolour1, defn->STRfillcolour2, defn->STRfillcolour3, defn->STRfillcolour4);
    i += strlen(out+i);
   }
  else if (defn->USEfillcolour1234)
   {
    if      (defn->Col1234Space==SW_COLSPACE_RGB ) sprintf(out+i, "fillcolour rgb%s:%s:%s " , S_RGB(defn->fillcolour1,0), S_RGB(defn->fillcolour2,1), S_RGB(defn->fillcolour3,2));
    else if (defn->Col1234Space==SW_COLSPACE_HSB ) sprintf(out+i, "fillcolour hsb%s:%s:%s " , S_RGB(defn->fillcolour1,0), S_RGB(defn->fillcolour2,1), S_RGB(defn->fillcolour3,2));
    else if (defn->Col1234Space==SW_COLSPACE_CMYK) sprintf(out+i, "fillcolour cmyk%s:%s:%s:%s " , S_RGB(defn->fillcolour1,0), S_RGB(defn->fillcolour2,1), S_RGB(defn->fillcolour3,2), S_RGB(defn->fillcolour4,2));
    i += strlen(out+i);
   }
  else if (defn->USEfillcolour)           { sprintf(out+i, "fillcolour %s " , *(char **)FetchSettingName(&context->errcontext,defn->fillcolour , SW_COLOUR_INT, (void *)SW_COLOUR_STR, sizeof(char *))); i += strlen(out+i); }
  if      (defn->STRlinetype!=NULL)       { sprintf(out+i, "linetype %s "            , defn->STRlinetype);                                                                          i += strlen(out+i); }
  else if (defn->USElinetype)             { sprintf(out+i, "linetype %d "            , defn->linetype);                                                                             i += strlen(out+i); }
  if      (defn->STRlinewidth!=NULL)      { sprintf(out+i, "linewidth %s "           , defn->STRlinewidth);                                                                         i += strlen(out+i); }
  else if (defn->USElinewidth)            { sprintf(out+i, "linewidth %s "           , NUMDISP(defn->linewidth));                                                                   i += strlen(out+i); }
  if      (defn->STRpointlinewidth!=NULL) { sprintf(out+i, "pointlinewidth %s "      , defn->STRpointlinewidth);                                                                    i += strlen(out+i); }
  else if (defn->USEpointlinewidth)       { sprintf(out+i, "pointlinewidth %s "      , NUMDISP(defn->pointlinewidth));                                                              i += strlen(out+i); }
  if      (defn->STRpointsize!=NULL)      { sprintf(out+i, "pointsize %s "           , defn->STRpointsize);                                                                         i += strlen(out+i); }
  else if (defn->USEpointsize)            { sprintf(out+i, "pointsize %s "           , NUMDISP(defn->pointsize));                                                                   i += strlen(out+i); }
  if      (defn->STRpointtype!=NULL)      { sprintf(out+i, "pointtype %s "           , defn->STRpointtype);                                                                         i += strlen(out+i); }
  else if (defn->USEpointtype)            { sprintf(out+i, "pointtype %d "           , defn->pointtype);                                                                            i += strlen(out+i); }
  if      (defn->USEstyle)                { sprintf(out+i, "style %d "               , defn->style);                                                                                i += strlen(out+i); }
  out[i]='\0';
  return;
 }

void ppl_withWordsDestroy(ppl_context *context, withWords *a)
 {
  if (!a->malloced) return;
  if (a->STRlinetype       != NULL) { free(a->STRlinetype      ); a->STRlinetype       = NULL; }
  if (a->STRlinewidth      != NULL) { free(a->STRlinewidth     ); a->STRlinewidth      = NULL; }
  if (a->STRpointlinewidth != NULL) { free(a->STRpointlinewidth); a->STRpointlinewidth = NULL; }
  if (a->STRpointsize      != NULL) { free(a->STRpointsize     ); a->STRpointsize      = NULL; }
  if (a->STRpointtype      != NULL) { free(a->STRpointtype     ); a->STRpointtype      = NULL; }
  if (a->STRcolour1        != NULL) { free(a->STRcolour1       ); a->STRcolour1        = NULL; }
  if (a->STRcolour2        != NULL) { free(a->STRcolour2       ); a->STRcolour2        = NULL; }
  if (a->STRcolour3        != NULL) { free(a->STRcolour3       ); a->STRcolour3        = NULL; }
  if (a->STRcolour4        != NULL) { free(a->STRcolour4       ); a->STRcolour4        = NULL; }
  if (a->STRfillcolour1    != NULL) { free(a->STRfillcolour1   ); a->STRfillcolour1    = NULL; }
  if (a->STRfillcolour2    != NULL) { free(a->STRfillcolour2   ); a->STRfillcolour2    = NULL; }
  if (a->STRfillcolour3    != NULL) { free(a->STRfillcolour3   ); a->STRfillcolour3    = NULL; }
  if (a->STRfillcolour4    != NULL) { free(a->STRfillcolour4   ); a->STRfillcolour4    = NULL; }
  return;
 }

void ppl_withWordsCpy(ppl_context *context, withWords *out, const withWords *in)
 {
  void *tmp;
  *out = *in;
  out->malloced = 1;
  if (in->STRlinetype      != NULL) { out->STRlinetype      = (char   *)XWWMALLOC(strlen(in->STRlinetype      )+1); strcpy(out->STRlinetype      , in->STRlinetype      ); }
  if (in->STRlinewidth     != NULL) { out->STRlinewidth     = (char   *)XWWMALLOC(strlen(in->STRlinewidth     )+1); strcpy(out->STRlinewidth     , in->STRlinewidth     ); }
  if (in->STRpointlinewidth!= NULL) { out->STRpointlinewidth= (char   *)XWWMALLOC(strlen(in->STRpointlinewidth)+1); strcpy(out->STRpointlinewidth, in->STRpointlinewidth); }
  if (in->STRpointsize     != NULL) { out->STRpointsize     = (char   *)XWWMALLOC(strlen(in->STRpointsize     )+1); strcpy(out->STRpointsize     , in->STRpointsize     ); }
  if (in->STRpointtype     != NULL) { out->STRpointtype     = (char   *)XWWMALLOC(strlen(in->STRpointtype     )+1); strcpy(out->STRpointtype     , in->STRpointtype     ); }
  if (in->STRcolour1       != NULL) { out->STRcolour1       = (char   *)XWWMALLOC(strlen(in->STRcolour1       )+1); strcpy(out->STRcolour1       , in->STRcolour1       ); }
  if (in->STRcolour2       != NULL) { out->STRcolour2       = (char   *)XWWMALLOC(strlen(in->STRcolour2       )+1); strcpy(out->STRcolour2       , in->STRcolour2       ); }
  if (in->STRcolour3       != NULL) { out->STRcolour3       = (char   *)XWWMALLOC(strlen(in->STRcolour3       )+1); strcpy(out->STRcolour3       , in->STRcolour3       ); }
  if (in->STRcolour4       != NULL) { out->STRcolour4       = (char   *)XWWMALLOC(strlen(in->STRcolour4       )+1); strcpy(out->STRcolour4       , in->STRcolour4       ); }
  if (in->STRfillcolour1   != NULL) { out->STRfillcolour1   = (char   *)XWWMALLOC(strlen(in->STRfillcolour1   )+1); strcpy(out->STRfillcolour1   , in->STRfillcolour1   ); }
  if (in->STRfillcolour2   != NULL) { out->STRfillcolour2   = (char   *)XWWMALLOC(strlen(in->STRfillcolour2   )+1); strcpy(out->STRfillcolour2   , in->STRfillcolour2   ); }
  if (in->STRfillcolour3   != NULL) { out->STRfillcolour3   = (char   *)XWWMALLOC(strlen(in->STRfillcolour3   )+1); strcpy(out->STRfillcolour3   , in->STRfillcolour3   ); }
  if (in->STRfillcolour4   != NULL) { out->STRfillcolour4   = (char   *)XWWMALLOC(strlen(in->STRfillcolour4   )+1); strcpy(out->STRfillcolour4   , in->STRfillcolour4   ); }
  return;
 }

