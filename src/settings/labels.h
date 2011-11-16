// labels.h
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

#ifndef _PPLSET_LABELS_H
#define _PPLSET_LABELS_H 1

#include "coreUtils/dict.h"
#include "settings/withWords.h"

#include "userspace/pplObj.h"
#include "userspace/context.h"

typedef struct ppllabel_object {
 int         id;
 pplObj      x       ,y       ,z;
 int         system_x,system_y,system_z;
 int         axis_x  ,axis_y  ,axis_z;
 char       *text;
 withWords   style;
 double      rotation, gap;
 int         HAlign, VAlign;
 struct ppllabel_object *next;
 } ppllabel_object;

void ppllabel_add         (ppl_context *context, ppllabel_object **inlist, dict *in);
void ppllabel_remove      (ppl_context *context, ppllabel_object **inlist, dict *in);
void ppllabel_unset       (ppl_context *context, ppllabel_object **inlist, dict *in);
void ppllabel_default     (ppl_context *context, ppllabel_object **inlist, dict *in);
unsigned char ppllabel_compare(ppl_context *context, ppllabel_object *a, ppllabel_object *b);
void ppllabel_list_copy   (ppl_context *context, ppllabel_object **out, ppllabel_object **in);
void ppllabel_list_destroy(ppl_context *context, ppllabel_object **inlist);
void ppllabel_print       (ppl_context *context, ppllabel_object  *in, char *out);

#endif
