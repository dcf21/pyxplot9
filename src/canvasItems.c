// canvasItems.c
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

#define _CANVASITEMS_C 1

#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "canvasItems.h"
#include "datafile.h"

#include "coreUtils/dict.h"
#include "coreUtils/memAlloc.h"
#include "epsMaker/canvasDraw.h"
#include "expressions/expCompile_fns.h"
#include "expressions/traceback_fns.h"
#include "settings/arrows_fns.h"
#include "settings/axes_fns.h"
#include "settings/labels_fns.h"
#include "settings/settings.h"
#include "settings/settingTypes.h"
#include "settings/withWords_fns.h"
#include "stringTools/asciidouble.h"
#include "parser/cmdList.h"
#include "parser/parser.h"
#include "userspace/context.h"
#include "userspace/pplObj.h"

#define TBADD(et,msg,pos) { strcpy(c->errStat.errBuff, msg); ppl_tbAdd(c,pl->srcLineN,pl->srcId,pl->srcFname,0,et,pos,pl->linetxt,""); }

#define TBADD2(et,pos) { ppl_tbAdd(c,pl->srcLineN,pl->srcId,pl->srcFname,0,et,pos,pl->linetxt,""); }

// Free a node in the multiplot canvas item list
static void canvas_item_delete(ppl_context *c, canvas_item *ptr)
 {
  int               i;
  canvas_itemlist  *canvas_items = c->canvas_items;
  canvas_plotrange *pr, *pr2;
  canvas_plotdesc  *pd, *pd2;

  if (ptr->text  != NULL) free(ptr->text);
  ppl_withWordsDestroy(c, &(ptr->settings.dataStyle));
  ppl_withWordsDestroy(c, &(ptr->settings.funcStyle));
  if (ptr->XAxes != NULL) { for (i=0; i<MAX_AXES; i++) pplaxis_destroy(c, &(ptr->XAxes[i]) ); free(ptr->XAxes); }
  if (ptr->YAxes != NULL) { for (i=0; i<MAX_AXES; i++) pplaxis_destroy(c, &(ptr->YAxes[i]) ); free(ptr->YAxes); }
  if (ptr->ZAxes != NULL) { for (i=0; i<MAX_AXES; i++) pplaxis_destroy(c, &(ptr->ZAxes[i]) ); free(ptr->ZAxes); }
  pplarrow_list_destroy(c, &(ptr->arrow_list));
  ppllabel_list_destroy(c, &(ptr->label_list));
  ppl_withWordsDestroy(c, &(ptr->with_data));

  // Delete range structures
  pr = ptr->plotranges;
  while (pr != NULL)
   {
    pr2 = pr->next;
    free(pr);
    pr = pr2;
   }

  // Delete plot item descriptors
  pd = ptr->plotitems;
  while (pd != NULL)
   {
    pd2 = pd->next;
    for (i=0; i<pd->NFunctions; i++) pplExpr_free(pd->functions[i]);
    for (i=0; i<pd->NUsing    ; i++) pplExpr_free(pd->UsingList[i]);
    if (pd->filename        != NULL) free(pd->filename);
    if (pd->functions       != NULL) free(pd->functions);
    if (pd->label           != NULL) pplExpr_free(pd->label);
    if (pd->SelectCriterion != NULL) pplExpr_free(pd->SelectCriterion);
    if (pd->title           != NULL) free(pd->title);
    if (pd->UsingList       != NULL) free(pd->UsingList);
    free(pd);
    pd = pd2;
   }

  // Reseal doubly-linked list
  if (ptr->prev != NULL) ptr->prev->next = ptr->next; else canvas_items->first = ptr->next;
  if (ptr->next != NULL) ptr->next->prev = ptr->prev; else canvas_items->last  = ptr->prev;

  free(ptr);
  return;
 }

// Add a new multiplot canvas item to the list above
static int canvas_itemlist_add(ppl_context *c, pplObj *command, int type, canvas_item **output, int *id, unsigned char includeAxes)
 {
  canvas_itemlist *canvas_items = c->canvas_items;
  canvas_item     *ptr, *next, *prev, **insertpointA, **insertpointB;
  int              i, PrevId=-2, editNo, gotEditNo;

  // If we're not in multiplot mode, clear the canvas now
  if (c->set->term_current.multiplot == SW_ONOFF_OFF) directive_clear(c, NULL, NULL, 0);

  // Ensure that multiplot canvas list is initialised before trying to use it
  if (canvas_items == NULL)
   {
    canvas_items = (canvas_itemlist *)malloc(sizeof(canvas_itemlist));
    if (canvas_items == NULL) return 1;
    c->canvas_items = (void *)canvas_items;
    canvas_items->first = NULL;
    canvas_items->last  = NULL;
   }

  gotEditNo = (command[PARSE_arc_editno].objType == PPLOBJ_NUM);
  editNo    = round(command[PARSE_arc_editno].real);
  if (!gotEditNo)
   {
    insertpointA = &(canvas_items->last);
    next         = NULL;
    prev         = (*insertpointA==NULL) ? NULL                   :   canvas_items->last;
    insertpointB = (*insertpointA==NULL) ? &(canvas_items->first) : &(canvas_items->last->next);
    PrevId       = (*insertpointA==NULL) ? 0                      :   canvas_items->last->id;
   }
  else
   {
    insertpointB = &(canvas_items->first);
    prev         = NULL;
    while ((*insertpointB != NULL) && ((*insertpointB)->id < editNo)) { prev = *insertpointB; insertpointB = &((*insertpointB)->next); }
    next = *insertpointB;
    if (next==NULL)
     {
      insertpointA = &(canvas_items->last);
     }
    else if (next->id == editNo)
     {
      next = next->next;
      if (next==NULL) insertpointA = &(canvas_items->last);
      else            insertpointA = &(next->prev);
      canvas_item_delete(c, *insertpointB);
     }
    else
     {
      insertpointA = &(next->prev);
     }
   }
  ptr = (canvas_item *)malloc(sizeof(canvas_item));
  if (ptr==NULL) return 1;
  *insertpointA = *insertpointB = ptr;
  ptr->next    = next; // Link doubly-linked list
  ptr->prev    = prev;
  ptr->text    = NULL;
  ptr->plotitems  = NULL;
  ptr->plotranges = NULL;
  ptr->id      = (!gotEditNo) ? (PrevId+1) : editNo;
  ptr->type    = type;
  ptr->deleted = 0;
  ppl_withWordsZero(c, &ptr->with_data);

  // Copy the user's current settings
  ptr->settings = c->set->graph_current;
  ppl_withWordsCpy(c, &ptr->settings.dataStyle , &c->set->graph_current.dataStyle);
  ppl_withWordsCpy(c, &ptr->settings.funcStyle , &c->set->graph_current.funcStyle);
  if (includeAxes)
   {
    ptr->XAxes = (pplset_axis *)malloc(MAX_AXES * sizeof(pplset_axis));
    ptr->YAxes = (pplset_axis *)malloc(MAX_AXES * sizeof(pplset_axis));
    ptr->ZAxes = (pplset_axis *)malloc(MAX_AXES * sizeof(pplset_axis));
    if ((ptr->XAxes==NULL)||(ptr->YAxes==NULL)||(ptr->ZAxes==NULL))
     {
      ppl_error(&c->errcontext, ERR_MEMORY, -1, -1,"Out of memory");
      if (ptr->XAxes!=NULL) { free(ptr->XAxes); ptr->XAxes = NULL; }
      if (ptr->YAxes!=NULL) { free(ptr->YAxes); ptr->YAxes = NULL; }
      if (ptr->ZAxes!=NULL) { free(ptr->ZAxes); ptr->ZAxes = NULL; }
     }
    else
     {
      for (i=0; i<MAX_AXES; i++) pplaxis_copy(c, &(ptr->XAxes[i]), &(c->set->XAxes[i]));
      for (i=0; i<MAX_AXES; i++) pplaxis_copy(c, &(ptr->YAxes[i]), &(c->set->YAxes[i]));
      for (i=0; i<MAX_AXES; i++) pplaxis_copy(c, &(ptr->ZAxes[i]), &(c->set->ZAxes[i]));
     }
    pplarrow_list_copy(c, &ptr->arrow_list , &c->set->pplarrow_list);
    ppllabel_list_copy(c, &ptr->label_list , &c->set->ppllabel_list);
   } else {
    ptr->XAxes = ptr->YAxes = ptr->ZAxes = NULL;
    ptr->arrow_list = NULL;
    ptr->label_list = NULL;
   }

  *output = ptr; // Return pointer to the newly-created canvas item
  *id     = ptr->id;
  return 0;
 }

