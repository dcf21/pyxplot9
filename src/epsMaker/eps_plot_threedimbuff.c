// eps_plot_threedimbuff.c
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

#define _PPL_EPS_PLOT_THREEDIMBUFF_C 1

#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <string.h>

#include "coreUtils/memAlloc.h"
#include "coreUtils/list.h"

#include "epsMaker/eps_comm.h"
#include "epsMaker/eps_core.h"
#include "epsMaker/eps_plot_threedimbuff.h"
#include "epsMaker/eps_settings.h"

// Internal state variables
unsigned char  ThreeDimBuffer_ACTIVE        = 0;
static long    ThreeDimBuffer_LineSegmentID = 0;
static double  ThreeDimBuffer_LineLength    = 0.0;
static list   *ThreeDimBuffer_buffer        = NULL;

static int     linetype_old  ; static int linetype_old_SET  ;
static double  linewidth_old ; static int linewidth_old_SET ;
static double  pointsize_old ; static int pointsize_old_SET ;
static char   *colstr_old;

void ThreeDimBuffer_Reset(EPSComm *x)
 {
  ThreeDimBuffer_ACTIVE        = 0;
  ThreeDimBuffer_LineSegmentID = 0;
  ThreeDimBuffer_LineLength    = 0.0;
  ThreeDimBuffer_buffer        = NULL;
  linetype_old_SET = linewidth_old_SET = pointsize_old_SET = 0;
  colstr_old = NULL;
  return;
 }

int ThreeDimBuffer_Activate(EPSComm *x)
 {
  ThreeDimBuffer_Deactivate(x);
  ThreeDimBuffer_ACTIVE        = 1;
  ThreeDimBuffer_LineSegmentID = 0;
  ThreeDimBuffer_LineLength    = 0.0;
  ThreeDimBuffer_buffer        = ppl_listInit(0);
  linetype_old_SET = linewidth_old_SET = pointsize_old_SET = 0;
  colstr_old = NULL;
  return (ThreeDimBuffer_buffer != NULL);
 }

struct DepthArrayEntry { double depth; void *item; };

static int ThreeDimBuffer_qsort_slave(const void *x, const void *y)
 {
  const struct DepthArrayEntry *x1 = (const struct DepthArrayEntry *)x;
  const struct DepthArrayEntry *y1 = (const struct DepthArrayEntry *)y;

  if      (x1->depth > y1->depth) return -1.0;
  else if (x1->depth < y1->depth) return  1.0;
  else                            return  0.0;
 }

int ThreeDimBuffer_Deactivate(EPSComm *x)
 {
  int   Nitems, i, j;
  struct DepthArrayEntry *DepthArray;
  ThreeDimBufferItem *item;
  listIterator *ListIter;

  if (!ThreeDimBuffer_ACTIVE) { ThreeDimBuffer_Reset(x); return 0; }
  Nitems = ppl_listLen(ThreeDimBuffer_buffer);
  ThreeDimBuffer_ACTIVE = 0;
  if (Nitems > 0)
   {
    eps_core_WritePSColor(x);

    // Display all items in 3D display buffer, from back to front
    DepthArray = (struct DepthArrayEntry *)ppl_memAlloc(Nitems * sizeof(struct DepthArrayEntry));
    i=0;
    ListIter = ppl_listIterateInit(ThreeDimBuffer_buffer);
    while (ListIter != NULL)
     {
      DepthArray[i].item  = ListIter->data;
      DepthArray[i].depth = ((ThreeDimBufferItem *)(ListIter->data))->depth;
      i++;
      ppl_listIterate(&ListIter);
     }
    qsort((void *)DepthArray, i, sizeof(struct DepthArrayEntry), ThreeDimBuffer_qsort_slave);
    ThreeDimBuffer_LineSegmentID = -5;
    for (j=0; j<i; j++)
     {
      item = (ThreeDimBufferItem *)DepthArray[j].item;
      if (item->FlagLineSegment)
       {
        ThreeDimBuffer_linesegment(x, item->depth, item->linetype, item->linewidth, item->colstr, item->x0, item->y0, item->x1, item->y1, item->x2, item->y2, item->FirstLineSegment, (ThreeDimBuffer_LineSegmentID!=(item->LineSegmentID-1)), item->LineLength);
        ThreeDimBuffer_LineSegmentID = item->LineSegmentID;
       }
      else
       {
        ThreeDimBuffer_linepenup(x);
        ThreeDimBuffer_writeps(x, item->depth, item->linetype, item->linewidth, item->offset, item->pointsize, item->colstr, item->psfrag);
       }
     }
   }
  ThreeDimBuffer_linepenup(x);
  ThreeDimBuffer_Reset(x);
  return 0;
 }

