// arrows.h
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

#ifndef _PPLSET_ARROWS_H
#define _PPLSET_ARROWS_H 1

#include "settings/withWords.h"
#include "userspace/pplObj.h"

typedef struct pplarrow_object {
 int        id;
 pplObj     x0       ,y0       ,z0       ,x1       ,y1       ,z1;
 int        system_x0,system_y0,system_z0,system_x1,system_y1,system_z1;
 int        axis_x0  ,axis_y0  ,axis_z0  ,axis_x1  ,axis_y1  ,axis_z1;
 int        pplarrow_style;
 withWords  style;
 struct pplarrow_object *next;
 } pplarrow_object;

#endif
