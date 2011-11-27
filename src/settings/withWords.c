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

#include "pplConstants.h"
#include "coreUtils/errorReport.h"
#include "settings/colors.h"
#include "settings/epsColors.h"
#include "settings/settings.h"
#include "settings/settingTypes.h"
#include "settings/withWords_fns.h"

#include "userspace/context.h"

// -----------------------------------------------
// ROUTINES FOR MANIPULATING WITH_WORDS STRUCTURES
// -----------------------------------------------

void ppl_withWordsZero(ppl_context *context, withWords *a, const unsigned char malloced)
 {
  a->color = a->fillcolor = a->linespoints = a->linetype = a->pointtype = a->style = a->Col1234Space = a->FillCol1234Space = 0;
  a->linewidth = a->pointlinewidth = a->pointsize = 1.0;
  a->color1 = a->color2 = a->color3 = a->color4 = a->fillcolor1 = a->fillcolor2 = a->fillcolor3 = a->fillcolor4 = 0.0;
  a->STRlinetype = a->STRlinewidth = a->STRpointlinewidth = a->STRpointsize = a->STRpointtype = NULL;
  a->STRcolor = a->STRcolor1 = a->STRcolor2 = a->STRcolor3 = a->STRcolor4 = a->STRfillcolor = a->STRfillcolor1 = a->STRfillcolor2 = a->STRfillcolor3 = a->STRfillcolor4 = NULL;
  a->USEcolor = a->USEfillcolor = a->USElinespoints = a->USElinetype = a->USElinewidth = a->USEpointlinewidth = a->USEpointsize = a->USEpointtype = a->USEstyle = a->USEcolor1234 = a->USEfillcolor1234 = 0;
  a->AUTOcolor = a->AUTOlinetype = a->AUTOpointtype = 0;
  a->malloced = malloced;
  return;
 }

#define XWWMALLOC(X) (tmp = malloc(X)); if (tmp==NULL) { ppl_error(&context->errcontext,ERR_MEMORY, -1, -1,"Out of memory"); ppl_withWordsZero(context,out,1); return; }

void ppl_withWordsFromDict(ppl_context *context, dict *in, withWords *out, const unsigned char MallocNew)
 {
  int    *tempint, i; // TO DO: Need to be able to read colors.
  double *tempdbl;
  char   *tempstr;
  void   *tmp;
  ppl_withWordsZero(context, out, MallocNew);

  // read color names
  ppl_colorFromDict(context, in,""    ,&out->color    ,&out->Col1234Space    ,&out->color1        ,&out->color2        ,&out->color3        ,&out->color4       ,
                                                          &out->STRcolor     ,&out->STRcolor1     ,&out->STRcolor2     ,&out->STRcolor3     ,&out->STRcolor4    ,
                    &out->USEcolor    ,&out->USEcolor1234    ,&i,MallocNew);
  ppl_colorFromDict(context, in,"fill",&out->fillcolor,&out->FillCol1234Space,&out->fillcolor1    ,&out->fillcolor2    ,&out->fillcolor3    ,&out->fillcolor4   ,
                                                       &out->STRfillcolor    ,&out->STRfillcolor1 ,&out->STRfillcolor2 ,&out->STRfillcolor3 ,&out->STRfillcolor4,
                    &out->USEfillcolor,&out->USEfillcolor1234,&i,MallocNew);

  // Other settings
  tempint = (int *)ppl_dictLookup(in,"linetype");
  if (tempint != NULL) { out->linetype = *tempint; out->USElinetype = 1; }
  tempstr = (char *)ppl_dictLookup(in,"linetype_string");
  if (tempstr != NULL) { if (!MallocNew) { out->STRlinetype       = tempstr; }
                         else            { out->STRlinetype       = (char   *)XWWMALLOC(strlen(tempstr)+1); strcpy(out->STRlinetype      , tempstr); }
                       }
  tempdbl = (double *)ppl_dictLookup(in,"linewidth");
  if (tempdbl != NULL) { out->linewidth = *tempdbl; out->USElinewidth = 1; }
  tempstr = (char *)ppl_dictLookup(in,"linewidth_string");
  if (tempstr != NULL) { if (!MallocNew) { out->STRlinewidth      = tempstr; }
                         else            { out->STRlinewidth      = (char   *)XWWMALLOC(strlen(tempstr)+1); strcpy(out->STRlinewidth     , tempstr); }
                       }
  tempdbl = (double *)ppl_dictLookup(in,"pointsize");
  if (tempdbl != NULL) { out->pointsize = *tempdbl; out->USEpointsize = 1; }
  tempstr = (char *)ppl_dictLookup(in,"pointsize_string");
  if (tempstr != NULL) { if (!MallocNew) { out->STRpointsize      = tempstr; }
                         else            { out->STRpointsize      = (char   *)XWWMALLOC(strlen(tempstr)+1); strcpy(out->STRpointsize     , tempstr); }
                       }
  tempint = (int *)ppl_dictLookup(in,"pointtype");
  if (tempint != NULL) { out->pointtype = *tempint; out->USEpointtype = 1; }
  tempstr = (char *)ppl_dictLookup(in,"pointtype_string");
  if (tempstr != NULL) { if (!MallocNew) { out->STRpointtype      = tempstr; }
                         else            { out->STRpointtype      = (char   *)XWWMALLOC(strlen(tempstr)+1); strcpy(out->STRpointtype     , tempstr); }
                       }
  tempint = (int *)ppl_dictLookup(in,"style_number");
  if (tempint != NULL) { out->style = *tempint; out->USEstyle = 1; }
  tempdbl = (double *)ppl_dictLookup(in,"pointlinewidth");
  if (tempdbl != NULL) { out->pointlinewidth = *tempdbl; out->USEpointlinewidth = 1; }
  tempstr = (char *)ppl_dictLookup(in,"pointlinewidth_string");
  if (tempstr != NULL) { if (!MallocNew) { out->STRpointlinewidth = tempstr; }
                         else            { out->STRpointlinewidth = (char   *)XWWMALLOC(strlen(tempstr)+1); strcpy(out->STRpointlinewidth, tempstr); }
                       }
  tempstr = (char *)ppl_dictLookup(in,"style");
  if (tempstr != NULL)
   {
    i = ppl_fetchSettingByName(&context->errcontext, tempstr, SW_STYLE_INT, SW_STYLE_STR);
    out->linespoints = i;
    out->USElinespoints = 1;
   }
  return;
 }


