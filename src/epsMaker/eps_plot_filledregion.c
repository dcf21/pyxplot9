// eps_plot_filledregion.c
//
// The code in this file is part of Pyxplot
// <http://www.pyxplot.org.uk>
//
// Copyright (C) 2006-2012 Dominic Ford <coders@pyxplot.org.uk>
//               2008-2012 Ross Church
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

#define _PPL_EPS_PLOT_FILLEDREGION_C 1

#include <stdlib.h>
#include <stdio.h>

#include <gsl/gsl_math.h>

#include "coreUtils/memAlloc.h"
#include "coreUtils/list.h"
#include "coreUtils/errorReport.h"

#include "epsMaker/eps_comm.h"
#include "epsMaker/eps_core.h"
#include "epsMaker/eps_plot_canvas.h"
#include "epsMaker/eps_plot_filledregion.h"
#include "epsMaker/eps_plot_linedraw.h"
#include "epsMaker/eps_settings.h"

FilledRegionHandle *FilledRegion_Init (EPSComm *x, pplset_axis *xa, pplset_axis *ya, pplset_axis *za, int xrn, int yrn, int zrn, pplset_graph *sg, unsigned char ThreeDim, double origin_x, double origin_y, double width, double height, double zdepth)
 {
  FilledRegionHandle *output;
  output = (FilledRegionHandle *)ppl_memAlloc(sizeof(FilledRegionHandle));
  if (output==NULL) return NULL;
  output->x           = x;
  output->xa          = xa;
  output->ya          = ya;
  output->za          = za;
  output->xrn         = xrn;
  output->yrn         = yrn;
  output->zrn         = zrn;
  output->sg          = sg;
  output->ThreeDim    = ThreeDim;
  output->origin_x    = origin_x;
  output->origin_y    = origin_y;
  output->width       = width;
  output->height      = height;
  output->zdepth      = zdepth;
  output->Naxiscrossings = 0;
  output->first       = 1;
  output->EverInside  = 0;
  output->points      = ppl_listInit(0);
  return output;
 }

void FilledRegion_Point(EPSComm *X, FilledRegionHandle *fr, double x, double y)
 {
  FilledRegionPoint p;
  double xpos, ypos, depth, xap, yap, zap, ap1, ap2;
  unsigned char f1, f2;
  int NCrossings, i1, i2;
  double cx1,cy1,cz1,cx2,cy2,cz2;

  // Work out where (x,y) in coordinate space lies on the canvas
  eps_plot_GetPosition(&xpos, &ypos, &depth, &xap, &yap, &zap, NULL, NULL, NULL, fr->ThreeDim, x, y, 0, fr->xa, fr->ya, fr->za, fr->xrn, fr->yrn, fr->zrn, fr->sg, fr->origin_x, fr->origin_y, fr->width, fr->height, fr->zdepth, 1);

  if ((!gsl_finite(xpos))||(!gsl_finite(ypos))||(!gsl_finite(depth))) { return; }

  // Add this to a list of points describing the outline of the region which we are going to fill
  p.inside = ((xap>=0)&&(xap<=1)&&(yap>=0)&&(yap<=1)&&(zap>=0)&&(zap<=1));
  p.FillSideFlip_fwd = 0;
  p.FillSideFlip_prv = 0;
  p.x      = xpos;
  p.y      = ypos;
  p.xa     = x;
  p.ya     = y;
  p.xap    = xap;
  p.yap    = yap;

  ppl_listAppendCpy(fr->points, (void *)&p, sizeof(p));

  // Count the number of times which the path crosses the boundary of the clip region
  if (!fr->first)
   {
    LineDraw_FindCrossingPoints(X, fr->lastx,fr->lasty,0,fr->lastxap,fr->lastyap,0.5,
                                xpos,ypos,0,xap,yap,0.5,&i1,&i2,&cx1,&cy1,&cz1,&cx2,&cy2,&cz2,&f1,&ap1,&f2,&ap2,&NCrossings);
    fr->Naxiscrossings += NCrossings;
    fr->EverInside = (fr->EverInside) || (NCrossings>0) || i1 || i2;
   }
  fr->lastx = xpos; fr->lasty = ypos; fr->lastxap = xap; fr->lastyap = yap;
  fr->first = 0;
  return;
 }

