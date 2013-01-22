// labels.h
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

#ifndef _PPLSET_LABELS_H
#define _PPLSET_LABELS_H 1

#include "settings/withWords.h"
#include "userspace/pplObj.h"

typedef struct ppllabel_object {
 int         id;
 pplObj      x       ,y       ,z;
 int         system_x,system_y,system_z;
 int         axis_x  ,axis_y  ,axis_z;
 char       *text;
 double      fontsize;
 int         fontsizeSet;
 withWords   style;
 double      rotation, gap;
 int         HAlign, VAlign;
 struct ppllabel_object *next;
 } ppllabel_object;

#endif