int ThreeDimBuffer_writeps(EPSComm *x, double z, int linetype, double linewidth, double offset, double pointsize, char *colstr, char *psfrag)
 {
  ThreeDimBufferItem *item;
  char               *tempstr;

  if (!ThreeDimBuffer_ACTIVE)
   {
    if ((!pointsize_old_SET) || (pointsize_old != pointsize)) { pointsize_old_SET=1; pointsize_old=pointsize; fprintf(x->epsbuffer, "/ps { %f } def\n", pointsize * EPS_DEFAULT_PS); }
    colstr_old = colstr;
    strcpy(x->CurrentColor, colstr);
    eps_core_WritePSColor(x);
    eps_core_SetLinewidth(x, linewidth * EPS_DEFAULT_LINEWIDTH, linetype, offset);
    fprintf(x->epsbuffer, "%s\n", psfrag);
   }
  else
   {
    item = (ThreeDimBufferItem *)ppl_memAlloc(sizeof(ThreeDimBufferItem));
    tempstr = (char *)ppl_memAlloc(strlen(psfrag)+1);
    if ((item == NULL) || (tempstr == NULL)) return 1;
    strcpy(tempstr, psfrag);
    item->FlagLineSegment = item->FirstLineSegment = 0;
    item->linetype        = linetype;
    item->linewidth       = linewidth;
    item->offset          = offset;
    item->pointsize       = pointsize;
    item->colstr          = colstr;
    item->psfrag          = tempstr;
    item->depth           = z;
    ppl_listAppend(ThreeDimBuffer_buffer, (void *)item);
   }
  return 0;
 }

int ThreeDimBuffer_linesegment(EPSComm *x, double z, int linetype, double linewidth, char *colstr, double x0, double y0, double x1, double y1, double x2, double y2, unsigned char FirstSegment, unsigned char broken, double LengthOffset)
 {
  ThreeDimBufferItem *item;

  if (!ThreeDimBuffer_ACTIVE)
   {
    if (FirstSegment && (ThreeDimBuffer_LineSegmentID > 0)) ThreeDimBuffer_linepenup(x); // If first segment of new line, finish old line
    if (broken || (!linetype_old_SET) || (linetype_old != linetype) || (!linewidth_old_SET) || (linewidth_old != linewidth) || (colstr_old == NULL) || (strcmp(colstr_old, colstr)!=0))
     {
      // Start line 0.1 linewidths along in the direction of x0 to get correct linecap
      if ((!FirstSegment) && ((x0!=x1) || (y0!=y1)))
       {
        double theta = atan2(x0-x1,y0-y1);
        x0 = x1 + 0.1 * sin(theta) * linewidth * EPS_DEFAULT_LINEWIDTH;
        y0 = y1 + 0.1 * cos(theta) * linewidth * EPS_DEFAULT_LINEWIDTH;
       } else {
        x0 = x1;
        y0 = y1;
       }

      ThreeDimBuffer_linepenup(x);
      linetype_old_SET = linewidth_old_SET = 1;
      linetype_old     = linetype;
      linewidth_old    = linewidth;
      eps_core_SetLinewidth(x, linewidth * EPS_DEFAULT_LINEWIDTH, linetype, broken ? (LengthOffset-hypot(x1-x0,y1-y0)) : 0.0);
      if ((colstr_old == NULL) || (strcmp(colstr_old, colstr)!=0)) { colstr_old = colstr; strcpy(x->CurrentColor, colstr); eps_core_WritePSColor(x); }


      if (FirstSegment) fprintf(x->epsbuffer, "newpath\n%.2f %.2f moveto\n%.2f %.2f lineto\n", x1, y1, x2, y2);
      else              fprintf(x->epsbuffer, "newpath\n%.2f %.2f moveto\n%.2f %.2f lineto\n%.2f %.2f lineto\n", x0, y0, x1, y1, x2, y2);
      eps_core_BoundingBox(x, x1, y1, linewidth * EPS_DEFAULT_LINEWIDTH);
      eps_core_BoundingBox(x, x2, y2, linewidth * EPS_DEFAULT_LINEWIDTH);
     }
    else
     {
      if (FirstSegment) fprintf(x->epsbuffer, "newpath\n%.2f %.2f moveto\n%.2f %.2f lineto\n", x1, y1, x2, y2);
      else              fprintf(x->epsbuffer, "%.2f %.2f lineto\n", x2, y2);
      eps_core_BoundingBox(x, x1, y1, linewidth * EPS_DEFAULT_LINEWIDTH);
      eps_core_BoundingBox(x, x2, y2, linewidth * EPS_DEFAULT_LINEWIDTH);
     }
    ThreeDimBuffer_LineSegmentID = 1;
   }
  else
   {
    if (FirstSegment) ThreeDimBuffer_LineLength = 0.0;
    if ((!linetype_old_SET) || (linetype_old != linetype))
     {
      ThreeDimBuffer_LineLength = 0.0;
      linetype_old_SET = 1;
      linetype_old     = linetype;
     }
    item = (ThreeDimBufferItem *)ppl_memAlloc(sizeof(ThreeDimBufferItem));
    if (item == NULL) return 1;
    item->FlagLineSegment  = 1;
    item->LineSegmentID    = ThreeDimBuffer_LineSegmentID++;
    item->LineLength       = ThreeDimBuffer_LineLength;
    item->FirstLineSegment = FirstSegment;
    item->linetype         = linetype;
    item->linewidth        = linewidth;
    item->colstr           = colstr;
    item->depth            = z;
    item->x0               = x0;
    item->y0               = y0;
    item->x1               = x1;
    item->y1               = y1;
    item->x2               = x2;
    item->y2               = y2;
    ppl_listAppend(ThreeDimBuffer_buffer, (void *)item);
    ThreeDimBuffer_LineLength += hypot(x2-x1,y2-y1);
   }
  return 0;
 }

int ThreeDimBuffer_linepenup(EPSComm *x)
 {
  if (!ThreeDimBuffer_ACTIVE)
   {
    if (ThreeDimBuffer_LineSegmentID >= 0) fprintf(x->epsbuffer, "stroke\n");
    ThreeDimBuffer_LineSegmentID = -5;
   }
  return 0;
 }

