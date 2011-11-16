// pplarrows.h
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

#ifndef _PPLSET_ARROWS_H
#define _PPLSET_ARROWS_H 1

#include "coreUtils/dict.h"
#include "settings/withWords.h"

#include "userspace/pplObj.h"
#include "userspace/context.h"

typedef struct pplarrow_object {
 int        id;
 pplObj     x0       ,y0       ,z0       ,x1       ,y1       ,z1;
 int        system_x0,system_y0,system_z0,system_x1,system_y1,system_z1;
 int        axis_x0  ,axis_y0  ,axis_z0  ,axis_x1  ,axis_y1  ,axis_z1;
 int        pplarrow_style;
 withWords  style;
 struct pplarrow_object *next;
 } pplarrow_object;

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
  else                 Y = FetchSettingByName(tempstr, SW_SYSTEM_INT, SW_SYSTEM_STR); \
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
      sprintf(ppl_tempErrStr, "Axis number %d is out of range; axis numbers must be in the range 0 - %d", *tempint, MAX_AXES-1); \
      ppl_error(ERR_GENERAL, -1, -1,ppl_tempErrStr); \
      return; \
     } \
   } \
 }

#define pplarrow_add_check_dimensions(X,Y) \
 { \
  tempval = (pplObj *)ppl_dictLookup(in,X); \
  if (tempval == NULL) { tempval = &tempvalobj; pplObjZero(tempval,0); } \
  if ((Y == SW_SYSTEM_GRAPH) || (Y == SW_SYSTEM_PAGE)) \
   if (!tempval->dimensionless) \
    for (i=0; i<UNITS_MAX_BASEUNITS; i++) if (tempval->exponent[i] != (i==UNIT_LENGTH)) \
     { \
      sprintf(ppl_tempErrStr, "Coordinates specified in the graph and page systems must have dimensions of length. Received coordinate with dimensions of <%s>.", ppl_units_GetUnitStr(tempval, NULL, NULL, 0, 1, 0)); \
      ppl_error(ERR_GENERAL, -1, -1, ppl_tempErrStr); return; \
     } \
  if (!gsl_finite(tempval->real)) \
   { \
    ppl_error(ERR_GENERAL, -1, -1, "Coordinates specified are not finite."); return; \
   } \
 }

#define pplarrow_add_copy_coordinate(X,Y,Z) \
 { \
  tempval = (pplObj *)ppl_dictLookup(in,X); \
  if (tempval == NULL) { tempval = &tempvalobj; pplObjZero(tempval,0); } \
  if ((Y == SW_SYSTEM_GRAPH) || (Y == SW_SYSTEM_PAGE)) \
   if (tempval->dimensionless) { tempval->dimensionless=0; tempval->exponent[UNIT_LENGTH]=1; tempval->real /= 100; } \
  Z = *tempval; \
 }


#endif
