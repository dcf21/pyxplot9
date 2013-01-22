// eps_piechart.c
//
// The code in this file is part of Pyxplot
// <http://www.pyxplot.org.uk>
//
// Copyright (C) 2006-2013 Dominic Ford <coders@pyxplot.org.uk>
//               2008-2013 Ross Church
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

#define _PPL_EPS_PIECHART 1

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

#include <gsl/gsl_math.h>

#include "coreUtils/errorReport.h"
#include "coreUtils/list.h"
#include "coreUtils/memAlloc.h"

#include "expressions/expCompile_fns.h"
#include "expressions/traceback_fns.h"
#include "expressions/expEval.h"

#include "mathsTools/dcfmath.h"

#include "settings/settings.h"
#include "settings/settingTypes.h"
#include "settings/withWords_fns.h"
#include "userspace/context.h"
#include "userspace/contextVarDef.h"
#include "userspace/garbageCollector.h"
#include "userspace/pplObj_fns.h"
#include "userspace/unitsArithmetic.h"
#include "userspace/unitsDisp.h"

#include "epsMaker/canvasDraw.h"
#include "epsMaker/eps_comm.h"
#include "epsMaker/eps_core.h"
#include "epsMaker/eps_piechart.h"
#include "epsMaker/eps_plot.h"
#include "epsMaker/eps_plot_legend.h"
#include "epsMaker/eps_plot_styles.h"
#include "epsMaker/eps_settings.h"

#include "datafile.h"
#include "datafile_rasters.h"

#define COUNTERR_BEGIN if (errCount> 0) { errCount--;
#define COUNTERR_END   if (errCount==0) { sprintf(c->errcontext.tempErrStr, "Too many errors: no more errors will be shown."); \
                       ppl_warning(&c->errcontext,ERR_STACKED,NULL); } }

#define STACK_POP \
   { \
    c->stackPtr--; \
    ppl_garbageObject(&c->stack[c->stackPtr]); \
    if (c->stack[c->stackPtr].refCount != 0) { ppl_warning(&c->errcontext,ERR_STACKED,"Stack forward reference detected."); } \
   }

#define STACK_CLEAN    while (c->stackPtr>stkLevelOld) { STACK_POP; }

