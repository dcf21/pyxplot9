// show.c
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
#include <math.h>
#include <ctype.h>

#include "coreUtils/dict.h"
#include "coreUtils/memAlloc.h"
#include "coreUtils/list.h"

#include "expressions/traceback_fns.h"

#include "parser/cmdList.h"
#include "parser/parser.h"

#include "settings/arrows_fns.h"
#include "settings/axes_fns.h"
#include "settings/epsColors.h"
#include "settings/labels_fns.h"
#include "settings/settings.h"
#include "settings/settingTypes.h"
#include "settings/textConstants.h"
#include "settings/withWords_fns.h"

#include "stringTools/asciidouble.h"

#include "userspace/context.h"
#include "userspace/pplObj_fns.h"
#include "userspace/pplObjFunc.h"
#include "userspace/pplObjPrint.h"
#include "userspace/unitsArithmetic.h"
#include "userspace/unitsDisp.h"

#include "canvasItems.h"
#include "pplConstants.h"


#define SHOW_HIGHLIGHT(modified) \
if (interactive!=0) /* On interactive sessions, highlight those settings which have been manually set by the user */ \
 { \
  if (modified == 0) strcpy(out+i, *(char **)ppl_fetchSettingName(&c->errcontext,  c->errcontext.session_default.color_wrn , SW_TERMCOL_INT , SW_TERMCOL_TXT , sizeof(char *)) ); \
  else               strcpy(out+i, *(char **)ppl_fetchSettingName(&c->errcontext,  c->errcontext.session_default.color_rep , SW_TERMCOL_INT , SW_TERMCOL_TXT , sizeof(char *)) ); \
  i += strlen(out+i); \
 }

#define SHOW_DEHIGHLIGHT \
if (interactive!=0) /* On interactive sessions, highlight those settings which have been manually set by the user */ \
 { \
  strcpy(out+i, *(char **)ppl_fetchSettingName(&c->errcontext,  SW_TERMCOL_NOR , SW_TERMCOL_INT , SW_TERMCOL_TXT , sizeof(char *)) ); \
  i += strlen(out+i); \
 } \


static void directive_show3(ppl_context *c, char *out, char *itemSet, unsigned char itemSetShow, int interactive, char *setting_name, char *setting_value, int modified, char *description)
 {
  int i=0,j,k;

  SHOW_HIGHLIGHT(modified);

  sprintf(out+i, "set %*s", (int)strlen(itemSet), itemSetShow ? itemSet : ""); i += strlen(out+i); // Start off with a set command

  if (strcmp(setting_value, "On")==0)
   {
    sprintf(out+i, "%-41s", setting_name);
    i += strlen(out+i);
   }
  else if (strcmp(setting_value, "Off")==0)
   {
    for (j=0,k=-1; setting_name[j]!='\0'; j++) if (setting_name[j]==' ') k=j; // Find last space in setting name
    for (j=0; j<=k; j++) out[i+j] = setting_name[j];
    out[i+k+1] = 'n'; out[i+k+2] = 'o'; // Insert 'no' after this space
    for (j=k+1; setting_name[j]!='\0'; j++) out[i+j+2] = setting_name[j];
    for (; j<39; j++) out[i+j+2] = ' '; // Pad with spaces up to 45 characters
    out[i+j+2] = '\0';
    i += strlen(out+i);
   }
  else
   {
    sprintf(out+i, "%-16s %-24s", setting_name, setting_value);
    i += strlen(out+i);
   }

  if (description!=NULL) { sprintf(out+i, " # %s.", description); i += strlen(out+i); } // Finally put a decriptive comment after the setting
  strcpy(out+i, "\n"); i += strlen(out+i); // and a linefeed

  SHOW_DEHIGHLIGHT;
  return;
 }