static int frac_sorter(const void *av, const void *bv)
 {
  const FilledRegionAxisCrossing *a = (const FilledRegionAxisCrossing *)av;
  const FilledRegionAxisCrossing *b = (const FilledRegionAxisCrossing *)bv;
  if (b->AxisFace > a->AxisFace) return -1;
  if (b->AxisFace < a->AxisFace) return  1;
  if (b->AxisFace < FACE_BOTTOM) // Sort top from right to left, and left from top to bottom
   {
    if      (b->AxisPos > a->AxisPos) return  1;
    else if (b->AxisPos < a->AxisPos) return -1;
   }
  else // Sort bottom from left to right, and right from bottom to top
   {
    if      (b->AxisPos > a->AxisPos) return -1;
    else if (b->AxisPos < a->AxisPos) return  1;
   }
  return 0;
 }

static unsigned char TestPointInside(FilledRegionHandle *fr, double X, double Y, int dir_x, int dir_y)
 {
  unsigned char inside;
  double x_intersect, y_intersect;
  int i, n, Npoints;
  listItem *li1, *li2;
  FilledRegionPoint *p1, *p2;

  // Move fractionally off test point
  if      (dir_x==-1) X -= fabs(1e-15*X);
  else if (dir_x== 1) X += fabs(1e-15*X);
  else if (dir_y==-1) Y -= fabs(1e-15*X);
  else if (dir_y== 1) Y += fabs(1e-15*X);

  Npoints = fr->points->length;
  li2     = fr->points->first;
  p2      = (FilledRegionPoint *)(li2->data);

  // Loop around perimeter of region which we are going to fill
  for (i=n=0; i<Npoints; i++)
   {
    li1 = li2;
    p1  = p2;
    li2 = li1->next;
    if (li2==NULL) li2=fr->points->first;
    p2  = (FilledRegionPoint *)(li2->data); // p1 -> p2 is a line segment of perimeter

    // Count how many segments cross lines travelling out up/left/down/right from (X,Y)

    if (dir_x==0)
     {
      y_intersect = (p2->y - p1->y)/(p2->x - p1->x)*(X - p1->x) + p1->y; // y point where line p1->p2 intersects x=X
      if ((gsl_finite(y_intersect)) && (((X>=p1->x)&&(X<p2->x)) || ((X<=p1->x)&&(X>p2->x)))
              && (((dir_y>0)&&(y_intersect>Y)) || ((dir_y<0)&&(y_intersect<Y)))) { n++; }
     }
    else
     {
      x_intersect = (p2->x - p1->x)/(p2->y - p1->y)*(Y - p1->y) + p1->x; // x point where line p1->p2 intersects y=Y
      if ((gsl_finite(x_intersect)) && (((Y>=p1->y)&&(Y<p2->y)) || ((Y<=p1->y)&&(Y>p2->y)))
              && (((dir_x>0)&&(x_intersect>X)) || ((dir_x<0)&&(x_intersect<X)))) { n++; }
     }
   }
  inside = (n%2); // If were an even number of segments, we are outside. If there were an odd number, we are inside.
  return inside;
 }

#define PS_POINT(X,Y) \
 { \
   if (DEBUG) { sprintf(fr->x->c->errcontext.tempErrStr,"Hopping to (%e,%e).",X,Y); ppl_log(&fr->x->c->errcontext,NULL); } \
   if      (first_subpath) { fprintf(fr->x->epsbuffer, "newpath "); first_subpath=0; } \
   if      (first_point)   { fprintf(fr->x->epsbuffer, "%.2f %.2f moveto\n", X, Y); first_point=0; } \
   else                    { fprintf(fr->x->epsbuffer, "%.2f %.2f lineto\n", X, Y); } \
   eps_core_BoundingBox(fr->x, X, Y, linewidth * EPS_DEFAULT_LINEWIDTH); \
 }