// Implementation of the clear command. Also called whenever the canvas is to be cleared.
int directive_clear(ppl_context *c, parserLine *pl, parserOutput *in, int interactive)
 {
  canvas_itemlist *canvas_items = c->canvas_items;
  canvas_item     *ptr, *next;

  if (canvas_items == NULL) return 0;
  ptr = canvas_items->first;
  while (ptr != NULL)
   {
    next = ptr->next;
    ptr->prev = NULL;
    canvas_item_delete(c, ptr);
    ptr = next;
   }
  free(canvas_items);
  c->canvas_items = NULL;
  return 0;
 }

// Produce a textual representation of the command which would need to be typed to produce any given canvas item
char *ppl_canvas_item_textify(ppl_context *c, canvas_item *ptr, char *output)
 {
  int i,j;
  if      (ptr->type == CANVAS_ARROW) // Produce textual representations of arrow commands
   {
    sprintf(output, "%s item %d from %s,%s to %s,%s with %s", (ptr->ArrowType==SW_ARROWTYPE_NOHEAD) ? "line" : "arrow", ptr->id,
             ppl_numericDisplay( ptr->xpos            *100, c->numdispBuff[0], c->set->term_current.SignificantFigures, (c->set->term_current.NumDisplay==SW_DISPLAY_L)),
             ppl_numericDisplay( ptr->ypos            *100, c->numdispBuff[1], c->set->term_current.SignificantFigures, (c->set->term_current.NumDisplay==SW_DISPLAY_L)),
             ppl_numericDisplay((ptr->xpos+ptr->xpos2)*100, c->numdispBuff[2], c->set->term_current.SignificantFigures, (c->set->term_current.NumDisplay==SW_DISPLAY_L)),
             ppl_numericDisplay((ptr->ypos+ptr->ypos2)*100, c->numdispBuff[3], c->set->term_current.SignificantFigures, (c->set->term_current.NumDisplay==SW_DISPLAY_L)),
             *(char **)ppl_fetchSettingName(&c->errcontext, ptr->ArrowType, SW_ARROWTYPE_INT, (void *)SW_ARROWTYPE_STR, sizeof(char *))
           );
    i = strlen(output);
    ppl_withWordsPrint(c, &ptr->with_data, output+i+1);
    if (strlen(output+i+1)>0) { output[i]=' ';  }
    else                      { output[i]='\0'; }
   }
  else if (ptr->type == CANVAS_BOX  ) // Produce textual representations of box commands
   {
    sprintf(output, "box item %d ", ptr->id);
    i = strlen(output);
    sprintf(output+i, ptr->xpos2set ? "at %s,%s width %s height %s" : "from %s,%s to %s,%s",
             ppl_numericDisplay( ptr->xpos                             *100, c->numdispBuff[0], c->set->term_current.SignificantFigures, (c->set->term_current.NumDisplay==SW_DISPLAY_L)),
             ppl_numericDisplay( ptr->ypos                             *100, c->numdispBuff[1], c->set->term_current.SignificantFigures, (c->set->term_current.NumDisplay==SW_DISPLAY_L)),
             ppl_numericDisplay((ptr->xpos*(!ptr->xpos2set)+ptr->xpos2)*100, c->numdispBuff[2], c->set->term_current.SignificantFigures, (c->set->term_current.NumDisplay==SW_DISPLAY_L)),
             ppl_numericDisplay((ptr->ypos*(!ptr->xpos2set)+ptr->ypos2)*100, c->numdispBuff[3], c->set->term_current.SignificantFigures, (c->set->term_current.NumDisplay==SW_DISPLAY_L))
           );
    i += strlen(output+i);
    sprintf(output+i, " rotate %s",
             ppl_numericDisplay(ptr->rotation * 180/M_PI , c->numdispBuff[2], c->set->term_current.SignificantFigures, (c->set->term_current.NumDisplay==SW_DISPLAY_L))
           );
    i += strlen(output+i);
    ppl_withWordsPrint(c, &ptr->with_data, output+i+6);
    if (strlen(output+i+6)>0) { sprintf(output+i, " with"); output[i+5]=' '; }
    else                      { output[i]='\0'; }
   }
  else if (ptr->type == CANVAS_CIRC ) // Produce textual representations of circle commands
   {
    sprintf(output, "%s item %d at %s,%s radius %s", ptr->xfset ? "arc" : "circle", ptr->id,
             ppl_numericDisplay( ptr->xpos            *100, c->numdispBuff[0], c->set->term_current.SignificantFigures, (c->set->term_current.NumDisplay==SW_DISPLAY_L)),
             ppl_numericDisplay( ptr->ypos            *100, c->numdispBuff[1], c->set->term_current.SignificantFigures, (c->set->term_current.NumDisplay==SW_DISPLAY_L)),
             ppl_numericDisplay( ptr->xpos2           *100, c->numdispBuff[2], c->set->term_current.SignificantFigures, (c->set->term_current.NumDisplay==SW_DISPLAY_L))
           );
    i = strlen(output);
    if (ptr->xfset)
     {
      sprintf(output+i," from %s to %s",
               ppl_numericDisplay( ptr->xf         *180/M_PI, c->numdispBuff[0], c->set->term_current.SignificantFigures, (c->set->term_current.NumDisplay==SW_DISPLAY_L)),
               ppl_numericDisplay( ptr->yf         *180/M_PI, c->numdispBuff[1], c->set->term_current.SignificantFigures, (c->set->term_current.NumDisplay==SW_DISPLAY_L))
             );
      i += strlen(output+i);
     }
    ppl_withWordsPrint(c, &ptr->with_data, output+i+6);
    if (strlen(output+i+6)>0) { sprintf(output+i, " with"); output[i+5]=' '; }
    else                      { output[i]='\0'; }
   }
  else if (ptr->type == CANVAS_ELLPS) // Produce textual representations of ellipse commands
   {
    sprintf(output, "ellipse item %d", ptr->id);
    i = strlen(output);
    if (ptr->x1set ) sprintf(output+i, " from %s,%s to %s,%s",
             ppl_numericDisplay( ptr->x1  *100, c->numdispBuff[0], c->set->term_current.SignificantFigures, (c->set->term_current.NumDisplay==SW_DISPLAY_L)),
             ppl_numericDisplay( ptr->y1  *100, c->numdispBuff[1], c->set->term_current.SignificantFigures, (c->set->term_current.NumDisplay==SW_DISPLAY_L)),
             ppl_numericDisplay( ptr->x2  *100, c->numdispBuff[2], c->set->term_current.SignificantFigures, (c->set->term_current.NumDisplay==SW_DISPLAY_L)),
             ppl_numericDisplay( ptr->y2  *100, c->numdispBuff[3], c->set->term_current.SignificantFigures, (c->set->term_current.NumDisplay==SW_DISPLAY_L))
           );
    i += strlen(output+i);
    if (ptr->xcset ) sprintf(output+i, " centre %s,%s",
             ppl_numericDisplay( ptr->xc  *100, c->numdispBuff[0], c->set->term_current.SignificantFigures, (c->set->term_current.NumDisplay==SW_DISPLAY_L)),
             ppl_numericDisplay( ptr->yc  *100, c->numdispBuff[1], c->set->term_current.SignificantFigures, (c->set->term_current.NumDisplay==SW_DISPLAY_L))
           );
    i += strlen(output+i);
    if (ptr->xfset ) sprintf(output+i, " focus %s,%s",
             ppl_numericDisplay( ptr->xf  *100, c->numdispBuff[0], c->set->term_current.SignificantFigures, (c->set->term_current.NumDisplay==SW_DISPLAY_L)),
             ppl_numericDisplay( ptr->yf  *100, c->numdispBuff[1], c->set->term_current.SignificantFigures, (c->set->term_current.NumDisplay==SW_DISPLAY_L))
           );
    i += strlen(output+i);
    if (ptr->aset  ) sprintf(output+i, " SemiMajorAxis %s",
             ppl_numericDisplay( ptr->a   *100, c->numdispBuff[0], c->set->term_current.SignificantFigures, (c->set->term_current.NumDisplay==SW_DISPLAY_L))
           );
    i += strlen(output+i);
    if (ptr->bset  ) sprintf(output+i, " SemiMinorAxis %s",
             ppl_numericDisplay( ptr->b   *100, c->numdispBuff[0], c->set->term_current.SignificantFigures, (c->set->term_current.NumDisplay==SW_DISPLAY_L))
           );
    i += strlen(output+i);
    if (ptr->eccset) sprintf(output+i, " eccentricity %s",
             ppl_numericDisplay( ptr->ecc     , c->numdispBuff[0], c->set->term_current.SignificantFigures, (c->set->term_current.NumDisplay==SW_DISPLAY_L))
           );
    i += strlen(output+i);
    if (ptr->slrset) sprintf(output+i, " SemiLatusRectum %s",
             ppl_numericDisplay( ptr->slr *100, c->numdispBuff[0], c->set->term_current.SignificantFigures, (c->set->term_current.NumDisplay==SW_DISPLAY_L))
           );
    i += strlen(output+i);
    sprintf(output+i, " rotate %s",
             ppl_numericDisplay( ptr->rotation *180/M_PI , c->numdispBuff[0], c->set->term_current.SignificantFigures, (c->set->term_current.NumDisplay==SW_DISPLAY_L))
           );
    i += strlen(output+i);
    ppl_withWordsPrint(c, &ptr->with_data, output+i+6);
    if (strlen(output+i+6)>0) { sprintf(output+i, " with"); output[i+5]=' '; }
    else                      { output[i]='\0'; }
   }
  else if (ptr->type == CANVAS_EPS  ) // Produce textual representations of eps commands
   {
    sprintf(output, "eps item %d ", ptr->id);
    i = strlen(output);
    ppl_strEscapify(ptr->text, output+i);
    i += strlen(output+i);
    sprintf(output+i, " at %s,%s",
             ppl_numericDisplay(ptr->xpos*100, c->numdispBuff[0], c->set->term_current.SignificantFigures, (c->set->term_current.NumDisplay==SW_DISPLAY_L)),
             ppl_numericDisplay(ptr->ypos*100, c->numdispBuff[1], c->set->term_current.SignificantFigures, (c->set->term_current.NumDisplay==SW_DISPLAY_L))
           );
    i += strlen(output+i);
    if (ptr->xpos2set) sprintf(output+i, " width %s" , ppl_numericDisplay(ptr->xpos2*100, c->numdispBuff[0], c->set->term_current.SignificantFigures, (c->set->term_current.NumDisplay==SW_DISPLAY_L)));
    i += strlen(output+i);
    if (ptr->ypos2set) sprintf(output+i, " height %s", ppl_numericDisplay(ptr->ypos2*100, c->numdispBuff[0], c->set->term_current.SignificantFigures, (c->set->term_current.NumDisplay==SW_DISPLAY_L)));
    i += strlen(output+i);
    sprintf(output+i, " rotate %s",
             ppl_numericDisplay(ptr->rotation * 180/M_PI , c->numdispBuff[2], c->set->term_current.SignificantFigures, (c->set->term_current.NumDisplay==SW_DISPLAY_L))
           );
    i += strlen(output+i);
    if (ptr->clip    ) { sprintf(output+i, " clip"    ); i += strlen(output+i); }
    if (ptr->calcbbox) { sprintf(output+i, " calcbbox"); i += strlen(output+i); }
   }
  else if (ptr->type == CANVAS_IMAGE) // Produce textual representations of image commands
   {
    sprintf(output, "image item %d ", ptr->id);
    i = strlen(output);
    ppl_strEscapify(ptr->text, output+i);
    i += strlen(output+i);
    sprintf(output+i, " at %s,%s",
             ppl_numericDisplay(ptr->xpos*100, c->numdispBuff[0], c->set->term_current.SignificantFigures, (c->set->term_current.NumDisplay==SW_DISPLAY_L)),
             ppl_numericDisplay(ptr->ypos*100, c->numdispBuff[1], c->set->term_current.SignificantFigures, (c->set->term_current.NumDisplay==SW_DISPLAY_L))
           );
    i += strlen(output+i);
    if (ptr->smooth            ) { sprintf(output+i, " smooth");                                                                  i += strlen(output+i); }
    if (ptr->NoTransparency    ) { sprintf(output+i, " NoTransparency");                                                          i += strlen(output+i); }
    if (ptr->CustomTransparency) { sprintf(output+i, " transparent rgb%d:%d:%d", ptr->TransColR, ptr->TransColG, ptr->TransColB); i += strlen(output+i); }
    if (ptr->xpos2set) sprintf(output+i, " width %s" , ppl_numericDisplay(ptr->xpos2*100, c->numdispBuff[0], c->set->term_current.SignificantFigures, (c->set->term_current.NumDisplay==SW_DISPLAY_L)));
    i += strlen(output+i);
    if (ptr->ypos2set) sprintf(output+i, " height %s", ppl_numericDisplay(ptr->ypos2*100, c->numdispBuff[0], c->set->term_current.SignificantFigures, (c->set->term_current.NumDisplay==SW_DISPLAY_L)));
    i += strlen(output+i);
    sprintf(output+i, " rotate %s",
             ppl_numericDisplay(ptr->rotation * 180/M_PI , c->numdispBuff[2], c->set->term_current.SignificantFigures, (c->set->term_current.NumDisplay==SW_DISPLAY_L))
           );
   }
  else if (ptr->type == CANVAS_PIE) // Produce textual representations of piechart commands
   {
    canvas_plotdesc  *pd;

    sprintf(output, "piechart item %d", ptr->id);
    i = strlen(output);
    pd = ptr->plotitems;
    if (pd!=NULL)
     {
      if (!pd->function) { output[i++]=' '; ppl_strEscapify(pd->filename, output+i); i+=strlen(output+i); } // Filename of datafile we are plotting
      else
       for (j=0; j<pd->NFunctions; j++) // Print out the list of functions which we are plotting
        {
         output[i++]=(j!=0)?':':' ';
         ppl_strStrip(pd->functions[j]->ascii , output+i);
         i+=strlen(output+i);
        }
      if (pd->EverySet>0) { sprintf(output+i, " every %d", pd->EveryList[0]); i+=strlen(output+i); } // Print out 'every' clause of plot command
      if (pd->EverySet>1) { sprintf(output+i, ":%d", pd->EveryList[1]); i+=strlen(output+i); }
      if (pd->EverySet>2) { sprintf(output+i, ":%d", pd->EveryList[2]); i+=strlen(output+i); }
      if (pd->EverySet>3) { sprintf(output+i, ":%d", pd->EveryList[3]); i+=strlen(output+i); }
      if (pd->EverySet>4) { sprintf(output+i, ":%d", pd->EveryList[4]); i+=strlen(output+i); }
      if (pd->EverySet>5) { sprintf(output+i, ":%d", pd->EveryList[5]); i+=strlen(output+i); }
      if (ptr->text==NULL) { sprintf(output+i, " format auto"); i+=strlen(output+i); }
      else                 { sprintf(output+i, " format %s", ptr->text); i+=strlen(output+i); }
      if (pd->IndexSet) { sprintf(output+i, " index %d", pd->index); i+=strlen(output+i); } // Print index to use
      if ((pd->label!=NULL) || (ptr->ArrowType!=SW_PIEKEYPOS_AUTO))
       {
        sprintf(output+i, " label %s", *(char **)ppl_fetchSettingName(&c->errcontext, ptr->ArrowType, SW_PIEKEYPOS_INT, (void *)SW_PIEKEYPOS_STR, sizeof(char *)));
        i+=strlen(output+i);
        if (pd->label!=NULL) sprintf(output+i, " %s", pd->label->ascii); // Print label string
        i+=strlen(output+i);
       }
      if (pd->SelectCriterion!=NULL) { sprintf(output+i, " select %s", pd->SelectCriterion->ascii); i+=strlen(output+i); } // Print select criterion
      ppl_withWordsPrint(c, &pd->ww, output+i+6);
      if (strlen(output+i+6)>0) { sprintf(output+i, " with"); output[i+5]=' '; }
      else                      { output[i]='\0'; }
      i+=strlen(output+i);
      sprintf(output+i, " using %s", (pd->UsingRowCols==DATAFILE_COL)?"columns":"rows"); i+=strlen(output+i); // Print using list
      for (j=0; j<pd->NUsing; j++)
       {
        output[i++]=(j!=0)?':':' ';
        strcpy(output+i, pd->UsingList[j]->ascii);
        i+=strlen(output+i);
       }
     }
   }
  else if (ptr->type == CANVAS_PLOT) // Produce textual representations of plot commands
   {
    canvas_plotrange *pr;
    canvas_plotdesc  *pd;
    pplObj            v;
    unsigned char     pr_first=1, pd_first=1;

    sprintf(output, "plot item %d", ptr->id);
    i = strlen(output);
    if (ptr->ThreeDim) strcpy(output+i, " 3d");
    i += strlen(output+i);
    pr = ptr->plotranges; // Print out plot ranges
    while (pr != NULL)
     {
      if (pr_first) { output[i++]=' '; pr_first=0; }
      output[i++]='[';
      if (pr->AutoMinSet) output[i++]='*';
      if (pr->MinSet) { v=pr->unit; v.real=pr->min; sprintf(output+i, "%s", ppl_unitsNumericDisplay(c,&v,0,0,0)); i+=strlen(output+i); }
      if (pr->MinSet || pr->MaxSet || pr->AutoMinSet || pr->AutoMaxSet) { strcpy(output+i,":"); i+=strlen(output+i); }
      if (pr->AutoMaxSet) output[i++]='*';
      if (pr->MaxSet) { v=pr->unit; v.real=pr->max; sprintf(output+i, "%s", ppl_unitsNumericDisplay(c,&v,0,0,0)); i+=strlen(output+i); }
      strcpy(output+i,"]"); i+=strlen(output+i);
      pr = pr->next;
     }
    pd = ptr->plotitems; // Print out plotted items one by one
    while (pd != NULL)
     {
      if (pd_first) { pd_first=0; } else { output[i++]=','; }
      if (pd->parametric) { sprintf(output+i, " parametric"); i+=strlen(output+i); }
      if (pd->TRangeSet)  { sprintf(output+i, " [%s:%s]", ppl_unitsNumericDisplay(c,&pd->Tmin,0,0,0), ppl_unitsNumericDisplay(c,&pd->Tmax,1,0,0)); i+=strlen(output+i); }
      if (pd->VRangeSet)  { sprintf(output+i, " [%s:%s]", ppl_unitsNumericDisplay(c,&pd->Vmin,0,0,0), ppl_unitsNumericDisplay(c,&pd->Vmax,1,0,0)); i+=strlen(output+i); }
      if (!pd->function) { output[i++]=' '; ppl_strEscapify(pd->filename, output+i); i+=strlen(output+i); } // Filename of datafile we are plotting
      else
       for (j=0; j<pd->NFunctions; j++) // Print out the list of functions which we are plotting
        {
         output[i++]=(j!=0)?':':' ';
         ppl_strStrip(pd->functions[j]->ascii , output+i);
         i+=strlen(output+i);
        }
      if (pd->axis1set || pd->axis2set || pd->axis3set) // Print axes to use
       {
        strcpy(output+i, " axes "); i+=strlen(output+i);
        if (pd->axis1set) { sprintf(output+i, "%c%d", "xyzc"[pd->axis1xyz], pd->axis1); i+=strlen(output+i); }
        if (pd->axis2set) { sprintf(output+i, "%c%d", "xyzc"[pd->axis2xyz], pd->axis2); i+=strlen(output+i); }
        if (pd->axis3set) { sprintf(output+i, "%c%d", "xyzc"[pd->axis3xyz], pd->axis3); i+=strlen(output+i); }
       }
      if (pd->EverySet>0) { sprintf(output+i, " every %d", pd->EveryList[0]); i+=strlen(output+i); } // Print out 'every' clause of plot command
      if (pd->EverySet>1) { sprintf(output+i, ":%d", pd->EveryList[1]); i+=strlen(output+i); }
      if (pd->EverySet>2) { sprintf(output+i, ":%d", pd->EveryList[2]); i+=strlen(output+i); }
      if (pd->EverySet>3) { sprintf(output+i, ":%d", pd->EveryList[3]); i+=strlen(output+i); }
      if (pd->EverySet>4) { sprintf(output+i, ":%d", pd->EveryList[4]); i+=strlen(output+i); }
      if (pd->EverySet>5) { sprintf(output+i, ":%d", pd->EveryList[5]); i+=strlen(output+i); }
      if (pd->IndexSet) { sprintf(output+i, " index %d", pd->index); i+=strlen(output+i); } // Print index to use
      if (pd->label!=NULL) { sprintf(output+i, " label %s", pd->label->ascii); i+=strlen(output+i); } // Print label string
      if (pd->SelectCriterion!=NULL) { sprintf(output+i, " select %s", pd->SelectCriterion->ascii); i+=strlen(output+i); } // Print select criterion
      if (pd->ContinuitySet) // Print continuous / discontinuous flag
       {
        if (pd->continuity == DATAFILE_DISCONTINUOUS) { sprintf(output+i, " discontinuous"); i+=strlen(output+i); }
        else                                          { sprintf(output+i,    " continuous"); i+=strlen(output+i); }
       }
      if      (pd->NoTitleSet) { strcpy(output+i, " notitle"); i+=strlen(output+i); } // notitle is set
      else if (pd->TitleSet  ) { strcpy(output+i, " title "); i+=strlen(output+i); ppl_strEscapify(pd->title, output+i); i+=strlen(output+i); }
      ppl_withWordsPrint(c, &pd->ww, output+i+6);
      if (strlen(output+i+6)>0) { sprintf(output+i, " with"); output[i+5]=' '; i+=strlen(output+i); }
      else                      { output[i]='\0'; }
      sprintf(output+i, " using %s", (pd->UsingRowCols==DATAFILE_COL)?"columns":"rows"); i+=strlen(output+i); // Print using list
      for (j=0; j<pd->NUsing; j++)
       {
        output[i++]=(j!=0)?':':' ';
        strcpy(output+i, pd->UsingList[j]->ascii);
        i+=strlen(output+i);
       }
      pd = pd->next;
     }
   }
  else if (ptr->type == CANVAS_POINT ) // Produce textual representations of point commands
   {
    sprintf(output, "point item %d at %s,%s", ptr->id,
             ppl_numericDisplay( ptr->xpos            *100, c->numdispBuff[0], c->set->term_current.SignificantFigures, (c->set->term_current.NumDisplay==SW_DISPLAY_L)),
             ppl_numericDisplay( ptr->ypos            *100, c->numdispBuff[1], c->set->term_current.SignificantFigures, (c->set->term_current.NumDisplay==SW_DISPLAY_L))
           );
    i = strlen(output);
    if (ptr->text != NULL)
     {
      sprintf(output+i, " label "); i+=strlen(output+i);
      ppl_strEscapify(ptr->text, output+i);
      i+=strlen(output+i);
     }
    ppl_withWordsPrint(c, &ptr->with_data, output+i+6);
    if (strlen(output+i+6)>0) { sprintf(output+i, " with"); output[i+5]=' '; }
    else                      { output[i]='\0'; }
   }
  else if (ptr->type == CANVAS_TEXT ) // Produce textual representations of text commands
   {
    sprintf(output, "text item %d ", ptr->id);
    i = strlen(output);
    ppl_strEscapify(ptr->text, output+i);
    i += strlen(output+i);
    sprintf(output+i, " at %s,%s rotate %s gap %s",
             ppl_numericDisplay( ptr->xpos     * 100     , c->numdispBuff[0], c->set->term_current.SignificantFigures, (c->set->term_current.NumDisplay==SW_DISPLAY_L)),
             ppl_numericDisplay( ptr->ypos     * 100     , c->numdispBuff[1], c->set->term_current.SignificantFigures, (c->set->term_current.NumDisplay==SW_DISPLAY_L)),
             ppl_numericDisplay( ptr->rotation * 180/M_PI, c->numdispBuff[2], c->set->term_current.SignificantFigures, (c->set->term_current.NumDisplay==SW_DISPLAY_L)),
             ppl_numericDisplay( ptr->xpos2    * 100     , c->numdispBuff[3], c->set->term_current.SignificantFigures, (c->set->term_current.NumDisplay==SW_DISPLAY_L))
           );
    i += strlen(output+i);
    ppl_withWordsPrint(c, &ptr->with_data, output+i+6);
    if (strlen(output+i+6)>0) { sprintf(output+i, " with"); output[i+5]=' '; }
    else                      { output[i]='\0'; }
   }
  else
   { sprintf(output, "[unknown object]"); } // Ooops.
  return output;
 }