static int directive_show2(ppl_context *c, char *word, char *itemSet, int interactive, pplset_graph *sg, pplarrow_object **al, ppllabel_object **ll, pplset_axis *xa, pplset_axis *ya, pplset_axis *za)
 {
  char *out, *buf, *buf2, *bufp, *bufp2, temp1[32], temp2[32];
  int   i=0, p=0,j,k,l,m,n;
  unsigned char unchanged;
  unit *ud = c->unit_database;
  int outLen = 8*LSTR_LENGTH;

  out = (char *)ppl_memAlloc(outLen      ); // Accumulate our whole output text here
  buf = (char *)ppl_memAlloc(LSTR_LENGTH ); // Put the value of each setting in here
  buf2= (char *)ppl_memAlloc(FNAME_LENGTH);

  if ((out==NULL)||(buf==NULL)||(buf2==NULL))
   {
    ppl_error(&c->errcontext, ERR_MEMORY, -1, -1, "Out of memory whilst trying to allocate buffers in show command.");
    if (out!=NULL) free(out); if (buf!=NULL) free(buf); if (buf2!=NULL) free(buf2);
    return 1;
   }

  out[0] = buf[0] = '\0';
  if ((ppl_strAutocomplete(word, "settings", 1)>=0) || (ppl_strAutocomplete(word, "axescolour",1)>=0) || (ppl_strAutocomplete(word, "axescolor",1)>=0))
   {
#define S_RGB(X,Y) ppl_numericDisplay(X,c->numdispBuff[Y],c->set->term_current.SignificantFigures,(c->set->term_current.NumDisplay==SW_DISPLAY_L))
#define SHOW_COLOR(COLOR,COL1234SPACE,COL1,COL2,COL3,COL4  ,  DEFAULT,DEF1234SPACE,DEF1,DEF2,DEF3,DEF4  ,  NAME1, NAME2) \
    { \
     if      (COLOR>0)                       sprintf(buf, "%s", *(char **)ppl_fetchSettingName(&c->errcontext, COLOR, SW_COLOR_INT, SW_COLOR_STR , sizeof(char *))); \
     else if (COL1234SPACE==SW_COLSPACE_RGB ) sprintf(buf, "rgb%s:%s:%s", S_RGB(COL1,0), S_RGB(COL2,1), S_RGB(COL3,2)); \
     else if (COL1234SPACE==SW_COLSPACE_HSB ) sprintf(buf, "hsb%s:%s:%s", S_RGB(COL1,0), S_RGB(COL2,1), S_RGB(COL3,2)); \
     else if (COL1234SPACE==SW_COLSPACE_CMYK) sprintf(buf, "cmyk%s:%s:%s:%s", S_RGB(COL1,0), S_RGB(COL2,1), S_RGB(COL3,2), S_RGB(COL4,3)); \
     directive_show3(c, out+i, itemSet, 1, interactive, NAME1, buf, ((COLOR==DEFAULT)&&(COL1234SPACE==DEF1234SPACE)&&(COL1==DEF1)&&(COL2==DEF2)&&(COL3==DEF3)&&(COL4==DEF4)), NAME2); \
    }

    SHOW_COLOR(sg->AxesColour,sg->AxesCol1234Space,sg->AxesColour1,sg->AxesColour2,sg->AxesColour3,sg->AxesColour4  ,
                c->set->graph_default.AxesColour,c->set->graph_default.AxesCol1234Space,c->set->graph_default.AxesColour1,c->set->graph_default.AxesColour2,c->set->graph_default.AxesColour3,c->set->graph_default.AxesColour4  ,
                "AxesColour", "The colour used to draw graph axes");
    i += strlen(out+i) ; p=1;
   }
  if ((ppl_strAutocomplete(word, "settings", 1)>=0) || (ppl_strAutocomplete(word, "axisunitstyle", 1)>=0))
   {
    sprintf(buf, "%s", *(char **)ppl_fetchSettingName(&c->errcontext, sg->AxisUnitStyle, SW_AXISUNITSTY_INT, SW_AXISUNITSTY_STR , sizeof(char *)));
    directive_show3(c, out+i, itemSet, 0, interactive, "AxisUnitStyle", buf, (c->set->graph_default.AxisUnitStyle == sg->AxisUnitStyle), "Select how the physical units associated with axes are appended to axis labels");
    i += strlen(out+i) ; p=1;
   }
  if ((ppl_strAutocomplete(word, "settings", 1)>=0) || (ppl_strAutocomplete(word, "backup", 1)>=0))
   {
    sprintf(buf, "%s", *(char **)ppl_fetchSettingName(&c->errcontext, c->set->term_current.backup, SW_ONOFF_INT, SW_ONOFF_STR , sizeof(char *)));
    directive_show3(c, out+i, itemSet, 0, interactive, "backup", buf, (c->set->term_default.backup == c->set->term_current.backup), "Selects whether existing files are overwritten (nobackup) or moved (backup)");
    i += strlen(out+i) ; p=1;
   }
  if ((ppl_strAutocomplete(word, "settings", 1)>=0) || (ppl_strAutocomplete(word, "bar",1)>=0))
   {
    sprintf(buf, "%s", ppl_numericDisplay(sg->bar,c->numdispBuff[0],c->set->term_current.SignificantFigures,(c->set->term_current.NumDisplay==SW_DISPLAY_L)));
    directive_show3(c, out+i, itemSet, 1, interactive, "bar", buf, (c->set->graph_default.bar == sg->bar), "Sets the size of the strokes which mark the lower and upper limits of errorbars");
    i += strlen(out+i) ; p=1;
   }
  if ((ppl_strAutocomplete(word, "settings", 1)>=0) || (ppl_strAutocomplete(word, "binorigin",1)>=0))
   {
    bufp = "Used to control the exact position of the edges of the bins used by the histogram command";
    if (c->set->term_current.BinOriginAuto)
     {
      directive_show3(c, out+i, itemSet, 0, interactive, "BinOrigin", "auto", c->set->term_current.BinOriginAuto==c->set->term_default.BinOriginAuto, bufp);
     } else {
      directive_show3(c, out+i, itemSet, 0, interactive, "BinOrigin", ppl_unitsNumericDisplay(c,&(c->set->term_current.BinOrigin),0,0,0),
                      (c->set->term_current.BinOriginAuto==c->set->term_default.BinOriginAuto) &&
                      ppl_dblEqual( c->set->term_default.BinOrigin.real , c->set->term_current.BinOrigin.real) &&
                      ppl_dblEqual( c->set->term_default.BinOrigin.imag , c->set->term_current.BinOrigin.imag) &&
                      ppl_unitsDimEqual(&c->set->term_default.BinOrigin      ,&c->set->term_current.BinOrigin     )    ,
                      bufp
                     );
     }
    i += strlen(out+i) ; p=1;
   }
  if ((ppl_strAutocomplete(word, "settings", 1)>=0) || (ppl_strAutocomplete(word, "binwidth",1)>=0))
   {
    bufp = "Sets the width of bins used by the histogram command";
    if (c->set->term_current.BinWidthAuto)
     {
      directive_show3(c, out+i, itemSet, 0, interactive, "BinWidth", "auto", c->set->term_current.BinWidthAuto==c->set->term_default.BinWidthAuto, bufp);
     } else {
      directive_show3(c, out+i, itemSet, 0, interactive, "BinWidth", ppl_unitsNumericDisplay(c,&(c->set->term_current.BinWidth),0,0,0),
                      (c->set->term_current.BinWidthAuto==c->set->term_default.BinWidthAuto) &&
                      ppl_dblEqual( c->set->term_default.BinWidth.real , c->set->term_current.BinWidth.real) &&
                      ppl_dblEqual( c->set->term_default.BinWidth.imag , c->set->term_current.BinWidth.imag) &&
                      ppl_unitsDimEqual(&c->set->term_default.BinWidth      ,&c->set->term_current.BinWidth     )    ,
                      bufp
                     );
     }
    i += strlen(out+i) ; p=1;
   }
  if ((ppl_strAutocomplete(word, "settings", 1)>=0) || (ppl_strAutocomplete(word, "boxfrom",1)>=0))
   {
    bufp = "Sets the vertical level from which the bars of barcharts and histograms are drawn";
    if (sg->BoxFromAuto)
     {
      directive_show3(c, out+i, itemSet, 0, interactive, "BoxFrom", "auto", sg->BoxFromAuto==c->set->graph_default.BoxFromAuto, bufp);
     } else {
      directive_show3(c, out+i, itemSet, 0, interactive, "BoxFrom", ppl_unitsNumericDisplay(c,&(sg->BoxFrom),0,0,0),
                      (sg->BoxFromAuto==c->set->graph_default.BoxFromAuto) &&
                      ppl_dblEqual( c->set->graph_default.BoxFrom.real , sg->BoxFrom.real) &&
                      ppl_dblEqual( c->set->graph_default.BoxFrom.imag , sg->BoxFrom.imag) &&
                      ppl_unitsDimEqual(&c->set->graph_default.BoxFrom      ,&sg->BoxFrom     )    ,
                      bufp
                     );
     }
    i += strlen(out+i) ; p=1;
   }
  if ((ppl_strAutocomplete(word, "settings", 1)>=0) || (ppl_strAutocomplete(word, "boxwidth",1)>=0))
   {
    bufp = "Sets the width of bars on barcharts and histograms";
    if (sg->BoxWidthAuto)
     {
      directive_show3(c, out+i, itemSet, 0, interactive, "BoxWidth", "auto", sg->BoxWidthAuto==c->set->graph_default.BoxWidthAuto, bufp);
     } else {
      directive_show3(c, out+i, itemSet, 0, interactive, "BoxWidth", ppl_unitsNumericDisplay(c,&(sg->BoxWidth),0,0,0),
                      (sg->BoxWidthAuto==c->set->graph_default.BoxWidthAuto) &&
                      ppl_dblEqual( c->set->graph_default.BoxWidth.real , sg->BoxWidth.real) &&
                      ppl_dblEqual( c->set->graph_default.BoxWidth.imag , sg->BoxWidth.imag) &&
                      ppl_unitsDimEqual(&c->set->graph_default.BoxWidth      ,&sg->BoxWidth     )    ,
                      bufp
                     );
     }
    i += strlen(out+i) ; p=1;
   }
  if ((ppl_strAutocomplete(word, "settings", 1)>=0) || (ppl_strAutocomplete(word, "c1format",1)>=0))
   {
    if (sg->c1formatset) sprintf(buf, "%s ", sg->c1format);
    else                         buf[0]='\0';
    m = strlen(buf);
    sprintf(buf+m, "%s", *(char **)ppl_fetchSettingName(&c->errcontext, sg->c1TickLabelRotation, SW_TICLABDIR_INT, SW_TICLABDIR_STR , sizeof(char *))); m += strlen(buf+m);     
    if (sg->c1TickLabelRotation == SW_TICLABDIR_ROT)
     {
      pplObj valobj; valobj.refCount=1;
      pplObjNum(&valobj,0,sg->c1TickLabelRotate,0); valobj.exponent[UNIT_ANGLE] = 1; valobj.dimensionless = 0;
      sprintf(buf+m, " %s", ppl_unitsNumericDisplay(c,&valobj,0,0,0));
     }
    directive_show3(c, out+i, itemSet, 1, interactive, "c1format", buf,
                    (  ( sg->c1TickLabelRotate   == c->set->graph_default.c1TickLabelRotate  ) &&
                       ( sg->c1TickLabelRotation == c->set->graph_default.c1TickLabelRotation) &&
                       ( sg->c1formatset         == c->set->graph_default.c1formatset        ) &&
                      ((!sg->c1formatset) || (strcmp(sg->c1format,c->set->graph_default.c1format)==0))
                    ) ,
                    "Format string for the tick labels on the c1 axis");
    i += strlen(out+i) ; p=1;
   }
  if ((ppl_strAutocomplete(word, "settings", 1)>=0) || (ppl_strAutocomplete(word, "c1label",1)>=0))
   {
    pplObj valobj; valobj.refCount=1;
    ppl_strEscapify(sg->c1label , buf); m = strlen(buf);
    pplObjNum(&valobj,0,sg->c1LabelRotate,0); valobj.exponent[UNIT_ANGLE] = 1; valobj.dimensionless = 0;
    sprintf(buf+m, " rotate %s", ppl_unitsNumericDisplay(c,&valobj,0,0,0));
    directive_show3(c, out+i, itemSet, 1, interactive, "c1label", buf,
                    (  ( sg->c1LabelRotate == c->set->graph_default.c1LabelRotate) &&
                       ( strcmp(sg->c1label,c->set->graph_default.c1label)==0)
                    ) ,
                    "Textual label for the c1 axis");
    i += strlen(out+i) ; p=1;
   }
  if ((ppl_strAutocomplete(word, "settings", 1)>=0) || (ppl_strAutocomplete(word, "calendarin",1)>=0))
   {
    sprintf(buf, "%s", *(char **)ppl_fetchSettingName(&c->errcontext, c->set->term_current.CalendarIn, SW_CALENDAR_INT, SW_CALENDAR_STR , sizeof(char *)));
    directive_show3(c, out+i, itemSet, 0, interactive, "calendarin", buf, (c->set->term_current.CalendarIn == c->set->term_default.CalendarIn), "Selects the historical year in which the transition is made between Julian and Gregorian calendars when dates are being input");
    i += strlen(out+i) ; p=1;
   }
  if ((ppl_strAutocomplete(word, "settings", 1)>=0) || (ppl_strAutocomplete(word, "calendarout",1)>=0))
   {
    sprintf(buf, "%s", *(char **)ppl_fetchSettingName(&c->errcontext, c->set->term_current.CalendarOut, SW_CALENDAR_INT, SW_CALENDAR_STR , sizeof(char *)));
    directive_show3(c, out+i, itemSet, 0, interactive, "calendarout", buf, (c->set->term_current.CalendarOut == c->set->term_default.CalendarOut), "Selects the historical year in which the transition is made between Julian and Gregorian calendars when displaying dates");
    i += strlen(out+i) ; p=1;
   }
  if ((ppl_strAutocomplete(word, "settings", 1)>=0) || (ppl_strAutocomplete(word, "clip",1)>=0))
   {
    sprintf(buf, "%s", *(char **)ppl_fetchSettingName(&c->errcontext, sg->clip, SW_ONOFF_INT, SW_ONOFF_STR , sizeof(char *)));
    directive_show3(c, out+i, itemSet, 0, interactive, "clip", buf, (sg->clip == c->set->graph_default.clip), "Selects whether point symbols which extend over the axes of graphs are allowed to do so, or are clipped at the edges");
    i += strlen(out+i) ; p=1;
   }
  if ((ppl_strAutocomplete(word, "settings", 1)>=0) || (ppl_strAutocomplete(word, "colkey",1)>=0) || (ppl_strAutocomplete(word, "colourkey",1)>=0) || (ppl_strAutocomplete(word, "colorkey",1)>=0))
   {
    if (sg->ColKey == SW_ONOFF_OFF)
     {
      sprintf(buf, "%s", *(char **)ppl_fetchSettingName(&c->errcontext, sg->ColKey, SW_ONOFF_INT, SW_ONOFF_STR , sizeof(char *)));
     } else {
      sprintf(buf, "%s", *(char **)ppl_fetchSettingName(&c->errcontext, sg->ColKeyPos, SW_COLKEYPOS_INT, SW_COLKEYPOS_STR , sizeof(char *)));
     }
    directive_show3(c, out+i, itemSet, 1, interactive, "colkey", buf, (c->set->graph_default.ColKey == sg->ColKey)&&((sg->ColKey==SW_ONOFF_OFF)||(c->set->graph_default.ColKeyPos == sg->ColKeyPos)), "Selects whether a colour scale is included on colourmap plots");
    i += strlen(out+i) ; p=1;
   }
  if ((ppl_strAutocomplete(word, "settings", 1)>=0) || (ppl_strAutocomplete(word, "colmap",1)>=0) || (ppl_strAutocomplete(word, "colourmap",1)>=0) || (ppl_strAutocomplete(word, "colormap",1)>=0))
   {
    int k;
    sprintf(buf, "%s", *(char **)ppl_fetchSettingName(&c->errcontext, sg->ColMapColSpace, SW_COLSPACE_INT, SW_COLSPACE_STR , sizeof(char *)));
    k =strlen(buf);
    sprintf(buf+k, "%s:%s:%s", sg->ColMapExpr1, sg->ColMapExpr2, sg->ColMapExpr3);
    k+=strlen(buf+k);
    if (sg->ColMapColSpace==SW_COLSPACE_CMYK) sprintf(buf+k, ":%s", sg->ColMapExpr4);
    k+=strlen(buf+k);
    sprintf(buf+k, " %smask %s", (sg->MaskExpr[0]=='\0')?"no":"", sg->MaskExpr);
    k+=strlen(buf+k);
    directive_show3(c, out+i, itemSet, 1, interactive, "colmap", buf, (c->set->graph_default.ColMapColSpace==sg->ColMapColSpace)&&(strcmp(sg->ColMapExpr1,c->set->graph_default.ColMapExpr1)==0)&&(strcmp(sg->ColMapExpr2,c->set->graph_default.ColMapExpr2)==0)&&(strcmp(sg->ColMapExpr3,c->set->graph_default.ColMapExpr3)==0)&&(strcmp(sg->ColMapExpr4,c->set->graph_default.ColMapExpr4)==0)&&(strcmp(sg->MaskExpr,c->set->graph_default.MaskExpr)==0), "The mapping of ordinate value to colour used by the colourmap plot style");
    i += strlen(out+i) ; p=1;
   }
  if ((ppl_strAutocomplete(word, "settings", 1)>=0) || (ppl_strAutocomplete(word, "contours",1)>=0))
   {
    if (sg->ContoursListLen < 0)
     {
      sprintf(buf, "%d %slabel", sg->ContoursN, (sg->ContoursLabel==SW_ONOFF_ON)?"":"no");
     }
    else
     {
      int p,q;
      pplObj v = sg->ContoursUnit;
      sprintf(buf, "("); p=strlen(buf);
      for (q=0; q<sg->ContoursListLen; q++)
       {
        if (q!=0) { sprintf(buf+p, ", "); p+=strlen(buf+p); }
        v.real = sg->ContoursList[q];
        sprintf(buf+p, "%s", ppl_unitsNumericDisplay(c,&v, 0, 0, 0));
        p+=strlen(buf+p);
       }
      sprintf(buf+p, ") %slabel", (sg->ContoursLabel==SW_ONOFF_ON)?"":"no"); p+=strlen(buf+p);
     }
    directive_show3(c, out+i, itemSet, 1, interactive, "contour", buf, (c->set->graph_default.ContoursN==sg->ContoursN)&&(c->set->graph_default.ContoursLabel==sg->ContoursLabel)&&(c->set->graph_default.ContoursListLen==sg->ContoursListLen)&&ppl_unitsDimEqual(&c->set->graph_default.ContoursUnit,&sg->ContoursUnit)&&((sg->ContoursListLen<0)||(memcmp((void *)c->set->graph_default.ContoursList,(void *)sg->ContoursList,sg->ContoursListLen*sizeof(double))==0)), "The number of contours drawn by the contourmap plot style");
    i += strlen(out+i) ; p=1;
   }
  if ((ppl_strAutocomplete(word, "settings", 1)>=0) || (ppl_strAutocomplete(word, "c1range",1)>=0))
   {

#define SHOW_CRANGE(C,X) \
    sprintf(buf, "[%s:%s] %s %s", (sg->Cminauto[C]==SW_BOOL_TRUE) ? "*" : ppl_unitsNumericDisplay(c,&(sg->Cmin[C]), 0, 0, 0), \
                                 (sg->Cmaxauto[C]==SW_BOOL_TRUE) ? "*" : ppl_unitsNumericDisplay(c,&(sg->Cmax[C]), 1, 0, 0), \
                                 (sg->Crenorm [C]==SW_BOOL_TRUE) ? "renormalise" : "norenormalise", \
                                 (sg->Creverse[C]==SW_BOOL_TRUE) ? "reverse" : "noreverse" ); \
    directive_show3(c, out+i, itemSet, 1, interactive, "c" X "range", buf, (c->set->graph_default.Cminauto[C]==sg->Cminauto[C])&&(c->set->graph_default.Cmaxauto[C]==sg->Cmaxauto[C])&&((sg->Cminauto[C]==SW_BOOL_TRUE)||((c->set->graph_default.Cmin[C].real==sg->Cmin[C].real)&&ppl_unitsDimEqual(&(c->set->graph_default.Cmin[C]),&(sg->Cmin[C]))))&&((sg->Cmaxauto[C]==SW_BOOL_TRUE)||((c->set->graph_default.Cmax[C].real==sg->Cmax[C].real)&&ppl_unitsDimEqual(&(c->set->graph_default.Cmax[C]),&(sg->Cmax[C]))))&&(c->set->graph_default.Crenorm[C]==sg->Crenorm[C])&&(c->set->graph_default.Creverse[C]==sg->Creverse[C]), "The range of values represented by different colours in the colourmap plot style, and by contours in the contourmap plot style"); \
    i += strlen(out+i) ; p=1;

    SHOW_CRANGE(0,"1");
   }
  if ((ppl_strAutocomplete(word, "settings", 1)>=0) || (ppl_strAutocomplete(word, "c2range",1)>=0))
   { SHOW_CRANGE(1,"2"); }
  if ((ppl_strAutocomplete(word, "settings", 1)>=0) || (ppl_strAutocomplete(word, "c3range",1)>=0))
   { SHOW_CRANGE(2,"3"); }
  if ((ppl_strAutocomplete(word, "settings", 1)>=0) || (ppl_strAutocomplete(word, "c4range",1)>=0))
   { SHOW_CRANGE(3,"4"); }
  if ((ppl_strAutocomplete(word, "settings", 1)>=0) || (ppl_strAutocomplete(word, "logscale", 1)>=0) || (ppl_strAutocomplete(word, "linearscale", 1)>=0))
   {
    int C;
    for (C=0; C<4; C++)
     {
      if (sg->Clog[C]==SW_BOOL_TRUE) bufp = "logscale";
      else                           bufp = "nologscale";
      sprintf(buf, "c%d",C+1);
      sprintf(buf2, "Sets whether colours in the colourmap plot style, and contours in the contourmap plot style, demark linear or logarithmic intervals");
      directive_show3(c, out+i, itemSet, 1, interactive, bufp, buf, (sg->Clog[C]==c->set->graph_default.Clog[C]), buf2);
      i += strlen(out+i) ; p=1;
     }
   }
  if ((ppl_strAutocomplete(word, "settings", 1)>=0) || (ppl_strAutocomplete(word, "display", 1)>=0))
   {
    sprintf(buf, "%s", *(char **)ppl_fetchSettingName(&c->errcontext, c->set->term_current.display, SW_ONOFF_INT, SW_ONOFF_STR , sizeof(char *)));
    directive_show3(c, out+i, itemSet, 0, interactive, "display", buf, (c->set->term_default.display == c->set->term_current.display), "Sets whether any output is produced; turn on to improve performance whilst setting up large multiplots");
    i += strlen(out+i) ; p=1;
   }
  if ((ppl_strAutocomplete(word, "settings", 1)>=0) || (ppl_strAutocomplete(word, "filters",1)>=0))
   {
    dictIterator *iter = ppl_dictIterateInit(c->set->filters);
    pplObj       *item = NULL;
    char         *key  = NULL;
    while ((item = (pplObj *)ppl_dictIterate(&iter, &key))!=NULL)
     {
      ppl_strEscapify(key, buf+16);
      ppl_strEscapify((char *)item->auxil, buf2);
      sprintf(buf,"%s %s",buf+16,buf2);
      directive_show3(c, out+i, itemSet, 0, interactive, "filter", buf, 1, "Sets an input filter to be used when reading datafiles");
      i += strlen(out+i) ; p=1;
     }
   }
  if ((ppl_strAutocomplete(word, "settings", 1)>=0) || (ppl_strAutocomplete(word, "fontsize",1)>=0) || (ppl_strAutocomplete(word, "fountsize",1)>=0))
   {
    sprintf(buf, "%s", ppl_numericDisplay(sg->FontSize,c->numdispBuff[0],c->set->term_current.SignificantFigures,(c->set->term_current.NumDisplay==SW_DISPLAY_L)));
    directive_show3(c, out+i, itemSet, 1, interactive, "FontSize", buf, (c->set->graph_default.FontSize == sg->FontSize), "Sets the font size of text output: 1.0 is the default, and other values multiply this default size");
    i += strlen(out+i) ; p=1;
   }
  if ((ppl_strAutocomplete(word, "axes_", 1)>=0) || (ppl_strAutocomplete(word, "axis", 1)>=0) || (ppl_strAutocomplete(word, "settings", 1)>=0) || (ppl_strAutocomplete(word, "grid",1)>=0))
   {
    sprintf(buf, "%s", *(char **)ppl_fetchSettingName(&c->errcontext, sg->grid, SW_ONOFF_INT, SW_ONOFF_STR , sizeof(char *)));
    directive_show3(c, out+i, itemSet, 0, interactive, "grid", buf, (c->set->graph_default.grid == sg->grid), "Selects whether a grid is drawn on plots");
    i += strlen(out+i) ; p=1;
    if (sg->grid == SW_ONOFF_ON)
     {
      bufp = buf; k=1;
      for (j=0; j<MAX_AXES; j++)
       {
        if (sg->GridAxisX[j] != 0                                   ) { sprintf(bufp, "x%d", j); bufp += strlen(bufp); }
        if (sg->GridAxisX[j] != c->set->graph_default.GridAxisX[j] ) k=0;
       }
      if (bufp != buf) directive_show3(c, out+i, itemSet, 1, interactive, "grid", buf, k, "Sets the x axis with whose ticks gridlines are associated");
      i += strlen(out+i);

      bufp = buf; k=1;
      for (j=0; j<MAX_AXES; j++)
       {
        if (sg->GridAxisY[j] != 0                                   ) { sprintf(bufp, "y%d", j); bufp += strlen(bufp); }
        if (sg->GridAxisY[j] != c->set->graph_default.GridAxisY[j] ) k=0;
       }
      if (bufp != buf) directive_show3(c, out+i, itemSet, 1, interactive, "grid", buf, k, "Sets the y axis with whose ticks gridlines are associated");
      i += strlen(out+i);

      bufp = buf; k=1;
      for (j=0; j<MAX_AXES; j++)
       {
        if (sg->GridAxisZ[j] != 0                                   ) { sprintf(bufp, "z%d", j); bufp += strlen(bufp); }
        if (sg->GridAxisZ[j] != c->set->graph_default.GridAxisZ[j] ) k=0;
       }
      if (bufp != buf) directive_show3(c, out+i, itemSet, 1, interactive, "grid", buf, k, "Sets the z axis with whose ticks gridlines are associated");
      i += strlen(out+i);
     }
   }
  if ((ppl_strAutocomplete(word, "settings", 1)>=0) || (ppl_strAutocomplete(word, "gridmajcolour",1)>=0) || (ppl_strAutocomplete(word, "gridmajcolor",1)>=0))
   {
    SHOW_COLOR(sg->GridMajColour,sg->GridMajCol1234Space,sg->GridMajColour1,sg->GridMajColour2,sg->GridMajColour3,sg->GridMajColour4  ,
                c->set->graph_default.GridMajColour,c->set->graph_default.GridMajCol1234Space,c->set->graph_default.GridMajColour1,c->set->graph_default.GridMajColour2,c->set->graph_default.GridMajColour3,c->set->graph_default.GridMajColour4  ,
                "GridMajColour", "The colour of the major gridlines on graphs");
    i += strlen(out+i) ; p=1;
   }
  if ((ppl_strAutocomplete(word, "settings", 1)>=0) || (ppl_strAutocomplete(word, "gridmincolour",1)>=0) || (ppl_strAutocomplete(word, "gridmincolor",1)>=0))
   {
    SHOW_COLOR(sg->GridMinColour,sg->GridMinCol1234Space,sg->GridMinColour1,sg->GridMinColour2,sg->GridMinColour3,sg->GridMinColour4  ,
                c->set->graph_default.GridMinColour,c->set->graph_default.GridMinCol1234Space,c->set->graph_default.GridMinColour1,c->set->graph_default.GridMinColour2,c->set->graph_default.GridMinColour3,c->set->graph_default.GridMinColour4  ,
                "GridMinColour", "The colour of the minor gridlines on graphs");
    i += strlen(out+i) ; p=1;
   }

  if ((ppl_strAutocomplete(word, "settings", 1)>=0) || (ppl_strAutocomplete(word, "key",1)>=0))
   {
    sprintf(buf, "%s", *(char **)ppl_fetchSettingName(&c->errcontext, sg->key, SW_ONOFF_INT, SW_ONOFF_STR , sizeof(char *)));
    directive_show3(c, out+i, itemSet, 1, interactive, "key", buf, (c->set->graph_default.key == sg->key), "Selects whether a legend is included on plots");
    i += strlen(out+i) ; p=1;
   }
  if ( ((ppl_strAutocomplete(word, "settings", 1)>=0) || (ppl_strAutocomplete(word, "key",1)>=0)) && (sg->key == SW_ONOFF_ON)  )
   {
    sprintf(buf, "%s %s , %s", *(char **)ppl_fetchSettingName(&c->errcontext, sg->KeyPos, SW_KEYPOS_INT, SW_KEYPOS_STR , sizeof(char *)),ppl_unitsNumericDisplay(c,&(sg->KeyXOff),0,0,0),ppl_unitsNumericDisplay(c,&(sg->KeyYOff),1,0,0));
    directive_show3(c, out+i, itemSet, 1, interactive, "key", buf, ((c->set->graph_default.KeyPos == sg->KeyPos)&&(c->set->graph_default.KeyXOff.real == sg->KeyXOff.real)&&(c->set->graph_default.KeyYOff.real == sg->KeyYOff.real)), "Selects where legends are orientated on graphs");
    i += strlen(out+i) ; p=1;
   }
  if ((ppl_strAutocomplete(word, "settings", 1)>=0) || (ppl_strAutocomplete(word, "keycolumns",1)>=0))
   {
    if (sg->KeyColumns>0) sprintf(buf, "%d", sg->KeyColumns);
    else                  sprintf(buf, "auto");
    directive_show3(c, out+i, itemSet, 1, interactive, "KeyColumns", buf, (c->set->graph_default.KeyColumns == sg->KeyColumns), "Sets the number of columns into which legends on graphs are sorted");
    i += strlen(out+i) ; p=1;
   }
  if ((ppl_strAutocomplete(word, "settings", 1)>=0) || (ppl_strAutocomplete(word, "linewidth", 1)>=0) || (ppl_strAutocomplete(word, "lw", 2)>=0))
   {
    sprintf(buf, "%s", ppl_numericDisplay(sg->LineWidth,c->numdispBuff[0],c->set->term_current.SignificantFigures,(c->set->term_current.NumDisplay==SW_DISPLAY_L)));
    directive_show3(c, out+i, itemSet, 1, interactive, "LineWidth", buf, (c->set->graph_default.LineWidth == sg->LineWidth), "Sets the widths of lines drawn on graphs");
    i += strlen(out+i) ; p=1;
   }
  if ((ppl_strAutocomplete(word, "settings", 1)>=0) || (ppl_strAutocomplete(word, "multiplot", 1)>=0))
   {
    sprintf(buf, "%s", *(char **)ppl_fetchSettingName(&c->errcontext, c->set->term_current.multiplot, SW_ONOFF_INT, SW_ONOFF_STR , sizeof(char *)));
    directive_show3(c, out+i, itemSet, 0, interactive, "multiplot", buf, (c->set->term_default.multiplot == c->set->term_current.multiplot), "Selects whether multiplot mode is currently active");
    i += strlen(out+i) ; p=1;
   }
  if ((ppl_strAutocomplete(word, "settings", 1)>=0) || (ppl_strAutocomplete(word, "numerics", 1)>=0))
   {
    sprintf(buf, "%s", *(char **)ppl_fetchSettingName(&c->errcontext, c->set->term_current.ComplexNumbers, SW_ONOFF_INT, SW_ONOFF_STR , sizeof(char *)));
    directive_show3(c, out+i, itemSet, 0, interactive, "numerics complex", buf, (c->set->term_default.ComplexNumbers==c->set->term_current.ComplexNumbers), "Selects whether numbers are allowed to have imagnary components; affects the behaviour of functions such as sqrt()");
    i += strlen(out+i) ; p=1;
    sprintf(buf, "%s", *(char **)ppl_fetchSettingName(&c->errcontext, c->set->term_current.ExplicitErrors,  SW_ONOFF_INT, SW_ONOFF_STR , sizeof(char *)));
    directive_show3(c, out+i, itemSet, 0, interactive, "numerics errors explicit", buf, (c->set->term_default.ExplicitErrors==c->set->term_current.ExplicitErrors), "Selects whether numerical errors quietly produce not-a-number results, or throw explicit errors");
    i += strlen(out+i) ; p=1;
    sprintf(buf, "%s", ppl_numericDisplay(c->set->term_current.SignificantFigures,c->numdispBuff[0],c->set->term_current.SignificantFigures,(c->set->term_current.NumDisplay==SW_DISPLAY_L)));
    directive_show3(c, out+i, itemSet, 0, interactive, "numerics sigfig", buf, (c->set->term_default.SignificantFigures == c->set->term_current.SignificantFigures), "Sets the (minimum) number of significant figures to which decimal numbers are displayed by default");
    i += strlen(out+i) ; p=1;
    sprintf(buf, "%s", *(char **)ppl_fetchSettingName(&c->errcontext, c->set->term_current.NumDisplay, SW_DISPLAY_INT, SW_DISPLAY_STR , sizeof(char *)));
    directive_show3(c, out+i, itemSet, 0, interactive, "numerics display", buf, (c->set->term_default.NumDisplay==c->set->term_current.NumDisplay), "Selects how numerical results are displayed: in a natural textual way, in a way which can be copied into a terminal, or as LaTeX");
    i += strlen(out+i) ; p=1;
   }
  if ((ppl_strAutocomplete(word, "settings", 1)>=0) || (ppl_strAutocomplete(word, "origin", 1)>=0))
   {
    sprintf(buf, "%s , %s", ppl_unitsNumericDisplay(c,&(sg->OriginX),0,0,0), ppl_unitsNumericDisplay(c,&(sg->OriginY),1,0,0));
    directive_show3(c, out+i, itemSet, 1, interactive, "origin", buf, ((c->set->graph_default.OriginX.real == sg->OriginX.real)&&(c->set->graph_default.OriginY.real == sg->OriginY.real)), "Selects where the bottom-left corners of graphs are located on multiplot pages");
    i += strlen(out+i) ; p=1;
   }
  if ((ppl_strAutocomplete(word, "settings", 1)>=0) || (ppl_strAutocomplete(word, "output", 1)>=0))
   {
    ppl_strEscapify(c->set->term_current.output, buf);
    directive_show3(c, out+i, itemSet, 0, interactive, "output", buf, (strcmp(c->set->term_default.output,c->set->term_current.output)==0), "Filename to which graphic output is sent");
    i += strlen(out+i) ; p=1;
   }
  if ((ppl_strAutocomplete(word, "settings", 1)>=0) || (ppl_strAutocomplete(word, "palette",1)>=0))
   {
    l=0;
    for (j=0; j<PALETTE_LENGTH; j++) // Check whether the palette has been changed from its default setting
     {
      if ((c->set->palette_current[j] == -1) && (c->set->palette_default[j] == -1)) break;
      if ((c->set->palette_current[j] == c->set->palette_default[j])&&(c->set->paletteS_current[j] == c->set->paletteS_default[j])&&(c->set->palette1_current[j] == c->set->palette1_default[j])&&(c->set->palette2_current[j] == c->set->palette2_default[j])&&(c->set->palette3_current[j] == c->set->palette3_default[j])&&(c->set->palette4_current[j] == c->set->palette4_default[j])) continue;
      l=1; break;
     }
    for (j=k=0; c->set->palette_current[j]>=0; j++)
     {
      if (j>0) { sprintf(buf+k, ", "); k+=strlen(buf+k); }
      if (c->set->palette_current[j]>0) sprintf(buf+k, "%s", *(char **)ppl_fetchSettingName(&c->errcontext, c->set->palette_current[j], SW_COLOR_INT, SW_COLOR_STR , sizeof(char *)));
      else if (c->set->paletteS_current[j]==SW_COLSPACE_RGB ) sprintf(buf, "rgb%s:%s:%s"    , S_RGB(c->set->palette1_current[j],0), S_RGB(c->set->palette2_current[j],1), S_RGB(c->set->palette3_current[j],2));
      else if (c->set->paletteS_current[j]==SW_COLSPACE_HSB ) sprintf(buf, "hsb%s:%s:%s"    , S_RGB(c->set->palette1_current[j],0), S_RGB(c->set->palette2_current[j],1), S_RGB(c->set->palette3_current[j],2));
      else if (c->set->paletteS_current[j]==SW_COLSPACE_CMYK) sprintf(buf, "cmyk%s:%s:%s:%s", S_RGB(c->set->palette1_current[j],0), S_RGB(c->set->palette2_current[j],1), S_RGB(c->set->palette3_current[j],2), S_RGB(c->set->palette4_current[j],3));
      k+=strlen(buf+k);
     }
    directive_show3(c, out+i, itemSet, 0, interactive, "palette", buf, !l, "The sequence of colours used to plot datasets on colour graphs");
    i += strlen(out+i) ; p=1;
   }
  if ((ppl_strAutocomplete(word, "settings", 1)>=0) || (ppl_strAutocomplete(word, "papersize", 1)>=0))
   {
    sprintf(buf, "%s , %s", ppl_unitsNumericDisplay(c,&(c->set->term_current.PaperWidth),0,0,0), ppl_unitsNumericDisplay(c,&(c->set->term_current.PaperHeight),1,0,0));
    directive_show3(c, out+i, itemSet, 0, interactive, "PaperSize", buf, ((c->set->term_default.PaperWidth.real==c->set->term_current.PaperWidth.real)&&(c->set->term_default.PaperHeight.real==c->set->term_current.PaperHeight.real)), "The current papersize for postscript output, in mm");
    i += strlen(out+i) ; p=1;
    if (ppl_strAutocomplete("user", c->set->term_current.PaperName, 1)<0)
     {
      ppl_strEscapify(c->set->term_current.PaperName,buf);
      directive_show3(c, out+i, itemSet, 0, interactive, "PaperSize", buf, (strcmp(c->set->term_default.PaperName, c->set->term_current.PaperName)==0), NULL);
      i += strlen(out+i) ; p=1;
     }
   }
  if ((ppl_strAutocomplete(word, "settings", 1)>=0) || (ppl_strAutocomplete(word, "pointlinewidth",1)>=0) || (ppl_strAutocomplete(word, "plw",3)>=0))
   {
    sprintf(buf, "%s", ppl_numericDisplay(sg->PointLineWidth,c->numdispBuff[0],c->set->term_current.SignificantFigures,(c->set->term_current.NumDisplay==SW_DISPLAY_L)));
    directive_show3(c, out+i, itemSet, 1, interactive, "PointLineWidth", buf, (c->set->graph_default.PointLineWidth==sg->PointLineWidth), "The width of the strokes used to mark points on graphs");
    i += strlen(out+i) ; p=1;
   }
  if ((ppl_strAutocomplete(word, "settings", 1)>=0) || (ppl_strAutocomplete(word, "pointsize",1)>=0) || (ppl_strAutocomplete(word, "ps",2)>=0))
   {
    sprintf(buf, "%s", ppl_numericDisplay(sg->PointSize,c->numdispBuff[0],c->set->term_current.SignificantFigures,(c->set->term_current.NumDisplay==SW_DISPLAY_L)));
    directive_show3(c, out+i, itemSet, 1, interactive, "PointSize", buf, (c->set->graph_default.PointSize==sg->PointSize), "The size of points marked on graphs");
    i += strlen(out+i) ; p=1;
   }
  if ((ppl_strAutocomplete(word, "settings", 1)>=0) || (ppl_strAutocomplete(word, "preamble", 1)>=0))
   {
    ppl_strEscapify(c->set->term_current.LatexPreamble,buf);
    directive_show3(c, out+i, itemSet, 0, interactive, "preamble", buf, (strcmp(c->set->term_default.LatexPreamble,c->set->term_current.LatexPreamble)==0), "Configuration options sent to the LaTeX typesetting system");
    i += strlen(out+i) ; p=1;
   }
//  if ((ppl_strAutocomplete(word, "settings", 1)>=0) || (ppl_strAutocomplete(word, "projection", 1)>=0))
//   {
//    sprintf(buf, "%s", *(char **)ppl_fetchSettingName(&c->errcontext, sg->projection, SW_PROJ_INT, SW_PROJ_STR , sizeof(char *)));
//    directive_show3(c, out+i, itemSet, 0, interactive, "projection", buf, (c->set->graph_default.projection==sg->projection), "The projection used when representing (x,y) data on a graph");
//    i += strlen(out+i) ; p=1;
//   }
  if ((ppl_strAutocomplete(word, "settings", 1)>=0) || (ppl_strAutocomplete(word, "samples",1)>=0))
   {
    int k;
    sprintf(buf, "%d grid ", sg->samples);
    k =strlen(buf);
    if (sg->SamplesXAuto == SW_BOOL_TRUE) sprintf(buf+k, "* x ");
    else                                  sprintf(buf+k, "%d x ", sg->SamplesX);
    k+=strlen(buf+k);
    if (sg->SamplesYAuto == SW_BOOL_TRUE) sprintf(buf+k, "*");
    else                                  sprintf(buf+k, "%d", sg->SamplesY);
    k+=strlen(buf+k);
    sprintf(buf+k, " interpolate %s", *(char **)ppl_fetchSettingName(&c->errcontext, sg->Sample2DMethod, SW_SAMPLEMETHOD_INT, SW_SAMPLEMETHOD_STR , sizeof(char *)));

    directive_show3(c, out+i, itemSet, 1, interactive, "samples", buf,
                    ((c->set->graph_default.samples==sg->samples) && (c->set->graph_default.SamplesXAuto==sg->SamplesXAuto) && (c->set->graph_default.SamplesYAuto==sg->SamplesYAuto) && ((sg->SamplesXAuto==SW_BOOL_TRUE)||(c->set->graph_default.SamplesX==sg->SamplesX)) && ((sg->SamplesYAuto==SW_BOOL_TRUE)||(c->set->graph_default.SamplesY==sg->SamplesY)) && (c->set->graph_default.Sample2DMethod==sg->Sample2DMethod)),
                    "The number of samples taken when functions are plotted");
    i += strlen(out+i) ; p=1;
   }
  if ((ppl_strAutocomplete(word, "settings", 1)>=0) || (ppl_strAutocomplete(word, "seed",1)>=0))
   {
    sprintf(buf, "%ld", c->set->term_current.RandomSeed);
    directive_show3(c, out+i, itemSet, 1, interactive, "seed", buf, (c->set->term_default.RandomSeed==c->set->term_current.RandomSeed), "The last seed set for the random number generator");
    i += strlen(out+i) ; p=1;
   }
  if ((ppl_strAutocomplete(word, "settings", 1)>=0) || (ppl_strAutocomplete(word, "size",1)>=0))
   {
    if (sg->AutoAspect == SW_ONOFF_ON) sprintf(buf, "auto");
    else                               sprintf(buf, "%s", ppl_numericDisplay(sg->aspect,c->numdispBuff[ 0],c->set->term_current.SignificantFigures,(c->set->term_current.NumDisplay==SW_DISPLAY_L)));
    directive_show3(c, out+i, itemSet, 1, interactive, "size ratio", buf, ((c->set->graph_default.aspect==sg->aspect)&&(c->set->graph_default.AutoAspect==sg->AutoAspect)), "The y/x aspect-ratio of graphs");
    i += strlen(out+i) ; p=1;
    if (sg->AutoZAspect == SW_ONOFF_ON) sprintf(buf, "auto");
    else                                sprintf(buf, "%s", ppl_numericDisplay(sg->zaspect,c->numdispBuff[ 0],c->set->term_current.SignificantFigures,(c->set->term_current.NumDisplay==SW_DISPLAY_L)));
    directive_show3(c, out+i, itemSet, 1, interactive, "size zratio", buf, ((c->set->graph_default.zaspect==sg->zaspect)&&(c->set->graph_default.AutoZAspect==sg->AutoZAspect)), "The z/x aspect-ratio of 3d graphs");
    i += strlen(out+i) ; p=1;
   }
  if ((ppl_strAutocomplete(word, "settings", 1)>=0) || (ppl_strAutocomplete(word, "data", 1)>=0) || (ppl_strAutocomplete(word, "style", 1)>=0))
   {
    ppl_withWordsPrint(c, &sg->DataStyle, buf);
    directive_show3(c, out+i, itemSet, 1, interactive, "data style", buf, ppl_withWordsCmp(c,&c->set->graph_default.DataStyle,&sg->DataStyle), "Default plot options for plotting datafiles");
    i += strlen(out+i) ; p=1;
   }
  if ((ppl_strAutocomplete(word, "settings", 1)>=0) || (ppl_strAutocomplete(word, "function", 1)>=0) || (ppl_strAutocomplete(word, "style", 1)>=0))
   {
    ppl_withWordsPrint(c, &sg->FuncStyle, buf);
    directive_show3(c, out+i, itemSet, 1, interactive, "function style", buf, ppl_withWordsCmp(c,&c->set->graph_default.FuncStyle,&sg->FuncStyle), "Default plot options for plotting functions");
    i += strlen(out+i) ; p=1;
   }
  if ((ppl_strAutocomplete(word, "settings", 1)>=0) || (ppl_strAutocomplete(word, "terminal", 1)>=0))
   {
    sprintf(buf, "%s", *(char **)ppl_fetchSettingName(&c->errcontext, c->set->term_current.TermType, SW_TERMTYPE_INT, SW_TERMTYPE_STR , sizeof(char *)));
    directive_show3(c, out+i, itemSet, 0, interactive, "terminal", buf, (c->set->term_default.TermType==c->set->term_current.TermType), "The type of graphic output to be produced");
    i += strlen(out+i) ; p=1;
   }
  if ((ppl_strAutocomplete(word, "settings", 1)>=0) || (ppl_strAutocomplete(word, "terminal", 1)>=0) || (ppl_strAutocomplete(word, "antialias",1)>=0))
   {
    sprintf(buf, "%s", *(char **)ppl_fetchSettingName(&c->errcontext, c->set->term_current.TermAntiAlias, SW_ONOFF_INT, SW_ONOFF_STR , sizeof(char *)));
    directive_show3(c, out+i, itemSet, 0, interactive, "terminal AntiAlias", buf, (c->set->term_default.TermAntiAlias==c->set->term_current.TermAntiAlias), "Selects whether anti-aliasing is applied to bitmap output");
    i += strlen(out+i) ; p=1;
   }
  if ((ppl_strAutocomplete(word, "settings", 1)>=0) || (ppl_strAutocomplete(word, "terminal", 1)>=0) || (ppl_strAutocomplete(word, "colour", 1)>=0) || (ppl_strAutocomplete(word, "color",1)>=0))
   {
    sprintf(buf, "%s", *(char **)ppl_fetchSettingName(&c->errcontext, c->set->term_current.color, SW_ONOFF_INT, SW_ONOFF_STR , sizeof(char *)));
    directive_show3(c, out+i, itemSet, 0, interactive, "terminal colour", buf, (c->set->term_default.color==c->set->term_current.color), "Selects whether output is colour or monochrome");
    i += strlen(out+i) ; p=1;
   }
  if ((ppl_strAutocomplete(word, "settings", 1)>=0) || (ppl_strAutocomplete(word, "terminal", 1)>=0) || (ppl_strAutocomplete(word, "dpi", 1)>=0))
   {
    sprintf(buf, "%s", ppl_numericDisplay(c->set->term_current.dpi,c->numdispBuff[0],c->set->term_current.SignificantFigures,(c->set->term_current.NumDisplay==SW_DISPLAY_L)));
    directive_show3(c, out+i, itemSet, 0, interactive, "terminal dpi", buf, (c->set->term_default.dpi == c->set->term_current.dpi), "Sets the pixel resolution used when producing bitmap graphic output");
    i += strlen(out+i) ; p=1;
   }
  if ((ppl_strAutocomplete(word, "settings", 1)>=0) || (ppl_strAutocomplete(word, "terminal", 1)>=0) || (ppl_strAutocomplete(word, "enlargement",1)>=0))
   {
    sprintf(buf, "%s", *(char **)ppl_fetchSettingName(&c->errcontext, c->set->term_current.TermEnlarge, SW_ONOFF_INT, SW_ONOFF_STR , sizeof(char *)));
    directive_show3(c, out+i, itemSet, 0, interactive, "terminal enlarge", buf, (c->set->term_default.TermEnlarge==c->set->term_current.TermEnlarge), "Selects whether output photo-enlarged to fill the page");
    i += strlen(out+i) ; p=1;
   }
  if ((ppl_strAutocomplete(word, "settings", 1)>=0) || (ppl_strAutocomplete(word, "terminal", 1)>=0) || (ppl_strAutocomplete(word, "invert",1)>=0))
   {
    sprintf(buf, "%s", *(char **)ppl_fetchSettingName(&c->errcontext, c->set->term_current.TermInvert, SW_ONOFF_INT, SW_ONOFF_STR , sizeof(char *)));
    directive_show3(c, out+i, itemSet, 0, interactive, "terminal invert", buf, (c->set->term_default.TermInvert==c->set->term_current.TermInvert), "Selects whether the colours of bitmap output are inverted");
    i += strlen(out+i) ; p=1;
   }
  if ((ppl_strAutocomplete(word, "settings", 1)>=0) || (ppl_strAutocomplete(word, "terminal", 1)>=0) || (ppl_strAutocomplete(word, "landscape", 1)>=0) || (ppl_strAutocomplete(word, "portrait", 1)>=0))
   {
    if (c->set->term_current.landscape == SW_ONOFF_ON) sprintf(buf, "Landscape");
    else                                                sprintf(buf, "Portrait");
    directive_show3(c, out+i, itemSet, 0, interactive, "terminal", buf, (c->set->term_default.landscape==c->set->term_current.landscape), "Selects the orientation of output");
    i += strlen(out+i) ; p=1;
   }
  if ((ppl_strAutocomplete(word, "settings", 1)>=0) || (ppl_strAutocomplete(word, "terminal", 1)>=0) || (ppl_strAutocomplete(word, "transparent", 1)>=0) || (ppl_strAutocomplete(word, "solid", 1)>=0))
   {
    sprintf(buf, "%s", *(char **)ppl_fetchSettingName(&c->errcontext, c->set->term_current.TermTransparent, SW_ONOFF_INT, SW_ONOFF_STR , sizeof(char *)));
    directive_show3(c, out+i, itemSet, 0, interactive, "terminal transparent", buf, (c->set->term_default.TermTransparent==c->set->term_current.TermTransparent), "Selects whether gif and png output is transparent");
    i += strlen(out+i) ; p=1;
   }
  if ((ppl_strAutocomplete(word, "settings", 1)>=0) || (ppl_strAutocomplete(word, "textcolour",1)>=0) || (ppl_strAutocomplete(word, "textcolor",1)>=0))
   {
    SHOW_COLOR(sg->TextColour,sg->TextCol1234Space,sg->TextColour1,sg->TextColour2,sg->TextColour3,sg->TextColour4  ,
                c->set->graph_default.TextColour,c->set->graph_default.TextCol1234Space,c->set->graph_default.TextColour1,c->set->graph_default.TextColour2,c->set->graph_default.TextColour3,c->set->graph_default.TextColour4  ,
                "TextColour", "Selects the colour of text labels");
    i += strlen(out+i) ; p=1;
   }
  if ((ppl_strAutocomplete(word, "settings", 1)>=0) || (ppl_strAutocomplete(word, "texthalign",1)>=0))
   {
    sprintf(buf, "%s", *(char **)ppl_fetchSettingName(&c->errcontext, sg->TextHAlign, SW_HALIGN_INT, SW_HALIGN_STR , sizeof(char *)));
    directive_show3(c, out+i, itemSet, 1, interactive, "TextHAlign", buf, (c->set->graph_default.TextHAlign==sg->TextHAlign), "Selects the horizontal alignment of text labels");
    i += strlen(out+i) ; p=1;
   }
  if ((ppl_strAutocomplete(word, "settings", 1)>=0) || (ppl_strAutocomplete(word, "textvalign",1)>=0))
   {
    sprintf(buf, "%s", *(char **)ppl_fetchSettingName(&c->errcontext, sg->TextVAlign, SW_VALIGN_INT, SW_VALIGN_STR , sizeof(char *)));
    directive_show3(c, out+i, itemSet, 1, interactive, "TextVAlign", buf, (c->set->graph_default.TextVAlign==sg->TextVAlign), "Selects the vertical alignment of text labels");
    i += strlen(out+i) ; p=1;
   }
  if ((ppl_strAutocomplete(word, "settings", 1)>=0) || (ppl_strAutocomplete(word, "title", 1)>=0))
   {
    ppl_strEscapify(sg->title, buf); k = strlen(buf);
    sprintf(buf+k, " %s , %s", ppl_unitsNumericDisplay(c,&(sg->TitleXOff), 0, 0, 0), ppl_unitsNumericDisplay(c,&(sg->TitleYOff), 1, 0, 0));
    directive_show3(c, out+i, itemSet, 1, interactive, "title", buf, ((strcmp(c->set->graph_default.title,sg->title)==0)&&(c->set->graph_default.TitleXOff.real==sg->TitleXOff.real)&&(c->set->graph_default.TitleYOff.real==sg->TitleYOff.real)), "A title to be displayed above graphs");
    i += strlen(out+i) ; p=1;
   }
  if ((ppl_strAutocomplete(word, "settings", 1)>=0) || (ppl_strAutocomplete(word, "trange", 1)>=0))
  if (sg->USE_T_or_uv)
   {
    sprintf(buf, "[%s:%s]", ppl_unitsNumericDisplay(c,&(sg->Tmin), 0, 0, 0), ppl_unitsNumericDisplay(c,&(sg->Tmax), 1, 0, 0));
    directive_show3(c, out+i, itemSet, 1, interactive, "trange", buf, (c->set->graph_default.USE_T_or_uv==sg->USE_T_or_uv)&&(c->set->graph_default.Tmin.real==sg->Tmin.real)&&ppl_unitsDimEqual(&(c->set->graph_default.Tmin),&(sg->Tmin))&&(c->set->graph_default.Tmax.real==sg->Tmax.real)&&ppl_unitsDimEqual(&(c->set->graph_default.Tmax),&(sg->Tmax)), "The range of input values used in constructing parametric function plots");
    i += strlen(out+i) ; p=1;
   }
  if ((ppl_strAutocomplete(word, "settings", 1)>=0) || (ppl_strAutocomplete(word, "logscale", 1)>=0) || (ppl_strAutocomplete(word, "linearscale", 1)>=0))
   {
    if (sg->Tlog==SW_BOOL_TRUE) bufp = "logscale";
    else                        bufp = "nologscale";
    sprintf(buf, "t");
    sprintf(buf2, "Sets whether the t-axis scales linearly or logarithmically");
    directive_show3(c, out+i, itemSet, 1, interactive, bufp, buf, (sg->Tlog==c->set->graph_default.Tlog), buf2);
    i += strlen(out+i) ; p=1;
   }
  if ((ppl_strAutocomplete(word, "settings", 1)>=0) || (ppl_strAutocomplete(word, "units", 1)>=0))
   {
    sprintf(buf, "%s", *(char **)ppl_fetchSettingName(&c->errcontext, c->set->term_current.UnitAngleDimless, SW_ONOFF_INT, SW_ONOFF_STR , sizeof(char *)));
    directive_show3(c, out+i, itemSet, 0, interactive, "unit angle dimensionless", buf, (c->set->term_default.UnitAngleDimless==c->set->term_current.UnitAngleDimless), "Selects whether angles are treated as dimensionless quantities");
    i += strlen(out+i) ; p=1;
    sprintf(buf, "%s", *(char **)ppl_fetchSettingName(&c->errcontext, c->set->term_current.UnitDisplayAbbrev, SW_ONOFF_INT, SW_ONOFF_STR , sizeof(char *)));
    directive_show3(c, out+i, itemSet, 0, interactive, "unit display abbreviated", buf, (c->set->term_default.UnitDisplayAbbrev==c->set->term_current.UnitDisplayAbbrev), "Selects whether units are displayed in abbreviated form ('m' vs. 'metres')");
    i += strlen(out+i) ; p=1;
    sprintf(buf, "%s", *(char **)ppl_fetchSettingName(&c->errcontext, c->set->term_current.UnitDisplayPrefix, SW_ONOFF_INT, SW_ONOFF_STR , sizeof(char *)));
    directive_show3(c, out+i, itemSet, 0, interactive, "unit display prefix", buf, (c->set->term_default.UnitDisplayPrefix==c->set->term_current.UnitDisplayPrefix), "Selects whether SI units are displayed with prefixes");
    i += strlen(out+i) ; p=1;
    sprintf(buf, "%s", *(char **)ppl_fetchSettingName(&c->errcontext, c->set->term_current.UnitScheme, SW_UNITSCH_INT, SW_UNITSCH_STR , sizeof(char *)));
    directive_show3(c, out+i, itemSet, 0, interactive, "unit scheme", buf, (c->set->term_default.UnitScheme==c->set->term_current.UnitScheme), "Selects the scheme (e.g. SI or Imperial) of preferred units");
    i += strlen(out+i) ; p=1;
    for (j=0; j<c->unit_pos; j++) if (ud[j].userSel != 0)
     {
      sprintf(buf, "unit of %s", ud[j].quantity);
      if (c->set->term_current.UnitDisplayAbbrev == SW_ONOFF_ON) sprintf(buf2, "%s%s", SIprefixes_abbrev[ud[j].userSelPrefix], ud[j].nameAs);
      else                                                       sprintf(buf2, "%s%s", SIprefixes_full  [ud[j].userSelPrefix], ud[j].nameFs);
      directive_show3(c, out+i, itemSet, 0, interactive, buf, buf2, 0, "Selects a user-preferred unit for a particular quantity");
      i += strlen(out+i) ; p=1;
     }

    // show preferred units
    {
    PreferredUnit *pu;
    listIterator *listiter = ppl_listIterateInit(c->unit_PreferredUnits);
    while ((pu = (PreferredUnit *)ppl_listIterate(&listiter))!=NULL)
     {
      int pbuf=0, ppu;
      buf[0]='\0';
      for (ppu=0; ppu<pu->NUnits; ppu++)
       {
        if (ppu>0) sprintf(buf+pbuf, "*");
        pbuf+=strlen(buf+pbuf);
        if (c->set->term_current.UnitDisplayAbbrev == SW_ONOFF_ON) sprintf(buf+pbuf, "%s%s", (pu->prefix[ppu]>=1)?SIprefixes_abbrev[ pu->prefix[ppu] ]:"", ud[ pu->UnitID[ppu] ].nameAs);
        else                                                        sprintf(buf+pbuf, "%s%s", (pu->prefix[ppu]>=1)?SIprefixes_full  [ pu->prefix[ppu] ]:"", ud[ pu->UnitID[ppu] ].nameFs);
        pbuf+=strlen(buf+pbuf);
        if (pu->exponent[ppu]!=1) sprintf(buf+pbuf, "**%s", ppl_numericDisplay(pu->exponent[ppu],c->numdispBuff[0],c->set->term_current.SignificantFigures,(c->set->term_current.NumDisplay==SW_DISPLAY_L)));
        pbuf+=strlen(buf+pbuf);
       }
      directive_show3(c, out+i, itemSet, 0, interactive, "unit preferred", buf, !pu->modified, "Specifies a user-preferred physical unit");
      i += strlen(out+i) ; p=1;
     }
    }
   }
  if ((ppl_strAutocomplete(word, "settings", 1)>=0) || (ppl_strAutocomplete(word, "urange", 1)>=0))
  if (!sg->USE_T_or_uv)
   {
    sprintf(buf, "[%s:%s]", ppl_unitsNumericDisplay(c,&(sg->Umin), 0, 0, 0), ppl_unitsNumericDisplay(c,&(sg->Umax), 1, 0, 0));
    directive_show3(c, out+i, itemSet, 1, interactive, "urange", buf, (c->set->graph_default.USE_T_or_uv==sg->USE_T_or_uv)&&(c->set->graph_default.Umin.real==sg->Umin.real)&&ppl_unitsDimEqual(&(c->set->graph_default.Umin),&(sg->Umin))&&(c->set->graph_default.Umax.real==sg->Umax.real)&&ppl_unitsDimEqual(&(c->set->graph_default.Umax),&(sg->Umax)), "The range of input values used in constructing 2d parametric function plots");
    i += strlen(out+i) ; p=1;
   }
  if ((ppl_strAutocomplete(word, "settings", 1)>=0) || (ppl_strAutocomplete(word, "logscale", 1)>=0) || (ppl_strAutocomplete(word, "linearscale", 1)>=0))
   {
    if (sg->Ulog==SW_BOOL_TRUE) bufp = "logscale";
    else                        bufp = "nologscale";
    sprintf(buf, "u");
    sprintf(buf2, "Sets whether the u-axis scales linearly or logarithmically");
    directive_show3(c, out+i, itemSet, 1, interactive, bufp, buf, (sg->Ulog==c->set->graph_default.Ulog), buf2);
    i += strlen(out+i) ; p=1;
   }
  if ((ppl_strAutocomplete(word, "settings", 1)>=0) || (ppl_strAutocomplete(word, "view", 1)>=0))
   {
    int SF = c->set->term_current.SignificantFigures;
    int TY = (c->set->term_current.NumDisplay==SW_DISPLAY_L);
    sprintf(buf,"%s,%s",ppl_numericDisplay(sg->XYview.real/M_PI*180,c->numdispBuff[0],SF,TY),ppl_numericDisplay(sg->YZview.real/M_PI*180,c->numdispBuff[1],SF,TY));
    directive_show3(c, out+i, itemSet, 1, interactive, "view", buf, (c->set->graph_default.XYview.real==sg->XYview.real)&&(c->set->graph_default.YZview.real==sg->YZview.real), "The rotation angle of 3d graphs");
    i += strlen(out+i) ; p=1;
   }
  if ((ppl_strAutocomplete(word, "settings", 1)>=0) || (ppl_strAutocomplete(word, "viewer", 1)>=0))
   {
    unsigned char changed = (c->set->term_current.viewer != c->set->term_default.viewer);
    sprintf(buf, "Selects the postscript viewer used by the X11 terminals%s%s%s", (c->set->term_current.viewer != SW_VIEWER_CUSTOM)?" (":"", (c->set->term_current.viewer ==SW_VIEWER_GGV)?"g":"", (c->set->term_current.viewer != SW_VIEWER_CUSTOM)?"gv)":"");
    if ((c->set->term_current.viewer == SW_VIEWER_CUSTOM) && (c->set->term_default.viewer == SW_VIEWER_CUSTOM)) changed=(strcmp(c->set->term_current.ViewerCmd,c->set->term_default.ViewerCmd)!=0);
    directive_show3(c, out+i, itemSet, 0, interactive, "viewer", (c->set->term_current.viewer != SW_VIEWER_CUSTOM)?"auto":c->set->term_current.ViewerCmd, !changed, buf);
    i += strlen(out+i) ; p=1;
   }
  if ((ppl_strAutocomplete(word, "settings", 1)>=0) || (ppl_strAutocomplete(word, "vrange", 1)>=0))
  if (!sg->USE_T_or_uv)
   {
    sprintf(buf, "[%s:%s]", ppl_unitsNumericDisplay(c,&(sg->Vmin), 0, 0, 0), ppl_unitsNumericDisplay(c,&(sg->Vmax), 1, 0, 0));
    directive_show3(c, out+i, itemSet, 1, interactive, "vrange", buf, (c->set->graph_default.USE_T_or_uv==sg->USE_T_or_uv)&&(c->set->graph_default.Vmin.real==sg->Vmin.real)&&ppl_unitsDimEqual(&(c->set->graph_default.Vmin),&(sg->Vmin))&&(c->set->graph_default.Vmax.real==sg->Vmax.real)&&ppl_unitsDimEqual(&(c->set->graph_default.Vmax),&(sg->Vmax)), "The range of input values used in constructing 2d parametric function plots");
    i += strlen(out+i) ; p=1;
   }
  if ((ppl_strAutocomplete(word, "settings", 1)>=0) || (ppl_strAutocomplete(word, "logscale", 1)>=0) || (ppl_strAutocomplete(word, "linearscale", 1)>=0))
   {
    if (sg->Vlog==SW_BOOL_TRUE) bufp = "logscale";
    else                        bufp = "nologscale";
    sprintf(buf, "v");
    sprintf(buf2, "Sets whether the t-axis scales linearly or logarithmically");
    directive_show3(c, out+i, itemSet, 1, interactive, bufp, buf, (sg->Vlog==c->set->graph_default.Vlog), buf2);
    i += strlen(out+i) ; p=1;
   }
  if ((ppl_strAutocomplete(word, "settings", 1)>=0) || (ppl_strAutocomplete(word, "width", 1)>=0) || (ppl_strAutocomplete(word, "size", 1)>=0))
   {
    sprintf(buf, "%s", ppl_unitsNumericDisplay(c,&(sg->width), 0, 0, 0));
    directive_show3(c, out+i, itemSet, 1, interactive, "width", buf, (c->set->graph_default.width.real==sg->width.real), "The width of graphs");
    i += strlen(out+i) ; p=1;
   }

  // Show axes
  l=0;
  if ((ppl_strAutocomplete(word, "axes_", 1)>=0) || (ppl_strAutocomplete(word, "axis", 1)>=0)) l=1;
  if ( !((xa==NULL)||(ya==NULL)||(za==NULL)) )
   {
    for (k=0; k<3; k++)
     for (j=0; j<MAX_AXES; j++)
      {
       pplset_axis *axisPtrDef, *axisPtr;
       switch (k)
        {
         case 1 : { axisPtr = &(ya[j]); axisPtrDef = &(c->set->YAxesDefault[j]); break; }
         case 2 : { axisPtr = &(za[j]); axisPtrDef = &(c->set->ZAxesDefault[j]); break; }
         default: { axisPtr = &(xa[j]); axisPtrDef = &(c->set->XAxesDefault[j]); break; }
        }
       if (!axisPtr->enabled) // Do not show any information for inactive axes, except that they're disabled
        {
         if (axisPtrDef->enabled)
          {
           sprintf(temp1, "%c%d", "xyzc"[k], j);
           sprintf(buf2, "Axis %s has been disabled", temp1);
           directive_show3(c, out+i, itemSet, 1, interactive, "noaxis", temp1, 0, buf2);
           i += strlen(out+i) ; p=1;
          }
         continue;
        }

       sprintf(temp1, "%c%d", "xyzc"[k], j);
       sprintf(temp2, "%c"  , "xyzc"[k]   );
       if (l || (ppl_strAutocomplete(word, temp1, 1)>=0) || ((j==1)&&(ppl_strAutocomplete(word, temp2, 1)>=0)))
        {
         sprintf(buf  , "%c%d ", "xyzc"[k], j); m = strlen(buf);
         sprintf(buf+m, "%s ", (axisPtr->invisible ? "invisible" : "visible"  )); m += strlen(buf+m);
         if      (k==1) sprintf(buf+m, "%s ", (axisPtr->topbottom ? "right" : "left"  ));
         else if (k==2) sprintf(buf+m, "%s ", (axisPtr->topbottom ? "back"  : "front" ));
         else           sprintf(buf+m, "%s ", (axisPtr->topbottom ? "top"   : "bottom"));
         m += strlen(buf+m);
         sprintf(buf+m, "%s ", *(char **)ppl_fetchSettingName(&c->errcontext, axisPtr->ArrowType, SW_AXISDISP_INT, SW_AXISDISP_STR , sizeof(char *))); m += strlen(buf+m);
         sprintf(buf+m, "%s ", (axisPtr->atzero    ? "atzero"    : "notatzero")); m += strlen(buf+m);
         sprintf(buf+m, "%s ", *(char **)ppl_fetchSettingName(&c->errcontext, axisPtr->MirrorType, SW_AXISMIRROR_INT, SW_AXISMIRROR_STR , sizeof(char *))); m += strlen(buf+m);
         if (!axisPtr->linked)
          {
           sprintf(buf+m, "notlinked"); m += strlen(buf+m);
          }
         else
          {
           strcpy(buf+m, "linked"); m += strlen(buf+m);
           if (axisPtr->LinkedAxisCanvasID > 0) { sprintf(buf+m, " item %d", axisPtr->LinkedAxisCanvasID); m += strlen(buf+m); }
           sprintf(buf+m, " %c%d", "xyzc"[axisPtr->LinkedAxisToXYZ], axisPtr->LinkedAxisToNum); m += strlen(buf+m);
           if (axisPtr->linkusing != NULL) { sprintf(buf+m, " using %s", axisPtr->linkusing); m += strlen(buf+m); }
          }
         sprintf(buf2, "Settings for the %c%d axis", "xyzc"[k], j);
         directive_show3(c, out+i, itemSet, 1, interactive, "axis", buf,
                         (axisPtr->atzero             == axisPtrDef->atzero            ) &&
                         (axisPtr->enabled            == axisPtrDef->enabled           ) &&
                         (axisPtr->invisible          == axisPtrDef->invisible         ) &&
                         (axisPtr->linked             == axisPtrDef->linked            ) &&
                         (axisPtr->topbottom          == axisPtrDef->topbottom         ) &&
                         (axisPtr->ArrowType          == axisPtrDef->ArrowType         ) &&
                         (axisPtr->LinkedAxisCanvasID == axisPtrDef->LinkedAxisCanvasID) &&
                         (axisPtr->LinkedAxisToXYZ    == axisPtrDef->LinkedAxisToXYZ   ) &&
                         (axisPtr->LinkedAxisToNum    == axisPtrDef->LinkedAxisToNum   ) &&
                         (axisPtr->MirrorType         == axisPtrDef->MirrorType        )    ,
                         buf2
                        );
         i += strlen(out+i) ; p=1;
        }

       sprintf(temp1, "%c%dformat", "xyzc"[k], j);
       sprintf(temp2, "%cformat"  , "xyzc"[k]   );
       if (l || (ppl_strAutocomplete(word, temp1, 1)>=0) || ((j==1)&&(ppl_strAutocomplete(word, temp2, 1)>=0)))
        {
         if (axisPtr->format != NULL) sprintf(buf, "%s ", axisPtr->format);
         else                         buf[0]='\0';
         m = strlen(buf);
         sprintf(buf+m, "%s", *(char **)ppl_fetchSettingName(&c->errcontext, axisPtr->TickLabelRotation, SW_TICLABDIR_INT, SW_TICLABDIR_STR , sizeof(char *))); m += strlen(buf+m);
         if (axisPtr->TickLabelRotation == SW_TICLABDIR_ROT)
          {
           pplObj valobj; valobj.refCount=1;
           pplObjNum(&valobj,0,axisPtr->TickLabelRotate,0); valobj.exponent[UNIT_ANGLE] = 1; valobj.dimensionless = 0;
           sprintf(buf+m, " %s", ppl_unitsNumericDisplay(c,&valobj,0,0,0));
          }
         sprintf(buf2, "Format string for the tick labels on the %c%d axis", "xyzc"[k], j);
         directive_show3(c, out+i, itemSet, 1, interactive, temp1, buf,
                         (  ( axisPtr->TickLabelRotate  ==axisPtrDef->TickLabelRotate  ) &&
                            ( axisPtr->TickLabelRotation==axisPtrDef->TickLabelRotation) &&
                           (((axisPtr->format==NULL)&&(axisPtrDef->format==NULL)) ||
                            ((axisPtr->format!=NULL)&&(axisPtrDef->format!=NULL)&&(strcmp(axisPtr->format,axisPtrDef->format)==0)))
                         ) ,
                         buf2);
         i += strlen(out+i) ; p=1;
        }

       sprintf(temp1, "%c%dlabel", "xyzc"[k], j);
       sprintf(temp2, "%clabel"  , "xyzc"[k]   );
       if (l || (ppl_strAutocomplete(word, temp1, 1)>=0) || ((j==1)&&(ppl_strAutocomplete(word, temp2, 1)>=0)))
        {
         pplObj valobj; valobj.refCount=1;
         ppl_strEscapify(axisPtr->label==NULL ? "" : axisPtr->label , buf); m = strlen(buf);
         pplObjNum(&valobj,0,axisPtr->LabelRotate,0); valobj.exponent[UNIT_ANGLE] = 1; valobj.dimensionless = 0;
         sprintf(buf+m, " rotate %s", ppl_unitsNumericDisplay(c,&valobj,0,0,0));
         sprintf(buf2, "Textual label for the %c%d axis", "xyzc"[k], j);
         directive_show3(c, out+i, itemSet, 1, interactive, temp1, buf,
                         (  ( axisPtr->LabelRotate==axisPtrDef->LabelRotate) &&
                           (((axisPtr->label==NULL)&&(axisPtrDef->label==NULL)) ||
                            ((axisPtr->label!=NULL)&&(axisPtrDef->label!=NULL)&&(strcmp(axisPtr->label,axisPtrDef->label)==0)))
                         ) ,
                         buf2);
         i += strlen(out+i) ; p=1;
        }

       if (l || (ppl_strAutocomplete(word, "logscale", 1)>=0) || ((j==1)&&(ppl_strAutocomplete(word, "linearscale", 1)>=0)))
        {
         if (axisPtr->log==SW_BOOL_TRUE) bufp = "logscale";
         else                            bufp = "nologscale";
         sprintf(buf, "%c%d", "xyzc"[k], j); m = strlen(buf);
         if (axisPtr->log==SW_BOOL_TRUE) sprintf(buf+m, " base %d", (int)axisPtr->LogBase);
         sprintf(buf2, "Sets whether the %c%d axis scales linearly or logarithmically", "xyzc"[k], j);
         directive_show3(c, out+i, itemSet, 1, interactive, bufp, buf, (axisPtr->log==axisPtrDef->log), buf2);
         i += strlen(out+i) ; p=1;
        }

       sprintf(temp1, "%c%drange", "xyzc"[k], j);
       sprintf(temp2, "%crange"  , "xyzc"[k]   );
       if (l || (ppl_strAutocomplete(word, "autoscale", 1)>=0) || (ppl_strAutocomplete(word, temp1, 1)>=0) || ((j==1)&&(ppl_strAutocomplete(word, temp2, 1)>=0)))
        {
         axisPtr->unit.real = axisPtr->min;
         if (axisPtr->MinSet==SW_BOOL_TRUE) bufp  = ppl_unitsNumericDisplay(c,&(axisPtr->unit),0,0,0);
         else                               bufp  = "*";
         axisPtr->unit.real = axisPtr->max;
         if (axisPtr->MaxSet==SW_BOOL_TRUE) bufp2 = ppl_unitsNumericDisplay(c,&(axisPtr->unit),1,0,0);
         else                               bufp2 = "*";
         sprintf(buf , "[%s:%s]%s", bufp, bufp2, axisPtr->RangeReversed ? " reversed" : "");
         sprintf(buf2, "Sets the range of the %c%d axis", "xyzc"[k], j);
         directive_show3(c, out+i, itemSet, 1, interactive, temp1, buf, (axisPtr->min    == axisPtrDef->min   ) &&
                                                                     (axisPtr->MinSet == axisPtrDef->MinSet) &&
                                                                     (axisPtr->max    == axisPtrDef->max   ) &&
                                                                     (axisPtr->MaxSet == axisPtrDef->MaxSet)    , buf2);
         i += strlen(out+i) ; p=1;
        }

       sprintf(temp1, "%c%dtics", "xyzc"[k], j);
       sprintf(temp2, "%ctics"  , "xyzc"[k]   );
       m=0;
       if (l || (ppl_strAutocomplete(word, temp1, 1)>=0) || ((j==1)&&(ppl_strAutocomplete(word, temp2, 1)>=0)))
        {
         sprintf(buf2, "Sets where the major ticks are placed along the %c%d axis, and how they appear", "xyzc"[k], j);
         sprintf(buf, "%s ", *(char **)ppl_fetchSettingName(&c->errcontext, axisPtr->TickDir, SW_TICDIR_INT, SW_TICDIR_STR , sizeof(char *))); m = strlen(buf);
         if      ((!axisPtr->TickStepSet) && (axisPtr->TickList == NULL))
          {
           sprintf(buf+m, "autofreq");
          }
         else if (axisPtr->TickList == NULL)
          {
           if (axisPtr->TickMinSet)
            {
             axisPtr->unit.real = axisPtr->TickMin;
             sprintf(buf+m, "%s", ppl_unitsNumericDisplay(c,&(axisPtr->unit),0,0,0)); m += strlen(buf+m);
            }
           if (axisPtr->TickStepSet)
            {
             axisPtr->unit.real = axisPtr->TickStep;
             if (axisPtr->log==SW_BOOL_FALSE) sprintf(buf+m, "%s%s", (axisPtr->TickMinSet)?", ":"", ppl_unitsNumericDisplay(c,&(axisPtr->unit),0,0,0));
             else                             sprintf(buf+m, "%s%s", (axisPtr->TickMinSet)?", ":"", ppl_numericDisplay(axisPtr->TickStep,c->numdispBuff[0],c->set->term_current.SignificantFigures,(c->set->term_current.NumDisplay==SW_DISPLAY_L)));
             m += strlen(buf+m);
            }
           if (axisPtr->TickMaxSet)
            {
             axisPtr->unit.real = axisPtr->TickMax;
             sprintf(buf+m, ", %s", ppl_unitsNumericDisplay(c,&(axisPtr->unit),0,0,0)); m += strlen(buf+m);
            }
          }
         else
          {
           buf[m++]='(';
           for (n=0; axisPtr->TickStrs[n]!=NULL; n++)
            {
             strcpy(buf+m, (n==0)?"":", "); m += strlen(buf+m);
             if (axisPtr->TickStrs[n][0]!='\xFF') { ppl_strEscapify(axisPtr->TickStrs[n], buf+m); m += strlen(buf+m); }
             axisPtr->unit.real = axisPtr->TickList[n];
             sprintf(buf+m, " %s", ppl_unitsNumericDisplay(c,&(axisPtr->unit),0,0,0));
             m += strlen(buf+m);
            }
           sprintf(buf+m, ")");
          }
         directive_show3(c, out+i, itemSet, 1, interactive, temp1, buf, (axisPtr->TickDir     == axisPtrDef->TickDir    ) &&
                                                                        (axisPtr->TickMax     == axisPtrDef->TickMax    ) &&
                                                                        (axisPtr->TickMaxSet  == axisPtrDef->TickMaxSet ) &&
                                                                        (axisPtr->TickStep    == axisPtrDef->TickStep   ) &&
                                                                        (axisPtr->TickStepSet == axisPtrDef->TickStepSet) &&
                                                                        (axisPtr->TickMin     == axisPtrDef->TickMin    ) &&
                                                                        (axisPtr->TickMinSet  == axisPtrDef->TickMinSet ) &&
                                                                        (pplaxis_cmpTics(c, axisPtr, axisPtrDef)        )    ,
                         buf2);
         i += strlen(out+i) ; p=1;
         m=1; // If we've shown major tics, also show minor ticks too.
        }

       sprintf(temp1, "m%c%dtics", "xyzc"[k], j);
       sprintf(temp2, "m%ctics"  , "xyzc"[k]   );
       if (l || m || (ppl_strAutocomplete(word, temp1, 1)>=0) || ((j==1)&&(ppl_strAutocomplete(word, temp2, 1)>=0)))
        {
         sprintf(buf2, "Sets where the minor ticks are placed along the %c%d axis, and how they appear", "xyzc"[k], j);
         sprintf(buf, "%s ", *(char **)ppl_fetchSettingName(&c->errcontext, axisPtr->MTickDir, SW_TICDIR_INT, SW_TICDIR_STR , sizeof(char *))); m = strlen(buf);
         if      ((!axisPtr->MTickStepSet) && (axisPtr->MTickList == NULL))
          {
           sprintf(buf+m, "autofreq");
          }
         else if (axisPtr->MTickList == NULL)
          {
           if (axisPtr->MTickMinSet)
            {
             axisPtr->unit.real = axisPtr->MTickMin;
             sprintf(buf+m, "%s", ppl_unitsNumericDisplay(c,&(axisPtr->unit),0,0,0)); m += strlen(buf+m);
            }
           if (axisPtr->MTickStepSet)
            {
             axisPtr->unit.real = axisPtr->MTickStep;
             sprintf(buf+m, "%s%s", (axisPtr->MTickMinSet)?", ":"", ppl_unitsNumericDisplay(c,&(axisPtr->unit),0,0,0)); m += strlen(buf+m);
            }
           if (axisPtr->MTickMaxSet)
            {
             axisPtr->unit.real = axisPtr->MTickMax;
             sprintf(buf+m, ", %s", ppl_unitsNumericDisplay(c,&(axisPtr->unit),0,0,0)); m += strlen(buf+m);
            }
          }
         else
          {
           buf[m++]='(';
           for (n=0; axisPtr->MTickStrs[n]!=NULL; n++)
            {
             strcpy(buf+m, (n==0)?"":", "); m += strlen(buf+m);
             if (axisPtr->MTickStrs[n][0]!='\xFF') { ppl_strEscapify(axisPtr->MTickStrs[n], buf+m); m += strlen(buf+m); }
             axisPtr->unit.real = axisPtr->MTickList[n];
             sprintf(buf+m, " %s", ppl_unitsNumericDisplay(c,&(axisPtr->unit),0,0,0));
             m += strlen(buf+m);
            }
           sprintf(buf+m, ")");
          }
         directive_show3(c, out+i, itemSet, 1, interactive, temp1, buf, (axisPtr->MTickDir      == axisPtrDef->MTickDir    ) &&
                                                                        (axisPtr->MTickMax      == axisPtrDef->MTickMax    ) &&
                                                                        (axisPtr->MTickMaxSet   == axisPtrDef->MTickMaxSet ) &&
                                                                        (axisPtr->MTickStep     == axisPtrDef->MTickStep   ) &&
                                                                        (axisPtr->MTickStepSet  == axisPtrDef->MTickStepSet) &&
                                                                        (axisPtr->MTickMin      == axisPtrDef->MTickMin    ) &&
                                                                        (axisPtr->MTickMinSet   == axisPtrDef->MTickMinSet ) &&
                                                                        (pplaxis_cmpMTics(c, axisPtr, axisPtrDef)          )    ,
                         buf2);
         i += strlen(out+i) ; p=1;
        }

      } // loop over axes
   } // if axis data structures are not null

  // Showed numbered arrows
  if (ppl_strAutocomplete(word, "arrows", 1)>=0)
   {
    if ( !((xa==NULL)||(ya==NULL)||(za==NULL)) )
     {
      pplarrow_object *ai;
      pplarrow_object *ai_default_prev = c->set->pplarrow_list_default;
      pplarrow_object *ai_default      = c->set->pplarrow_list_default;
      SHOW_HIGHLIGHT(1);
      sprintf(out+i, "\n# Numbered arrows:\n\n"); i += strlen(out+i); p=1;
      SHOW_DEHIGHLIGHT;
      for (ai=*al; ai!=NULL; ai=ai->next)
       {
        while ((ai_default != NULL) && (ai_default->id <= ai->id))
         {
          if (ai_default->id < ai->id)
           {
            sprintf(buf2, "noarrow %6d", ai_default->id);
            sprintf(buf,"remove arrow %6d", ai_default->id);
            directive_show3(c, out+i, itemSet, 1, interactive, buf2, "", 1, buf);
            i += strlen(out+i);
           }
          ai_default_prev = ai_default;
          ai_default      = ai_default->next;
         }
        pplarrow_print(c,ai,buf);
        sprintf(buf2, "arrow %6d", ai->id);
        if ((unchanged = ((ai_default_prev != NULL) && (ai_default_prev->id == ai->id)))!=0) unchanged = pplarrow_compare(c , ai , ai_default_prev);
        directive_show3(c, out+i, itemSet, 1, interactive, buf2, buf, unchanged, buf2);
        i += strlen(out+i);
       }
      while (ai_default != NULL)
       {
        sprintf(buf2, "noarrow %6d", ai_default->id);
        sprintf(buf,"remove arrow %6d", ai_default->id);
        directive_show3(c, out+i, itemSet, 1, interactive, buf2, "", 1, buf);
        i += strlen(out+i);
        ai_default      = ai_default->next;
       }
     }
   }

  // Show numbered text labels
  if (ppl_strAutocomplete(word, "labels", 1)>=0)
   {
    if ( !((xa==NULL)||(ya==NULL)||(za==NULL)) )
     {
      ppllabel_object *li;
      ppllabel_object *li_default_prev = c->set->ppllabel_list_default;
      ppllabel_object *li_default      = c->set->ppllabel_list_default;
      SHOW_HIGHLIGHT(1);
      sprintf(out+i, "\n# Numbered text labels:\n\n"); i += strlen(out+i); p=1;
      SHOW_DEHIGHLIGHT;
      for (li=*ll; li!=NULL; li=li->next)
       {
        while ((li_default != NULL) && (li_default->id <= li->id))
         {
          if (li_default->id < li->id)
           {
            sprintf(buf2, "nolabel %6d", li_default->id);
            sprintf(buf,"remove label %6d", li_default->id);
            directive_show3(c, out+i, itemSet, 1, interactive, buf2, "", 1, buf);
            i += strlen(out+i);
           }
          li_default_prev = li_default;
          li_default      = li_default->next;
         }
        ppllabel_print(c,li,buf);
        sprintf(buf2, "label %6d", li->id);
        if ((unchanged = ((li_default_prev != NULL) && (li_default_prev->id == li->id)))!=0) unchanged = ppllabel_compare(c , li , li_default_prev);
        directive_show3(c, out+i, itemSet, 1, interactive, buf2, buf, unchanged, buf2);
        i += strlen(out+i);
       }
      while (li_default != NULL)
       {
        sprintf(buf2, "nolabel %6d", li_default->id);
        sprintf(buf,"remove label %6d", li_default->id);
        directive_show3(c, out+i, itemSet, 1, interactive, buf2, "", 1, buf);
        i += strlen(out+i);
        li_default      = li_default->next;
       }
     }
   }

  // Show numbered styles
  if ((ppl_strAutocomplete(word, "styles", 1)>=0) || (ppl_strAutocomplete(word, "linestyles", 1)>=0))
   {
    SHOW_HIGHLIGHT(1);
    sprintf(out+i, "\n# Numbered styles:\n\n"); i += strlen(out+i); p=1;
    SHOW_DEHIGHLIGHT;
    for (j=0; j<MAX_PLOTSTYLES; j++)
     {
      if (ppl_withWordsCmp_zero(c,&(c->set->plot_styles[j]))) continue;
      ppl_withWordsPrint(c,&(c->set->plot_styles[j]),buf);
      sprintf(buf2, "style %4d", j);
      directive_show3(c, out+i, itemSet, 0, interactive, buf2, buf, !ppl_withWordsCmp(c,&(c->set->plot_styles[j]),&(c->set->plot_styles_default[j])), buf2);
      i += strlen(out+i);
     }
   }


  // Show variables
  if ((ppl_strAutocomplete(word, "variables", 1)>=0) || (ppl_strAutocomplete(word, "vars", 1)>=0))
   {
    int l;
    for (l=c->ns_ptr ; l>=0 ; l=(l>1)?1:l-1)
     {
      char         *key;
      pplObj       *item;
      dictIterator *di = ppl_dictIterateInit( c->namespaces[l] );
      SHOW_HIGHLIGHT(1);
      if      (l >1) sprintf(out+i, "\n# Local variables:\n\n");
      else if (l==1) sprintf(out+i, "\n# Global variables:\n\n");
      else           sprintf(out+i, "\n# Default variables:\n\n");
      i+=strlen(out+i); p=1;
      SHOW_DEHIGHLIGHT;
      while ((item = (pplObj *)ppl_dictIterate(&di,&key))!=NULL)
       {
        if ((item->objType==PPLOBJ_ZOM) || (item->objType==PPLOBJ_GLOB) || (item->objType==PPLOBJ_FUNC)) continue;
        SHOW_HIGHLIGHT((l==0));
        sprintf(out+i, "%s = ", key);
        i+=strlen(out+i);
        pplObjPrint(c,item,key,out+i,outLen-i,0,1);
        i+=strlen(out+i);
        sprintf(out+i, "\n");
        i+=strlen(out+i);
        SHOW_DEHIGHLIGHT;
       }
     }
   }

  // Show system functions
  if ((ppl_strAutocomplete(word, "functions", 1)>=0) || (ppl_strAutocomplete(word, "funcs", 1)>=0))
   {
    char         *key;
    pplObj       *item;
    dictIterator *di = ppl_dictIterateInit( c->namespaces[0] );
    SHOW_HIGHLIGHT(1);
    sprintf(out+i, "\n# System-defined functions:\n\n"); i+=strlen(out+i); p=1;
    SHOW_DEHIGHLIGHT;
    while ((item = (pplObj *)ppl_dictIterate(&di,&key))!=NULL)
     {
      if (item->objType!=PPLOBJ_FUNC) continue;
      SHOW_HIGHLIGHT(1);
      pplObjPrint(c,item,key,out+i,outLen-i,0,1);
      i+=strlen(out+i);
      sprintf(out+i, "\n");
      i+=strlen(out+i);
      SHOW_DEHIGHLIGHT;
     }
   }

  // Show user functions
  if ((ppl_strAutocomplete(word, "functions", 1)>=0) || (ppl_strAutocomplete(word, "funcs", 1)>=0) || (ppl_strAutocomplete(word, "userfunctions", 1)>=0) || (ppl_strAutocomplete(word, "userfuncs", 1)>=0))
   {
    int l;
    for (l=c->ns_ptr ; l>0 ; l=(l>1)?1:l-1)
     {
      char         *key;
      pplObj       *item;
      dictIterator *di = ppl_dictIterateInit( c->namespaces[l] );
      SHOW_HIGHLIGHT(1);
      if   (l >1) sprintf(out+i, "\n# Local functions:\n\n");
      else        sprintf(out+i, "\n# Global functions:\n\n");
      i+=strlen(out+i); p=1;
      SHOW_DEHIGHLIGHT;
      while ((item = (pplObj *)ppl_dictIterate(&di,&key))!=NULL)
       {
        if (item->objType!=PPLOBJ_FUNC) continue;
        SHOW_HIGHLIGHT(0);
        pplObjPrint(c,item,key,out+i,outLen-i,0,1);
        i+=strlen(out+i);
        sprintf(out+i, "\n");
        i+=strlen(out+i);
        SHOW_DEHIGHLIGHT;
       }
     }
   }

  // Show list of recognised units
  if (ppl_strAutocomplete(word, "units", 5)>=0)
   {
    SHOW_HIGHLIGHT(1);
    sprintf(out+i, "\n# Recognised Physical Units:\n\n"); i += strlen(out+i); p=1;
    SHOW_DEHIGHLIGHT;
    l=-1;
    do
     {
      m=-1;
      for (j=0; j<c->unit_pos; j++)
       {
        if      ( (l==-1) && (m==-1)                                                                                                           ) m=j;
        else if ( (l==-1) && (ppl_strCmpNoCase(ud[j].nameFs , ud[m].nameFs)<0)                                                                 ) m=j;
        else if ( (l>= 0) && (m==-1) &&                                                      (ppl_strCmpNoCase(ud[j].nameFs , ud[l].nameFs)>0) ) m=j;
        else if ( (l>= 0) && (m>= 0) && (ppl_strCmpNoCase(ud[j].nameFs , ud[m].nameFs)<0) && (ppl_strCmpNoCase(ud[j].nameFs , ud[l].nameFs)>0) ) m=j;
       }
      l=m;
      if (m!=-1)
       {
        k=0;
        SHOW_HIGHLIGHT((ud[m].modified==0));

        #define SHOW_ALL_UNIT_NAMES 0

        sprintf(out+i, "# The '%s', also known as", ud[m].nameFs); i+=strlen(out+i);
        if ((SHOW_ALL_UNIT_NAMES) || (strcmp(ud[m].nameFp, ud[m].nameFs) != 0)) { sprintf(out+i, " '%s' or", ud[m].nameFp); i+=strlen(out+i); k=1; }
        if ((SHOW_ALL_UNIT_NAMES) || (strcmp(ud[m].nameAs, ud[m].nameFs) != 0)) { sprintf(out+i, " '%s' or", ud[m].nameAs); i+=strlen(out+i); k=1; }
        if ((SHOW_ALL_UNIT_NAMES) ||((strcmp(ud[m].nameAp, ud[m].nameAs) != 0) &&
           (strcmp(ud[m].nameAp, ud[m].nameFp) != 0))){sprintf(out+i, " '%s' or", ud[m].nameAp); i+=strlen(out+i); k=1; }

        if (       ud[m].alt1 != NULL                             ) { sprintf(out+i, " '%s' or", ud[m].alt1  ); i+=strlen(out+i); k=1; }
        if (       ud[m].alt2 != NULL                             ) { sprintf(out+i, " '%s' or", ud[m].alt2  ); i+=strlen(out+i); k=1; }
        if (       ud[m].alt3 != NULL                             ) { sprintf(out+i, " '%s' or", ud[m].alt3  ); i+=strlen(out+i); k=1; }
        if (       ud[m].alt4 != NULL                             ) { sprintf(out+i, " '%s' or", ud[m].alt4  ); i+=strlen(out+i); k=1; }
        if (k==0) { i-=15; } else { i-=3; out[i++]=','; }
        sprintf(out+i, " is a unit of %s", ud[m].quantity); i += strlen(out+i);
        if (ud[m].comment != NULL) { sprintf(out+i, " (%s)", ud[m].comment); i += strlen(out+i); }
        sprintf(out+i, ".\n"); i += strlen(out+i);
        SHOW_DEHIGHLIGHT;
       }
     }
    while (m!=-1);
   }

  if (p!=0) ppl_report(&c->errcontext,out);
  return p;
 }