static void OutputPath(FilledRegionHandle *fr, FilledRegionAxisCrossing *CrossPointList, int nac, char *EndText, double linewidth)
 {
  unsigned char face, sense, inside_ahead, FillSide, CurrentFace, first_point=1, first_subpath=1, fail=0;
  int i, inew, j, k, l, dir_x, dir_y;
  listItem *li, *lil=NULL;
  FilledRegionPoint *p;

  // Clear used flags
  for (i=0; i<nac; i++) CrossPointList[i].used=0;

  // Loop over multiple detached segments of filled region which may be caused by clipping it into pieces
  if (DEBUG) { ppl_log(&fr->x->c->errcontext,"New filled region."); }
  while (!fail)
   {
    // Find an axis crossing point which is unique
    l=-1;
    for (i=0; i<nac; i++)
     {
      if (CrossPointList[i].used) continue;
      l=i; // Fallback option; an unused crossing point
      j=(i+1)%nac; // Next axis crossing
      k=(i+nac-1)%nac; // Prev axis crossing
      if ((CrossPointList[i].x == CrossPointList[k].x) && (CrossPointList[i].y == CrossPointList[k].y)) continue;
      if ((CrossPointList[i].x == CrossPointList[j].x) && (CrossPointList[i].y == CrossPointList[j].y)) continue;
      break;
     }
    if (l==-1) { if (DEBUG) { ppl_log(&fr->x->c->errcontext,"All axis-crossing points used."); } break; }
    i=l;

    // Debugging lines
    if (DEBUG)
     {
      int i;
      ppl_log(&fr->x->c->errcontext,"New segment of filled region.");
      ppl_log(&fr->x->c->errcontext,"Summary of axis crossing points:");
      for (i=0; i<nac; i++) { sprintf(fr->x->c->errcontext.tempErrStr,"%d %s (%e,%e) face %d position %.4f sense %s %s",i, CrossPointList[i].used?"[used]":"      ", CrossPointList[i].x, CrossPointList[i].y, (int)CrossPointList[i].AxisFace, CrossPointList[i].AxisPos, (CrossPointList[i].sense==INCOMING)?"incoming":"outgoing", CrossPointList[i].singleton?"[singleton]":""); ppl_log(&fr->x->c->errcontext,NULL); }
     }

    // Work out which face first axis crossing is on, and which direction we're moving going to next item in crossing point list
    face = CrossPointList[i].AxisFace;
    if      (face==FACE_TOP   ) { dir_x=-1; dir_y= 0; }
    else if (face==FACE_LEFT  ) { dir_x= 0; dir_y=-1; }
    else if (face==FACE_BOTTOM) { dir_x= 1; dir_y= 0; }
    else                        { dir_x= 0; dir_y= 1; }
    inside_ahead = TestPointInside(fr, CrossPointList[i].x, CrossPointList[i].y, dir_x, dir_y);

    // Decide whether we are proceeding clockwise or anticlockwise around the region we're going to fill
    FillSide = inside_ahead;
    if (DEBUG) { sprintf(fr->x->c->errcontext.tempErrStr,"Starting from %d; filling on %s.",i,FillSide?"right":"left"); ppl_log(&fr->x->c->errcontext,NULL); }

    while (1)
     {
      CrossPointList[i].used = 1;
      if (CrossPointList[i].singleton) // Move along a line segment which streaks across canvas in one step
       {
        PS_POINT(CrossPointList[i].x , CrossPointList[i].y ); // Cut point where line enters canvas
        PS_POINT(CrossPointList[i].x2, CrossPointList[i].y2); // Cut point where line leaves canvas
        if (((FilledRegionPoint *)CrossPointList[i].point->data)->FillSideFlip_prv%2) FillSide = !FillSide;
        for (inew=0; inew<nac; inew++) if (CrossPointList[inew].id == CrossPointList[i].twin) break;
        if (inew>=nac) { ppl_error(&fr->x->c->errcontext,ERR_INTERNAL, -1, -1, "Failure within FilledRegion"); fail=1; break; }
        if (DEBUG) { sprintf(fr->x->c->errcontext.tempErrStr,"Twin of %d is %d.",i,inew); ppl_log(&fr->x->c->errcontext,NULL); }
        i = inew;
        CrossPointList[i].used = 1;
       }
      else // Follow a path across the canvas, step by step
       {
        PS_POINT(CrossPointList[i].x, CrossPointList[i].y); // Cut point where line enters canvas
        li = CrossPointList[i].point;
        sense = CrossPointList[i].sense;
        if (sense == OUTGOING) { li=li->prev; if (li==NULL) li=fr->points->last; }
        p  = (FilledRegionPoint *)(li->data);
        while (p->inside)
         {
          PS_POINT(p->x, p->y);
          if      ((sense == OUTGOING) && (p->FillSideFlip_fwd%2)) FillSide = !FillSide;
          else if ((sense == INCOMING) && (p->FillSideFlip_prv%2)) FillSide = !FillSide;
          lil=li;
          if (sense == OUTGOING) { li=li->prev; if (li==NULL) li=fr->points->last; } // Moving against flow in which line was originally drawn
          else                   { li=li->next; if (li==NULL) li=fr->points->first; } // Moving with flow in which line was originally drawn
          p=(FilledRegionPoint *)(li->data);
         }
        if      ((sense == OUTGOING) && (p->FillSideFlip_fwd%2)) FillSide = !FillSide;
        else if ((sense == INCOMING) && (p->FillSideFlip_prv%2)) FillSide = !FillSide;
        for (i=0; i<nac; i++) if ((!CrossPointList[i].used)&&((CrossPointList[i].point==lil)||(CrossPointList[i].point==li))) break;
        if (!((!CrossPointList[i].used)&&((CrossPointList[i].point==lil)||(CrossPointList[i].point==li)))) { ppl_error(&fr->x->c->errcontext,ERR_INTERNAL, -1, -1, "Failure within FilledRegion"); fail=1; break; }
        PS_POINT(CrossPointList[i].x, CrossPointList[i].y); // Cut point where line leaves canvas
        if (DEBUG) { sprintf(fr->x->c->errcontext.tempErrStr, "Departed canvas at crossing point %d.", i); ppl_log(&fr->x->c->errcontext,NULL); }
        CrossPointList[i].used = 1;
       }

      // Skirt along edge of clip region to find next entry point
      for (k=i; ((CrossPointList[i].x==CrossPointList[k].x)&&(CrossPointList[i].y==CrossPointList[k].y)); k=(k+1)%nac) if (!CrossPointList[k].used) { i=k; FillSide = !FillSide; continue; }
      for (k=i; ((CrossPointList[i].x==CrossPointList[k].x)&&(CrossPointList[i].y==CrossPointList[k].y)); k=(k+nac-1)%nac) if (!CrossPointList[k].used) { i=k; FillSide = !FillSide; continue; }
      if (FillSide) j=(i+nac-1)%nac; // Move anticlockwise around path
      else          j=(i+1)%nac;     // Move clockwise around path
      CurrentFace = CrossPointList[i].AxisFace;
      if (DEBUG) { sprintf(fr->x->c->errcontext.tempErrStr, "Move along edge of canvas from %d to %d; filling on %s; moving from axis face %d to %d.",i,j,FillSide?"right":"left",(int)CrossPointList[i].AxisFace,(int)CrossPointList[j].AxisFace); ppl_log(&fr->x->c->errcontext,NULL); }
      while (CurrentFace != CrossPointList[j].AxisFace) // If we've changed side of clip region, add corner to path
       {
        double xap,yap;
        const double xaps[6] = {9,1,0,0,1,1};
        const double yaps[6] = {9,1,1,0,0,1};
        if (!FillSide) { xap=xaps[CurrentFace+1]; yap=yaps[CurrentFace+1]; CurrentFace= (CurrentFace   %4)+1; }
        else           { xap=xaps[CurrentFace  ]; yap=yaps[CurrentFace  ]; CurrentFace=((CurrentFace+2)%4)+1; }
        if (DEBUG) { sprintf(fr->x->c->errcontext.tempErrStr, "Corner (%e,%e)", xap, yap); ppl_log(&fr->x->c->errcontext,NULL); }
        if (!fr->ThreeDim) { PS_POINT(fr->origin_x + fr->width * xap, fr->origin_y + fr->height * yap); }
        else               { PS_POINT(fr->origin_x + fr->width/2 + (xap-0.5)*fr->width*cos(fr->sg->XYview.real) + (yap-0.5)*fr->height*sin(fr->sg->XYview.real), fr->origin_y + fr->height/2 - (xap-0.5)*fr->width*cos(fr->sg->YZview.real)*sin(fr->sg->XYview.real) + (yap-0.5)*fr->height*cos(fr->sg->YZview.real)*cos(fr->sg->XYview.real)); }
       }
      i=j;
      if (CrossPointList[i].used) break;
     }
    if (!first_point) { fprintf(fr->x->epsbuffer, "closepath\n"); }
    first_point=1;
    if (DEBUG) { ppl_log(&fr->x->c->errcontext,"End segment."); }
   }
  if (DEBUG) { ppl_log(&fr->x->c->errcontext,"End filled region."); }
  if (!first_point) { fprintf(fr->x->epsbuffer, "closepath\n"); }
  fprintf(fr->x->epsbuffer, "%s\n", EndText);
  return;
 }