// Implementation of the list command.
int directive_list(ppl_context *c, parserLine *pl, parserOutput *in, int interactive)
 {
  canvas_itemlist  *canvas_items = c->canvas_items;
  int               i;
  canvas_item      *ptr;

  ppl_report(&c->errcontext, "# ID   Command");
  if (canvas_items == NULL) return 0;
  ptr = canvas_items->first;
  while (ptr != NULL)
   {
    sprintf(c->errcontext.tempErrStr, "%5d  %s", ptr->id, (ptr->deleted) ? "[deleted] " : "");
    i = strlen(c->errcontext.tempErrStr);
    ppl_canvas_item_textify(c, ptr, c->errcontext.tempErrStr+i);
    ppl_report(&c->errcontext, NULL);
    ptr = ptr->next;
   }
  return 0;
 }

// Implementation of the delete command.
static int canvas_delete(ppl_context *c, const int id)
 {
  canvas_itemlist *canvas_items = c->canvas_items;
  canvas_item     *ptr = canvas_items->first;
  while ((ptr!=NULL)&&(ptr->id!=id)) ptr=ptr->next;
  if (ptr==NULL) { sprintf(c->errcontext.tempErrStr, "There is no multiplot item with ID %d.", id); ppl_warning(&c->errcontext, ERR_GENERAL, 0); return 1; }
  else           { ptr->deleted = 1; }
  return 0;
 }