int ppl_withWordsCmp_zero(ppl_context *context, const withWords *a)
 {
  if (a->STRlinetype != NULL) return 0;
  if (a->STRlinewidth != NULL) return 0;
  if (a->STRpointlinewidth != NULL) return 0;
  if (a->STRpointsize != NULL) return 0;
  if (a->STRpointtype != NULL) return 0;
  if (a->STRcolor1 != NULL) return 0;
  if (a->STRcolor2 != NULL) return 0;
  if (a->STRcolor3 != NULL) return 0;
  if (a->STRcolor4 != NULL) return 0;
  if (a->STRfillcolor1 != NULL) return 0;
  if (a->STRfillcolor2 != NULL) return 0;
  if (a->STRfillcolor3 != NULL) return 0;
  if (a->STRfillcolor4 !=NULL) return 0;
  if (a->USEcolor) return 0;
  if (a->USEfillcolor) return 0;
  if (a->USElinespoints) return 0;
  if (a->USElinetype) return 0;
  if (a->USElinewidth) return 0;
  if (a->USEpointlinewidth) return 0;
  if (a->USEpointsize) return 0;
  if (a->USEpointtype) return 0;
  if (a->USEstyle) return 0;
  if (a->USEcolor1234) return 0;
  if (a->USEfillcolor1234) return 0;
  return 1;
 }

