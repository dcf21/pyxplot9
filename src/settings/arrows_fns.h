// arrows_fns.h
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

#ifndef _PPLSET_ARROWS_FNS_H
#define _PPLSET_ARROWS_FNS_H 1

#include "coreUtils/dict.h"
#include "settings/arrows.h"
#include "userspace/context.h"
#include "userspace/unitsDisp.h"

void pplarrow_add         (ppl_context *context, pplarrow_object **inlist, dict *in);
void pplarrow_remove      (ppl_context *context, pplarrow_object **inlist, dict *in);
void pplarrow_unset       (ppl_context *context, pplarrow_object **inlist, dict *in);
void pplarrow_default     (ppl_context *context, pplarrow_object **inlist, dict *in);
unsigned char pplarrow_compare(ppl_context *context, pplarrow_object *a, pplarrow_object *b);
void pplarrow_list_copy   (ppl_context *context, pplarrow_object **out, pplarrow_object **in);
void pplarrow_list_destroy(ppl_context *context, pplarrow_object **inlist);
void pplarrow_print       (ppl_context *context, pplarrow_object  *in, char *out);

#define pplarrow_add_get_system(X,Y) \
 { \
  tempstr = (char *)ppl_dictLookup(in,X "_system"); \
  if (tempstr == NULL) Y = SW_SYSTEM_FIRST; \
  else                 Y = ppl_fetchSettingByName(&context->errcontext, tempstr, SW_SYSTEM_INT, SW_SYSTEM_STR); \
 }

#define pplarrow_add_get_axis(X,Y) \
 { \
  tempint = (int *)ppl_dictLookup(in,X "_axis"); \
  if   (tempint == NULL) Y = 0; \
  else                   Y = *tempint; \
 }

#define pplarrow_add_check_axis(X) \
 { \
  tempint = (int *)ppl_dictLookup(in,X "_axis"); \
  if (tempint != NULL) \
   { \
    if ((*tempint<0)||(*tempint>MAX_AXES)) \
     { \
      sprintf(context->errcontext.tempErrStr, "Axis number %d is out of range; axis numbers must be in the range 0 - %d", *tempint, MAX_AXES-1); \
      ppl_error(&context->errcontext, ERR_GENERAL, -1, -1, NULL); \
      return; \
     } \
   } \
 }

#define pplarrow_add_check_dimensions(X,Y) \
 { \
  tempval = (pplObj *)ppl_dictLookup(in,X); \
  if (tempval == NULL) { tempval = &tempvalobj; pplObjNum(tempval,0,0,0); } \
  if ((Y == SW_SYSTEM_GRAPH) || (Y == SW_SYSTEM_PAGE)) \
   if (!tempval->dimensionless) \
    for (i=0; i<UNITS_MAX_BASEUNITS; i++) if (tempval->exponent[i] != (i==UNIT_LENGTH)) \
     { \
      sprintf(context->errcontext.tempErrStr, "Coordinates specified in the graph and page systems must have dimensions of length. Received coordinate with dimensions of <%s>.", ppl_printUnit(context, tempval, NULL, NULL, 0, 1, 0)); \
      ppl_error(&context->errcontext, ERR_GENERAL, -1, -1, NULL); return; \
     } \
  if (!gsl_finite(tempval->real)) \
   { \
    ppl_error(&context->errcontext, ERR_GENERAL, -1, -1, "Coordinates specified are not finite."); return; \
   } \
 }

#define pplarrow_add_copy_coordinate(X,Y,Z) \
 { \
  tempval = (pplObj *)ppl_dictLookup(in,X); \
  if (tempval == NULL) { tempval = &tempvalobj; pplObjNum(tempval,0,0,0); } \
  if ((Y == SW_SYSTEM_GRAPH) || (Y == SW_SYSTEM_PAGE)) \
   if (tempval->dimensionless) { tempval->dimensionless=0; tempval->exponent[UNIT_LENGTH]=1; tempval->real /= 100; } \
  Z = *tempval; \
 }


#endif
