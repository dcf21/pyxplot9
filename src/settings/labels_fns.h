// labels_fns.h
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

#ifndef _PPLSET_LABELS_FNS_H
#define _PPLSET_LABELS_FNS_H 1

#include "coreUtils/dict.h"
#include "settings/labels.h"
#include "settings/withWords.h"
#include "userspace/context.h"
#include "userspace/pplObj.h"

void          ppllabel_add         (ppl_context *context, ppllabel_object **inlist, parserOutput *in, parserLine *pl, const int *ptab);
void          ppllabel_remove      (ppl_context *context, ppllabel_object **inlist, parserOutput *in, parserLine *pl, const int *ptab, int quiet);
void          ppllabel_unset       (ppl_context *context, ppllabel_object **inlist, parserOutput *in, parserLine *pl, const int *ptab);
unsigned char ppllabel_compare     (ppl_context *context, ppllabel_object *a, ppllabel_object *b);
void          ppllabel_list_copy   (ppl_context *context, ppllabel_object **out, ppllabel_object **in);
void          ppllabel_list_destroy(ppl_context *context, ppllabel_object **inlist);
void          ppllabel_print       (ppl_context *context, ppllabel_object  *in, char *out);

#endif