int directive_delete(ppl_context *c, parserLine *pl, parserOutput *in, int interactive)
 {
  pplObj           *stk = in->stk;
  canvas_itemlist  *canvas_items = c->canvas_items;
  int               pos;

  if (canvas_items==NULL) { TBADD(ERR_GENERAL, "There are currently no items on the multiplot canvas.", 0); return 1; }

  pos = PARSE_delete_deleteno;
  while ((stk[pos].objType == PPLOBJ_NUM) && (stk[pos].real > 0))
   {
    int id;
    pos = (int)round(stk[pos].real);
    id  = (int)round(stk[pos+PARSE_delete_number_deleteno].real);
    canvas_delete(c, id);
   }

  // Redisplay the canvas as required
  if (c->set->term_current.display == SW_ONOFF_ON)
   {
    unsigned char *unsuccessful_ops = (unsigned char *)ppl_memAlloc(MULTIPLOT_MAXINDEX);
    ppl_canvas_draw(c, unsuccessful_ops);
   }
  return 0;
 }

// Implementation of the undelete command.
int directive_undelete(ppl_context *c, parserLine *pl, parserOutput *in, int interactive)
 {
  pplObj           *stk = in->stk;
  canvas_itemlist  *canvas_items = c->canvas_items;

  int               pos;

  if (canvas_items==NULL) { TBADD(ERR_GENERAL, "There are currently no items on the multiplot canvas.", 0); return 1; }

  pos = PARSE_undelete_undeleteno;
  while ((stk[pos].objType == PPLOBJ_NUM) && (stk[pos].real > 0))
   {
    canvas_item *ptr;
    int id;
    pos = (int)round(stk[pos].real);
    id  = (int)round(stk[pos+PARSE_undelete_number_undeleteno].real);
    ptr = canvas_items->first;
    while ((ptr!=NULL)&&(ptr->id!=id)) ptr=ptr->next;
    if (ptr==NULL) { sprintf(c->errcontext.tempErrStr, "There is no multiplot item with ID %d.", id); ppl_warning(&c->errcontext, ERR_GENERAL, 0); return 1; }
    else           { ptr->deleted = 0; }
   }

  // Redisplay the canvas as required
  if (c->set->term_current.display == SW_ONOFF_ON)
   {
    unsigned char *unsuccessful_ops = (unsigned char *)ppl_memAlloc(MULTIPLOT_MAXINDEX);
    ppl_canvas_draw(c, unsuccessful_ops);
   }
  return 0;
 }

