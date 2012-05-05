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
#include <math.h>

#include "stringTools/asciidouble.h"
#include "stringTools/strConstants.h"

#include "pplConstants.h"
#include "coreUtils/errorReport.h"
#include "expressions/expCompile_fns.h"
#include "parser/cmdList.h"
#include "parser/parser.h"
#include "settings/colors.h"
#include "settings/epsColors.h"
#include "settings/settings.h"
#include "settings/settingTypes.h"
#include "settings/withWords_fns.h"

#include "userspace/context.h"
#include "userspace/pplObj.h"

// -----------------------------------------------
// ROUTINES FOR MANIPULATING WITH_WORDS STRUCTURES
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

void ppl_withWordsFromDict(ppl_context *context, parserOutput *in, parserLine *pl, const int *ptab, withWords *out)
 {
  pplObj  *stk = in->stk;
  int      tempint, i, pos, got;
  double   tempdbl;
  char    *tempstr;
  pplExpr *tempexp;
  ppl_withWordsZero(context, out);

  // read color names
  ppl_colorFromDict(context, in, pl, ptab, 0, &out->color, &out->Col1234Space, &out->EXPcolor, &out->color1, &out->color2, &out->color3, &out->color4, &out->USEcolor, &out->USEcolor1234);
  ppl_colorFromDict(context, in, pl, ptab, 1, &out->fillcolor, &out->FillCol1234Space, &out->EXPfillcolor, &out->fillcolor1, &out->fillcolor2, &out->fillcolor3, &out->fillcolor4, &out->USEfillcolor, &out->USEfillcolor1234);

  // Other settings
  pos     = ptab[PARSE_INDEX_linetype      ];  got = (pos>=0) && (stk[pos].objType==PPLOBJ_NUM);
  if (got) { tempint = (int)round(stk[pos].real); out->linetype          = tempint; out->USElinetype = 1; }
  pos     = ptab[PARSE_INDEX_linetype      ];  got = (pos>=0) && (stk[pos].objType==PPLOBJ_EXP);
  if (got) { tempexp = (pplExpr *)stk[pos].auxil; out->EXPlinetype       = tempexp; tempexp->refCount++; }
  pos     = ptab[PARSE_INDEX_linewidth     ];  got = (pos>=0) && (stk[pos].objType==PPLOBJ_NUM);
  if (got) { tempdbl = stk[pos].real;             out->linewidth         = tempdbl; out->USElinewidth = 1; }
  pos     = ptab[PARSE_INDEX_linewidth     ];  got = (pos>=0) && (stk[pos].objType==PPLOBJ_EXP);
  if (got) { tempexp = (pplExpr *)stk[pos].auxil; out->EXPlinewidth      = tempexp; tempexp->refCount++; }
  pos     = ptab[PARSE_INDEX_pointsize     ];  got = (pos>=0) && (stk[pos].objType==PPLOBJ_NUM);
  if (got) { tempdbl = stk[pos].real;             out->pointsize         = tempdbl; out->USEpointsize = 1; }
  pos     = ptab[PARSE_INDEX_pointsize     ];  got = (pos>=0) && (stk[pos].objType==PPLOBJ_EXP);
  if (got) { tempexp = (pplExpr *)stk[pos].auxil; out->EXPpointsize      = tempexp; tempexp->refCount++; }
  pos     = ptab[PARSE_INDEX_pointtype     ];  got = (pos>=0) && (stk[pos].objType==PPLOBJ_NUM);
  if (got) { tempint = (int)round(stk[pos].real); out->pointtype         = tempint; out->USEpointtype = 1; }
  pos     = ptab[PARSE_INDEX_pointtype     ];  got = (pos>=0) && (stk[pos].objType==PPLOBJ_EXP);
  if (got) { tempexp = (pplExpr *)stk[pos].auxil; out->EXPpointtype      = tempexp; tempexp->refCount++; }
  pos     = ptab[PARSE_INDEX_style_number  ];  got = (pos>=0) && (stk[pos].objType==PPLOBJ_NUM);
  if (got) { tempint = (int)round(stk[pos].real); out->style             = tempint; out->USEstyle = 1; }
  pos     = ptab[PARSE_INDEX_pointlinewidth];  got = (pos>=0) && (stk[pos].objType==PPLOBJ_NUM);
  if (got) { tempdbl = stk[pos].real;             out->pointlinewidth    = tempdbl; out->USEpointlinewidth = 1; }
  pos     = ptab[PARSE_INDEX_pointlinewidth];  got = (pos>=0) && (stk[pos].objType==PPLOBJ_EXP);
  if (got) { tempexp = (pplExpr *)stk[pos].auxil; out->EXPpointlinewidth = tempexp; tempexp->refCount++; }
  pos     = ptab[PARSE_INDEX_style         ];  got = (pos>=0) && (stk[pos].objType==PPLOBJ_STR);
  if (got)
   {
    tempstr = (char *)stk[pos].auxil;
    i = ppl_fetchSettingByName(&context->errcontext, tempstr, SW_STYLE_INT, SW_STYLE_STR);
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
  if ((a->EXPcolor         ==NULL) != (b->EXPcolor         ==NULL)                                                   ) return 0;
  if ((a->EXPfillcolor     ==NULL) != (b->EXPfillcolor     ==NULL)                                                   ) return 0;
  if (                                                                 (a->USElinespoints    != b->USElinespoints   )) return 0;
  if ((a->EXPlinetype      ==NULL) != (b->EXPlinetype      ==NULL)                                                   ) return 0;
  if ((a->EXPlinetype      ==NULL)                                  && (a->USElinetype       != b->USElinetype      )) return 0;
  if ((a->EXPlinewidth     ==NULL) != (b->EXPlinewidth     ==NULL)                                                   ) return 0;
  if ((a->EXPlinewidth     ==NULL)                                  && (a->USElinewidth      != b->USElinewidth     )) return 0;
  if ((a->EXPpointlinewidth==NULL) != (b->EXPpointlinewidth==NULL)                                                   ) return 0;
  if ((a->EXPpointlinewidth==NULL)                                  && (a->USEpointlinewidth != b->USEpointlinewidth)) return 0;
  if ((a->EXPpointsize     ==NULL) != (b->EXPpointsize     ==NULL)                                                   ) return 0;
  if ((a->EXPpointsize     ==NULL)                                  && (a->USEpointsize      != b->USEpointsize     )) return 0;
  if ((a->EXPpointtype     ==NULL) != (b->EXPpointtype     ==NULL)                                                   ) return 0;
  if ((a->EXPpointtype     ==NULL)                                  && (a->USEpointtype      != b->USEpointtype     )) return 0;
  if (                                                                 (a->USEstyle          != b->USEstyle         )) return 0;

  // Check that the actual values are the same in both structures
  if (a->EXPcolor != NULL)
   {
    if ((strcmp(a->EXPcolor->ascii,b->EXPcolor->ascii)!=0)) return 0;
    if (a->Col1234Space!=b->Col1234Space) return 0;
   }
  else if ((a->USEcolor1234           ) && ((a->color1!=b->color1)||(a->color2!=b->color2)||(a->color3!=b->color3)||(a->color4!=b->color4)||(a->Col1234Space!=b->Col1234Space))) return 0;
  else if ((a->USEcolor               ) && ((a->color !=b->color ))) return 0;
  if (a->EXPfillcolor != NULL)
   {
    if ((strcmp(a->EXPfillcolor->ascii,b->EXPfillcolor->ascii)!=0)) return 0;
    if (a->FillCol1234Space!=b->FillCol1234Space) return 0;
   }
  else if ((a->USEfillcolor1234       ) && ((a->fillcolor1!=b->fillcolor1)||(a->fillcolor2!=b->fillcolor2)||(a->fillcolor3!=b->fillcolor3)||(a->fillcolor4!=b->fillcolor4)||(a->FillCol1234Space!=b->FillCol1234Space))) return 0;
  else if ((a->USEfillcolor           ) && ((a->fillcolor     !=b->fillcolor     ))) return 0;
  if      ((a->USElinespoints         ) && ((a->linespoints   !=b->linespoints   ))) return 0;
  if      ((a->EXPlinetype      !=NULL) && ((strcmp(a->EXPlinetype->ascii      ,b->EXPlinetype->ascii      )!=0))) return 0;
  else if ((a->USElinetype            ) && ((a->linetype      !=b->linetype      ))) return 0;
  if      ((a->EXPlinewidth     !=NULL) && ((strcmp(a->EXPlinewidth->ascii     ,b->EXPlinewidth->ascii     )!=0))) return 0;
  else if ((a->USElinewidth           ) && ((a->linewidth     !=b->linewidth     ))) return 0;
  if      ((a->EXPpointlinewidth!=NULL) && ((strcmp(a->EXPpointlinewidth->ascii,b->EXPpointlinewidth->ascii)!=0))) return 0;
  else if ((a->USEpointlinewidth      ) && ((a->pointlinewidth!=b->pointlinewidth))) return 0;
  if      ((a->EXPpointsize     !=NULL) && ((strcmp(a->EXPpointsize->ascii     ,b->EXPpointsize->ascii     )!=0))) return 0;
  else if ((a->USEpointsize           ) && ((a->pointsize     !=b->pointsize     ))) return 0;
  if      ((a->EXPpointtype     !=NULL) && ((strcmp(a->EXPpointtype->ascii     ,b->EXPpointtype->ascii     )!=0))) return 0;
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

  ppl_withWordsZero(context,out);

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
    if (x->USEcolor1234           ) { out->color1 = x->color1; out->color2 = x->color2; out->color3 = x->color3; out->color4 = x->color4; out->Col1234Space = x->Col1234Space; out->USEcolor1234 = 1; out->USEcolor = 0; pplExpr_free(out->EXPcolor); out->EXPcolor = NULL; out->AUTOcolor = x->AUTOcolor; }
    if (x->USEcolor               ) { out->color = x->color; out->USEcolor = 1; out->USEcolor1234 = 0; pplExpr_free(out->EXPcolor); out->EXPcolor = NULL; out->AUTOcolor = x->AUTOcolor; }
    if (x->EXPcolor         !=NULL) { pplExpr_free(out->EXPcolor); out->EXPcolor = x->EXPcolor; out->EXPcolor->refCount++; out->USEcolor = 0; out->USEcolor1234 = 0; out->AUTOcolor = x->AUTOcolor; }
    if (x->USEfillcolor1234       ) { out->fillcolor1 = x->fillcolor1; out->fillcolor2 = x->fillcolor2; out->fillcolor3 = x->fillcolor3; out->fillcolor4 = x->fillcolor4; out->FillCol1234Space = x->FillCol1234Space; out->USEfillcolor1234 = 1; out->USEfillcolor = 0; pplExpr_free(out->EXPfillcolor); out->EXPfillcolor = NULL; }
    if (x->USEfillcolor           ) { out->fillcolor = x->fillcolor; out->USEfillcolor = 1; out->USEfillcolor1234 = 0; pplExpr_free(out->EXPfillcolor); out->EXPfillcolor = NULL; }
    if (x->EXPfillcolor     !=NULL) { pplExpr_free(out->EXPfillcolor); out->EXPfillcolor = x->EXPfillcolor; out->EXPfillcolor->refCount++; out->USEfillcolor = 0; out->USEfillcolor1234 = 0; }
    if (x->USElinespoints         ) { out->linespoints = x->linespoints; out->USElinespoints = 1; }
    if (x->USElinetype            ) { out->linetype = x->linetype; out->USElinetype = 1; pplExpr_free(out->EXPlinetype); out->EXPlinetype = NULL; out->AUTOlinetype = x->AUTOlinetype; }
    if (x->EXPlinetype      !=NULL) { pplExpr_free(out->EXPlinetype); out->EXPlinetype = x->EXPlinetype; out->EXPlinetype->refCount++; }
    if (x->USElinewidth           ) { out->linewidth = x->linewidth; out->USElinewidth = 1; pplExpr_free(out->EXPlinewidth); out->EXPlinewidth = NULL; }
    if (x->EXPlinewidth     !=NULL) { pplExpr_free(out->EXPlinewidth); out->EXPlinewidth = x->EXPlinewidth; out->EXPlinewidth->refCount++; out->USElinewidth = 0; }
    if (x->USEpointlinewidth      ) { out->pointlinewidth = x->pointlinewidth; out->USEpointlinewidth = 1; pplExpr_free(out->EXPpointlinewidth); out->EXPpointlinewidth = NULL; }
    if (x->EXPpointlinewidth!=NULL) { pplExpr_free(out->EXPpointlinewidth); out->EXPpointlinewidth = x->EXPpointlinewidth; out->EXPpointlinewidth->refCount++; out->USEpointlinewidth = 0; }
    if (x->USEpointsize           ) { out->pointsize = x->pointsize; out->USEpointsize = 1; pplExpr_free(out->EXPpointsize); out->EXPpointsize = NULL; }
    if (x->EXPpointsize     !=NULL) { pplExpr_free(out->EXPpointsize); out->EXPpointsize = x->EXPpointsize; out->EXPpointsize->refCount++; out->USEpointsize = 0; }
    if (x->USEpointtype           ) { out->pointtype = x->pointtype; out->USEpointtype = 1; pplExpr_free(out->EXPpointtype); out->EXPpointtype = NULL; out->AUTOpointtype = x->AUTOpointtype; }
    if (x->EXPpointtype     !=NULL) { pplExpr_free(out->EXPpointtype); out->EXPpointtype = x->EXPpointtype; out->EXPpointtype->refCount++; }
   }
  return;
 }

