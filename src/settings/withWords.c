// withWords.c
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
#include "userspace/pplObj.h"

// -----------------------------------------------
// ROUTINES FOR MANIPULATING WITH_WORDS EXPUCTURES
// -----------------------------------------------

void ppl_withWordsZero(ppl_context *context, withWords *a)
 {
  a->color = a->fillcolor = a->linespoints = a->linetype = a->pointtype = a->style = a->Col1234Space = a->FillCol1234Space = 0;
  a->linewidth = a->pointlinewidth = a->pointsize = 1.0;
  a->color1 = a->color2 = a->color3 = a->color4 = a->fillcolor1 = a->fillcolor2 = a->fillcolor3 = a->fillcolor4 = 0.0;
  a->EXPlinetype = a->EXPlinewidth = a->EXPpointlinewidth = a->EXPpointsize = a->EXPpointtype = NULL;
  a->EXPcolor = a->EXPfillcolor = NULL;
  a->USEcolor = a->USEfillcolor = a->USElinespoints = a->USElinetype = a->USElinewidth = a->USEpointlinewidth = a->USEpointsize = a->USEpointtype = a->USEstyle = a->USEcolor1234 = a->USEfillcolor1234 = 0;
  a->AUTOcolor = a->AUTOlinetype = a->AUTOpointtype = 0;
  return;
 }

void ppl_withWordsFromDict(ppl_context *context, pplObj *in, const int *ptab, withWords *out)
 {
  int      tempint, i;
  double   tempdbl;
  char    *tempstr;
  pplExpr *tempexp;
  ppl_withWordsZero(context, out, MallocNew);

  // read color names
  ppl_colorFromDict(context, in,""    ,&out->color    ,&out->Col1234Space    ,&out->color1        ,&out->color2        ,&out->color3        ,&out->color4       ,
                                                          &out->EXPcolor     ,&out->EXPcolor1     ,&out->EXPcolor2     ,&out->EXPcolor3     ,&out->EXPcolor4    ,
                    &out->USEcolor    ,&out->USEcolor1234    ,&i,MallocNew);
  ppl_colorFromDict(context, in,"fill",&out->fillcolor,&out->FillCol1234Space,&out->fillcolor1    ,&out->fillcolor2    ,&out->fillcolor3    ,&out->fillcolor4   ,
                                                       &out->EXPfillcolor    ,&out->EXPfillcolor1 ,&out->EXPfillcolor2 ,&out->EXPfillcolor3 ,&out->EXPfillcolor4,
                    &out->USEfillcolor,&out->USEfillcolor1234,&i,MallocNew);

  // Other settings
  pos     = ptab[PARSE_INDEX_linetype]; tempint = (int)round(in[pos]->real); got = (in[pos]->objType==PPLOBJ_NUM);
  if (got) { out->linetype = tempint; out->USElinetype = 1; }
  pos     = ptab[PARSE_INDEX_linetype_string]; tempexp = (pplExpr *)in[pos]->auxil; got = (in[pos]->objType==PPLOBJ_EXP);
  if (got) { out->EXPlinetype       = tempexp; tempexp->refCount++; }
  pos     = ptab[PARSE_INDEX_linewidth]; tempdbl = in[pos]->real; got = (in[pos]->objType==PPLOBJ_NUM);
  if (got) { out->linewidth = tempdbl; out->USElinewidth = 1; }
  pos     = ptab[PARSE_INDEX_linewidth_string]; tempexp = (pplExpr *)in[pos]->auxil; got = (in[pos]->objType==PPLOBJ_EXP);
  if (got) { out->EXPlinewidth      = tempexp; tempexp->refCount++; }
  pos     = ptab[PARSE_INDEX_pointsize]; tempdbl = in[pos]->real; got = (in[pos]->objType==PPLOBJ_NUM);
  if (got) { out->pointsize = tempdbl; out->USEpointsize = 1; }
  pos     = ptab[PARSE_INDEX_pointsize_string]; tempexp = (pplExpr *)in[pos]->auxil; got = (in[pos]->objType==PPLOBJ_EXP);
  if (got) { out->EXPpointsize      = tempexp; tempexp->refCount++; }
  pos     = ptab[PARSE_INDEX_pointtype]; tempint = (int)round(in[pos]->real); got = (in[pos]->objType==PPLOBJ_NUM);
  if (got) { out->pointtype = tempint; out->USEpointtype = 1; }
  pos     = ptab[PARSE_INDEX_pointtype_string]; tempexp = (pplExpr *)in[pos]->auxil; got = (in[pos]->objType==PPLOBJ_EXP);
  if (got) { out->EXPpointtype      = tempexp; tempexp->refCount++; }
  pos     = ptab[PARSE_INDEX_style_number]; tempint = (int)round(in[pos]->real); got = (in[pos]->objType==PPLOBJ_NUM);
  if (got) { out->style = tempint; out->USEstyle = 1; }
  pos     = ptab[PARSE_INDEX_pointlinewidth]; tempdbl = in[pos]->real; got = (in[pos]->objType==PPLOBJ_NUM);
  if (got) { out->pointlinewidth = tempdbl; out->USEpointlinewidth = 1; }
  pos     = ptab[PARSE_INDEX_pointlinewidth_string]; tempexp = (pplExpr *)in[pos]->auxil; got = (in[pos]->objType==PPLOBJ_EXP);
  if (got) { out->EXPpointlinewidth = tempexp; tempexp->refCount++; }
  pos     = ptab[PARSE_INDEX_style]; tempstr = (char *)in[pos]->auxil; got = (in[pos]->objType==PPLOBJ_EXP);
  if (got)
   {
    i = ppl_fetchSettingByName(&context->errcontext, tempstr, SW_STYLE_INT, SW_STYLE_EXP);
    out->linespoints = i;
    out->USElinespoints = 1;
   }
  return;
 }