void eps_pie_ReadAccessibleData(EPSComm *x)
 {
  ppl_context     *c = x->c;
  int              j, status=0, errCount=DATAFILE_NERRS, NExpect=1, Ncolumns;
  canvas_plotdesc *pd=x->current->plotitems;
  pplExpr         *LabelExpr;
  pplExpr        **UsingList;
  unsigned char    autoUsingList=0;
  int              NUsing;
  int              nObjs=0;
  withWords        ww_default;
  double          *ordinate_raster, acc;
  dataBlock       *blk;

  if (pd==NULL) return;
  NUsing=pd->NUsing;
  UsingList = (pplExpr **)ppl_memAlloc( (USING_ITEMS_MAX+8) * sizeof(pplExpr *) );
  if (UsingList == NULL) { ppl_error(&c->errcontext,ERR_MEMORY, -1, -1,"Out of memory. (A)"); *(x->status) = 1; return; }
  memcpy(UsingList, pd->UsingList, NUsing*sizeof(pplExpr *));

  // Malloc pointer to data table where data to be plotted will be stored
  x->current->plotdata      = (dataTable **)ppl_memAlloc(1 * sizeof(dataTable *));
  x->current->DatasetTextID = (int *)ppl_memAlloc(1 * sizeof(int));
  if (x->current->plotdata == NULL) { ppl_error(&c->errcontext,ERR_MEMORY, -1, -1,"Out of memory. (B)"); *(x->status) = 1; return; }

  // Merge together with words to form a final set
  eps_withwords_default(x, &ww_default, &x->current->settings, 1, 0, 0, 0, c->set->term_current.color==SW_ONOFF_ON);
  ppl_withWordsDestroy(c, &pd->ww_final);
  ppl_withWordsMerge(c, &pd->ww_final, &pd->ww, &x->current->settings.funcStyle, &ww_default, NULL, NULL, 1);
  pd->ww_final.linespoints = SW_STYLE_LINES; // In case FuncStyle is something bonkers like contourplot

  // Make raster on which to evaluate parametric functions
  ordinate_raster = (double *)ppl_memAlloc(x->current->settings.samples * sizeof(double));
  if (x->current->settings.Tlog == SW_BOOL_TRUE) ppl_logRaster(ordinate_raster, x->current->settings.Tmin.real, x->current->settings.Tmax.real, x->current->settings.samples);
  else                                           ppl_linRaster(ordinate_raster, x->current->settings.Tmin.real, x->current->settings.Tmax.real, x->current->settings.samples);

  // Work out what label string to use
  if (pd->label != NULL)
   {
    LabelExpr = pd->label;
   }
  else
   {
    int      end=0,ep=0,es=0;
    char     ascii[] = "\"Item %d\"%($0)";
    pplExpr *exptmp=NULL;
    ppl_expCompile(c,0,0,"",ascii,&end,1,0,0,&exptmp,&ep,&es,c->errcontext.tempErrStr);
    if (es || c->errStat.status) { ppl_tbClear(c); ppl_error(&c->errcontext,ERR_MEMORY, -1, -1,"Out of memory. (C)"); *(x->status) = 1; return; }
    LabelExpr = pplExpr_tmpcpy(exptmp);
    pplExpr_free(exptmp);
    if (LabelExpr==NULL) { ppl_error(&c->errcontext,ERR_MEMORY, -1, -1,"Out of memory. (D)"); *(x->status) = 1; return; }
   }

  if (eps_plot_AddUsingItemsForWithWords(c, &pd->ww_final, &NExpect, &autoUsingList, &UsingList, &NUsing, &nObjs, c->errcontext.tempErrStr)) { ppl_error(&c->errcontext,ERR_GENERIC, -1, -1, NULL); *(x->status) = 1; return; } // Add extra using items for, e.g. "linewidth $3".

  if (pd->filename != NULL) // Read data from file
   {
    if (DEBUG) { sprintf(c->errcontext.tempErrStr, "Reading data from file '%s' for piechart item %d", pd->filename, x->current->id); ppl_log(&c->errcontext,NULL); }
    if (pd->PersistentDataTable==NULL) ppldata_fromFile(c, x->current->plotdata, pd->filename, 0, NULL, NULL, pd->index, UsingList, autoUsingList, NExpect, nObjs, LabelExpr, pd->SelectCriterion, NULL, pd->UsingRowCols, pd->EveryList, pd->continuity, 0, &status, c->errcontext.tempErrStr, &errCount, x->iterDepth+1);
    else                               x->current->plotdata[0] = pd->PersistentDataTable;
   }
  else if (pd->vectors != NULL)
   {
    if (DEBUG) { sprintf(c->errcontext.tempErrStr, "Reading data from vectors for piechart item %d", x->current->id); ppl_log(&c->errcontext,NULL); }
    ppldata_fromVectors(c, x->current->plotdata, pd->vectors, pd->NFunctions, UsingList, autoUsingList, NExpect, nObjs, LabelExpr, pd->SelectCriterion, NULL, pd->continuity, &status, c->errcontext.tempErrStr, &errCount, x->iterDepth+1);
   }
  else
   {
    double *rX = ordinate_raster;
    int     rXl= x->current->settings.samples;
    pplObj *uX = &x->current->settings.Tmin;

    if (DEBUG) { sprintf(c->errcontext.tempErrStr, "Reading data from functions for piechart item %d", x->current->id); ppl_log(&c->errcontext,NULL); }
    ppldata_fromFuncs_checkSpecialRaster(c, pd->functions, pd->NFunctions, "t",
                          &x->current->settings.Tmin.real,
                          &x->current->settings.Tmax.real,
                          uX, &rX, &rXl);
    ppldata_fromFuncs(c, x->current->plotdata, pd->functions, pd->NFunctions, rX, rXl, 1, uX, NULL, 0, NULL, UsingList, autoUsingList, NExpect, nObjs, LabelExpr, pd->SelectCriterion, NULL, pd->continuity, &status, c->errcontext.tempErrStr, &errCount, x->iterDepth+1);
   }
  if (status) { ppl_error(&c->errcontext,ERR_GENERIC, -1, -1, NULL); x->current->plotdata[0]=NULL; }

  // Work out sum of all pie segment sizes
  if (x->current->plotdata[0]==NULL) return;
  Ncolumns = x->current->plotdata[0]->Ncolumns_real;
  blk      = x->current->plotdata[0]->first;
  acc      = 0.0;
  while (blk != NULL)
   {
    for (j=0; j<blk->blockPosition; j++) { acc += fabs(blk->data_real[0 + Ncolumns*j]); }
    blk=blk->next;
   }
  if ((!gsl_finite(acc))||(acc<=0.0)) { sprintf(c->errcontext.tempErrStr, "Sum of sizes of all pie wedges is not a finite number."); ppl_error(&c->errcontext,ERR_GENERIC, -1, -1,NULL); x->current->plotdata[0]=NULL; acc=1; *(x->status) = 1; }
  if (DEBUG) { sprintf(c->errcontext.tempErrStr, "Sum of sizes of all pie wedges = %e", acc); ppl_log(&c->errcontext,NULL); }
  pd->PieChart_total = acc;
  return;
 }