#define NUMDISP(X) ppl_numericDisplay(X,context->numdispBuff[0],context->set->term_current.SignificantFigures,(context->set->term_current.NumDisplay==SW_DISPLAY_L))

#define S_RGB(X,Y) (char *)ppl_numericDisplay(X,context->numdispBuff[Y],context->set->term_current.SignificantFigures,(context->set->term_current.NumDisplay==SW_DISPLAY_L))

void ppl_withWordsPrint(ppl_context *context, const withWords *defn, char *out)
 {
  int i=0;
  out[i]='\0';
  if      (defn->USElinespoints)     { sprintf(out+i, "%s ", *(char **)ppl_fetchSettingName(&context->errcontext,defn->linespoints, SW_STYLE_INT , (void *)SW_STYLE_STR , sizeof(char *))); i += strlen(out+i); }
  if      (defn->EXPcolor!=NULL)     { sprintf(out+i, "color %s", defn->EXPcolor->ascii); }
  else if (defn->USEcolor1234)
   {
    if      (defn->Col1234Space==SW_COLSPACE_RGB ) sprintf(out+i, "color rgb(%s,%s,%s) ", S_RGB(defn->color1,0), S_RGB(defn->color2,1), S_RGB(defn->color3,2));
    else if (defn->Col1234Space==SW_COLSPACE_HSB ) sprintf(out+i, "color hsb(%s,%s,%s) ", S_RGB(defn->color1,0), S_RGB(defn->color2,1), S_RGB(defn->color3,2));
    else if (defn->Col1234Space==SW_COLSPACE_CMYK) sprintf(out+i, "color cmyk(%s,%s,%s,%s) ", S_RGB(defn->color1,0), S_RGB(defn->color2,1), S_RGB(defn->color3,2), S_RGB(defn->color4,3));
   }
  else if (defn->USEcolor)           { sprintf(out+i, "color %s ", *(char **)ppl_fetchSettingName(&context->errcontext,defn->color     , SW_COLOR_INT, (void *)SW_COLOR_STR, sizeof(char *))); }
  i += strlen(out+i);
  if      (defn->EXPfillcolor!=NULL) { sprintf(out+i, "fillcolor %s", defn->EXPfillcolor->ascii); }
  else if (defn->USEfillcolor1234)
   {
    if      (defn->FillCol1234Space==SW_COLSPACE_RGB ) sprintf(out+i, "fillcolor rgb(%s,%s,%s) ", S_RGB(defn->fillcolor1,0), S_RGB(defn->fillcolor2,1), S_RGB(defn->fillcolor3,2));
    else if (defn->FillCol1234Space==SW_COLSPACE_HSB ) sprintf(out+i, "fillcolor hsb(%s,%s,%s) ", S_RGB(defn->fillcolor1,0), S_RGB(defn->fillcolor2,1), S_RGB(defn->fillcolor3,2));
    else if (defn->FillCol1234Space==SW_COLSPACE_CMYK) sprintf(out+i, "fillcolor cmyk(%s,%s,%s,%s) ", S_RGB(defn->fillcolor1,0), S_RGB(defn->fillcolor2,1), S_RGB(defn->fillcolor3,2), S_RGB(defn->fillcolor4,2));
   }
  else if (defn->USEfillcolor)       { sprintf(out+i, "fillcolor %s ", *(char **)ppl_fetchSettingName(&context->errcontext,defn->fillcolor , SW_COLOR_INT, (void *)SW_COLOR_STR, sizeof(char *))); }
  i += strlen(out+i);
  if      (defn->EXPlinetype!=NULL)       { sprintf(out+i, "linetype %s "       , defn->EXPlinetype->ascii);         i += strlen(out+i); }
  else if (defn->USElinetype)             { sprintf(out+i, "linetype %d "       , defn->linetype);                   i += strlen(out+i); }
  if      (defn->EXPlinewidth!=NULL)      { sprintf(out+i, "linewidth %s "      , defn->EXPlinewidth->ascii);        i += strlen(out+i); }
  else if (defn->USElinewidth)            { sprintf(out+i, "linewidth %s "      , NUMDISP(defn->linewidth));         i += strlen(out+i); }
  if      (defn->EXPpointlinewidth!=NULL) { sprintf(out+i, "pointlinewidth %s " , defn->EXPpointlinewidth->ascii);   i += strlen(out+i); }
  else if (defn->USEpointlinewidth)       { sprintf(out+i, "pointlinewidth %s " , NUMDISP(defn->pointlinewidth));    i += strlen(out+i); }
  if      (defn->EXPpointsize!=NULL)      { sprintf(out+i, "pointsize %s "      , defn->EXPpointsize->ascii);        i += strlen(out+i); }
  else if (defn->USEpointsize)            { sprintf(out+i, "pointsize %s "      , NUMDISP(defn->pointsize));         i += strlen(out+i); }
  if      (defn->EXPpointtype!=NULL)      { sprintf(out+i, "pointtype %s "      , defn->EXPpointtype->ascii);        i += strlen(out+i); }
  else if (defn->USEpointtype)            { sprintf(out+i, "pointtype %d "      , defn->pointtype);                  i += strlen(out+i); }
  if      (defn->USEstyle)                { sprintf(out+i, "style %d "          , defn->style);                      i += strlen(out+i); }
  out[i]='\0';
  return;
 }

void ppl_withWordsDestroy(ppl_context *context, withWords *a)
 {
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
  *out = *in;
  if (in->EXPlinetype      != NULL) { out->EXPlinetype      = in->EXPlinetype      ; out->EXPlinetype      ->refCount++; }
  if (in->EXPlinewidth     != NULL) { out->EXPlinewidth     = in->EXPlinewidth     ; out->EXPlinewidth     ->refCount++; }
  if (in->EXPpointlinewidth!= NULL) { out->EXPpointlinewidth= in->EXPpointlinewidth; out->EXPpointlinewidth->refCount++; }
  if (in->EXPpointsize     != NULL) { out->EXPpointsize     = in->EXPpointsize     ; out->EXPpointsize     ->refCount++; }
  if (in->EXPpointtype     != NULL) { out->EXPpointtype     = in->EXPpointtype     ; out->EXPpointtype     ->refCount++; }
  if (in->EXPcolor         != NULL) { out->EXPcolor         = in->EXPcolor         ; out->EXPcolor         ->refCount++; }
  if (in->EXPfillcolor     != NULL) { out->EXPfillcolor     = in->EXPfillcolor     ; out->EXPfillcolor     ->refCount++; }
  return;
 }