int ppl_withWordsCmp_zero(ppl_context *context, const withWords *a)
 {
  if (a->EXPlinetype != NULL) return 0;
  if (a->EXPlinewidth != NULL) return 0;
  if (a->EXPpointlinewidth != NULL) return 0;
  if (a->EXPpointsize != NULL) return 0;
  if (a->EXPpointtype != NULL) return 0;
  if (a->EXPcolor != NULL) return 0;
  if (a->EXPfillcolor != NULL) return 0;
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
  if ((a->EXPcolor1        ==NULL) != (b->EXPcolor1       ==NULL)                                                                             ) return 0;
  if ((a->EXPcolor4        ==NULL) != (b->EXPcolor4       ==NULL)                                                                             ) return 0;
  if ((a->EXPcolor1        ==NULL)                                 &&                           (a->USEcolor1234      != b->USEcolor1234     )) return 0;
  if ((a->EXPcolor1        ==NULL)                                 && ( a->USEcolor1234    ) && (a->Col1234Space      != b->Col1234Space     )) return 0;
  if ((a->EXPcolor1        ==NULL)                                 && (!a->USEcolor1234    ) && (a->USEcolor          != b->USEcolor         )) return 0;
  if ((a->EXPfillcolor1    ==NULL) != (b->EXPfillcolor1   ==NULL)                                                                             ) return 0;
  if ((a->EXPfillcolor4    ==NULL) != (b->EXPfillcolor4   ==NULL)                                                                             ) return 0;
  if ((a->EXPfillcolor1    ==NULL)                                 &&                            (a->USEfillcolor1234 != b->USEfillcolor1234 )) return 0;
  if ((a->EXPfillcolor1    ==NULL)                                 && ( a->USEfillcolor1234) && (a->FillCol1234Space  != b->FillCol1234Space )) return 0;
  if ((a->EXPfillcolor1    ==NULL)                                 && (!a->USEfillcolor1234) && (a->USEfillcolor      != b->USEfillcolor     )) return 0;
  if (                                                                                          (a->USElinespoints    != b->USElinespoints   )) return 0;
  if ((a->EXPlinetype      ==NULL) != (b->EXPlinetype      ==NULL)                                                                            ) return 0;
  if ((a->EXPlinetype      ==NULL)                                                           && (a->USElinetype       != b->USElinetype      )) return 0;
  if ((a->EXPlinewidth     ==NULL) != (b->EXPlinewidth     ==NULL)                                                                            ) return 0;
  if ((a->EXPlinewidth     ==NULL)                                                           && (a->USElinewidth      != b->USElinewidth     )) return 0;
  if ((a->EXPpointlinewidth==NULL) != (b->EXPpointlinewidth==NULL)                                                                            ) return 0;
  if ((a->EXPpointlinewidth==NULL)                                                           && (a->USEpointlinewidth != b->USEpointlinewidth)) return 0;
  if ((a->EXPpointsize     ==NULL) != (b->EXPpointsize     ==NULL)                                                                            ) return 0;
  if ((a->EXPpointsize     ==NULL)                                                           && (a->USEpointsize      != b->USEpointsize     )) return 0;
  if ((a->EXPpointtype     ==NULL) != (b->EXPpointtype     ==NULL)                                                                            ) return 0;
  if ((a->EXPpointtype     ==NULL)                                                           && (a->USEpointtype      != b->USEpointtype     )) return 0;
  if (                                                                                          (a->USEstyle          != b->USEstyle         )) return 0;

  // Check that the actual values are the same in both structures
  if (a->EXPcolor1 != NULL)
   {
    if ((strcmp(a->EXPcolor1,b->EXPcolor1)!=0)||(strcmp(a->EXPcolor2,b->EXPcolor2)!=0)||(strcmp(a->EXPcolor3,b->EXPcolor3)!=0)) return 0;
    if (a->Col1234Space!=b->Col1234Space) return 0;
    if ((a->EXPcolor4 != NULL) && ((strcmp(a->EXPcolor4,b->EXPcolor4)!=0))) return 0;
   }
  else if ((a->USEcolor1234           ) && ((a->color1!=b->color1)||(a->color2!=b->color2)||(a->color3!=b->color3)||(a->color4!=b->color4)||(a->Col1234Space!=b->Col1234Space))) return 0;
  else if ((a->USEcolor               ) && ((a->color !=b->color ))) return 0;
  if (a->EXPfillcolor1 != NULL)
   {
    if ((strcmp(a->EXPfillcolor1,b->EXPfillcolor1)!=0)||(strcmp(a->EXPfillcolor2,b->EXPfillcolor2)!=0)||(strcmp(a->EXPfillcolor3,b->EXPfillcolor3)!=0)) return 0;
    if (a->FillCol1234Space!=b->FillCol1234Space) return 0;
    if ((a->EXPfillcolor4 != NULL) && ((strcmp(a->EXPfillcolor4,b->EXPfillcolor4)!=0))) return 0;
   }
  else if ((a->USEfillcolor1234       ) && ((a->fillcolor1!=b->fillcolor1)||(a->fillcolor2!=b->fillcolor2)||(a->fillcolor3!=b->fillcolor3)||(a->fillcolor4!=b->fillcolor4)||(a->FillCol1234Space!=b->FillCol1234Space))) return 0;
  else if ((a->USEfillcolor           ) && ((a->fillcolor     !=b->fillcolor     ))) return 0;
  if      ((a->USElinespoints         ) && ((a->linespoints   !=b->linespoints   ))) return 0;
  if      ((a->EXPlinetype      !=NULL) && ((strcmp(a->EXPlinetype      ,b->EXPlinetype      )!=0))) return 0;
  else if ((a->USElinetype            ) && ((a->linetype      !=b->linetype      ))) return 0;
  if      ((a->EXPlinewidth     !=NULL) && ((strcmp(a->EXPlinewidth     ,b->EXPlinewidth     )!=0))) return 0;
  else if ((a->USElinewidth           ) && ((a->linewidth     !=b->linewidth     ))) return 0;
  if      ((a->EXPpointlinewidth!=NULL) && ((strcmp(a->EXPpointlinewidth,b->EXPpointlinewidth)!=0))) return 0;
  else if ((a->USEpointlinewidth      ) && ((a->pointlinewidth!=b->pointlinewidth))) return 0;
  if      ((a->EXPpointsize     !=NULL) && ((strcmp(a->EXPpointsize     ,b->EXPpointsize     )!=0))) return 0;
  else if ((a->USEpointsize           ) && ((a->pointsize     !=b->pointsize     ))) return 0;
  if      ((a->EXPpointtype     !=NULL) && ((strcmp(a->EXPpointtype     ,b->EXPpointtype     )!=0))) return 0;
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
    if (x->USEcolor1234          ) { out->color1 = x->color1; out->color2 = x->color2; out->color3 = x->color3; out->color4 = x->color4; out->Col1234Space = x->Col1234Space; out->USEcolor1234 = 1; out->USEcolor = 0; out->EXPcolor1 = out->EXPcolor2 = out->EXPcolor3 = out->EXPcolor4 = NULL; out->AUTOcolor = x->AUTOcolor; }
    if (x->USEcolor              ) { out->color = x->color; out->USEcolor = 1; out->USEcolor1234 = 0; out->EXPcolor1 = out->EXPcolor2 = out->EXPcolor3 = out->EXPcolor4 = NULL; out->AUTOcolor = x->AUTOcolor; }
    if (x->EXPcolor        !=NULL) { out->EXPcolor = x->EXPcolor; }
    if (x->EXPcolor1       !=NULL) { out->EXPcolor1 = x->EXPcolor1; out->EXPcolor2 = x->EXPcolor2; out->EXPcolor3 = x->EXPcolor3; out->EXPcolor4 = x->EXPcolor4; out->Col1234Space = x->Col1234Space; }
    if (x->EXPfillcolor1   !=NULL) { out->EXPfillcolor1 = x->EXPfillcolor1; out->EXPfillcolor2 = x->EXPfillcolor2; out->EXPfillcolor3 = x->EXPfillcolor3; out->EXPfillcolor4 = x->EXPfillcolor4; out->FillCol1234Space = x->FillCol1234Space; out->USEfillcolor1234 = 0; out->USEfillcolor = 0; }
    if (x->USEfillcolor1234      ) { out->fillcolor1 = x->fillcolor1; out->fillcolor2 = x->fillcolor2; out->fillcolor3 = x->fillcolor3; out->fillcolor4 = x->fillcolor4; out->FillCol1234Space = x->FillCol1234Space; out->USEfillcolor1234 = 1; out->USEfillcolor = 0; out->EXPfillcolor1 = out->EXPfillcolor2 = out->EXPfillcolor3 = out->EXPfillcolor4 = NULL; }
    if (x->USEfillcolor          ) { out->fillcolor = x->fillcolor; out->USEfillcolor = 1; out->USEfillcolor1234 = 0; out->EXPfillcolor1 = out->EXPfillcolor2 = out->EXPfillcolor3 = out->EXPfillcolor4 = NULL; }
    if (x->USElinespoints         ) { out->linespoints = x->linespoints; out->USElinespoints = 1; }
    if (x->USElinetype            ) { out->linetype = x->linetype; out->USElinetype = 1; out->EXPlinetype = NULL; out->AUTOlinetype = x->AUTOlinetype; }
    if (x->EXPlinetype      !=NULL) { out->EXPlinetype = x->EXPlinetype; }
    if (x->EXPlinewidth     !=NULL) { out->EXPlinewidth = x->EXPlinewidth; out->USElinewidth = 0; }
    if (x->USElinewidth           ) { out->linewidth = x->linewidth; out->USElinewidth = 1; out->EXPlinewidth = NULL; }
    if (x->EXPpointlinewidth!=NULL) { out->EXPpointlinewidth = x->EXPpointlinewidth; out->USEpointlinewidth = 0; }
    if (x->USEpointlinewidth      ) { out->pointlinewidth = x->pointlinewidth; out->USEpointlinewidth = 1; out->EXPpointlinewidth = NULL; }
    if (x->EXPpointsize     !=NULL) { out->EXPpointsize = x->EXPpointsize; }
    if (x->USEpointsize           ) { out->pointsize = x->pointsize; out->USEpointsize = 1; out->EXPpointsize = NULL; }
    if (x->USEpointtype           ) { out->pointtype = x->pointtype; out->USEpointtype = 1; out->EXPpointtype = NULL; out->AUTOpointtype = x->AUTOpointtype; }
    if (x->EXPpointtype     !=NULL) { out->EXPpointtype = x->EXPpointtype; }
   }
  return;
 }