#define TBADD(et,pos) ppl_tbAdd(c,pl->srcLineN,pl->srcId,pl->srcFname,0,et,pos,pl->linetxt,"")

void directive_show(ppl_context *c, parserLine *pl, parserOutput *in, int interactive)
 {
  pplObj            *stk = in->stk;
  int                pos;
  char               itemSet[32];
  pplset_graph      *sg;
  pplarrow_object  **al;
  ppllabel_object  **ll;
  pplset_axis       *xa, *ya, *za;

  interactive = ( interactive && (c->errcontext.session_default.color == SW_ONOFF_ON) );

  if (stk[PARSE_show_editno].objType != PPLOBJ_NUM)
   {
    sg = &c->set->graph_current;
    al = &c->set->pplarrow_list;
    ll = &c->set->ppllabel_list;
    xa = c->set->XAxes; ya = c->set->YAxes; za = c->set->ZAxes;
    itemSet[0]='\0';
   }
  else
   {
    canvas_item *ptr = canvas_items->first;
    int i, editNo = (int)round(stk[PARSE_show_editno].real);
    if ((editNo<1) || (editNo>MULTIPLOT_MAXINDEX) || (canvas_items == NULL)) { sprintf(c->errcontext.tempErrStr, "No multiplot item with index %d.", editNo); ppl_error(&c->errcontext, ERR_GENERAL, -1, -1, NULL); return; }
    for (i=1; i<editNo; i++)
     {
      if (ptr==NULL) break;
      ptr=ptr->next;
     }
    if (ptr == NULL) { sprintf(c->errcontext.tempErrStr, "No multiplot item with index %d.", editNo); ppl_error(&c->errcontext, ERR_GENERAL, -1, -1, NULL); return; }

    sg = &(ptr->settings);
    al = &(ptr->arrow_list);
    ll = &(ptr->label_list);
    xa = ptr->XAxes; ya = ptr->YAxes; za = ptr->ZAxes;
    sprintf(itemSet, "item %d ", editNo);
   }

  pos = PARSE_show_0setting_list;
  if ((stk[pos].objType != PPLOBJ_NUM) || (stk[pos].real <= 0))
   { ppl_error(&c->errcontext, ERR_PREFORMED, -1, -1, ppltxt_show); }
  else
   {
    char textBuffer[SSTR_LENGTH], *showWord=NULL;
    int  p=0,i=0;
    if (interactive!=0) // On interactive sessions, highlight those settings which have been manually set by the user
     {
      sprintf(textBuffer+i,"%sSettings which have not been changed by the user are shown in %s.%s\n",
              *(char **)ppl_fetchSettingName(&c->errcontext,  c->errcontext.session_default.color_rep , SW_TERMCOL_INT , SW_TERMCOL_TXT , sizeof(char *)),
              *(char **)ppl_fetchSettingName(&c->errcontext,  c->errcontext.session_default.color_rep , SW_TERMCOL_INT , SW_TERMCOL_STR , sizeof(char *)),
              *(char **)ppl_fetchSettingName(&c->errcontext,  SW_TERMCOL_NOR                          , SW_TERMCOL_INT , SW_TERMCOL_TXT , sizeof(char *))
             );
      i += strlen(textBuffer+i);
      sprintf(textBuffer+i,"%sSettings which have been changed by the user are shown in %s.%s\n",
              *(char **)ppl_fetchSettingName(&c->errcontext,  c->errcontext.session_default.color_wrn , SW_TERMCOL_INT , SW_TERMCOL_TXT , sizeof(char *)),
              *(char **)ppl_fetchSettingName(&c->errcontext,  c->errcontext.session_default.color_wrn , SW_TERMCOL_INT , SW_TERMCOL_STR , sizeof(char *)),
              *(char **)ppl_fetchSettingName(&c->errcontext,  SW_TERMCOL_NOR                          , SW_TERMCOL_INT , SW_TERMCOL_TXT , sizeof(char *))
             );
      i += strlen(textBuffer+i);
      ppl_report(&c->errcontext,textBuffer);
     }
    while ((stk[pos].objType == PPLOBJ_NUM) && (stk[pos].real > 0))
     {
      pos = (int)round(stk[pos].real);
      showWord = (char *)stk[pos+PARSE_show_setting_0setting_list].auxil;
      if (ppl_strAutocomplete(showWord,"all",1)>=0)
       {
        directive_show2(c, "settings"       ,itemSet, interactive, sg, al, ll, xa, ya, za);
        directive_show2(c, "axes_"          ,itemSet, interactive, sg, al, ll, xa, ya, za);
        directive_show2(c, "arrows"         ,itemSet, interactive, sg, al, ll, xa, ya, za);
        directive_show2(c, "labels"         ,itemSet, interactive, sg, al, ll, xa, ya, za);
        directive_show2(c, "linestyles"     ,itemSet, interactive, sg, al, ll, xa, ya, za);
        directive_show2(c, "variables"      ,itemSet, interactive, sg, al, ll, xa, ya, za);
        directive_show2(c, "userfunctions"  ,itemSet, interactive, sg, al, ll, xa, ya, za);
        //directive_show2(c, "units"         ,itemSet, interactive, sg, al, ll, xa, ya, za);
        p=1;
       }
      else
       {
        p = (directive_show2(c, showWord, itemSet, interactive, sg, al, ll, xa, ya, za) || p);
       }
     }
    if ((p==0) && (showWord!=NULL))
     {
      snprintf(c->errStat.errBuff, LSTR_LENGTH, "Unrecognised show option '%s'.\n\n%s", showWord, ppltxt_show);
      TBADD(ERR_SYNTAX,in->stkCharPos[pos+PARSE_show_setting_0setting_list]);
     }
   }
  return;
 }