// Implementation of the move command.
int directive_move(ppl_context *c, parserLine *pl, parserOutput *in, int interactive)
 {
  pplObj           *stk = in->stk;
  canvas_itemlist  *canvas_items = c->canvas_items;
  int               moveno, gotRotation;
  double            x, y, rotation;
  unsigned char     rotatable;
  canvas_item      *ptr;

  if (canvas_items==NULL) { TBADD(ERR_GENERAL, "There are currently no items on the multiplot canvas.", 0); return 1; }

  moveno   = (int)round(stk[PARSE_move_moveno  ].real);
  x        =            stk[PARSE_move_p       ].real ;
  y        =            stk[PARSE_move_p+1     ].real ;
  rotation =            stk[PARSE_move_rotation].real ; gotRotation = (stk[PARSE_move_rotation].objType == PPLOBJ_NUM);

  ptr = canvas_items->first;
  while ((ptr!=NULL)&&(ptr->id!=moveno)) ptr=ptr->next;
  if (ptr==NULL) { sprintf(c->errStat.errBuff, "There is no multiplot item with ID %d.", moveno); TBADD2(ERR_GENERAL, 0); return 1; }
  rotatable = ((ptr->type!=CANVAS_ARROW)&&(ptr->type!=CANVAS_CIRC)&&(ptr->type!=CANVAS_PIE)&&(ptr->type!=CANVAS_PLOT)&&(ptr->type!=CANVAS_POINT));
  if (gotRotation && !rotatable) { sprintf(c->errcontext.tempErrStr, "It is not possible to rotate multiplot item %d.", moveno); ppl_warning(&c->errcontext, ERR_GENERAL, NULL); }

  // Most canvas items are moved using the xpos and ypos fields
  if ((ptr->type!=CANVAS_PLOT)&&(ptr->type!=CANVAS_PIE))
   {
    ptr->xpos = x;
    ptr->ypos = y;
    if (gotRotation && rotatable) ptr->rotation = rotation;
   }
  else // Plots are moved using the origin fields in settings_graph
   {
    ptr->settings.OriginX.real = x;
    ptr->settings.OriginY.real = y;
   }

  // Redisplay the canvas as required
  if (c->set->term_current.display == SW_ONOFF_ON)
   {
    unsigned char *unsuccessful_ops = (unsigned char *)ppl_memAlloc(MULTIPLOT_MAXINDEX);
    ppl_canvas_draw(c, unsuccessful_ops);
   }
  return 0;
 }

// Implementation of the swap command.
int directive_swap(ppl_context *c, parserLine *pl, parserOutput *in, int interactive)
 {
  pplObj           *stk = in->stk;
  canvas_itemlist  *canvas_items = c->canvas_items;
  int               item1, item2;
  canvas_item     **ptr1, **ptr2, *temp;

  if (canvas_items==NULL) { TBADD(ERR_GENERAL, "There are currently no items on the multiplot canvas.", 0); return 1; }

  // Read the ID numbers of the items to be swapped
  item1 = (int)round(stk[PARSE_swap_item1].real);
  item2 = (int)round(stk[PARSE_swap_item2].real);

  // Seek the first item to be swapped
  ptr1 = &canvas_items->first;
  while ((*ptr1!=NULL)&&((*ptr1)->id!=item1)) ptr1=&((*ptr1)->next);
  if (*ptr1==NULL) { sprintf(c->errStat.errBuff, "There is no multiplot item with ID %d.", item1); TBADD2(ERR_GENERAL, 0); return 1; }

  // Seek the second item to be swapped
  ptr2 = &canvas_items->first;
  while ((*ptr2!=NULL)&&((*ptr2)->id!=item2)) ptr2=&((*ptr2)->next);
  if (*ptr2==NULL) { sprintf(c->errStat.errBuff, "There is no multiplot item with ID %d.", item2); TBADD2(ERR_GENERAL, 0); return 1; }

  // Do swap
  (*ptr1)->id = item2;
  (*ptr2)->id = item1;
  temp = *ptr1;
  *ptr1 = *ptr2;
  *ptr2 = temp;
  temp = (*ptr1)->next;
  (*ptr1)->next = (*ptr2)->next;
  (*ptr2)->next = temp;

  // Redisplay the canvas as required
  if (c->set->term_current.display == SW_ONOFF_ON)
   {
    unsigned char *unsuccessful_ops = (unsigned char *)ppl_memAlloc(MULTIPLOT_MAXINDEX);
    ppl_canvas_draw(c, unsuccessful_ops);
   }
  return 0;
 }

// Implementation of the arrow command.
int directive_arrow(ppl_context *c, parserLine *pl, parserOutput *in, int interactive)
 {
  pplObj           *stk = in->stk;
  canvas_item      *ptr;
  int               id, gotTempstr, gotTempstr2;
  double            x1, x2, y1, y2;
  char             *tempstr, *tempstr2;

  // Look up the start and end point of the arrow, and ensure that they are either dimensionless or in units of length
  x1 = stk[PARSE_arrow_p1  ].real;
  y1 = stk[PARSE_arrow_p1+1].real;
  x2 = stk[PARSE_arrow_p2  ].real;
  y2 = stk[PARSE_arrow_p2+1].real;

  // Add this arrow to the linked list which decribes the canvas
  if (canvas_itemlist_add(c,stk,CANVAS_ARROW,&ptr,&id,0)) { ppl_error(&c->errcontext, ERR_MEMORY, -1, -1,"Out of memory."); return 1; }
  ptr->xpos  = x1;
  ptr->ypos  = y1;
  ptr->xpos2 = x2 - x1;
  ptr->ypos2 = y2 - y1;

  // Read in colour and linewidth information, if available
  ppl_withWordsFromDict(c, stk, PARSE_TABLE_arrow_, &ptr->with_data);

  // Work out whether this arrow is in the 'head', 'nohead' or 'twoway' style
  tempstr  = (char *)stk[PARSE_arrow_arrow_style].auxil; gotTempstr  = (stk[PARSE_arrow_arrow_style].objType == PPLOBJ_STR);
  tempstr2 = (char *)stk[PARSE_arrow_directive  ].auxil; gotTempstr2 = (stk[PARSE_arrow_directive  ].objType == PPLOBJ_STR);
  if (gotTempstr) ptr->ArrowType = ppl_fetchSettingByName(&c->errcontext, tempstr, SW_ARROWTYPE_INT, SW_ARROWTYPE_STR);
  else            ptr->ArrowType = (strcmp(tempstr2,"arrow")==0) ? SW_ARROWTYPE_HEAD : SW_ARROWTYPE_NOHEAD;

  // Redisplay the canvas as required
  if (c->set->term_current.display == SW_ONOFF_ON)
   {
    unsigned char *unsuccessful_ops = (unsigned char *)ppl_memAlloc(MULTIPLOT_MAXINDEX);
    ppl_canvas_draw(c, unsuccessful_ops);
    if (unsuccessful_ops[id]) { canvas_delete(c, id); ppl_error(&c->errcontext, ERR_GENERAL, -1, -1, "Arrow has been removed from multiplot, because it generated an error."); return 1; }
   }
  return 0;
 }