void FilledRegion_Finish(EPSComm *X, FilledRegionHandle *fr, int linetype, double linewidth, unsigned char StrokeOutline)
 {
  unsigned char f1, f2, first_point=1, first_subpath=1;
  int i, j, k, kn, l, NCrossings, i1, i2;
  double lastx, lasty, lastxap, lastyap;
  double cx1,cy1,cz1,cx2,cy2,cz2,ap1,ap2;
  listItem *li;
  FilledRegionPoint *p, *last_p;
  FilledRegionAxisCrossing *CrossPointList;
  double ct_y_m1, ct_y_c1, ct_x_m1, ct_x_c1, ct_y_m2, ct_y_c2, ct_x_m2, ct_x_c2, ct_xi, ct_yi;
  listItem *ct_li;
  FilledRegionPoint *ct_pA, *ct_pB;
  const EPSComm *x = fr->x; // Makes IF_NOT_INVISIBLE macro work

  l = fr->points->length;
  li = fr->points->first;
  if (l < 2) return; // No points on outline to stroke
  if (!fr->EverInside) return; // Path never ventures within clip region
  eps_core_WritePSColor(fr->x);

  // Path never goes outside the clip region
  if (fr->Naxiscrossings < 1)
   {
    if (DEBUG) { ppl_log(&fr->x->c->errcontext,"New filled region never leaves clip region."); }
    if (DEBUG) { ppl_log(&fr->x->c->errcontext,"1. Fill it."); }
    IF_NOT_INVISIBLE
     {
      for (i=0; i<l; i++)
       {
        p = (FilledRegionPoint *)(li->data);
        PS_POINT(p->x, p->y);
        li=li->next;
       }
      fprintf(fr->x->epsbuffer, "closepath eofill\n");
     }
    eps_core_SwitchFrom_FillColor(fr->x,1);
    if (!StrokeOutline) return;
    eps_core_SetLinewidth(fr->x, linewidth, linetype, 0.0);
    li = fr->points->first;
    first_point=first_subpath=1;
    if (DEBUG) { ppl_log(&fr->x->c->errcontext,"2. Stroke its outline."); }
    IF_NOT_INVISIBLE
     {
      for (i=0; i<l; i++)
       {
        p = (FilledRegionPoint *)(li->data);
        PS_POINT(p->x, p->y);
        li=li->next;
       }
      fprintf(fr->x->epsbuffer, "closepath stroke\n");
     }
    return;
   }

  // Close path to get potential final axis crossing
  p = (FilledRegionPoint *)(li->data);
  FilledRegion_Point(fr->x, fr, p->xa, p->ya); // Add first point to end of point list
  l = fr->points->length;
  lastx   = p->x;
  lasty   = p->y;
  lastxap = p->xap;
  lastyap = p->yap;
  last_p  = p;
  li=li->next;

  // Malloc a structure to hold the positions of all of the points where we cross the boundary of the clip region
  CrossPointList = (FilledRegionAxisCrossing *)ppl_memAlloc(fr->Naxiscrossings * 2 * sizeof(FilledRegionAxisCrossing));
  if (CrossPointList == NULL) return;
  for (i=1,j=0; i<l; i++)
   {
    p = (FilledRegionPoint *)(li->data);
    LineDraw_FindCrossingPoints(fr->x,lastx,lasty,0,lastxap,lastyap,0.5,p->x,p->y,0,p->xap,p->yap,0.5,&i1,&i2,&cx1,&cy1,&cz1,&cx2,&cy2,&cz2,&f1,&ap1,&f2,&ap2,&NCrossings);

    // Check for whether line crosses a previous line segment; if so, flip fill side
    ct_y_m1 = (cy2-cy1)/(cx2-cx1);
    ct_y_c1 = cy1 - ct_y_m1 * cx1;
    ct_x_m1 = (cx2-cx1)/(cy2-cy1);
    ct_x_c1 = cx1 - ct_x_m1 * cy1;
    ct_li   = fr->points->first;
    ct_pB   = (FilledRegionPoint *)(ct_li->data);
    ct_li   = ct_li->next;
    for (k=1,kn=0; k<(i-1); k++)
     {
      ct_pA    = ct_pB;
      ct_pB    = (FilledRegionPoint *)(ct_li->data);
      ct_li    = ct_li->next;
      ct_y_m2  = (ct_pB->y-ct_pA->y)/(ct_pB->x-ct_pA->x);
      ct_y_c2  = ct_pA->y - ct_y_m2 * ct_pA->x;
      ct_x_m2  = (ct_pB->x-ct_pA->x)/(ct_pB->y-ct_pA->y);
      ct_x_c2  = ct_pA->x - ct_x_m2 * ct_pA->y;
      ct_xi    = (ct_y_c2-ct_y_c1)/(ct_y_m1-ct_y_m2);
      ct_yi    = (ct_x_c2-ct_x_c1)/(ct_x_m1-ct_x_m2);
      if      ((gsl_finite(ct_xi))&&(((ct_xi>=cx1     )&&(ct_xi<cx2     )) || ((ct_xi<=cx1     )&&(ct_xi>cx2     )))
                                  &&(((ct_xi>=ct_pA->x)&&(ct_xi<ct_pB->x)) || ((ct_xi<=ct_pA->x)&&(ct_xi>ct_pB->x))) )
                        { kn++; ct_pB->FillSideFlip_prv++; ct_pB->FillSideFlip_fwd++; }
      else if ((gsl_finite(ct_yi))&&(((ct_yi>=cy1     )&&(ct_yi<cy2     )) || ((ct_yi<=cy1     )&&(ct_yi>cy2     )))
                                  &&(((ct_yi>=ct_pA->y)&&(ct_yi<ct_pB->y)) || ((ct_yi<=ct_pA->y)&&(ct_yi>ct_pB->y))) )
                        { kn++; ct_pB->FillSideFlip_prv++; ct_pB->FillSideFlip_fwd++; }
     }
    p     ->FillSideFlip_prv = kn;
    last_p->FillSideFlip_fwd = kn;


    if ( ((!i1)||(!i2)) && (NCrossings>0) ) // We have crossed boundary
     {
      if (!i1) { CrossPointList[j].x = cx1; CrossPointList[j].y = cy1; CrossPointList[j].AxisFace = f1; CrossPointList[j].AxisPos = ap1; }
      else     { CrossPointList[j].x = cx2; CrossPointList[j].y = cy2; CrossPointList[j].AxisFace = f2; CrossPointList[j].AxisPos = ap2; }
      if ((!i1)&&(!i2)) { CrossPointList[j].x2 = cx2; CrossPointList[j].y2 = cy2; }
      CrossPointList[j].id        = j;
      CrossPointList[j].singleton = ((!i1)&&(!i2));
      CrossPointList[j].twin      = j+1;
      CrossPointList[j].sense     = (i1) ? OUTGOING : INCOMING;
      CrossPointList[j].point     = li;
      CrossPointList[j].used      = 0;
      j++;
     }
    if ( ((!i1)&&(!i2)) && (NCrossings>0) )
     {
      CrossPointList[j] = CrossPointList[j-1];
      CrossPointList[j].x = cx2; CrossPointList[j].y = cy2;
      CrossPointList[j].x2= cx1; CrossPointList[j].y2= cy1;
      CrossPointList[j].id       = j;
      CrossPointList[j].AxisFace = f2;
      CrossPointList[j].AxisPos  = ap2;
      CrossPointList[j].sense    = OUTGOING;
      CrossPointList[j].twin     = j-1;
      j++;
     }
    lastx   = p->x;
    lasty   = p->y;
    lastxap = p->xap;
    lastyap = p->yap;
    last_p  = p;
    li=li->next;
   }

  // Sort list into order around perimeter of clipping area
  qsort((void *)CrossPointList, j, sizeof(FilledRegionAxisCrossing), &frac_sorter);

  // Output path
  if (DEBUG) { ppl_log(&fr->x->c->errcontext,"New filled region which crosses edge of clip region."); }
  if (DEBUG) { ppl_log(&fr->x->c->errcontext,"1. Fill it."); }
  IF_NOT_INVISIBLE OutputPath(fr, CrossPointList, j, "eofill", linewidth);
  eps_core_SwitchFrom_FillColor(fr->x,1);
  if (!StrokeOutline) return;
  eps_core_SetLinewidth(fr->x, linewidth, linetype, 0.0);
  li = fr->points->first;
  if (DEBUG) { ppl_log(&fr->x->c->errcontext,"2. Stroke its outline."); }
  IF_NOT_INVISIBLE OutputPath(fr, CrossPointList, j, "stroke", linewidth);
  return;
 }