#define NUMDISP(X) ppl_numericDisplay(X,context->numdispBuff[0],context->set->term_current.SignificantFigures,(context->set->term_current.NumDisplay==SW_DISPLAY_L))

#define S_RGB(X,Y) (char *)ppl_numericDisplay(X,context->numdispBuff[Y],context->set->term_current.SignificantFigures,(context->set->term_current.NumDisplay==SW_DISPLAY_L))

void ppl_withWordsPrint(ppl_context *context, const withWords *defn, char *out)
 {
  int i=0;
  if      (defn->USElinespoints)          { sprintf(out+i, "%s "            , *(char **)ppl_fetchSettingName(&context->errcontext,defn->linespoints, SW_STYLE_INT , (void *)SW_STYLE_EXP , sizeof(char *))); }
  if      (defn->EXPcolor!=NULL)          { sprintf(out+i, "color %s", defn->EXPcolor->auxil); }
  else if (defn->USEcolor1234)
   {
    if      (defn->Col1234Space==SW_COLSPACE_RGB ) sprintf(out+i, "color rgb%s:%s:%s "     , S_RGB(defn->color1,0), S_RGB(defn->color2,1), S_RGB(defn->color3,2));
    else if (defn->Col1234Space==SW_COLSPACE_HSB ) sprintf(out+i, "color hsb%s:%s:%s "     , S_RGB(defn->color1,0), S_RGB(defn->color2,1), S_RGB(defn->color3,2));
    else if (defn->Col1234Space==SW_COLSPACE_CMYK) sprintf(out+i, "color cmyk%s:%s:%s:%s " , S_RGB(defn->color1,0), S_RGB(defn->color2,1), S_RGB(defn->color3,2), S_RGB(defn->color4,3));
   }
  else if (defn->USEcolor)               { sprintf(out+i, "color %s "     , *(char **)ppl_fetchSettingName(&context->errcontext,defn->color     , SW_COLOR_INT, (void *)SW_COLOR_EXP, sizeof(char *))); }
  i += strlen(out+i);
  if      (defn->EXPfillcolor!=NULL)     { sprintf(out+i, "fillcolor %s", defn->EXPfillcolor->auxil); }
  else if (defn->USEfillcolor1234)
   {
    if      (defn->Col1234Space==SW_COLSPACE_RGB ) sprintf(out+i, "fillcolor rgb%s:%s:%s " , S_RGB(defn->fillcolor1,0), S_RGB(defn->fillcolor2,1), S_RGB(defn->fillcolor3,2));
    else if (defn->Col1234Space==SW_COLSPACE_HSB ) sprintf(out+i, "fillcolor hsb%s:%s:%s " , S_RGB(defn->fillcolor1,0), S_RGB(defn->fillcolor2,1), S_RGB(defn->fillcolor3,2));
    else if (defn->Col1234Space==SW_COLSPACE_CMYK) sprintf(out+i, "fillcolor cmyk%s:%s:%s:%s " , S_RGB(defn->fillcolor1,0), S_RGB(defn->fillcolor2,1), S_RGB(defn->fillcolor3,2), S_RGB(defn->fillcolor4,2));
   }
  else if (defn->USEfillcolor)            { sprintf(out+i, "fillcolor %s " , *(char **)ppl_fetchSettingName(&context->errcontext,defn->fillcolor , SW_COLOR_INT, (void *)SW_COLOR_EXP, sizeof(char *))); }
  i += strlen(out+i);
  if      (defn->EXPlinetype!=NULL)       { sprintf(out+i, "linetype %s "            , defn->EXPlinetype->auxil);                                                                   i += strlen(out+i); }
  else if (defn->USElinetype)             { sprintf(out+i, "linetype %d "            , defn->linetype);                                                                             i += strlen(out+i); }
  if      (defn->EXPlinewidth!=NULL)      { sprintf(out+i, "linewidth %s "           , defn->EXPlinewidth->auxil);                                                                  i += strlen(out+i); }
  else if (defn->USElinewidth)            { sprintf(out+i, "linewidth %s "           , NUMDISP(defn->linewidth));                                                                   i += strlen(out+i); }
  if      (defn->EXPpointlinewidth!=NULL) { sprintf(out+i, "pointlinewidth %s "      , defn->EXPpointlinewidth->auxil);                                                             i += strlen(out+i); }
  else if (defn->USEpointlinewidth)       { sprintf(out+i, "pointlinewidth %s "      , NUMDISP(defn->pointlinewidth));                                                              i += strlen(out+i); }
  if      (defn->EXPpointsize!=NULL)      { sprintf(out+i, "pointsize %s "           , defn->EXPpointsize->auxil);                                                                  i += strlen(out+i); }
  else if (defn->USEpointsize)            { sprintf(out+i, "pointsize %s "           , NUMDISP(defn->pointsize));                                                                   i += strlen(out+i); }
  if      (defn->EXPpointtype!=NULL)      { sprintf(out+i, "pointtype %s "           , defn->EXPpointtype->auxil);                                                                  i += strlen(out+i); }
  else if (defn->USEpointtype)            { sprintf(out+i, "pointtype %d "           , defn->pointtype);                                                                            i += strlen(out+i); }
  if      (defn->USEstyle)                { sprintf(out+i, "style %d "               , defn->style);                                                                                i += strlen(out+i); }
  out[i]='\0';
  return;
 }