// Implementation of the box command.
int directive_box(ppl_context *c, parserLine *pl, parserOutput *in, int interactive)
 {
  pplObj           *stk = in->stk;
  canvas_item      *ptr;
  int               id, gotx2, gotWidth, gotHeight, gotAng;
  double            x1, x2, y1, y2, ang, width, height;

  // Look up the positions of the two corners of the box
  x1    = stk[PARSE_box_p1        ].real;
  y1    = stk[PARSE_box_p1      +1].real;
  x2    = stk[PARSE_box_p2        ].real; gotx2     = (stk[PARSE_box_p2        ].objType==PPLOBJ_NUM);
  y2    = stk[PARSE_box_p2      +1].real;
  ang   = stk[PARSE_box_rotation  ].real; gotAng    = (stk[PARSE_box_rotation  ].objType==PPLOBJ_NUM);
  width = stk[PARSE_box_width     ].real; gotWidth  = (stk[PARSE_box_width     ].objType==PPLOBJ_NUM);
  height= stk[PARSE_box_height    ].real; gotHeight = (stk[PARSE_box_height    ].objType==PPLOBJ_NUM);

  if ((!gotx2) && ((!gotWidth)||(!gotHeight))) // If box is specified in width/height format, both must be specified
   { sprintf(c->errStat.errBuff, "When a box is specified with given width and height, both width and height must be specified."); TBADD2(ERR_SYNTAX, 0); return 1; }

  // Add this box to the linked list which decribes the canvas
  if (canvas_itemlist_add(c,stk,CANVAS_BOX,&ptr,&id,0)) { ppl_error(&c->errcontext, ERR_MEMORY, -1, -1,"Out of memory."); return 1; }
  ptr->xpos  = x1;
  ptr->ypos  = y1;
  if (gotx2) // Box is specified by two corners
   {
    ptr->xpos2 = x2 - x1;
    ptr->ypos2 = y2 - y1;
    ptr->xpos2set = 0; // Rotation should be about CENTRE of box
   }
  else // Box is specified with width and height
   {
    ptr->xpos2 = width;
    ptr->ypos2 = height;
    ptr->xpos2set = 1; // Rotation should be about fixed corner of box
   }
  if (gotAng) { ptr->rotation = ang; } // Rotation angle is zero if not specified
  else        { ptr->rotation = 0.0; }

  // Read in colour and linewidth information, if available
  ppl_withWordsFromDict(c, stk, PARSE_TABLE_box_, &ptr->with_data);

  // Redisplay the canvas as required
  if (c->set->term_current.display == SW_ONOFF_ON)
   {
    unsigned char *unsuccessful_ops = (unsigned char *)ppl_memAlloc(MULTIPLOT_MAXINDEX);
    ppl_canvas_draw(c, unsuccessful_ops);
    if (unsuccessful_ops[id]) { canvas_delete(c, id); ppl_error(&c->errcontext, ERR_GENERAL, -1, -1, "Box has been removed from multiplot, because it generated an error."); return 1; }
   }
  return 0;
 }

// Implementation of the circle command.
int directive_circle(ppl_context *c, parserLine *pl, parserOutput *in, int interactive)
 {
  pplObj       *stk = in->stk;
  canvas_item  *ptr;
  int           id, gota1, amArc;
  double        x, y, r, a1, a2;
  char         *tempstr;

  // Look up the position of the centre of the circle and its radius
  tempstr = (char *)stk[PARSE_circle_directive  ].auxil;
  amArc   = (strcmp(tempstr,"arc")==0);
  x       = stk[PARSE_circle_p  ].real;
  y       = stk[PARSE_circle_p+1].real;
  r       = stk[PARSE_circle_r  ].real;
  a1      = stk[PARSE_arc_angle1].real; gota1 = amArc && (stk[PARSE_arc_angle1].objType==PPLOBJ_NUM);
  a2      = stk[PARSE_arc_angle2].real;

  // Add this circle to the linked list which decribes the canvas
  if (canvas_itemlist_add(c,stk,CANVAS_CIRC,&ptr,&id,0)) { ppl_error(&c->errcontext, ERR_MEMORY, -1, -1,"Out of memory."); return 1; }
  ptr->xpos  = x;
  ptr->ypos  = y;
  ptr->xpos2 = r;
  if (gota1) { ptr->xfset = 1; ptr->xf = a1; ptr->yf = a2; } // arc command
  else       { ptr->xfset = 0; } // circle command

  // Read in colour and linewidth information, if available
  ppl_withWordsFromDict(c, stk, amArc?PARSE_TABLE_arc_:PARSE_TABLE_circle_, &ptr->with_data);

  // Redisplay the canvas as required
  if (c->set->term_current.display == SW_ONOFF_ON)
   {
    unsigned char *unsuccessful_ops = (unsigned char *)ppl_memAlloc(MULTIPLOT_MAXINDEX);
    ppl_canvas_draw(c, unsuccessful_ops);
    if (unsuccessful_ops[id]) { canvas_delete(c, id); ppl_error(&c->errcontext, ERR_GENERAL, -1, -1, "Circle has been removed from multiplot, because it generated an error."); return 1; }
   }
  return 0;
 }