void eps_pie_YieldUpText(EPSComm *x)
 {
  ppl_context     *c = x->c;
  int              j, Ncolumns, errCount=DATAFILE_NERRS;
  pplExpr         *formatExpr;
  canvas_plotdesc *pd;
  dataBlock       *blk;
  pplObj           DummyTemp_l, DummyTemp_p, DummyTemp_w;
  pplObj          *var_l      , *var_p     , *var_w;

  if ((pd=x->current->plotitems)==NULL) return;
  if (x->current->plotdata[0]==NULL) return;
  x->current->FirstTextID = x->NTextItems;

  // Work out what format string to use
  if (x->current->format!=NULL)
   {
    formatExpr = x->current->format;
   }
  else
   {
    char     ascii[]="\"%.0f\\%% %s\"%(percentage,label)";
    int      end=0,ep=0,es=0;
    pplExpr *exptmp=NULL;
    ppl_expCompile(c,0,0,"",ascii,&end,1,0,0,&exptmp,&ep,&es,c->errcontext.tempErrStr);
    if (es || c->errStat.status) { ppl_tbClear(c); ppl_error(&c->errcontext,ERR_MEMORY, -1, -1,"Out of memory. (E)"); *(x->status) = 1; return; }
    formatExpr = pplExpr_tmpcpy(exptmp);
    pplExpr_free(exptmp);
    if (formatExpr==NULL) { ppl_error(&c->errcontext,ERR_MEMORY, -1, -1,"Out of memory. (F)"); *(x->status) = 1; return; }
   }

  // Labels of pie wedges
  Ncolumns = x->current->plotdata[0]->Ncolumns_real;
  blk      = x->current->plotdata[0]->first;
  while (blk != NULL)
   {
    for (j=0; j<blk->blockPosition; j++)
     {
      // Set values of variables [percentage, label, wedgesize] for this wedge
      const int stkLevelOld = c->stackPtr;
      int       lOp=0;
      char     *tmp;
      pplObj   *o;

      // Get pointers to the variables [percentage, label, wedgesize]
      ppl_contextGetVarPointer(c, "label"     , &var_l, &DummyTemp_l);
      ppl_contextGetVarPointer(c, "percentage", &var_p, &DummyTemp_p);
      ppl_contextGetVarPointer(c, "wedgesize" , &var_w, &DummyTemp_w);
      pplObjStr(var_l,0,0,blk->text[j]);
      *var_w            = x->current->plotdata[0]->firstEntries[0];
      var_w->amMalloced = var_w->auxilMalloced = 0;
      var_w->real       = blk->data_real[0 + Ncolumns*j];
      pplObjNum(var_p,0,var_w->real / pd->PieChart_total * 100.0,0);
      o = ppl_expEval(c, formatExpr, &lOp, 1, x->iterDepth+1);
      if (c->errStat.status || (c->dollarStat.warntxt[0]!='\0') || (o==NULL))
       {
        COUNTERR_BEGIN;
        char *errt = NULL;
        if      (c->dollarStat.warntxt[0]!='\0')  errt=c->dollarStat.warntxt;
        else if (c->errStat.errMsgExpr[0]!='\0')  errt=c->errStat.errMsgExpr;
        else if (c->errStat.errMsgCmd [0]!='\0')  errt=c->errStat.errMsgCmd;
        else                                      errt="Fail occurred.";
        sprintf(c->errcontext.tempErrStr, "Error in format expression <%s>: %s", formatExpr->ascii, errt);
        ppl_warning(&c->errcontext,ERR_TYPE,c->errcontext.tempErrStr);
        COUNTERR_END;
        ppl_tbClear(c);
        tmp="?";
       }
      else if (o->objType != PPLOBJ_STR)
       {
        COUNTERR_BEGIN;
        sprintf(c->errcontext.tempErrStr, "Format expression <%s> did not evaluate to a string, but to an object of type <%s>.", formatExpr->ascii, pplObjTypeNames[o->objType]);
        ppl_warning(&c->errcontext,ERR_TYPE,c->errcontext.tempErrStr);
        COUNTERR_END;
        tmp=NULL;
       }
      else
       {
        tmp = (char *)o->auxil;
       }
      if ((tmp==NULL)||(tmp[0]=='\0')) blk->text[j]=NULL;
      YIELD_TEXTITEM_CPY(tmp);

      // Restore values of the variables [percentage, label, wedgesize]
      ppl_contextRestoreVarPointer(c, "label"     , &DummyTemp_l);
      ppl_contextRestoreVarPointer(c, "percentage", &DummyTemp_p);
      ppl_contextRestoreVarPointer(c, "wedgesize" , &DummyTemp_w);
      STACK_CLEAN;
     }
    blk=blk->next;
   }

  // Title of piechart
  x->current->TitleTextID = x->NTextItems;
  YIELD_TEXTITEM(x->current->settings.title);
  return;
 }