void ppl_withWordsDestroy(ppl_context *context, withWords *a)
 {
  if (!a->malloced) return;
  if (a->EXPlinetype       != NULL) { pplExpr_free(a->EXPlinetype      ); a->EXPlinetype       = NULL; }
  if (a->EXPlinewidth      != NULL) { pplExpr_free(a->EXPlinewidth     ); a->EXPlinewidth      = NULL; }
  if (a->EXPpointlinewidth != NULL) { pplExpr_free(a->EXPpointlinewidth); a->EXPpointlinewidth = NULL; }
  if (a->EXPpointsize      != NULL) { pplExpr_free(a->EXPpointsize     ); a->EXPpointsize      = NULL; }
  if (a->EXPpointtype      != NULL) { pplExpr_free(a->EXPpointtype     ); a->EXPpointtype      = NULL; }
  if (a->EXPcolor          != NULL) { pplExpr_free(a->EXPcolor         ); a->EXPcolor          = NULL; }
  if (a->EXPfillcolor      != NULL) { pplExpr_free(a->EXPfillcolor     ); a->EXPfillcolor      = NULL; }
  return;
 }

void ppl_withWordsCpy(ppl_context *context, withWords *out, const withWords *in)
 {
  void *tmp;
  *out = *in;
  out->malloced = 1;
  if (in->EXPlinetype      != NULL) { out->EXPlinetype      = in->EXPlinetype      ; out->EXPlinetype      ->refCount++; }
  if (in->EXPlinewidth     != NULL) { out->EXPlinewidth     = in->EXPlinewidth     ; out->EXPlinewidth     ->refCount++; }
  if (in->EXPpointlinewidth!= NULL) { out->EXPpointlinewidth= in->EXPpointlinewidth; out->EXPpointlinewidth->refCount++; }
  if (in->EXPpointsize     != NULL) { out->EXPpointsize     = in->EXPpointsize     ; out->EXPpointsize     ->refCount++; }
  if (in->EXPpointtype     != NULL) { out->EXPpointtype     = in->EXPpointtype     ; out->EXPpointtype     ->refCount++; }
  if (in->EXPcolor         != NULL) { out->EXPcolor         = in->EXPcolor         ; out->EXPcolor         ->refCount++; }
  if (in->EXPfillcolor     != NULL) { out->EXPfillcolor     = in->EXPfillcolor     ; out->EXPfillcolor     ->refCount++; }
  return;
 }