int ppl_withWordsCmp(ppl_context *context, const withWords *a, const withWords *b)
 {
  // Check that the range of items which are defined in both structures are the same
  if ((a->STRcolor1        ==NULL) != (b->STRcolor1       ==NULL)                                                                             ) return 0;
  if ((a->STRcolor4        ==NULL) != (b->STRcolor4       ==NULL)                                                                             ) return 0;
  if ((a->STRcolor1        ==NULL)                                 &&                           (a->USEcolor1234      != b->USEcolor1234     )) return 0;
  if ((a->STRcolor1        ==NULL)                                 && ( a->USEcolor1234    ) && (a->Col1234Space      != b->Col1234Space     )) return 0;
  if ((a->STRcolor1        ==NULL)                                 && (!a->USEcolor1234    ) && (a->USEcolor          != b->USEcolor         )) return 0;
  if ((a->STRfillcolor1    ==NULL) != (b->STRfillcolor1   ==NULL)                                                                             ) return 0;
  if ((a->STRfillcolor4    ==NULL) != (b->STRfillcolor4   ==NULL)                                                                             ) return 0;
  if ((a->STRfillcolor1    ==NULL)                                 &&                            (a->USEfillcolor1234 != b->USEfillcolor1234 )) return 0;
  if ((a->STRfillcolor1    ==NULL)                                 && ( a->USEfillcolor1234) && (a->FillCol1234Space  != b->FillCol1234Space )) return 0;
  if ((a->STRfillcolor1    ==NULL)                                 && (!a->USEfillcolor1234) && (a->USEfillcolor      != b->USEfillcolor     )) return 0;
  if (                                                                                          (a->USElinespoints    != b->USElinespoints   )) return 0;
  if ((a->STRlinetype      ==NULL) != (b->STRlinetype      ==NULL)                                                                            ) return 0;
  if ((a->STRlinetype      ==NULL)                                                           && (a->USElinetype       != b->USElinetype      )) return 0;
  if ((a->STRlinewidth     ==NULL) != (b->STRlinewidth     ==NULL)                                                                            ) return 0;
  if ((a->STRlinewidth     ==NULL)                                                           && (a->USElinewidth      != b->USElinewidth     )) return 0;
  if ((a->STRpointlinewidth==NULL) != (b->STRpointlinewidth==NULL)                                                                            ) return 0;
  if ((a->STRpointlinewidth==NULL)                                                           && (a->USEpointlinewidth != b->USEpointlinewidth)) return 0;
  if ((a->STRpointsize     ==NULL) != (b->STRpointsize     ==NULL)                                                                            ) return 0;
  if ((a->STRpointsize     ==NULL)                                                           && (a->USEpointsize      != b->USEpointsize     )) return 0;
  if ((a->STRpointtype     ==NULL) != (b->STRpointtype     ==NULL)                                                                            ) return 0;
  if ((a->STRpointtype     ==NULL)                                                           && (a->USEpointtype      != b->USEpointtype     )) return 0;
  if (                                                                                          (a->USEstyle          != b->USEstyle         )) return 0;

  // Check that the actual values are the same in both structures
  if (a->STRcolor1 != NULL)
   {
    if ((strcmp(a->STRcolor1,b->STRcolor1)!=0)||(strcmp(a->STRcolor2,b->STRcolor2)!=0)||(strcmp(a->STRcolor3,b->STRcolor3)!=0)) return 0;
    if (a->Col1234Space!=b->Col1234Space) return 0;
    if ((a->STRcolor4 != NULL) && ((strcmp(a->STRcolor4,b->STRcolor4)!=0))) return 0;
   }
  else if ((a->USEcolor1234           ) && ((a->color1!=b->color1)||(a->color2!=b->color2)||(a->color3!=b->color3)||(a->color4!=b->color4)||(a->Col1234Space!=b->Col1234Space))) return 0;
  else if ((a->USEcolor               ) && ((a->color !=b->color ))) return 0;
  if (a->STRfillcolor1 != NULL)
   {
    if ((strcmp(a->STRfillcolor1,b->STRfillcolor1)!=0)||(strcmp(a->STRfillcolor2,b->STRfillcolor2)!=0)||(strcmp(a->STRfillcolor3,b->STRfillcolor3)!=0)) return 0;
    if (a->FillCol1234Space!=b->FillCol1234Space) return 0;
    if ((a->STRfillcolor4 != NULL) && ((strcmp(a->STRfillcolor4,b->STRfillcolor4)!=0))) return 0;
   }
  else if ((a->USEfillcolor1234       ) && ((a->fillcolor1!=b->fillcolor1)||(a->fillcolor2!=b->fillcolor2)||(a->fillcolor3!=b->fillcolor3)||(a->fillcolor4!=b->fillcolor4)||(a->FillCol1234Space!=b->FillCol1234Space))) return 0;
  else if ((a->USEfillcolor           ) && ((a->fillcolor     !=b->fillcolor     ))) return 0;
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
    if (x->USEcolor1234          ) { out->color1 = x->color1; out->color2 = x->color2; out->color3 = x->color3; out->color4 = x->color4; out->Col1234Space = x->Col1234Space; out->USEcolor1234 = 1; out->USEcolor = 0; out->STRcolor1 = out->STRcolor2 = out->STRcolor3 = out->STRcolor4 = NULL; out->AUTOcolor = x->AUTOcolor; }
    if (x->USEcolor              ) { out->color = x->color; out->USEcolor = 1; out->USEcolor1234 = 0; out->STRcolor1 = out->STRcolor2 = out->STRcolor3 = out->STRcolor4 = NULL; out->AUTOcolor = x->AUTOcolor; }
    if (x->STRcolor        !=NULL) { out->STRcolor = x->STRcolor; }
    if (x->STRcolor1       !=NULL) { out->STRcolor1 = x->STRcolor1; out->STRcolor2 = x->STRcolor2; out->STRcolor3 = x->STRcolor3; out->STRcolor4 = x->STRcolor4; out->Col1234Space = x->Col1234Space; }
    if (x->STRfillcolor1   !=NULL) { out->STRfillcolor1 = x->STRfillcolor1; out->STRfillcolor2 = x->STRfillcolor2; out->STRfillcolor3 = x->STRfillcolor3; out->STRfillcolor4 = x->STRfillcolor4; out->FillCol1234Space = x->FillCol1234Space; out->USEfillcolor1234 = 0; out->USEfillcolor = 0; }
    if (x->USEfillcolor1234      ) { out->fillcolor1 = x->fillcolor1; out->fillcolor2 = x->fillcolor2; out->fillcolor3 = x->fillcolor3; out->fillcolor4 = x->fillcolor4; out->FillCol1234Space = x->FillCol1234Space; out->USEfillcolor1234 = 1; out->USEfillcolor = 0; out->STRfillcolor1 = out->STRfillcolor2 = out->STRfillcolor3 = out->STRfillcolor4 = NULL; }
    if (x->USEfillcolor          ) { out->fillcolor = x->fillcolor; out->USEfillcolor = 1; out->USEfillcolor1234 = 0; out->STRfillcolor1 = out->STRfillcolor2 = out->STRfillcolor3 = out->STRfillcolor4 = NULL; }
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

#define NUMDISP(X) ppl_numericDisplay(X,context->numdispBuff[0],context->set->term_current.SignificantFigures,(context->set->term_current.NumDisplay==SW_DISPLAY_L))

#define S_RGB(X,Y) (char *)ppl_numericDisplay(X,context->numdispBuff[Y],context->set->term_current.SignificantFigures,(context->set->term_current.NumDisplay==SW_DISPLAY_L))

void ppl_withWordsPrint(ppl_context *context, const withWords *defn, char *out)
 {
  int i=0;
  if      (defn->USElinespoints)          { sprintf(out+i, "%s "            , *(char **)ppl_fetchSettingName(&context->errcontext,defn->linespoints, SW_STYLE_INT , (void *)SW_STYLE_STR , sizeof(char *))); i += strlen(out+i); }
  if      (defn->STRcolor1!=NULL)
   {
    if      (defn->Col1234Space==SW_COLSPACE_RGB ) sprintf(out+i, "color rgb%s:%s:%s "     , defn->STRcolor1, defn->STRcolor2, defn->STRcolor3);
    else if (defn->Col1234Space==SW_COLSPACE_HSB ) sprintf(out+i, "color hsb%s:%s:%s "     , defn->STRcolor1, defn->STRcolor2, defn->STRcolor3);
    else if (defn->Col1234Space==SW_COLSPACE_CMYK) sprintf(out+i, "color cmyk%s:%s:%s:%s "  , defn->STRcolor1, defn->STRcolor2, defn->STRcolor3, defn->STRcolor4);
    i += strlen(out+i);
   }
  else if (defn->USEcolor1234)
   {
    if      (defn->Col1234Space==SW_COLSPACE_RGB ) sprintf(out+i, "color rgb%s:%s:%s "     , S_RGB(defn->color1,0), S_RGB(defn->color2,1), S_RGB(defn->color3,2));
    else if (defn->Col1234Space==SW_COLSPACE_HSB ) sprintf(out+i, "color hsb%s:%s:%s "     , S_RGB(defn->color1,0), S_RGB(defn->color2,1), S_RGB(defn->color3,2));
    else if (defn->Col1234Space==SW_COLSPACE_CMYK) sprintf(out+i, "color cmyk%s:%s:%s:%s " , S_RGB(defn->color1,0), S_RGB(defn->color2,1), S_RGB(defn->color3,2), S_RGB(defn->color4,3));
    i += strlen(out+i);
   }
  else if (defn->USEcolor)               { sprintf(out+i, "color %s "     , *(char **)ppl_fetchSettingName(&context->errcontext,defn->color     , SW_COLOR_INT, (void *)SW_COLOR_STR, sizeof(char *))); i += strlen(out+i); }
  if      (defn->STRfillcolor1!=NULL)
   {
    if      (defn->Col1234Space==SW_COLSPACE_RGB ) sprintf(out+i, "fillcolor rgb%s:%s:%s " , defn->STRfillcolor1, defn->STRfillcolor2, defn->STRfillcolor3);
    else if (defn->Col1234Space==SW_COLSPACE_HSB ) sprintf(out+i, "fillcolor hsb%s:%s:%s " , defn->STRfillcolor1, defn->STRfillcolor2, defn->STRfillcolor3);
    else if (defn->Col1234Space==SW_COLSPACE_CMYK) sprintf(out+i, "fillcolor cmyk%s:%s:%s:%s " , defn->STRfillcolor1, defn->STRfillcolor2, defn->STRfillcolor3, defn->STRfillcolor4);
    i += strlen(out+i);
   }
  else if (defn->USEfillcolor1234)
   {
    if      (defn->Col1234Space==SW_COLSPACE_RGB ) sprintf(out+i, "fillcolor rgb%s:%s:%s " , S_RGB(defn->fillcolor1,0), S_RGB(defn->fillcolor2,1), S_RGB(defn->fillcolor3,2));
    else if (defn->Col1234Space==SW_COLSPACE_HSB ) sprintf(out+i, "fillcolor hsb%s:%s:%s " , S_RGB(defn->fillcolor1,0), S_RGB(defn->fillcolor2,1), S_RGB(defn->fillcolor3,2));
    else if (defn->Col1234Space==SW_COLSPACE_CMYK) sprintf(out+i, "fillcolor cmyk%s:%s:%s:%s " , S_RGB(defn->fillcolor1,0), S_RGB(defn->fillcolor2,1), S_RGB(defn->fillcolor3,2), S_RGB(defn->fillcolor4,2));
    i += strlen(out+i);
   }
  else if (defn->USEfillcolor)            { sprintf(out+i, "fillcolor %s " , *(char **)ppl_fetchSettingName(&context->errcontext,defn->fillcolor , SW_COLOR_INT, (void *)SW_COLOR_STR, sizeof(char *))); i += strlen(out+i); }
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
  if (a->STRcolor1         != NULL) { free(a->STRcolor1        ); a->STRcolor1         = NULL; }
  if (a->STRcolor2         != NULL) { free(a->STRcolor2        ); a->STRcolor2         = NULL; }
  if (a->STRcolor3         != NULL) { free(a->STRcolor3        ); a->STRcolor3         = NULL; }
  if (a->STRcolor4         != NULL) { free(a->STRcolor4        ); a->STRcolor4         = NULL; }
  if (a->STRfillcolor1     != NULL) { free(a->STRfillcolor1    ); a->STRfillcolor1     = NULL; }
  if (a->STRfillcolor2     != NULL) { free(a->STRfillcolor2    ); a->STRfillcolor2     = NULL; }
  if (a->STRfillcolor3     != NULL) { free(a->STRfillcolor3    ); a->STRfillcolor3     = NULL; }
  if (a->STRfillcolor4     != NULL) { free(a->STRfillcolor4    ); a->STRfillcolor4     = NULL; }
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
  if (in->STRcolor1        != NULL) { out->STRcolor1        = (char   *)XWWMALLOC(strlen(in->STRcolor1       )+1); strcpy(out->STRcolor1       , in->STRcolor1       ); }
  if (in->STRcolor2        != NULL) { out->STRcolor2        = (char   *)XWWMALLOC(strlen(in->STRcolor2       )+1); strcpy(out->STRcolor2       , in->STRcolor2       ); }
  if (in->STRcolor3        != NULL) { out->STRcolor3        = (char   *)XWWMALLOC(strlen(in->STRcolor3       )+1); strcpy(out->STRcolor3       , in->STRcolor3       ); }
  if (in->STRcolor4        != NULL) { out->STRcolor4        = (char   *)XWWMALLOC(strlen(in->STRcolor4       )+1); strcpy(out->STRcolor4       , in->STRcolor4       ); }
  if (in->STRfillcolor1    != NULL) { out->STRfillcolor1    = (char   *)XWWMALLOC(strlen(in->STRfillcolor1   )+1); strcpy(out->STRfillcolor1   , in->STRfillcolor1   ); }
  if (in->STRfillcolor2    != NULL) { out->STRfillcolor2    = (char   *)XWWMALLOC(strlen(in->STRfillcolor2   )+1); strcpy(out->STRfillcolor2   , in->STRfillcolor2   ); }
  if (in->STRfillcolor3    != NULL) { out->STRfillcolor3    = (char   *)XWWMALLOC(strlen(in->STRfillcolor3   )+1); strcpy(out->STRfillcolor3   , in->STRfillcolor3   ); }
  if (in->STRfillcolor4    != NULL) { out->STRfillcolor4    = (char   *)XWWMALLOC(strlen(in->STRfillcolor4   )+1); strcpy(out->STRfillcolor4   , in->STRfillcolor4   ); }
  return;
 }