// Implementation of the ellipse command.
int directive_ellipse(ppl_context *c, parserLine *pl, parserOutput *in, int interactive)
 {
  pplObj           *stk = in->stk;
  canvas_item      *ptr;
  int               e=0, r=0, p=0, id;
  int               gotAng, gotX1, gotX2, gotY1, gotY2, gotXc, gotYc, gotXf, gotYf, gotA2, gotB2, gotA, gotB, gotSlr, gotLr, gotEcc;
  double            ang, x1, x2, y1, y2, xc, yc, xf, yf, a2, b2, a, b, slr, lr, ecc;
  double            ratio;

  // Look up the input parameters which define the ellipse
  ang  = stk[PARSE_ellipse_rotation     ].real; gotAng  = (stk[PARSE_ellipse_rotation     ].objType==PPLOBJ_NUM);
  x1   = stk[PARSE_ellipse_p1           ].real; gotX1   = (stk[PARSE_ellipse_p1           ].objType==PPLOBJ_NUM);
  y1   = stk[PARSE_ellipse_p1+1         ].real; gotY1   = (stk[PARSE_ellipse_p1+1         ].objType==PPLOBJ_NUM);
  x2   = stk[PARSE_ellipse_p2           ].real; gotX2   = (stk[PARSE_ellipse_p2           ].objType==PPLOBJ_NUM);
  y2   = stk[PARSE_ellipse_p2+1         ].real; gotY2   = (stk[PARSE_ellipse_p2+1         ].objType==PPLOBJ_NUM);
  xc   = stk[PARSE_ellipse_center       ].real; gotXc   = (stk[PARSE_ellipse_center       ].objType==PPLOBJ_NUM);
  yc   = stk[PARSE_ellipse_center+1     ].real; gotYc   = (stk[PARSE_ellipse_center+1     ].objType==PPLOBJ_NUM);
  xf   = stk[PARSE_ellipse_focus        ].real; gotXf   = (stk[PARSE_ellipse_focus        ].objType==PPLOBJ_NUM);
  yf   = stk[PARSE_ellipse_focus+1      ].real; gotYf   = (stk[PARSE_ellipse_focus+1      ].objType==PPLOBJ_NUM);
  a2   = stk[PARSE_ellipse_majoraxis    ].real; gotA2   = (stk[PARSE_ellipse_majoraxis    ].objType==PPLOBJ_NUM);
  b2   = stk[PARSE_ellipse_minoraxis    ].real; gotB2   = (stk[PARSE_ellipse_minoraxis    ].objType==PPLOBJ_NUM);
  a    = stk[PARSE_ellipse_semimajoraxis].real; gotA    = (stk[PARSE_ellipse_semimajoraxis].objType==PPLOBJ_NUM);
  b    = stk[PARSE_ellipse_semiminoraxis].real; gotB    = (stk[PARSE_ellipse_semiminoraxis].objType==PPLOBJ_NUM);
  slr  = stk[PARSE_ellipse_slr          ].real; gotSlr  = (stk[PARSE_ellipse_slr          ].objType==PPLOBJ_NUM);
  lr   = stk[PARSE_ellipse_lr           ].real; gotLr   = (stk[PARSE_ellipse_lr           ].objType==PPLOBJ_NUM);
  ecc  = stk[PARSE_ellipse_eccentricity ].real; gotEcc  = (stk[PARSE_ellipse_eccentricity ].objType==PPLOBJ_NUM);

  // Check that input parameters have the right units, and convert dimensionless lengths into cm
  if (gotAng) { r++; } else { ang=0.0; }
  if (gotXc ) { p++; }
  if (gotXf ) { p++; }
  if (gotA2 ) { e++; }
  if (gotB2 ) { e++; }
  if (gotA  ) { e++; }
  if (gotB  ) { e++; }
  if (gotEcc) { e++; if ((ecc<0.0) || (ecc>=1.0)) { strcpy(c->errStat.errBuff, "Supplied eccentricity is not in the range 0 <= e < 1."); TBADD2(ERR_NUMERIC,0); return 1; } }
  if (gotSlr) { e++; }
  if (gotLr ) { e++; }

  // Major axis length is a drop-in replacement for the semi-major axis length
  if (gotA2) { a   = a2/2; }
  if (gotB2) { b   = b2/2; }
  if (gotLr) { slr = lr/2; }

  // Check that we have been supplied an appropriate set of inputs
  if ( (!gotX1) && (((p==2)&&((e!=1)||(r!=0))) || ((p<2)&&(e!=2))) )
   { strcpy(c->errStat.errBuff, "Ellipse command has received an inappropriate set of inputs. Must specify either the position of both the centre and focus of the ellipse, and one further piece of information out of the major axis length, the minor axis length, the eccentricity or the semi-latus rectum, or the position of one of these two points, the rotation angle of the major axis of the ellipse, and two further pieces of information."); TBADD2(ERR_GENERAL,0); return 1; }

  // Convert inputs such that we have the position of the centre of the ellipse and major/minor axes
  if (gotX1) // User has specified two corners of the ellipse
   {
    xc =     (x2 + x1) / 2.0;
    yc =     (y2 + y1) / 2.0;
    a  = fabs(x2 - x1) / 2.0;
    b  = fabs(y2 - y1) / 2.0;
   }
  else if (p==2) // User has specified both centre and focus of the ellipse, and one further piece of information
   {
    if (ppl_dblEqual(xc, xf) && ppl_dblEqual(yc, yf)) { ang = 0.0; }
    else                                              { ang = atan2(yc - yf , xc - xf); }
    xc = xc;
    yc = yc;

    if      (gotA) // Additional piece of information was major axis...
     {
      a   = fabs(a);
      ecc = hypot(xc - xf , yc - yf) / a;
      if ((ecc < 0.0) || (ecc >= 1.0)) { strcpy(c->errStat.errBuff, "Supplied semi-major axis length is shorter than the distance between the supplied focus and centre of the ellipse. No ellipse may have such parameters."); TBADD2(ERR_NUMERIC,0); return 1; }
      if (ppl_dblEqual(ecc,0.0)) { b = a; }
      else                             { b = a * sqrt(1.0-pow(ecc,2)); }
     }
    else if (gotB) // minor axis...
     {
      b   = fabs(b);
      a   = hypot(hypot(xc - xf , yc - yf) , b);
      if (b > a) { strcpy(c->errStat.errBuff, "Supplied minor axis length is longer than the implied major axis length of the ellipse."); TBADD2(ERR_NUMERIC,0); return 1; }
      ecc = sqrt(1.0 - pow(b/a , 2.0));
     }
    else if (gotEcc) // eccentricity...
     {
      a = hypot(xc - xf , yc - yf) / ecc;
      if (ppl_dblEqual(ecc,0.0)) { b = a; }
      else                       { b = a * sqrt(1.0-pow(ecc,2)); }
     }
    else if (gotSlr) // or semi-latus rectum...
     {
      ratio = hypot(xc - xf , yc - yf) / slr;
      ecc   = (sqrt(1+4*pow(ratio,2))-1.0) / (2*ratio);
      if ((ecc<0.0) || (ecc>=1.0)) { strcpy(c->errStat.errBuff, "Eccentricity implied for ellipse is not in the range 0 <= e < 1."); TBADD2(ERR_NUMERIC,0); return 1; }
      a     = hypot(xc - xf , yc - yf) / ecc;
      b     = a * sqrt(1.0 - pow(ecc,2.0));
     }
    else { strcpy(c->errStat.errBuff, "Flow control error in ellipse command."); TBADD2(ERR_INTERNAL,0); return 1; }
   }
  else // User has specified centre / focus of ellipse and two further pieces of information...
   {
    if      (gotA && gotB) // major and minor axes...
     {
      a   = fabs(a);
      b   = fabs(b);
      if (b>a) { strcpy(c->errStat.errBuff, "Supplied minor axis length is longer than the supplied major axis length of the ellipse."); TBADD2(ERR_NUMERIC,0); TBADD2(ERR_NUMERIC,0); return 1; }
      ecc = sqrt(1.0 - pow(b/a , 2.0));
     }
    else if (gotA && gotEcc) // major axis and eccentricity...
     {
      a   = fabs(a);
      b   = a * sqrt(1.0 - pow(ecc,2.0));
     }
    else if (gotA && gotSlr) // major axis and SLR...
     {
      a   = fabs(a);
      if (fabs(slr) > a) { strcpy(c->errStat.errBuff, "Supplied semi-latus rectum is longer than the supplied semi-major axis length of the ellipse. No ellipse may have such parameters."); TBADD2(ERR_NUMERIC,0); return 1; }
      ecc = sqrt(1.0 - fabs(slr) / a);
      b   = a * sqrt(1.0 - pow(ecc,2.0));
     }
    else if (gotB && gotEcc) // minor axis and eccentricity...
     {
      b   = fabs(b);
      a   = b / sqrt(1.0 - pow(ecc,2.0));
     }
    else if (gotB && gotSlr) // minor axis and SLR...
     {
      b   = fabs(b);
      if (fabs(slr) > b) { strcpy(c->errStat.errBuff, "Supplied semi-latus rectum is longer than the supplied semi-minor axis length of the ellipse. No ellipse may have such parameters."); TBADD2(ERR_NUMERIC,0); return 1; }
      ecc = sqrt(1.0 - pow(fabs(slr) / b,2.0));
      a   = b / sqrt(1.0 - pow(ecc,2.0));
     }
    else if (gotEcc && gotSlr) // eccentricity and SLR...
     {
      a   = fabs(slr) / (1.0 - pow(ecc,2.0));
      b   = a * sqrt(1.0 - pow(ecc,2.0));
     }
    else { strcpy(c->errStat.errBuff, "Flow control error in ellipse command."); TBADD2(ERR_INTERNAL,0); return 1; }

    if (gotXc) // User has specified the centre of the ellipse
     {
      xc = xc;
      yc = yc;
     }
    else if (!gotXf) // User has specified neither the centre nor a focus of the ellipse; use origin as centre
     {
      xc = c->set->graph_current.OriginX.real;
      yc = c->set->graph_current.OriginY.real;
     }
    else // User has specified the focus of the ellipse... convert to the centre by translating distance a * eccentricity
     {
      xc = xf + a * ecc * cos( ang);
      yc = yf + a * ecc * sin( ang);
     }
   }

  // Add this ellipse to the linked list which decribes the canvas
  if (canvas_itemlist_add(c,stk,CANVAS_ELLPS,&ptr,&id,0)) { ppl_error(&c->errcontext, ERR_MEMORY, -1, -1,"Out of memory."); return 1; }

  // Add the exact parameterisation which we have been given to canvas item, so that "list" command prints it out in the form originally supplied
  ptr->x1set = ptr->xcset = ptr->xfset = ptr->aset = ptr->bset = ptr->eccset = ptr->slrset = 0;
  ptr->x1 = ptr->y1 = ptr->x2 = ptr->y2 = ptr->xc = ptr->yc = ptr->xf = ptr->yf = ptr->ecc = ptr->slr = 0.0;
  if       (gotX1 )         { ptr->x1set = 1; ptr->x1 = x1; ptr->y1 = y1; ptr->x2 = x2; ptr->y2 = y2; }
  else if (gotXc || !gotXf) { ptr->xcset = 1; ptr->xc = xc; ptr->yc = yc; }
  if (gotXf ) { ptr->xfset = 1; ptr->xf = xf; ptr->yf = yf; }
  if (gotA  ) { ptr-> aset = 1; ptr->a  = a  ; }
  if (gotB  ) { ptr-> bset = 1; ptr->b  = b  ; }
  if (gotEcc) { ptr->eccset= 1; ptr->ecc= ecc; }
  if (gotSlr) { ptr->slrset= 1; ptr->slr= slr; }

  // Read in colour and linewidth information, if available
  ppl_withWordsFromDict(c, stk, PARSE_TABLE_ellipse_, &ptr->with_data);

  // Redisplay the canvas as required
  if (c->set->term_current.display == SW_ONOFF_ON)
   {
    unsigned char *unsuccessful_ops = (unsigned char *)ppl_memAlloc(MULTIPLOT_MAXINDEX);
    ppl_canvas_draw(c, unsuccessful_ops);
    if (unsuccessful_ops[id]) { canvas_delete(c, id); ppl_error(&c->errcontext, ERR_GENERAL, -1, -1, "Ellipse has been removed from multiplot, because it generated an error."); return 1; }
   }
  return 0;
 }