void eps_pie_RenderEPS(EPSComm *x)
 {
  ppl_context     *c = x->c;
  int              j, Ncolumns, lt, WedgeNumber, l, m;
  double           xpos,ypos,rad,angle,size,vtitle;
  canvas_plotdesc *pd;
  dataBlock       *blk;
  double           lw, lw_scale;
  withWords        ww, ww_txt;

  if ((pd=x->current->plotitems)==NULL) return;
  if (x->current->plotdata[0]==NULL) return;
  x->LaTeXpageno = x->current->FirstTextID;

  // Print label at top of postscript description of box
  fprintf(x->epsbuffer, "%% Canvas item %d [piechart]\n", x->current->id);
  eps_core_clear(x);

  // Calculate position of centre of piechart, and its radius
  xpos   = x->current->settings.OriginX.real * M_TO_PS;
  ypos   = x->current->settings.OriginX.real * M_TO_PS;
  rad    = x->current->settings.width  .real * M_TO_PS / 2;
  vtitle = ypos+rad;

  // Expand any numbered styles which may appear in the with words we are passed
  ppl_withWordsMerge(c, &ww, &x->current->with_data, NULL, NULL, NULL, NULL, 1);

  // Fill piechart segments
  Ncolumns = x->current->plotdata[0]->Ncolumns_real;
  blk      = x->current->plotdata[0]->first;
  angle    = 0.0;
  WedgeNumber = 0;
  while (blk != NULL)
   {
    for (j=0; j<blk->blockPosition; j++)
     {
      // Work out what fillcolor to use
      for (l=0; l<PALETTE_LENGTH; l++) if (c->set->palette_current[l]==-1) break; // l now contains length of palette
      m = WedgeNumber % l; // m is now the palette color number to use
      while (m<0) m+=l;
      if (c->set->palette_current[m] > 0) { ww.fillcolor = c->set->palette_current[m]; ww.USEfillcolor = 1; ww.USEfillcolor1234 = 0; }
      else                                { ww.FillCol1234Space = c->set->paletteS_current[m]; ww.fillcolor1 = c->set->palette1_current[m]; ww.fillcolor2 = c->set->palette2_current[m]; ww.fillcolor3 = c->set->palette3_current[m]; ww.fillcolor4 = c->set->palette4_current[m]; ww.USEfillcolor = 0; ww.USEfillcolor1234 = 1; }

      // Work out size of wedge and fill it
      size = fabs(blk->data_real[0 + Ncolumns*j]) / pd->PieChart_total * 360.0;
      eps_core_SetFillColor(x, &ww);
      eps_core_SwitchTo_FillColor(x,1);
      IF_NOT_INVISIBLE fprintf(x->epsbuffer, "newpath\n%.2f %.2f %.2f %.2f %.2f arc\n%.2f %.2f lineto\nclosepath\nfill\n",xpos,ypos,rad,90-angle-size,90-angle,xpos,ypos);
      angle += size;
      WedgeNumber++;
     }
    blk=blk->next;
   }

  // Set color of outline of piechart
  eps_core_SetColor(x, &ww, 1);

  // Set linewidth and linetype of outline
  if (ww.USElinewidth) lw_scale = ww.linewidth;
  else                 lw_scale = x->current->settings.LineWidth;
  lw = EPS_DEFAULT_LINEWIDTH * lw_scale;

  if (ww.USElinetype)  lt = ww.linetype;
  else                 lt = 1;

  IF_NOT_INVISIBLE eps_core_SetLinewidth(x, lw, lt, 0.0);

  // Draw circumference of piechart
  IF_NOT_INVISIBLE fprintf(x->epsbuffer, "newpath\n%.2f %.2f %.2f 0 360 arc\nclosepath\nstroke\n", xpos,ypos,rad);

  // Draw pie wedges one-by-one
  Ncolumns = x->current->plotdata[0]->Ncolumns_real;
  blk      = x->current->plotdata[0]->first;
  angle    = 0.0;
  while (blk != NULL)
   {
    for (j=0; j<blk->blockPosition; j++)
     {
      size = fabs(blk->data_real[0 + Ncolumns*j]) / pd->PieChart_total * 360.0;
      IF_NOT_INVISIBLE fprintf(x->epsbuffer, "newpath\n%.2f %.2f moveto\n%.2f %.2f lineto\nclosepath\nstroke\n",xpos,ypos,xpos+rad*sin(angle*M_PI/180),ypos+rad*cos(angle*M_PI/180));
      angle += size;
     }
    blk=blk->next;
   }

  // Update bounding box
  eps_core_BoundingBox(x, xpos-rad, ypos-rad, lw);
  eps_core_BoundingBox(x, xpos+rad, ypos-rad, lw);
  eps_core_BoundingBox(x, xpos-rad, ypos+rad, lw);
  eps_core_BoundingBox(x, xpos+rad, ypos+rad, lw);

  // Write text on piechart
  ppl_withWordsZero(c, &ww_txt);
  if (x->current->settings.TextColor > 0) { ww_txt.color = x->current->settings.TextColor; ww_txt.USEcolor = 1; }
  else                                     { ww_txt.Col1234Space = x->current->settings.TextCol1234Space; ww_txt.color1 = x->current->settings.TextColor1; ww_txt.color2 = x->current->settings.TextColor2; ww_txt.color3 = x->current->settings.TextColor3; ww_txt.color4 = x->current->settings.TextColor4; ww_txt.USEcolor1234 = 1; }
  eps_core_SetColor(x, &ww_txt, 1);

  IF_NOT_INVISIBLE
   {
    int ItemNo=0;
    double *TextWidth, *TextHeight, TotalHeight=0.0;
    double height1,height2,bb_top,bb_bottom,ab_left,ab_right,ab_top,ab_bottom;
    double label_rpos, best_label_rpos, best_label_margin, min_margin, key_vpos=0.0;
    postscriptPage *dviPage;

    // Calculate dimensions of text items
    TextWidth  = (double *)ppl_memAlloc(x->current->plotdata[0]->Nrows * sizeof(double));
    TextHeight = (double *)ppl_memAlloc(x->current->plotdata[0]->Nrows * sizeof(double));
    if ((TextWidth==NULL)||(TextHeight==NULL)) goto END_LABEL_PRINTING;
    blk        = x->current->plotdata[0]->first;
    while (blk != NULL)
     {
      for (j=0; j<blk->blockPosition; j++)
       {
        dviPage = (postscriptPage *)ppl_listGetItem(x->dvi->output->pages, x->LaTeXpageno+ItemNo);
        if (dviPage== NULL) { ItemNo++; continue; } // Such doom will trigger errors later
        //bb_left = dviPage->boundingBox[0];
        bb_bottom = dviPage->boundingBox[1];
        //bb_right= dviPage->boundingBox[2];
        bb_top    = dviPage->boundingBox[3];
        ab_left   = dviPage->textSizeBox[0];
        ab_bottom = dviPage->textSizeBox[1];
        ab_right  = dviPage->textSizeBox[2];
        ab_top    = dviPage->textSizeBox[3];
        height1   = fabs(ab_top - ab_bottom) * AB_ENLARGE_FACTOR;
        height2   = fabs(bb_top - bb_bottom) * BB_ENLARGE_FACTOR;
        TextWidth [ItemNo] = ((ab_right - ab_left) + MARGIN_HSIZE  ) * x->current->settings.FontSize;
        TextHeight[ItemNo] = ((height2<height1) ? height2 : height1) * x->current->settings.FontSize;
        TotalHeight += TextHeight[ItemNo];
        ItemNo++;
       }
      blk=blk->next;
     }

    // Labels of pie wedges
    Ncolumns = x->current->plotdata[0]->Ncolumns_real;
    blk      = x->current->plotdata[0]->first;
    angle    = 0.0;
    ItemNo   = 0;
    while (blk != NULL)
     {
      for (j=0; j<blk->blockPosition; j++)
       {
        size = fabs(blk->data_real[0 + Ncolumns*j]) / pd->PieChart_total * 2 * M_PI; // Angular size of wedge
        if (blk->text[j]!=NULL)
         {
          int    ArrowType;
          double a = angle+size/2; // Central angle of wedge

          // Test different radii where label can be placed
          best_label_rpos   = 0.7;
          best_label_margin = -1e10;
          for (label_rpos=0.7; label_rpos>=0.3; label_rpos-=0.05)
           {
            double r,theta,t;
            min_margin = 1e10;

#define CHECK_POSITION(X,Y) \
   r     = hypot(X,Y); \
   theta = fmod(atan2(X,Y) - a, 2*M_PI); if (theta<-M_PI) theta+=2*M_PI; if (theta>M_PI) theta-=2*M_PI; \
   t     = theta + size/2; \
   if (t < min_margin) min_margin = t; \
   t     = size/2 - theta; \
   if (t < min_margin) min_margin = t; \
   t     = r / rad; \
   if (t < min_margin) min_margin = t; \
   t     = (rad-r) / rad; \
   if (t < min_margin) min_margin = t; \

            CHECK_POSITION( rad*sin(a)*label_rpos-TextWidth[ItemNo]/2 , rad*cos(a)*label_rpos-TextHeight[ItemNo]/2 );
            CHECK_POSITION( rad*sin(a)*label_rpos-TextWidth[ItemNo]/2 , rad*cos(a)*label_rpos+TextHeight[ItemNo]/2 );
            CHECK_POSITION( rad*sin(a)*label_rpos+TextWidth[ItemNo]/2 , rad*cos(a)*label_rpos-TextHeight[ItemNo]/2 );
            CHECK_POSITION( rad*sin(a)*label_rpos+TextWidth[ItemNo]/2 , rad*cos(a)*label_rpos+TextHeight[ItemNo]/2 );
            if (min_margin>best_label_margin) { best_label_margin = min_margin; best_label_rpos = label_rpos; }
           }

          // Work out whether label will go inside, outside or in a key
          ArrowType = x->current->ArrowType;
          if (ArrowType==SW_PIEKEYPOS_AUTO) ArrowType = (best_label_margin<0.0) ? SW_PIEKEYPOS_OUTSIDE : SW_PIEKEYPOS_INSIDE;

          if (ArrowType==SW_PIEKEYPOS_INSIDE)
           { // Labelling pie wedges inside pie
            int pageno = x->LaTeXpageno++;
            eps_core_SetColor(x, &ww_txt, 1);
            canvas_EPSRenderTextItem(x, NULL, pageno, (xpos+rad*sin(a)*best_label_rpos)/M_TO_PS, (ypos+rad*cos(a)*best_label_rpos)/M_TO_PS, SW_HALIGN_CENT, SW_VALIGN_CENT, x->CurrentColor, x->current->settings.FontSize, 0.0, NULL, NULL);
           }
          else if (ArrowType==SW_PIEKEYPOS_OUTSIDE)
           { // Labelling pie wedges around edge of pie
            int    hal, val, pageno = x->LaTeXpageno++;
            double vtop;
            if      (a <   5*M_PI/180) { hal=SW_HALIGN_CENT ; val=SW_VALIGN_BOT;  }
            else if (a <  30*M_PI/180) { hal=SW_HALIGN_LEFT ; val=SW_VALIGN_BOT;  }
            else if (a < 150*M_PI/180) { hal=SW_HALIGN_LEFT ; val=SW_VALIGN_CENT; }
            else if (a < 175*M_PI/180) { hal=SW_HALIGN_LEFT ; val=SW_VALIGN_TOP;  }
            else if (a < 185*M_PI/180) { hal=SW_HALIGN_CENT ; val=SW_VALIGN_TOP;  }
            else if (a < 210*M_PI/180) { hal=SW_HALIGN_RIGHT; val=SW_VALIGN_TOP;  }
            else if (a < 330*M_PI/180) { hal=SW_HALIGN_RIGHT; val=SW_VALIGN_CENT; }
            else if (a < 355*M_PI/180) { hal=SW_HALIGN_RIGHT; val=SW_VALIGN_BOT;  }
            else                       { hal=SW_HALIGN_CENT ; val=SW_VALIGN_BOT;  }
            if      (val == SW_VALIGN_BOT ) vtop = ypos+rad*cos(a)*1.08 + TextHeight[ItemNo];
            else if (val == SW_VALIGN_CENT) vtop = ypos+rad*cos(a)*1.08 + TextHeight[ItemNo]/2;
            else                            vtop = ypos+rad*cos(a)*1.08;
            if (vtop > vtitle) vtitle=vtop;
            eps_core_SetColor(x, &ww, 1);
            IF_NOT_INVISIBLE fprintf(x->epsbuffer, "newpath\n%.2f %.2f moveto\n%.2f %.2f lineto\nclosepath\nstroke\n",xpos+rad*sin(a),ypos+rad*cos(a),xpos+rad*sin(a)*1.05,ypos+rad*cos(a)*1.05);
            eps_core_SetColor(x, &ww_txt, 1);
            canvas_EPSRenderTextItem(x, NULL, pageno, (xpos+rad*sin(a)*1.08)/M_TO_PS, (ypos+rad*cos(a)*1.08)/M_TO_PS, hal, val, x->CurrentColor, x->current->settings.FontSize, 0.0, NULL, NULL);
           }
          else // Labelling pie wedges in a key
           {
            int    pageno = x->LaTeXpageno++;
            double h = xpos + rad*1.2; // Position of left of key
            double v = ypos + TotalHeight/2 - key_vpos - TextHeight[ItemNo]/2; // Vertical position of centre of key item
            double s = MARGIN_HSIZE_LEFT*0.45/3; // Controls size of filled square which appears next to wedge label
            double st= MARGIN_HSIZE_LEFT*0.67;

            // Work out what fillcolor to use
            for (l=0; l<PALETTE_LENGTH; l++) if (c->set->palette_current[l]==-1) break; // l now contains length of palette
            m = ItemNo % l; // m is now the palette color number to use
            while (m<0) m+=l;
            if (c->set->palette_current[m] > 0) { ww.fillcolor = c->set->palette_current[m]; ww.USEfillcolor = 1; ww.USEfillcolor1234 = 0; }
            else                                { ww.FillCol1234Space = c->set->paletteS_current[m]; ww.fillcolor1 = c->set->palette1_current[m]; ww.fillcolor2 = c->set->palette2_current[m]; ww.fillcolor3 = c->set->palette3_current[m]; ww.fillcolor4 = c->set->palette4_current[m]; ww.USEfillcolor = 0; ww.USEfillcolor1234 = 1; }

            // Fill icon
            eps_core_SetFillColor(x, &ww);
            eps_core_SwitchTo_FillColor(x,1);
            IF_NOT_INVISIBLE
             {
              fprintf(x->epsbuffer, "newpath %.2f %.2f moveto %.2f %.2f lineto %.2f %.2f lineto %.2f %.2f lineto closepath fill\n", h, v-s, h, v+s, h+2*s, v+s, h+2*s, v-s);
              eps_core_BoundingBox(x, h    , v-s, 0);
              eps_core_BoundingBox(x, h    , v+s, 0);
              eps_core_BoundingBox(x, h+2*s, v-s, 0);
              eps_core_BoundingBox(x, h+2*s, v+s, 0);
             }

            // Stroke icon
            eps_core_SetColor(x, &ww, 1);
            eps_core_SetLinewidth(x, EPS_DEFAULT_LINEWIDTH * ww.linewidth, ww.USElinetype ? ww.linetype : 1, 0);
            IF_NOT_INVISIBLE
             {
              fprintf(x->epsbuffer, "newpath %.2f %.2f moveto %.2f %.2f lineto %.2f %.2f lineto %.2f %.2f lineto closepath stroke\n", h, v-s, h, v+s, h+2*s, v+s, h+2*s, v-s);
              eps_core_BoundingBox(x, h    , v-s, ww.linewidth);
              eps_core_BoundingBox(x, h    , v+s, ww.linewidth);
              eps_core_BoundingBox(x, h+2*s, v-s, ww.linewidth);
              eps_core_BoundingBox(x, h+2*s, v+s, ww.linewidth);
             }

            // Write text next to icon
            eps_core_SetColor(x, &ww_txt, 1);
            canvas_EPSRenderTextItem(x, NULL, pageno, (h+st)/M_TO_PS, v/M_TO_PS, SW_HALIGN_LEFT, SW_VALIGN_CENT, x->CurrentColor, x->current->settings.FontSize, 0.0, NULL, NULL);
            key_vpos += TextHeight[ItemNo];
           }
         }
        angle += size;
        ItemNo++;
       }
      blk=blk->next;
     }

END_LABEL_PRINTING:

    // Title of piechart
    x->LaTeXpageno = x->current->TitleTextID;
    if ((x->current->settings.title != NULL) && (x->current->settings.title[0] != '\0'))
     {
      int pageno = x->LaTeXpageno++;
      canvas_EPSRenderTextItem(x, NULL, pageno, xpos/M_TO_PS, (vtitle+8)/M_TO_PS, SW_HALIGN_CENT, SW_VALIGN_BOT, x->CurrentColor, x->current->settings.FontSize, 0.0, NULL, NULL);
     }
   }

  // Final newline at end of canvas item
  fprintf(x->epsbuffer, "\n");
  return;
 }