// Implementation of the eps command.
int directive_eps(ppl_context *c, parserLine *pl, parserOutput *in, int interactive)
 {
  pplObj       *stk = in->stk;
  canvas_item  *ptr;
  int           id;
  double        x, y, ang, width, height;
  int           gotX, gotY, gotAng, gotWidth, gotHeight, clip, calcbbox;
  char         *text, *fname;

  // Read in positional information for this eps image
  x       = stk[PARSE_eps_p       ].real; gotX      = (stk[PARSE_eps_p       ].objType==PPLOBJ_NUM);
  y       = stk[PARSE_eps_p+1     ].real; gotY      = (stk[PARSE_eps_p+1     ].objType==PPLOBJ_NUM);
  ang     = stk[PARSE_eps_rotation].real; gotAng    = (stk[PARSE_eps_rotation].objType==PPLOBJ_NUM);
  width   = stk[PARSE_eps_width   ].real; gotWidth  = (stk[PARSE_eps_width   ].objType==PPLOBJ_NUM);
  height  = stk[PARSE_eps_height  ].real; gotHeight = (stk[PARSE_eps_height  ].objType==PPLOBJ_NUM);
  clip    = (stk[PARSE_eps_clip    ].objType!=PPLOBJ_ZOM);
  calcbbox= (stk[PARSE_eps_calcbbox].objType!=PPLOBJ_ZOM);
  fname   = (char *)stk[PARSE_eps_filename].auxil;
  text    = (char *)malloc(strlen(fname)+1);
  if (text == NULL) { ppl_error(&c->errcontext, ERR_MEMORY, -1, -1,"Out of memory."); return 1; }
  strcpy(text, fname);

  // Add this ellipse to the linked list which decribes the canvas
  if (canvas_itemlist_add(c,stk,CANVAS_EPS,&ptr,&id,0)) { ppl_error(&c->errcontext, ERR_MEMORY, -1, -1,"Out of memory."); free(text); return 1; }

  if (gotX     ) { ptr->xpos     = x     ; }                    else { ptr->xpos     = c->set->graph_current.OriginX.real; }
  if (gotY     ) { ptr->ypos     = y     ; }                    else { ptr->ypos     = c->set->graph_current.OriginY.real; }
  if (gotAng   ) { ptr->rotation = ang   ; }                    else { ptr->rotation = 0.0;                                 }
  if (gotWidth ) { ptr->xpos2    = width ; ptr->xpos2set = 1; } else { ptr->xpos2    = 0.0; ptr->xpos2set = 0; }
  if (gotHeight) { ptr->ypos2    = height; ptr->ypos2set = 1; } else { ptr->ypos2    = 0.0; ptr->ypos2set = 0; }
  ptr->text     = text;
  ptr->clip     = clip;
  ptr->calcbbox = calcbbox;

  // Redisplay the canvas as required
  if (c->set->term_current.display == SW_ONOFF_ON)
   {
    unsigned char *unsuccessful_ops = (unsigned char *)ppl_memAlloc(MULTIPLOT_MAXINDEX);
    ppl_canvas_draw(c, unsuccessful_ops);
    if (unsuccessful_ops[id]) { canvas_delete(c, id); ppl_error(&c->errcontext, ERR_GENERAL, -1, -1, "EPS image has been removed from multiplot, because it generated an error."); return 1; }
   }
  return 0;
 }

// Implementation of the point command.
int directive_point(ppl_context *c, parserLine *pl, parserOutput *in, int interactive)
 {
  pplObj        *stk = in->stk; 
  canvas_item   *ptr;
  int            id;
  double         x, y;

  // Look up the position of the point, and ensure that it is either dimensionless or in units of length
  x = stk[PARSE_point_p  ].real;
  y = stk[PARSE_point_p+1].real;

  // Add this point to the linked list which decribes the canvas
  if (canvas_itemlist_add(c,stk,CANVAS_POINT,&ptr,&id,0)) { ppl_error(&c->errcontext, ERR_MEMORY, -1, -1,"Out of memory."); return 1; }
  ptr->xpos  = x;
  ptr->ypos  = y;

  // Read in colour and linewidth information, if available
  ppl_withWordsFromDict(c, stk, PARSE_TABLE_point_, &ptr->with_data);

  // See whether this point is labelled
  if (stk[PARSE_point_label].objType==PPLOBJ_STR)
   {
    char *tempstr = (char *)stk[PARSE_point_label].auxil;
    char *text = (char *)malloc(strlen(tempstr)+1);
    if (text == NULL) { ppl_error(&c->errcontext, ERR_MEMORY, -1, -1,"Out of memory."); return 1; }
    strcpy(text, tempstr);
    ptr->text = text;
   }
  else { ptr->text = NULL; }

  // Redisplay the canvas as required
  if (c->set->term_current.display == SW_ONOFF_ON)
   {
    unsigned char *unsuccessful_ops = (unsigned char *)ppl_memAlloc(MULTIPLOT_MAXINDEX);
    ppl_canvas_draw(c, unsuccessful_ops);
    if (unsuccessful_ops[id]) { canvas_delete(c, id); ppl_error(&c->errcontext, ERR_GENERAL, -1, -1, "Point has been removed from multiplot, because it generated an error."); return 1; }
   } 
  return 0;
 }

// Implementation of the text command.
int directive_text(ppl_context *c, parserLine *pl, parserOutput *in, int interactive)
 {
  pplObj        *stk = in->stk;
  canvas_item   *ptr;
  int            id, gotX, gotY, gotAng, gotGap, gotTempstr;
  double         x, y, ang, gap;
  char          *tempstr, *text;

  // Look up the position of the text item
  x       = stk[PARSE_text_p       ].real; gotX   = (stk[PARSE_text_p       ].objType==PPLOBJ_NUM);
  y       = stk[PARSE_text_p+1     ].real; gotY   = (stk[PARSE_text_p+1     ].objType==PPLOBJ_NUM);
  gap     = stk[PARSE_text_gap     ].real; gotGap = (stk[PARSE_text_gap     ].objType==PPLOBJ_NUM);
  ang     = stk[PARSE_text_rotation].real; gotAng = (stk[PARSE_text_rotation].objType==PPLOBJ_NUM);

  // Read the string which we are to render
  tempstr = (char *)stk[PARSE_text_string].auxil;
  text = (char *)malloc(strlen(tempstr)+1);
  if (text == NULL) { ppl_error(&c->errcontext, ERR_MEMORY, -1, -1,"Out of memory."); return 1; }
  strcpy(text, tempstr);

  if (canvas_itemlist_add(c,stk,CANVAS_TEXT,&ptr,&id,0)) { ppl_error(&c->errcontext, ERR_MEMORY, -1, -1,"Out of memory."); free(text); return 1; }

  // Check for halign or valign modifiers
  tempstr = (char *)stk[PARSE_text_halign].auxil; gotTempstr = (stk[PARSE_text_halign].objType==PPLOBJ_STR);
  if (gotTempstr) ptr->settings.TextHAlign = ppl_fetchSettingByName(&c->errcontext, tempstr, SW_HALIGN_INT, SW_HALIGN_STR);
  tempstr = (char *)stk[PARSE_text_valign].auxil; gotTempstr = (stk[PARSE_text_valign].objType==PPLOBJ_STR);
  if (gotTempstr) ptr->settings.TextVAlign = ppl_fetchSettingByName(&c->errcontext, tempstr, SW_VALIGN_INT, SW_VALIGN_STR);

  if (gotX  ) { ptr->xpos     = x  ; } else { ptr->xpos      = c->set->graph_current.OriginX.real; }
  if (gotY  ) { ptr->ypos     = y  ; } else { ptr->ypos      = c->set->graph_current.OriginY.real; }
  if (gotGap) { ptr->xpos2    = gap; } else { ptr->xpos2     = 0.0;                                }
  if (gotAng) { ptr->rotation = ang; } else { ptr->rotation  = 0.0;                                }
  ptr->text = text;

  // Read in colour information, if available
  ppl_withWordsFromDict(c, stk, PARSE_TABLE_text_, &ptr->with_data);

  // Redisplay the canvas as required
  if (c->set->term_current.display == SW_ONOFF_ON)
   {
    unsigned char *unsuccessful_ops = (unsigned char *)ppl_memAlloc(MULTIPLOT_MAXINDEX);
    ppl_canvas_draw(c, unsuccessful_ops);
    if (unsuccessful_ops[id]) { canvas_delete(c, id); ppl_error(&c->errcontext, ERR_GENERAL, -1, -1, "Text item has been removed from multiplot, because it generated an error."); return 1; }
   }
  return 0;
 }

