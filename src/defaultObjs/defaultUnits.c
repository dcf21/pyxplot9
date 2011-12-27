// defaultUnits.c
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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <math.h>

#include <gsl/gsl_const_mksa.h>
#include <gsl/gsl_const_num.h>
#include <gsl/gsl_math.h>
#include <gsl/gsl_sf.h>

#include "coreUtils/errorReport.h"
#include "coreUtils/list.h"

#include "userspace/context.h"

#include "defaultObjs/defaultUnits.h"

void ppl_makeDefaultUnits(ppl_context *context)
 {
  int   i=0,j=0;
  int   unit_pos = 0;
  unit *unit_database;
  list *unit_PreferredUnits;

  context->unit_database = (unit *)malloc(UNITS_MAX*sizeof(unit));
  if (context->unit_database == NULL) { ppl_fatal(&context->errcontext,__FILE__,__LINE__,"Out of memory error whilst trying to malloc units database."); exit(1); }
  unit_database = context->unit_database;

  context->unit_PreferredUnits = ppl_listInit(1);
  if (context->unit_PreferredUnits == NULL) { ppl_fatal(&context->errcontext,__FILE__,__LINE__,"Out of memory error whilst trying to malloc units database."); exit(1); }
  unit_PreferredUnits = context->unit_PreferredUnits;

  context->unit_pos     = 0;
  context->baseunit_pos = UNIT_FIRSTUSER;

  // Set up database of known units
  for (i=0;i<UNITS_MAX;i++)
   {
    unit_database[i].nameAs     = NULL;
    unit_database[i].nameAp     = NULL;
    unit_database[i].nameLs     = NULL;
    unit_database[i].nameLp     = NULL;
    unit_database[i].nameFs     = NULL;
    unit_database[i].nameFp     = NULL;
    unit_database[i].alt1       = NULL;
    unit_database[i].alt2       = NULL;
    unit_database[i].alt3       = NULL;
    unit_database[i].alt4       = NULL;
    unit_database[i].quantity   = NULL;
    unit_database[i].comment    = NULL;
    unit_database[i].multiplier = 1.0;
    unit_database[i].tempType   = 0;
    unit_database[i].offset     = 0.0;
    unit_database[i].userSel    = 0;
    unit_database[i].notToBeCompounded = 0;
    unit_database[i].si         = unit_database[i].cgs       = unit_database[i].imperial  = unit_database[i].us = unit_database[i].planck =
    unit_database[i].ancient    = unit_database[i].userSel   = unit_database[i].modified  = 0;
    unit_database[i].maxPrefix  = unit_database[i].minPrefix = 0;
    unit_database[i].userSelPrefix = 0;
    for (j=0; j<UNITS_MAX_BASEUNITS; j++) unit_database[i].exponent[j] = 0;
   }

  // Set up default list of units
  unit_database[unit_pos].nameAs     = "percent"; // Percent
  unit_database[unit_pos].nameAp     = "percent";
  unit_database[unit_pos].nameLs     = "\\%";
  unit_database[unit_pos].nameLp     = "\\%";
  unit_database[unit_pos].nameFs     = "percent";
  unit_database[unit_pos].nameFp     = "percent";
  unit_database[unit_pos].quantity   = "dimensionlessness";
  unit_database[unit_pos].multiplier = 0.01;
  unit_pos++;

  unit_database[unit_pos].nameAs     = "ppm"; // Parts per million
  unit_database[unit_pos].nameAp     = "ppm";
  unit_database[unit_pos].nameLs     = unit_database[unit_pos].nameAs;
  unit_database[unit_pos].nameLp     = unit_database[unit_pos].nameAp;
  unit_database[unit_pos].nameFs     = "parts_per_million";
  unit_database[unit_pos].nameFp     = "parts_per_million";
  unit_database[unit_pos].quantity   = "dimensionlessness";
  unit_database[unit_pos].multiplier = 1e-6;
  unit_pos++;

  unit_database[unit_pos].nameAs     = "ppb"; // Parts per billion
  unit_database[unit_pos].nameAp     = "ppb";
  unit_database[unit_pos].nameLs     = unit_database[unit_pos].nameAs;
  unit_database[unit_pos].nameLp     = unit_database[unit_pos].nameAp;
  unit_database[unit_pos].nameFs     = "parts_per_billion";
  unit_database[unit_pos].nameFp     = "parts_per_billion";
  unit_database[unit_pos].quantity   = "dimensionlessness";
  unit_database[unit_pos].multiplier = 1e-9;
  unit_pos++;

  unit_database[unit_pos].nameAs     = "m"; // Metre
  unit_database[unit_pos].nameAp     = "m";
  unit_database[unit_pos].nameLs     = unit_database[unit_pos].nameAs;
  unit_database[unit_pos].nameLp     = unit_database[unit_pos].nameAp;
  unit_database[unit_pos].nameFs     = "metre";
  unit_database[unit_pos].nameFp     = "metres";
  unit_database[unit_pos].quantity   = "length";
  unit_database[unit_pos].si         = 1;
  unit_database[unit_pos].minPrefix  = -24;
  unit_database[unit_pos].maxPrefix  = 3;
  unit_database[unit_pos].exponent[UNIT_LENGTH]=1;
  unit_pos++;

  unit_database[unit_pos].nameAs     = "cm"; // Centimetre
  unit_database[unit_pos].nameAp     = "cm";
  unit_database[unit_pos].nameLs     = unit_database[unit_pos].nameAs;
  unit_database[unit_pos].nameLp     = unit_database[unit_pos].nameAp;
  unit_database[unit_pos].nameFs     = "centimetre";
  unit_database[unit_pos].nameFp     = "centimetres";
  unit_database[unit_pos].quantity   = "length";
  unit_database[unit_pos].multiplier = 0.01;
  unit_database[unit_pos].cgs        = 1;
  unit_database[unit_pos].exponent[UNIT_LENGTH]=1;
  unit_pos++;

  unit_database[unit_pos].nameAs     = "dm"; // Decimetre
  unit_database[unit_pos].nameAp     = "dm";
  unit_database[unit_pos].nameLs     = unit_database[unit_pos].nameAs;
  unit_database[unit_pos].nameLp     = unit_database[unit_pos].nameAp;
  unit_database[unit_pos].nameFs     = "decimetre";
  unit_database[unit_pos].nameFp     = "decimetres";
  unit_database[unit_pos].quantity   = "length";
  unit_database[unit_pos].multiplier = 0.1;
  unit_database[unit_pos].exponent[UNIT_LENGTH]=1;
  unit_pos++;

  unit_database[unit_pos].nameAs     = "ang"; // Angstrom
  unit_database[unit_pos].nameAp     = "ang";
  unit_database[unit_pos].nameLs     = "\\AA";
  unit_database[unit_pos].nameLp     = "\\AA";
  unit_database[unit_pos].nameFs     = "angstrom";
  unit_database[unit_pos].nameFp     = "angstroms";
  unit_database[unit_pos].quantity   = "length";
  unit_database[unit_pos].multiplier = 1e-10;
  unit_database[unit_pos].exponent[UNIT_LENGTH]=1;
  unit_pos++;

  unit_database[unit_pos].nameAs     = "in"; // Inch
  unit_database[unit_pos].nameAp     = "in";
  unit_database[unit_pos].nameLs     = unit_database[unit_pos].nameAs;
  unit_database[unit_pos].nameLp     = unit_database[unit_pos].nameAp;
  unit_database[unit_pos].nameFs     = "inch";
  unit_database[unit_pos].nameFp     = "inches";
  unit_database[unit_pos].quantity   = "length";
  unit_database[unit_pos].multiplier = GSL_CONST_MKSA_INCH;
  unit_database[unit_pos].imperial   = 1;
  unit_database[unit_pos].us         = 1;
  unit_database[unit_pos].exponent[UNIT_LENGTH]=1;
  unit_pos++;

  unit_database[unit_pos].nameAs     = "ft"; // Foot
  unit_database[unit_pos].nameAp     = "ft";
  unit_database[unit_pos].nameLs     = unit_database[unit_pos].nameAs;
  unit_database[unit_pos].nameLp     = unit_database[unit_pos].nameAp;
  unit_database[unit_pos].nameFs     = "foot";
  unit_database[unit_pos].nameFp     = "feet";
  unit_database[unit_pos].quantity   = "length";
  unit_database[unit_pos].multiplier = GSL_CONST_MKSA_FOOT;
  unit_database[unit_pos].imperial   = 1;
  unit_database[unit_pos].us         = 1;
  unit_database[unit_pos].exponent[UNIT_LENGTH]=1;
  unit_pos++;

  unit_database[unit_pos].nameAs     = "yd"; // Yard
  unit_database[unit_pos].nameAp     = "yd";
  unit_database[unit_pos].nameLs     = unit_database[unit_pos].nameAs;
  unit_database[unit_pos].nameLp     = unit_database[unit_pos].nameAp;
  unit_database[unit_pos].nameFs     = "yard";
  unit_database[unit_pos].nameFp     = "yards";
  unit_database[unit_pos].quantity   = "length";
  unit_database[unit_pos].multiplier = GSL_CONST_MKSA_YARD;
  unit_database[unit_pos].imperial   = 1;
  unit_database[unit_pos].us         = 1;
  unit_database[unit_pos].exponent[UNIT_LENGTH]=1;
  unit_pos++;

  unit_database[unit_pos].nameAs     = "mi"; // Mile
  unit_database[unit_pos].nameAp     = "mi";
  unit_database[unit_pos].nameLs     = unit_database[unit_pos].nameAs;
  unit_database[unit_pos].nameLp     = unit_database[unit_pos].nameAp;
  unit_database[unit_pos].nameFs     = "mile";
  unit_database[unit_pos].nameFp     = "miles";
  unit_database[unit_pos].quantity   = "length";
  unit_database[unit_pos].multiplier = GSL_CONST_MKSA_MILE;
  unit_database[unit_pos].imperial   = 1;
  unit_database[unit_pos].us         = 1;
  unit_database[unit_pos].exponent[UNIT_LENGTH]=1;
  unit_pos++;

  unit_database[unit_pos].nameAs     = "furlong"; // Furlong
  unit_database[unit_pos].nameAp     = "furlongs";
  unit_database[unit_pos].nameLs     = unit_database[unit_pos].nameAs;
  unit_database[unit_pos].nameLp     = unit_database[unit_pos].nameAp;
  unit_database[unit_pos].nameFs     = "furlong";
  unit_database[unit_pos].nameFp     = "furlongs";
  unit_database[unit_pos].quantity   = "length";
  unit_database[unit_pos].multiplier = 201.168;
  unit_database[unit_pos].exponent[UNIT_LENGTH]=1;
  unit_pos++;

  unit_database[unit_pos].nameAs     = "chain"; // Chain
  unit_database[unit_pos].nameAp     = "chains";
  unit_database[unit_pos].nameLs     = unit_database[unit_pos].nameAs;
  unit_database[unit_pos].nameLp     = unit_database[unit_pos].nameAp;
  unit_database[unit_pos].nameFs     = "chain";
  unit_database[unit_pos].nameFp     = "chains";
  unit_database[unit_pos].quantity   = "length";
  unit_database[unit_pos].multiplier = 20.1168;
  unit_database[unit_pos].exponent[UNIT_LENGTH]=1;
  unit_pos++;

  unit_database[unit_pos].nameAs     = "link"; // Link
  unit_database[unit_pos].nameAp     = "links";
  unit_database[unit_pos].nameLs     = unit_database[unit_pos].nameAs;
  unit_database[unit_pos].nameLp     = unit_database[unit_pos].nameAp;
  unit_database[unit_pos].nameFs     = "link";
  unit_database[unit_pos].nameFp     = "links";
  unit_database[unit_pos].quantity   = "length";
  unit_database[unit_pos].multiplier = 0.201168;
  unit_database[unit_pos].exponent[UNIT_LENGTH]=1;
  unit_pos++;

  unit_database[unit_pos].nameAs     = "cable"; // Cable
  unit_database[unit_pos].nameAp     = "cables";
  unit_database[unit_pos].nameLs     = unit_database[unit_pos].nameAs;
  unit_database[unit_pos].nameLp     = unit_database[unit_pos].nameAp;
  unit_database[unit_pos].nameFs     = "cable";
  unit_database[unit_pos].nameFp     = "cables";
  unit_database[unit_pos].quantity   = "length";
  unit_database[unit_pos].multiplier = 185.3184;
  unit_database[unit_pos].exponent[UNIT_LENGTH]=1;
  unit_pos++;

  unit_database[unit_pos].nameAs     = "fathom"; // Fathom
  unit_database[unit_pos].nameAp     = "fathoms";
  unit_database[unit_pos].nameLs     = unit_database[unit_pos].nameAs;
  unit_database[unit_pos].nameLp     = unit_database[unit_pos].nameAp;
  unit_database[unit_pos].nameFs     = "fathom";
  unit_database[unit_pos].nameFp     = "fathoms";
  unit_database[unit_pos].quantity   = "length";
  unit_database[unit_pos].multiplier = GSL_CONST_MKSA_FATHOM;
  unit_database[unit_pos].exponent[UNIT_LENGTH]=1;
  unit_pos++;

  unit_database[unit_pos].nameAs     = "roman_mile"; // Roman Mile
  unit_database[unit_pos].nameAp     = "roman_miles";
  unit_database[unit_pos].nameLs     = unit_database[unit_pos].nameAs;
  unit_database[unit_pos].nameLp     = unit_database[unit_pos].nameAp;
  unit_database[unit_pos].nameFs     = "roman_mile";
  unit_database[unit_pos].nameFp     = "roman_miles";
  unit_database[unit_pos].quantity   = "length";
  unit_database[unit_pos].multiplier = 1479.0;
  unit_database[unit_pos].exponent[UNIT_LENGTH]=1;
  unit_pos++;

  unit_database[unit_pos].nameAs     = "roman_league"; // Roman League
  unit_database[unit_pos].nameAp     = "roman_leagues";
  unit_database[unit_pos].nameLs     = unit_database[unit_pos].nameAs;
  unit_database[unit_pos].nameLp     = unit_database[unit_pos].nameAp;
  unit_database[unit_pos].nameFs     = "roman_league";
  unit_database[unit_pos].nameFp     = "roman_leagues";
  unit_database[unit_pos].quantity   = "length";
  unit_database[unit_pos].multiplier = 1.5 * 1479.0;
  unit_database[unit_pos].exponent[UNIT_LENGTH]=1;
  unit_pos++;

  unit_database[unit_pos].nameAs     = "nautical_mile"; // Nautical mile
  unit_database[unit_pos].nameAp     = "nautical_miles";
  unit_database[unit_pos].nameLs     = unit_database[unit_pos].nameAs;
  unit_database[unit_pos].nameLp     = unit_database[unit_pos].nameAp;
  unit_database[unit_pos].nameFs     = "nautical_mile";
  unit_database[unit_pos].nameFp     = "nautical_miles";
  unit_database[unit_pos].quantity   = "length";
  unit_database[unit_pos].multiplier = GSL_CONST_MKSA_NAUTICAL_MILE;
  unit_database[unit_pos].exponent[UNIT_LENGTH]=1;
  unit_pos++;

  unit_database[unit_pos].nameAs     = "pt"; // Point (typesetting unit)
  unit_database[unit_pos].nameAp     = "pt";
  unit_database[unit_pos].nameLs     = unit_database[unit_pos].nameAs;
  unit_database[unit_pos].nameLp     = unit_database[unit_pos].nameAp;
  unit_database[unit_pos].nameFs     = "point";
  unit_database[unit_pos].nameFp     = "points";
  unit_database[unit_pos].quantity   = "length";
  unit_database[unit_pos].multiplier = GSL_CONST_MKSA_INCH / 72.0;
  unit_database[unit_pos].exponent[UNIT_LENGTH]=1;
  unit_pos++;

  unit_database[unit_pos].nameAs     = "pica"; // Pica (typesetting unit)
  unit_database[unit_pos].nameAp     = "picas";
  unit_database[unit_pos].nameLs     = unit_database[unit_pos].nameAs;
  unit_database[unit_pos].nameLp     = unit_database[unit_pos].nameAp;
  unit_database[unit_pos].nameFs     = "pica";
  unit_database[unit_pos].nameFp     = "picas";
  unit_database[unit_pos].quantity   = "length";
  unit_database[unit_pos].multiplier = GSL_CONST_MKSA_INCH / 6.0;
  unit_database[unit_pos].exponent[UNIT_LENGTH]=1;
  unit_pos++;

  unit_database[unit_pos].nameAs     = "cubit"; // Cubit
  unit_database[unit_pos].nameAp     = "cubits";
  unit_database[unit_pos].nameLs     = unit_database[unit_pos].nameAs;
  unit_database[unit_pos].nameLp     = unit_database[unit_pos].nameAp;
  unit_database[unit_pos].nameFs     = "cubit";
  unit_database[unit_pos].nameFp     = "cubits";
  unit_database[unit_pos].quantity   = "length";
  unit_database[unit_pos].multiplier = 0.4572;
  unit_database[unit_pos].ancient    = 1;
  unit_database[unit_pos].exponent[UNIT_LENGTH]=1;
  unit_pos++;

  unit_database[unit_pos].nameAs     = "rod"; // Rod
  unit_database[unit_pos].nameAp     = "rods";
  unit_database[unit_pos].nameLs     = unit_database[unit_pos].nameAs;
  unit_database[unit_pos].nameLp     = unit_database[unit_pos].nameAp;
  unit_database[unit_pos].nameFs     = "rod";
  unit_database[unit_pos].nameFp     = "rods";
  unit_database[unit_pos].quantity   = "length";
  unit_database[unit_pos].multiplier = 5.02920;
  unit_database[unit_pos].exponent[UNIT_LENGTH]=1;
  unit_pos++;

  unit_database[unit_pos].nameAs     = "perch"; // Perch
  unit_database[unit_pos].nameAp     = "perches";
  unit_database[unit_pos].nameLs     = unit_database[unit_pos].nameAs;
  unit_database[unit_pos].nameLp     = unit_database[unit_pos].nameAp;
  unit_database[unit_pos].nameFs     = "perch";
  unit_database[unit_pos].nameFp     = "perches";
  unit_database[unit_pos].quantity   = "length";
  unit_database[unit_pos].multiplier = 5.02920;
  unit_database[unit_pos].exponent[UNIT_LENGTH]=1;
  unit_pos++;

  unit_database[unit_pos].nameAs     = "pole"; // Pole
  unit_database[unit_pos].nameAp     = "poles";
  unit_database[unit_pos].nameLs     = unit_database[unit_pos].nameAs;
  unit_database[unit_pos].nameLp     = unit_database[unit_pos].nameAp;
  unit_database[unit_pos].nameFs     = "pole";
  unit_database[unit_pos].nameFp     = "poles";
  unit_database[unit_pos].quantity   = "length";
  unit_database[unit_pos].multiplier = 5.02920;
  unit_database[unit_pos].exponent[UNIT_LENGTH]=1;
  unit_pos++;

  unit_database[unit_pos].nameAs     = "AU"; // Astronomical unit
  unit_database[unit_pos].nameAp     = "AU";
  unit_database[unit_pos].nameLs     = unit_database[unit_pos].nameAs;
  unit_database[unit_pos].nameLp     = unit_database[unit_pos].nameAp;
  unit_database[unit_pos].nameFs     = "astronomical_unit";
  unit_database[unit_pos].nameFp     = "astronomical_units";
  unit_database[unit_pos].quantity   = "length";
  unit_database[unit_pos].multiplier = GSL_CONST_MKSA_ASTRONOMICAL_UNIT;
  unit_database[unit_pos].exponent[UNIT_LENGTH]=1;
  unit_pos++;

  unit_database[unit_pos].nameAs     = "lyr"; // Lightyear
  unit_database[unit_pos].nameAp     = "lyr";
  unit_database[unit_pos].nameLs     = unit_database[unit_pos].nameAs;
  unit_database[unit_pos].nameLp     = unit_database[unit_pos].nameAp;
  unit_database[unit_pos].nameFs     = "light_year";
  unit_database[unit_pos].nameFp     = "light_years";
  unit_database[unit_pos].maxPrefix  = 9;
  unit_database[unit_pos].quantity   = "length";
  unit_database[unit_pos].multiplier = GSL_CONST_MKSA_LIGHT_YEAR;
  unit_database[unit_pos].exponent[UNIT_LENGTH]=1;
  unit_pos++;

  unit_database[unit_pos].nameAs     = "pc"; // Parsec
  unit_database[unit_pos].nameAp     = "pc";
  unit_database[unit_pos].nameLs     = unit_database[unit_pos].nameAs;
  unit_database[unit_pos].nameLp     = unit_database[unit_pos].nameAp;
  unit_database[unit_pos].nameFs     = "parsec";
  unit_database[unit_pos].nameFp     = "parsecs";
  unit_database[unit_pos].quantity   = "length";
  unit_database[unit_pos].multiplier = GSL_CONST_MKSA_PARSEC;
  unit_database[unit_pos].maxPrefix  = 9;
  unit_database[unit_pos].exponent[UNIT_LENGTH]=1;
  unit_pos++;

  unit_database[unit_pos].nameAs     = "Rsun"; // Solar radii
  unit_database[unit_pos].nameAp     = "Rsun";
  unit_database[unit_pos].nameLs     = "R_\\odot";
  unit_database[unit_pos].nameLp     = "R_\\odot";
  unit_database[unit_pos].nameFs     = "solar_radius";
  unit_database[unit_pos].nameFp     = "solar_radii";
  unit_database[unit_pos].alt1       = "Rsolar";
  unit_database[unit_pos].quantity   = "length";
  unit_database[unit_pos].multiplier = 6.955e8;
  unit_database[unit_pos].exponent[UNIT_LENGTH]=1;
  unit_pos++;

  unit_database[unit_pos].nameAs     = "Rearth"; // Earth radii
  unit_database[unit_pos].nameAp     = "Rearth";
  unit_database[unit_pos].nameLs     = "R_E";
  unit_database[unit_pos].nameLp     = "R_E";
  unit_database[unit_pos].nameFs     = "earth_radius";
  unit_database[unit_pos].nameFp     = "earth_radii";
  unit_database[unit_pos].quantity   = "length";
  unit_database[unit_pos].multiplier = 6371000;
  unit_database[unit_pos].exponent[UNIT_LENGTH]=1;
  unit_pos++;

  unit_database[unit_pos].nameAs     = "Rjupiter"; // Jupiter radii
  unit_database[unit_pos].nameAp     = "Rjupiter";
  unit_database[unit_pos].nameLs     = "R_J";
  unit_database[unit_pos].nameLp     = "R_J";
  unit_database[unit_pos].nameFs     = "jupiter_radius";
  unit_database[unit_pos].nameFp     = "jupiter_radii";
  unit_database[unit_pos].alt1       = "Rjove";
  unit_database[unit_pos].alt2       = "Rjovian";
  unit_database[unit_pos].quantity   = "length";
  unit_database[unit_pos].multiplier = 71492000;
  unit_database[unit_pos].exponent[UNIT_LENGTH]=1;
  unit_pos++;

  unit_database[unit_pos].nameAs     = "lunar_distance"; // Lunar distances
  unit_database[unit_pos].nameAp     = "lunar_distances";
  unit_database[unit_pos].nameLs     = unit_database[unit_pos].nameAs;
  unit_database[unit_pos].nameLp     = unit_database[unit_pos].nameAp;
  unit_database[unit_pos].nameFs     = "lunar_distance";
  unit_database[unit_pos].nameFp     = "lunar_distances";
  unit_database[unit_pos].quantity   = "length";
  unit_database[unit_pos].multiplier = 384403000;
  unit_database[unit_pos].exponent[UNIT_LENGTH]=1;
  unit_pos++;

  unit_database[unit_pos].nameAs     = "s"; // Second
  unit_database[unit_pos].nameAp     = "s";
  unit_database[unit_pos].nameLs     = unit_database[unit_pos].nameAs;
  unit_database[unit_pos].nameLp     = unit_database[unit_pos].nameAp;
  unit_database[unit_pos].nameFs     = "second";
  unit_database[unit_pos].nameFp     = "seconds";
  unit_database[unit_pos].alt1       = "sec";
  unit_database[unit_pos].alt2       = "secs";
  unit_database[unit_pos].quantity   = "time";
  unit_database[unit_pos].si         = 1;
  unit_database[unit_pos].cgs        = 1;
  unit_database[unit_pos].imperial   = 1;
  unit_database[unit_pos].us         = 1;
  unit_database[unit_pos].minPrefix  = -24;
  unit_database[unit_pos].exponent[UNIT_TIME]=1;
  unit_pos++;

  unit_database[unit_pos].nameAs     = "min"; // Minute
  unit_database[unit_pos].nameAp     = "min";
  unit_database[unit_pos].nameLs     = unit_database[unit_pos].nameAs;
  unit_database[unit_pos].nameLp     = unit_database[unit_pos].nameAp;
  unit_database[unit_pos].nameFs     = "minute";
  unit_database[unit_pos].nameFp     = "minutes";
  unit_database[unit_pos].quantity   = "time";
  unit_database[unit_pos].multiplier = 60;
  unit_database[unit_pos].exponent[UNIT_TIME]=1;
  unit_pos++;

  unit_database[unit_pos].nameAs     = "hr"; // Hour
  unit_database[unit_pos].nameAp     = "hr";
  unit_database[unit_pos].nameLs     = unit_database[unit_pos].nameAs;
  unit_database[unit_pos].nameLp     = unit_database[unit_pos].nameAp;
  unit_database[unit_pos].nameFs     = "hour";
  unit_database[unit_pos].nameFp     = "hours";
  unit_database[unit_pos].quantity   = "time";
  unit_database[unit_pos].multiplier = 3600;
  unit_database[unit_pos].ancient    = 1;
  unit_database[unit_pos].exponent[UNIT_TIME]=1;
  unit_pos++;

  unit_database[unit_pos].nameAs     = "day"; // Day
  unit_database[unit_pos].nameAp     = "days";
  unit_database[unit_pos].nameLs     = unit_database[unit_pos].nameAs;
  unit_database[unit_pos].nameLp     = unit_database[unit_pos].nameAp;
  unit_database[unit_pos].nameFs     = "day";
  unit_database[unit_pos].nameFp     = "days";
  unit_database[unit_pos].quantity   = "time";
  unit_database[unit_pos].multiplier = 86400;
  unit_database[unit_pos].ancient    = 1;
  unit_database[unit_pos].exponent[UNIT_TIME]=1;
  unit_pos++;

  unit_database[unit_pos].nameAs     = "week"; // Week
  unit_database[unit_pos].nameAp     = "weeks";
  unit_database[unit_pos].nameLs     = unit_database[unit_pos].nameAs;
  unit_database[unit_pos].nameLp     = unit_database[unit_pos].nameAp;
  unit_database[unit_pos].nameFs     = "week";
  unit_database[unit_pos].nameFp     = "weeks";
  unit_database[unit_pos].quantity   = "time";
  unit_database[unit_pos].multiplier = 604800;
  unit_database[unit_pos].exponent[UNIT_TIME]=1;
  unit_pos++;

  unit_database[unit_pos].nameAs     = "yr"; // Year
  unit_database[unit_pos].nameAp     = "yr";
  unit_database[unit_pos].nameLs     = unit_database[unit_pos].nameAs;
  unit_database[unit_pos].nameLp     = unit_database[unit_pos].nameAp;
  unit_database[unit_pos].nameFs     = "year";
  unit_database[unit_pos].nameFp     = "years";
  unit_database[unit_pos].quantity   = "time";
  unit_database[unit_pos].maxPrefix  = 9;
  unit_database[unit_pos].multiplier = 31557600;
  unit_database[unit_pos].ancient    = 1;
  unit_database[unit_pos].exponent[UNIT_TIME]=1;
  unit_pos++;

  unit_database[unit_pos].nameAs     = "sol"; // Sol (Martian Day)
  unit_database[unit_pos].nameAp     = "sols";
  unit_database[unit_pos].nameLs     = unit_database[unit_pos].nameAs;
  unit_database[unit_pos].nameLp     = unit_database[unit_pos].nameAp;
  unit_database[unit_pos].nameFs     = "sol";
  unit_database[unit_pos].nameFp     = "sols";
  unit_database[unit_pos].quantity   = "time";
  unit_database[unit_pos].multiplier = 88775.244;
  unit_database[unit_pos].exponent[UNIT_TIME]=1;
  unit_pos++;

  unit_database[unit_pos].nameAs     = "kg"; // Kilogram
  unit_database[unit_pos].nameAp     = "kg";
  unit_database[unit_pos].nameLs     = unit_database[unit_pos].nameAs;
  unit_database[unit_pos].nameLp     = unit_database[unit_pos].nameAp;
  unit_database[unit_pos].nameFs     = "kilogram";
  unit_database[unit_pos].nameFp     = "kilograms";
  unit_database[unit_pos].quantity   = "mass";
  unit_database[unit_pos].si         = 1;
  unit_database[unit_pos].exponent[UNIT_MASS]=1;
  unit_pos++;

  unit_database[unit_pos].nameAs     = "g"; // Gramme
  unit_database[unit_pos].nameAp     = "g";
  unit_database[unit_pos].nameLs     = unit_database[unit_pos].nameAs;
  unit_database[unit_pos].nameLp     = unit_database[unit_pos].nameAp;
  unit_database[unit_pos].nameFs     = "gramme";
  unit_database[unit_pos].nameFp     = "grammes";
  unit_database[unit_pos].quantity   = "mass";
  unit_database[unit_pos].multiplier = 1e-3;
  unit_database[unit_pos].minPrefix  = -24;
  unit_database[unit_pos].exponent[UNIT_MASS]=1;
  unit_pos++;

  unit_database[unit_pos].nameAs     = "g"; // Gram
  unit_database[unit_pos].nameAp     = "g";
  unit_database[unit_pos].nameLs     = unit_database[unit_pos].nameAs;
  unit_database[unit_pos].nameLp     = unit_database[unit_pos].nameAp;
  unit_database[unit_pos].nameFs     = "gram";
  unit_database[unit_pos].nameFp     = "grams";
  unit_database[unit_pos].quantity   = "mass";
  unit_database[unit_pos].multiplier = 1e-3;
  unit_database[unit_pos].cgs        = 1;
  unit_database[unit_pos].minPrefix  = -24;
  unit_database[unit_pos].exponent[UNIT_MASS]=1;
  unit_pos++;

  unit_database[unit_pos].nameAs     = "t"; // Metric Tonne
  unit_database[unit_pos].nameAp     = "t";
  unit_database[unit_pos].nameLs     = unit_database[unit_pos].nameAs;
  unit_database[unit_pos].nameLp     = unit_database[unit_pos].nameAp;
  unit_database[unit_pos].nameFs     = "tonne";
  unit_database[unit_pos].nameFp     = "tonnes";
  unit_database[unit_pos].alt1       = "metric_tonne";
  unit_database[unit_pos].alt2       = "metric_tonnes";
  unit_database[unit_pos].quantity   = "mass";
  unit_database[unit_pos].multiplier = 1e3;
  unit_database[unit_pos].exponent[UNIT_MASS]=1;
  unit_pos++;

  unit_database[unit_pos].nameAs     = "short_ton"; // US Ton
  unit_database[unit_pos].nameAp     = "short_tons";
  unit_database[unit_pos].nameLs     = "short\\_ton";
  unit_database[unit_pos].nameLp     = "short\\_tons";
  unit_database[unit_pos].nameFs     = "short_ton";
  unit_database[unit_pos].nameFp     = "short_tons";
  unit_database[unit_pos].comment    = "US customary";
  unit_database[unit_pos].quantity   = "mass";
  unit_database[unit_pos].multiplier = GSL_CONST_MKSA_TON;
  unit_database[unit_pos].us         = 1;
  unit_database[unit_pos].exponent[UNIT_MASS]=1;
  unit_pos++;

  unit_database[unit_pos].nameAs     = "ton"; // UK Ton
  unit_database[unit_pos].nameAp     = "tons";
  unit_database[unit_pos].nameLs     = "long\\_ton";
  unit_database[unit_pos].nameLp     = "long\\_tons";
  unit_database[unit_pos].nameFs     = "long_ton";
  unit_database[unit_pos].nameFp     = "long_tons";
  unit_database[unit_pos].comment    = "UK imperial";
  unit_database[unit_pos].quantity   = "mass";
  unit_database[unit_pos].multiplier = GSL_CONST_MKSA_UK_TON;
  unit_database[unit_pos].imperial   = 1;
  unit_database[unit_pos].exponent[UNIT_MASS]=1;
  unit_pos++;

  unit_database[unit_pos].nameAs     = "oz"; // Ounce
  unit_database[unit_pos].nameAp     = "oz";
  unit_database[unit_pos].nameLs     = unit_database[unit_pos].nameAs;
  unit_database[unit_pos].nameLp     = unit_database[unit_pos].nameAp;
  unit_database[unit_pos].nameFs     = "ounce";
  unit_database[unit_pos].nameFp     = "ounces";
  unit_database[unit_pos].quantity   = "mass";
  unit_database[unit_pos].multiplier = GSL_CONST_MKSA_OUNCE_MASS;
  unit_database[unit_pos].imperial   = unit_database[unit_pos].us = 1;
  unit_database[unit_pos].exponent[UNIT_MASS]=1;
  unit_pos++;

  unit_database[unit_pos].nameAs     = "oz_troy"; // Troy Ounce
  unit_database[unit_pos].nameAp     = "oz_troy";
  unit_database[unit_pos].nameLs     = "oz\\_troy";
  unit_database[unit_pos].nameLp     = "oz\\_troy";
  unit_database[unit_pos].nameFs     = "troy_ounce";
  unit_database[unit_pos].nameFp     = "troy_ounces";
  unit_database[unit_pos].quantity   = "mass";
  unit_database[unit_pos].multiplier = GSL_CONST_MKSA_TROY_OUNCE;
  unit_database[unit_pos].exponent[UNIT_MASS]=1;
  unit_pos++;

  unit_database[unit_pos].nameAs     = "lb"; // Pound
  unit_database[unit_pos].nameAp     = "lbs";
  unit_database[unit_pos].nameLs     = unit_database[unit_pos].nameAs;
  unit_database[unit_pos].nameLp     = unit_database[unit_pos].nameAp;
  unit_database[unit_pos].nameFs     = "pound";
  unit_database[unit_pos].nameFp     = "pounds";
  unit_database[unit_pos].quantity   = "mass";
  unit_database[unit_pos].multiplier = GSL_CONST_MKSA_POUND_MASS;
  unit_database[unit_pos].imperial   = unit_database[unit_pos].us = 1;
  unit_database[unit_pos].exponent[UNIT_MASS]=1;
  unit_pos++;

  unit_database[unit_pos].nameAs     = "stone"; // Stone
  unit_database[unit_pos].nameAp     = "stone";
  unit_database[unit_pos].nameLs     = unit_database[unit_pos].nameAs;
  unit_database[unit_pos].nameLp     = unit_database[unit_pos].nameAp;
  unit_database[unit_pos].nameFs     = "stone";
  unit_database[unit_pos].nameFp     = "stone";
  unit_database[unit_pos].quantity   = "mass";
  unit_database[unit_pos].multiplier = 6.35029318;
  unit_database[unit_pos].imperial   = unit_database[unit_pos].us = 1;
  unit_database[unit_pos].exponent[UNIT_MASS]=1;
  unit_pos++;

  unit_database[unit_pos].nameAs     = "slug"; // Slug
  unit_database[unit_pos].nameAp     = "slugs";
  unit_database[unit_pos].nameLs     = unit_database[unit_pos].nameAs;
  unit_database[unit_pos].nameLp     = unit_database[unit_pos].nameAp;
  unit_database[unit_pos].nameFs     = "slug";
  unit_database[unit_pos].nameFp     = "slugs";
  unit_database[unit_pos].quantity   = "mass";
  unit_database[unit_pos].multiplier = 14.5939;
  unit_database[unit_pos].exponent[UNIT_MASS]=1;
  unit_pos++;

  unit_database[unit_pos].nameAs     = "cwt_UK"; // UK hundredweight
  unit_database[unit_pos].nameAp     = "cwt_UK";
  unit_database[unit_pos].nameLs     = unit_database[unit_pos].nameAs;
  unit_database[unit_pos].nameLp     = unit_database[unit_pos].nameAp;
  unit_database[unit_pos].nameFs     = "hundredweight_UK";
  unit_database[unit_pos].nameFp     = "hundredweight_UK";
  unit_database[unit_pos].comment    = "UK imperial";
  unit_database[unit_pos].quantity   = "mass";
  unit_database[unit_pos].multiplier = 50.80234544;
  unit_database[unit_pos].imperial   = 1;
  unit_database[unit_pos].exponent[UNIT_MASS]=1;
  unit_pos++;

  unit_database[unit_pos].nameAs     = "cwt_US"; // US hundredweight
  unit_database[unit_pos].nameAp     = "cwt_US";
  unit_database[unit_pos].nameLs     = unit_database[unit_pos].nameAs;
  unit_database[unit_pos].nameLp     = unit_database[unit_pos].nameAp;
  unit_database[unit_pos].nameFs     = "hundredweight_US";
  unit_database[unit_pos].nameFp     = "hundredweight_US";
  unit_database[unit_pos].comment    = "US customary";
  unit_database[unit_pos].quantity   = "mass";
  unit_database[unit_pos].multiplier = 45.359237;
  unit_database[unit_pos].us         = 1;
  unit_database[unit_pos].exponent[UNIT_MASS]=1;
  unit_pos++;

  unit_database[unit_pos].nameAs     = "drachm"; // drachm
  unit_database[unit_pos].nameAp     = "drachms";
  unit_database[unit_pos].nameLs     = unit_database[unit_pos].nameAs;
  unit_database[unit_pos].nameLp     = unit_database[unit_pos].nameAp;
  unit_database[unit_pos].nameFs     = "drachm";
  unit_database[unit_pos].nameFp     = "drachms";
  unit_database[unit_pos].quantity   = "mass";
  unit_database[unit_pos].multiplier = 1.7718451953125e-3;
  unit_database[unit_pos].imperial   = unit_database[unit_pos].us = 1;
  unit_database[unit_pos].exponent[UNIT_MASS]=1;
  unit_pos++;

  unit_database[unit_pos].nameAs     = "grain"; // grain
  unit_database[unit_pos].nameAp     = "grains";
  unit_database[unit_pos].nameLs     = unit_database[unit_pos].nameAs;
  unit_database[unit_pos].nameLp     = unit_database[unit_pos].nameAp;
  unit_database[unit_pos].nameFs     = "grain";
  unit_database[unit_pos].nameFp     = "grains";
  unit_database[unit_pos].quantity   = "mass";
  unit_database[unit_pos].multiplier = 64.79891e-6;
  unit_database[unit_pos].imperial   = unit_database[unit_pos].us = 1;
  unit_database[unit_pos].exponent[UNIT_MASS]=1;
  unit_pos++;

  unit_database[unit_pos].nameAs     = "CD"; // carat
  unit_database[unit_pos].nameAp     = "CDs";
  unit_database[unit_pos].nameLs     = unit_database[unit_pos].nameAs;
  unit_database[unit_pos].nameLp     = unit_database[unit_pos].nameAp;
  unit_database[unit_pos].nameFs     = "carat";
  unit_database[unit_pos].nameFp     = "carats";
  unit_database[unit_pos].quantity   = "mass";
  unit_database[unit_pos].multiplier = 200e-6;
  unit_database[unit_pos].exponent[UNIT_MASS]=1;
  unit_pos++;

  unit_database[unit_pos].nameAs     = "shekel"; // Shekel
  unit_database[unit_pos].nameAp     = "shekels";
  unit_database[unit_pos].nameLs     = unit_database[unit_pos].nameAs;
  unit_database[unit_pos].nameLp     = unit_database[unit_pos].nameAp;
  unit_database[unit_pos].nameFs     = "shekel";
  unit_database[unit_pos].nameFp     = "shekels";
  unit_database[unit_pos].quantity   = "mass";
  unit_database[unit_pos].multiplier = 0.011;
  unit_database[unit_pos].ancient    = 1;
  unit_database[unit_pos].exponent[UNIT_MASS]=1;
  unit_pos++;

  unit_database[unit_pos].nameAs     = "mina"; // Mina
  unit_database[unit_pos].nameAp     = "minas";
  unit_database[unit_pos].nameLs     = unit_database[unit_pos].nameAs;
  unit_database[unit_pos].nameLp     = unit_database[unit_pos].nameAp;
  unit_database[unit_pos].nameFs     = "mina";
  unit_database[unit_pos].nameFp     = "minas";
  unit_database[unit_pos].quantity   = "mass";
  unit_database[unit_pos].multiplier = 0.011 * 60;
  unit_database[unit_pos].ancient    = 1;
  unit_database[unit_pos].exponent[UNIT_MASS]=1;
  unit_pos++;

  unit_database[unit_pos].nameAs     = "talent"; // Talent
  unit_database[unit_pos].nameAp     = "talents";
  unit_database[unit_pos].nameLs     = unit_database[unit_pos].nameAs;
  unit_database[unit_pos].nameLp     = unit_database[unit_pos].nameAp;
  unit_database[unit_pos].nameFs     = "talent";
  unit_database[unit_pos].nameFp     = "talents";
  unit_database[unit_pos].quantity   = "mass";
  unit_database[unit_pos].multiplier = 0.011 * 360;
  unit_database[unit_pos].ancient    = 1;
  unit_database[unit_pos].exponent[UNIT_MASS]=1;
  unit_pos++;

  unit_database[unit_pos].nameAs     = "Msun"; // Solar mass
  unit_database[unit_pos].nameAp     = "Msun";
  unit_database[unit_pos].nameLs     = "M_\\odot";
  unit_database[unit_pos].nameLp     = "M_\\odot";
  unit_database[unit_pos].nameFs     = "solar_mass";
  unit_database[unit_pos].nameFp     = "solar_masses";
  unit_database[unit_pos].alt1       = "Msolar";
  unit_database[unit_pos].quantity   = "mass";
  unit_database[unit_pos].multiplier = GSL_CONST_MKSA_SOLAR_MASS;
  unit_database[unit_pos].exponent[UNIT_MASS]=1;
  unit_pos++;

  unit_database[unit_pos].nameAs     = "Mearth"; // Earth mass
  unit_database[unit_pos].nameAp     = "Mearth";
  unit_database[unit_pos].nameLs     = "M_E";
  unit_database[unit_pos].nameLp     = "M_E";
  unit_database[unit_pos].nameFs     = "earth_mass";
  unit_database[unit_pos].nameFp     = "earth_masses";
  unit_database[unit_pos].quantity   = "mass";
  unit_database[unit_pos].multiplier = 5.9742e24;
  unit_database[unit_pos].exponent[UNIT_MASS]=1;
  unit_pos++;

  unit_database[unit_pos].nameAs     = "Mjupiter"; // Jupiter mass
  unit_database[unit_pos].nameAp     = "Mjupiter";
  unit_database[unit_pos].nameLs     = "M_J";
  unit_database[unit_pos].nameLp     = "M_J";
  unit_database[unit_pos].nameFs     = "jupiter_mass";
  unit_database[unit_pos].nameFp     = "jupiter_masses";
  unit_database[unit_pos].alt1       = "Mjove";
  unit_database[unit_pos].alt2       = "Mjovian";
  unit_database[unit_pos].quantity   = "mass";
  unit_database[unit_pos].multiplier = 1.8986e27;
  unit_database[unit_pos].exponent[UNIT_MASS]=1;
  unit_pos++;

  unit_database[unit_pos].nameAs     = "A"; // Ampere
  unit_database[unit_pos].nameAp     = "A";
  unit_database[unit_pos].nameLs     = unit_database[unit_pos].nameAs;
  unit_database[unit_pos].nameLp     = unit_database[unit_pos].nameAp;
  unit_database[unit_pos].nameFs     = "ampere";
  unit_database[unit_pos].nameFp     = "amperes";
  unit_database[unit_pos].alt1       = "amp";
  unit_database[unit_pos].alt2       = "amps";
  unit_database[unit_pos].quantity   = "current";
  unit_database[unit_pos].minPrefix  = -24;
  unit_database[unit_pos].maxPrefix  =  24;
  unit_database[unit_pos].si = unit_database[unit_pos].cgs = unit_database[unit_pos].imperial = unit_database[unit_pos].us = unit_database[unit_pos].ancient    = 1;
  unit_database[unit_pos].exponent[UNIT_CURRENT]=1;
  unit_pos++;

  unit_database[unit_pos].nameAs     = "K"; // Kelvin
  unit_database[unit_pos].nameAp     = "K";
  unit_database[unit_pos].nameLs     = unit_database[unit_pos].nameAs;
  unit_database[unit_pos].nameLp     = unit_database[unit_pos].nameAp;
  unit_database[unit_pos].nameFs     = "kelvin";
  unit_database[unit_pos].nameFp     = "kelvin";
  unit_database[unit_pos].quantity   = "temperature";
  unit_database[unit_pos].tempType   = 1;
  unit_database[unit_pos].minPrefix  = -24;
  unit_database[unit_pos].si         = 1;
  unit_database[unit_pos].cgs        = 1;
  unit_database[unit_pos].exponent[UNIT_TEMPERATURE]=1;
  context->tempTypeMultiplier[unit_database[unit_pos].tempType] = unit_database[unit_pos].multiplier;
  context->tempTypeOffset    [unit_database[unit_pos].tempType] = unit_database[unit_pos].offset;
  unit_pos++;

  unit_database[unit_pos].nameAs     = "R"; // Rankin
  unit_database[unit_pos].nameAp     = "R";
  unit_database[unit_pos].nameLs     = unit_database[unit_pos].nameAs;
  unit_database[unit_pos].nameLp     = unit_database[unit_pos].nameAp;
  unit_database[unit_pos].nameFs     = "rankin";
  unit_database[unit_pos].nameFp     = "rankin";
  unit_database[unit_pos].quantity   = "temperature";
  unit_database[unit_pos].tempType   = 2;
  unit_database[unit_pos].multiplier = 5.0/9.0;
  unit_database[unit_pos].exponent[UNIT_TEMPERATURE]=1;
  context->tempTypeMultiplier[unit_database[unit_pos].tempType] = unit_database[unit_pos].multiplier;
  context->tempTypeOffset    [unit_database[unit_pos].tempType] = unit_database[unit_pos].offset;
  unit_pos++;

  unit_database[unit_pos].nameAs     = "oC"; // oC
  unit_database[unit_pos].nameAp     = "oC";
  unit_database[unit_pos].nameLs     = "^\\circ C";
  unit_database[unit_pos].nameLp     = "^\\circ C";
  unit_database[unit_pos].nameFs     = "degree_celsius";
  unit_database[unit_pos].nameFp     = "degrees_celsius";
  unit_database[unit_pos].alt1       = "degree_centigrade";
  unit_database[unit_pos].alt2       = "degrees_centigrade";
  unit_database[unit_pos].alt3       = "centigrade";
  unit_database[unit_pos].alt4       = "celsius";
  unit_database[unit_pos].quantity   = "temperature";
  unit_database[unit_pos].tempType   = 3;
  unit_database[unit_pos].offset     = 273.15;
  unit_database[unit_pos].exponent[UNIT_TEMPERATURE]=1;
  context->tempTypeMultiplier[unit_database[unit_pos].tempType] = unit_database[unit_pos].multiplier;
  context->tempTypeOffset    [unit_database[unit_pos].tempType] = unit_database[unit_pos].offset;
  unit_pos++;

  unit_database[unit_pos].nameAs     = "oF"; // oF
  unit_database[unit_pos].nameAp     = "oF";
  unit_database[unit_pos].nameLs     = "^\\circ F";
  unit_database[unit_pos].nameLp     = "^\\circ F";
  unit_database[unit_pos].nameFs     = "degree_fahrenheit";
  unit_database[unit_pos].nameFp     = "degrees_fahrenheit";
  unit_database[unit_pos].alt1       = "fahrenheit";
  unit_database[unit_pos].quantity   = "temperature";
  unit_database[unit_pos].tempType   = 4;
  unit_database[unit_pos].multiplier = 5.0/9.0;
  unit_database[unit_pos].offset     = 459.67 * 5.0/9.0;
  unit_database[unit_pos].imperial = unit_database[unit_pos].us = unit_database[unit_pos].ancient    = 1;
  unit_database[unit_pos].exponent[UNIT_TEMPERATURE]=1;
  context->tempTypeMultiplier[unit_database[unit_pos].tempType] = unit_database[unit_pos].multiplier;
  context->tempTypeOffset    [unit_database[unit_pos].tempType] = unit_database[unit_pos].offset;
  unit_pos++;

  unit_database[unit_pos].nameAs     = "mol"; // mole
  unit_database[unit_pos].nameAp     = "mol";
  unit_database[unit_pos].nameLs     = unit_database[unit_pos].nameAs;
  unit_database[unit_pos].nameLp     = unit_database[unit_pos].nameAp;
  unit_database[unit_pos].nameFs     = "mole";
  unit_database[unit_pos].nameFp     = "moles";
  unit_database[unit_pos].quantity   = "moles";
  unit_database[unit_pos].multiplier = 1;
  unit_database[unit_pos].minPrefix  = -24;
  unit_database[unit_pos].maxPrefix  =  24;
  unit_database[unit_pos].si = unit_database[unit_pos].cgs = unit_database[unit_pos].imperial = unit_database[unit_pos].us = unit_database[unit_pos].ancient    = 1;
  unit_database[unit_pos].exponent[UNIT_MOLE]=1;
  unit_pos++;

  unit_database[unit_pos].nameAs     = "cd"; // candela
  unit_database[unit_pos].nameAp     = "cd";
  unit_database[unit_pos].nameLs     = unit_database[unit_pos].nameAs;
  unit_database[unit_pos].nameLp     = unit_database[unit_pos].nameAp;
  unit_database[unit_pos].nameFs     = "candela";
  unit_database[unit_pos].nameFp     = "candelas";
  unit_database[unit_pos].quantity   = "light_intensity";
  unit_database[unit_pos].multiplier = 1.0/683;
  unit_database[unit_pos].minPrefix  = -24;
  unit_database[unit_pos].maxPrefix  =  24;
  unit_database[unit_pos].si = 1;
  unit_database[unit_pos].exponent[UNIT_MASS]   = 1;
  unit_database[unit_pos].exponent[UNIT_LENGTH] = 2;
  unit_database[unit_pos].exponent[UNIT_TIME]   =-3;
  unit_database[unit_pos].exponent[UNIT_ANGLE]  =-2;
  unit_pos++;

  unit_database[unit_pos].nameAs     = "candlepower"; // candlepower
  unit_database[unit_pos].nameAp     = "candlepower";
  unit_database[unit_pos].nameLs     = unit_database[unit_pos].nameAs;
  unit_database[unit_pos].nameLp     = unit_database[unit_pos].nameAp;
  unit_database[unit_pos].nameFs     = "candlepower";
  unit_database[unit_pos].nameFp     = "candlepower";
  unit_database[unit_pos].quantity   = "light_intensity";
  unit_database[unit_pos].multiplier = 1.0/683/0.981;
  unit_database[unit_pos].imperial = unit_database[unit_pos].us = unit_database[unit_pos].ancient    = 1;
  unit_database[unit_pos].exponent[UNIT_MASS]   = 1;
  unit_database[unit_pos].exponent[UNIT_LENGTH] = 2;
  unit_database[unit_pos].exponent[UNIT_TIME]   =-3;
  unit_database[unit_pos].exponent[UNIT_ANGLE]  =-2;
  unit_pos++;

  unit_database[unit_pos].nameAs     = "lm"; // lumen
  unit_database[unit_pos].nameAp     = "lm";
  unit_database[unit_pos].nameLs     = unit_database[unit_pos].nameAs;
  unit_database[unit_pos].nameLp     = unit_database[unit_pos].nameAp;
  unit_database[unit_pos].nameFs     = "lumen";
  unit_database[unit_pos].nameFp     = "lumens";
  unit_database[unit_pos].quantity   = "power";
  unit_database[unit_pos].multiplier = GSL_CONST_MKSA_LUMEN;
  unit_database[unit_pos].minPrefix  = -24;
  unit_database[unit_pos].maxPrefix  =  24;
  unit_database[unit_pos].exponent[UNIT_MASS]   = 1;
  unit_database[unit_pos].exponent[UNIT_LENGTH] = 2;
  unit_database[unit_pos].exponent[UNIT_TIME]   =-3;
  unit_pos++;

  unit_database[unit_pos].nameAs     = "lx"; // lux
  unit_database[unit_pos].nameAp     = "lx";
  unit_database[unit_pos].nameLs     = unit_database[unit_pos].nameAs;
  unit_database[unit_pos].nameLp     = unit_database[unit_pos].nameAp;
  unit_database[unit_pos].nameFs     = "lux";
  unit_database[unit_pos].nameFp     = "luxs";
  unit_database[unit_pos].quantity   = "power";
  unit_database[unit_pos].multiplier = GSL_CONST_MKSA_LUX;
  unit_database[unit_pos].minPrefix  = -24;
  unit_database[unit_pos].maxPrefix  =  24;
  unit_database[unit_pos].exponent[UNIT_MASS]   = 1;
  unit_database[unit_pos].exponent[UNIT_TIME]   =-3;
  unit_pos++;

  unit_database[unit_pos].nameAs     = "Jy"; // jansky
  unit_database[unit_pos].nameAp     = "Jy";
  unit_database[unit_pos].nameLs     = unit_database[unit_pos].nameAs;
  unit_database[unit_pos].nameLp     = unit_database[unit_pos].nameAp;
  unit_database[unit_pos].nameFs     = "jansky";
  unit_database[unit_pos].nameFp     = "janskys";
  unit_database[unit_pos].quantity   = "flux_density";
  unit_database[unit_pos].multiplier = 1e-26;
  unit_database[unit_pos].minPrefix  = -24;
  unit_database[unit_pos].maxPrefix  =  24;
  unit_database[unit_pos].exponent[UNIT_MASS]   = 1; // Watt per square metre per Hz (NOT per steradian!)
  unit_database[unit_pos].exponent[UNIT_TIME]   =-2;
  unit_pos++;

  unit_database[unit_pos].nameAs     = "rad"; // radians
  unit_database[unit_pos].nameAp     = "rad";
  unit_database[unit_pos].nameLs     = unit_database[unit_pos].nameAs;
  unit_database[unit_pos].nameLp     = unit_database[unit_pos].nameAp;
  unit_database[unit_pos].nameFs     = "radian";
  unit_database[unit_pos].nameFp     = "radians";
  unit_database[unit_pos].quantity   = "angle";
  unit_database[unit_pos].multiplier = 1;
  unit_database[unit_pos].si = unit_database[unit_pos].cgs = 1;
  unit_database[unit_pos].exponent[UNIT_ANGLE]=1;
  unit_pos++;

  unit_database[unit_pos].nameAs     = "deg"; // degrees
  unit_database[unit_pos].nameAp     = "deg";
  unit_database[unit_pos].nameLs     = "^\\circ";
  unit_database[unit_pos].nameLp     = "^\\circ";
  unit_database[unit_pos].nameFs     = "degree";
  unit_database[unit_pos].nameFp     = "degrees";
  unit_database[unit_pos].quantity   = "angle";
  unit_database[unit_pos].multiplier = M_PI / 180;
  unit_database[unit_pos].imperial = unit_database[unit_pos].us = 1;
  unit_database[unit_pos].exponent[UNIT_ANGLE]=1;
  unit_pos++;

  unit_database[unit_pos].nameAs     = "rev"; // revolution
  unit_database[unit_pos].nameAp     = "rev";
  unit_database[unit_pos].nameLs     = unit_database[unit_pos].nameAs;
  unit_database[unit_pos].nameLp     = unit_database[unit_pos].nameAp;
  unit_database[unit_pos].nameFs     = "revolution";
  unit_database[unit_pos].nameFp     = "revolutions";
  unit_database[unit_pos].quantity   = "angle";
  unit_database[unit_pos].multiplier = 2 * M_PI;
  unit_database[unit_pos].ancient    = 1;
  unit_database[unit_pos].exponent[UNIT_ANGLE]=1;
  unit_pos++;

  unit_database[unit_pos].nameAs     = "arcmin"; // arcminute
  unit_database[unit_pos].nameAp     = "arcmins";
  unit_database[unit_pos].nameLs     = unit_database[unit_pos].nameAs;
  unit_database[unit_pos].nameLp     = unit_database[unit_pos].nameAp;
  unit_database[unit_pos].nameFs     = "arcminute";
  unit_database[unit_pos].nameFp     = "arcminutes";
  unit_database[unit_pos].quantity   = "angle";
  unit_database[unit_pos].multiplier = M_PI / 180 / 60;
  unit_database[unit_pos].exponent[UNIT_ANGLE]=1;
  unit_pos++;

  unit_database[unit_pos].nameAs     = "arcsec"; // arcsecond
  unit_database[unit_pos].nameAp     = "arcsecs";
  unit_database[unit_pos].nameLs     = unit_database[unit_pos].nameAs;
  unit_database[unit_pos].nameLp     = unit_database[unit_pos].nameAp;
  unit_database[unit_pos].nameFs     = "arcsecond";
  unit_database[unit_pos].nameFp     = "arcseconds";
  unit_database[unit_pos].quantity   = "angle";
  unit_database[unit_pos].multiplier = M_PI / 180 / 3600;
  unit_database[unit_pos].exponent[UNIT_ANGLE]=1;
  unit_pos++;

  unit_database[unit_pos].nameAs     = "bit"; // bit
  unit_database[unit_pos].nameAp     = "bits";
  unit_database[unit_pos].nameLs     = unit_database[unit_pos].nameAs;
  unit_database[unit_pos].nameLp     = unit_database[unit_pos].nameAp;
  unit_database[unit_pos].nameFs     = "bit";
  unit_database[unit_pos].nameFp     = "bits";
  unit_database[unit_pos].quantity   = "information_content";
  unit_database[unit_pos].multiplier = 1;
  unit_database[unit_pos].maxPrefix  =  24;
  unit_database[unit_pos].exponent[UNIT_BIT]=1;
  unit_pos++;

  unit_database[unit_pos].nameAs     = "euro"; // cost
  unit_database[unit_pos].nameAp     = "euros";
  unit_database[unit_pos].nameLs     = unit_database[unit_pos].nameAs;
  unit_database[unit_pos].nameLp     = unit_database[unit_pos].nameAp;
  unit_database[unit_pos].nameFs     = "euro";
  unit_database[unit_pos].nameFp     = "euros";
  unit_database[unit_pos].quantity   = "cost";
  unit_database[unit_pos].multiplier = 1;
  unit_database[unit_pos].maxPrefix  =  24;
  unit_database[unit_pos].si = unit_database[unit_pos].cgs = unit_database[unit_pos].imperial = unit_database[unit_pos].us = unit_database[unit_pos].ancient = 1;
  unit_database[unit_pos].exponent[UNIT_COST]=1;
  unit_pos++;


  // -------------
  // Derived units
  // -------------

  unit_database[unit_pos].nameAs     = "dioptre"; // dioptre
  unit_database[unit_pos].nameAp     = "dioptres";
  unit_database[unit_pos].nameLs     = unit_database[unit_pos].nameAs;
  unit_database[unit_pos].nameLp     = unit_database[unit_pos].nameAp;
  unit_database[unit_pos].nameFs     = "dioptre";
  unit_database[unit_pos].nameFp     = "dioptres";
  unit_database[unit_pos].quantity   = "lens_power";
  unit_database[unit_pos].multiplier = 1.0;
  unit_database[unit_pos].exponent[UNIT_LENGTH]=-1;
  unit_pos++;

  unit_database[unit_pos].nameAs     = "mph"; // mile_per_hour
  unit_database[unit_pos].nameAp     = "mph";
  unit_database[unit_pos].nameLs     = unit_database[unit_pos].nameAs;
  unit_database[unit_pos].nameLp     = unit_database[unit_pos].nameAp;
  unit_database[unit_pos].nameFs     = "mile_per_hour";
  unit_database[unit_pos].nameFp     = "miles_per_hour";
  unit_database[unit_pos].quantity   = "velocity";
  unit_database[unit_pos].multiplier = GSL_CONST_MKSA_MILE / 3600;
  unit_database[unit_pos].imperial = unit_database[unit_pos].us = 1;
  unit_database[unit_pos].exponent[UNIT_LENGTH]= 1;
  unit_database[unit_pos].exponent[UNIT_TIME]  =-1;
  unit_pos++;

  unit_database[unit_pos].nameAs     = "kn"; // knot
  unit_database[unit_pos].nameAp     = "kn";
  unit_database[unit_pos].nameLs     = unit_database[unit_pos].nameAs;
  unit_database[unit_pos].nameLp     = unit_database[unit_pos].nameAp;
  unit_database[unit_pos].nameFs     = "knot";
  unit_database[unit_pos].nameFp     = "knots";
  unit_database[unit_pos].quantity   = "velocity";
  unit_database[unit_pos].multiplier = GSL_CONST_MKSA_KNOT;
  unit_database[unit_pos].exponent[UNIT_LENGTH]= 1;
  unit_database[unit_pos].exponent[UNIT_TIME]  =-1;
  unit_pos++;

  unit_database[unit_pos].nameAs     = "acre"; // acre
  unit_database[unit_pos].nameAp     = "acres";
  unit_database[unit_pos].nameLs     = unit_database[unit_pos].nameAs;
  unit_database[unit_pos].nameLp     = unit_database[unit_pos].nameAp;
  unit_database[unit_pos].nameFs     = "acre";
  unit_database[unit_pos].nameFp     = "acres";
  unit_database[unit_pos].quantity   = "area";
  unit_database[unit_pos].multiplier = GSL_CONST_MKSA_ACRE;
  //unit_database[unit_pos].imperial = unit_database[unit_pos].us = 1;
  unit_database[unit_pos].exponent[UNIT_LENGTH]=2;
  unit_pos++;

  unit_database[unit_pos].nameAs     = "are"; // are
  unit_database[unit_pos].nameAp     = "ares";
  unit_database[unit_pos].nameLs     = unit_database[unit_pos].nameAs;
  unit_database[unit_pos].nameLp     = unit_database[unit_pos].nameAp;
  unit_database[unit_pos].nameFs     = "are";
  unit_database[unit_pos].nameFp     = "ares";
  unit_database[unit_pos].quantity   = "area";
  unit_database[unit_pos].multiplier = 100;
  unit_database[unit_pos].exponent[UNIT_LENGTH]=2;
  unit_pos++;

  unit_database[unit_pos].nameAs     = "barn"; // barn
  unit_database[unit_pos].nameAp     = "barns";
  unit_database[unit_pos].nameLs     = unit_database[unit_pos].nameAs;
  unit_database[unit_pos].nameLp     = unit_database[unit_pos].nameAp;
  unit_database[unit_pos].nameFs     = "barn";
  unit_database[unit_pos].nameFp     = "barns";
  unit_database[unit_pos].quantity   = "area";
  unit_database[unit_pos].multiplier = 1e-28;
  unit_database[unit_pos].exponent[UNIT_LENGTH]=2;
  unit_pos++;

  unit_database[unit_pos].nameAs     = "hectare"; // hectare
  unit_database[unit_pos].nameAp     = "hectares";
  unit_database[unit_pos].nameLs     = unit_database[unit_pos].nameAs;
  unit_database[unit_pos].nameLp     = unit_database[unit_pos].nameAp;
  unit_database[unit_pos].nameFs     = "hectare";
  unit_database[unit_pos].nameFp     = "hectares";
  unit_database[unit_pos].quantity   = "area";
  unit_database[unit_pos].multiplier = 1e4;
  unit_database[unit_pos].exponent[UNIT_LENGTH]=2;
  unit_pos++;

  unit_database[unit_pos].nameAs     = "sq_mi"; // square mile
  unit_database[unit_pos].nameAp     = "sq_mi";
  unit_database[unit_pos].nameLs     = "sq\\_mi";
  unit_database[unit_pos].nameLp     = "sq\\_mi";
  unit_database[unit_pos].nameFs     = "square_mile";
  unit_database[unit_pos].nameFp     = "square_miles";
  unit_database[unit_pos].quantity   = "area";
  unit_database[unit_pos].multiplier = pow(GSL_CONST_MKSA_MILE,2);
  //unit_database[unit_pos].imperial   = unit_database[unit_pos].us = 1;
  unit_database[unit_pos].exponent[UNIT_LENGTH]=2;
  unit_pos++;

  unit_database[unit_pos].nameAs     = "sq_km"; // square kilometre
  unit_database[unit_pos].nameAp     = "sq_km";
  unit_database[unit_pos].nameLs     = "sq\\_km";
  unit_database[unit_pos].nameLp     = "sq\\_km";
  unit_database[unit_pos].nameFs     = "square_kilometre";
  unit_database[unit_pos].nameFp     = "square_kilometres";
  unit_database[unit_pos].quantity   = "area";
  unit_database[unit_pos].multiplier = 1e6;
  //unit_database[unit_pos].si         = 1;
  unit_database[unit_pos].exponent[UNIT_LENGTH]=2;
  unit_pos++;

  unit_database[unit_pos].nameAs     = "sq_m"; // square metre
  unit_database[unit_pos].nameAp     = "sq_m";
  unit_database[unit_pos].nameLs     = "sq\\_m";
  unit_database[unit_pos].nameLp     = "sq\\_m";
  unit_database[unit_pos].nameFs     = "square_metre";
  unit_database[unit_pos].nameFp     = "square_metres";
  unit_database[unit_pos].quantity   = "area";
  unit_database[unit_pos].multiplier = 1;
  //unit_database[unit_pos].si         = 1;
  unit_database[unit_pos].exponent[UNIT_LENGTH]=2;
  unit_pos++;

  unit_database[unit_pos].nameAs     = "sq_cm"; // square centimetre
  unit_database[unit_pos].nameAp     = "sq_cm";
  unit_database[unit_pos].nameLs     = "sq\\_cm";
  unit_database[unit_pos].nameLp     = "sq\\_cm";
  unit_database[unit_pos].nameFs     = "square_centimetre";
  unit_database[unit_pos].nameFp     = "square_centimetres";
  unit_database[unit_pos].quantity   = "area";
  unit_database[unit_pos].multiplier = 1e-4;
  //unit_database[unit_pos].cgs        = 1;
  unit_database[unit_pos].exponent[UNIT_LENGTH]=2;
  unit_pos++;

  unit_database[unit_pos].nameAs     = "sq_ft"; // square foot
  unit_database[unit_pos].nameAp     = "sq_ft";
  unit_database[unit_pos].nameLs     = "sq\\_ft";
  unit_database[unit_pos].nameLp     = "sq\\_ft";
  unit_database[unit_pos].nameFs     = "square_foot";
  unit_database[unit_pos].nameFp     = "square_feet";
  unit_database[unit_pos].quantity   = "area";
  unit_database[unit_pos].multiplier = pow(GSL_CONST_MKSA_FOOT,2);
  //unit_database[unit_pos].imperial = unit_database[unit_pos].us = 1;
  unit_database[unit_pos].exponent[UNIT_LENGTH]=2;
  unit_pos++;

  unit_database[unit_pos].nameAs     = "sq_in"; // square inch
  unit_database[unit_pos].nameAp     = "sq_in";
  unit_database[unit_pos].nameLs     = "sq\\_in";
  unit_database[unit_pos].nameLp     = "sq\\_in";
  unit_database[unit_pos].nameFs     = "square_inch";
  unit_database[unit_pos].nameFp     = "square_inches";
  unit_database[unit_pos].quantity   = "area";
  unit_database[unit_pos].multiplier = pow(GSL_CONST_MKSA_INCH,2);
  //unit_database[unit_pos].imperial = unit_database[unit_pos].us = 1;
  unit_database[unit_pos].exponent[UNIT_LENGTH]=2;
  unit_pos++;

  unit_database[unit_pos].nameAs     = "cubic_m"; // cubic metre
  unit_database[unit_pos].nameAp     = "cubic_m";
  unit_database[unit_pos].nameLs     = "cubic\\_m";
  unit_database[unit_pos].nameLp     = "cubic\\_m";
  unit_database[unit_pos].nameFs     = "cubic_metre";
  unit_database[unit_pos].nameFp     = "cubic_metres";
  unit_database[unit_pos].quantity   = "volume";
  unit_database[unit_pos].multiplier = 1;
  //unit_database[unit_pos].si         = 1;
  unit_database[unit_pos].exponent[UNIT_LENGTH]=3;
  unit_pos++;

  unit_database[unit_pos].nameAs     = "cubic_cm"; // cubic centimetre
  unit_database[unit_pos].nameAp     = "cubic_cm";
  unit_database[unit_pos].nameLs     = "cubic\\_cm";
  unit_database[unit_pos].nameLp     = "cubic\\_cm";
  unit_database[unit_pos].nameFs     = "cubic_centimetre";
  unit_database[unit_pos].nameFp     = "cubic_centimetres";
  unit_database[unit_pos].quantity   = "volume";
  unit_database[unit_pos].multiplier = 1e-6;
  //unit_database[unit_pos].cgs        = 1;
  unit_database[unit_pos].exponent[UNIT_LENGTH]=3;
  unit_pos++;

  unit_database[unit_pos].nameAs     = "cubic_ft"; // cubic foot
  unit_database[unit_pos].nameAp     = "cubic_ft";
  unit_database[unit_pos].nameLs     = "cubic\\_ft";
  unit_database[unit_pos].nameLp     = "cubic\\_ft";
  unit_database[unit_pos].nameFs     = "cubic_foot";
  unit_database[unit_pos].nameFp     = "cubic_feet";
  unit_database[unit_pos].quantity   = "volume";
  unit_database[unit_pos].multiplier = pow(GSL_CONST_MKSA_FOOT,3);
  //unit_database[unit_pos].imperial = unit_database[unit_pos].us = 1;
  unit_database[unit_pos].exponent[UNIT_LENGTH]=3;
  unit_pos++;

  unit_database[unit_pos].nameAs     = "cubic_in"; // cubic inch
  unit_database[unit_pos].nameAp     = "cubic_in";
  unit_database[unit_pos].nameLs     = "cubic\\_in";
  unit_database[unit_pos].nameLp     = "cubic\\_in";
  unit_database[unit_pos].nameFs     = "cubic_inch";
  unit_database[unit_pos].nameFp     = "cubic_inches";
  unit_database[unit_pos].quantity   = "volume";
  unit_database[unit_pos].multiplier = pow(GSL_CONST_MKSA_INCH,3);
  //unit_database[unit_pos].imperial = unit_database[unit_pos].us = 1;
  unit_database[unit_pos].exponent[UNIT_LENGTH]=3;
  unit_pos++;

  unit_database[unit_pos].nameAs     = "l"; // litre
  unit_database[unit_pos].nameAp     = "l";
  unit_database[unit_pos].nameLs     = unit_database[unit_pos].nameAs;
  unit_database[unit_pos].nameLp     = unit_database[unit_pos].nameAp;
  unit_database[unit_pos].nameFs     = "litre";
  unit_database[unit_pos].nameFp     = "litres";
  unit_database[unit_pos].quantity   = "volume";
  unit_database[unit_pos].minPrefix  = -3;
  unit_database[unit_pos].multiplier = 1e-3;
  unit_database[unit_pos].exponent[UNIT_LENGTH]=3;
  unit_pos++;

  unit_database[unit_pos].nameAs     = "fl_oz_UK"; // UK fluid ounce
  unit_database[unit_pos].nameAp     = "fl_oz_UK";
  unit_database[unit_pos].nameLs     = "fl\\_oz\\_UK";
  unit_database[unit_pos].nameLp     = "fl\\_oz\\_UK";
  unit_database[unit_pos].nameFs     = "fluid_ounce_UK";
  unit_database[unit_pos].nameFp     = "fluid_ounce_UK";
  unit_database[unit_pos].comment    = "UK imperial";
  unit_database[unit_pos].quantity   = "volume";
  unit_database[unit_pos].multiplier = 28.4130625e-6;
  //unit_database[unit_pos].imperial   = 1;
  unit_database[unit_pos].exponent[UNIT_LENGTH]=3;
  unit_pos++;

  unit_database[unit_pos].nameAs     = "fl_oz_US"; // US fluid ounce
  unit_database[unit_pos].nameAp     = "fl_oz_US";
  unit_database[unit_pos].nameLs     = "fl\\_oz\\_US";
  unit_database[unit_pos].nameLp     = "fl\\_oz\\_US";
  unit_database[unit_pos].nameFs     = "fluid_ounce_US";
  unit_database[unit_pos].nameFp     = "fluid_ounce_US";
  unit_database[unit_pos].comment    = "US customary";
  unit_database[unit_pos].quantity   = "volume";
  unit_database[unit_pos].multiplier = GSL_CONST_MKSA_FLUID_OUNCE;
  //unit_database[unit_pos].us         = 1;
  unit_database[unit_pos].exponent[UNIT_LENGTH]=3;
  unit_pos++;

  unit_database[unit_pos].nameAs     = "pint_UK"; // UK pint
  unit_database[unit_pos].nameAp     = "pints_UK";
  unit_database[unit_pos].nameLs     = "pint\\_UK";
  unit_database[unit_pos].nameLp     = "pints\\_UK";
  unit_database[unit_pos].nameFs     = "pint_UK";
  unit_database[unit_pos].nameFp     = "pints_UK";
  unit_database[unit_pos].comment    = "UK imperial";
  unit_database[unit_pos].quantity   = "volume";
  unit_database[unit_pos].multiplier = 568.26125e-6;
  //unit_database[unit_pos].imperial   = 1;
  unit_database[unit_pos].exponent[UNIT_LENGTH]=3;
  unit_pos++;

  unit_database[unit_pos].nameAs     = "pint_US"; // US pint
  unit_database[unit_pos].nameAp     = "pints_US";
  unit_database[unit_pos].nameLs     = "pint\\_US";
  unit_database[unit_pos].nameLp     = "pints\\_US";
  unit_database[unit_pos].nameFs     = "pint_US";
  unit_database[unit_pos].nameFp     = "pints_US";
  unit_database[unit_pos].comment    = "US customary";
  unit_database[unit_pos].quantity   = "volume";
  unit_database[unit_pos].multiplier = GSL_CONST_MKSA_PINT;
  //unit_database[unit_pos].us         = 1;
  unit_database[unit_pos].exponent[UNIT_LENGTH]=3;
  unit_pos++;

  unit_database[unit_pos].nameAs     = "quart_UK"; // UK quart
  unit_database[unit_pos].nameAp     = "quarts_UK";
  unit_database[unit_pos].nameLs     = "quart\\_UK";
  unit_database[unit_pos].nameLp     = "quarts\\_UK";
  unit_database[unit_pos].nameFs     = "quart_UK";
  unit_database[unit_pos].nameFp     = "quarts_UK";
  unit_database[unit_pos].comment    = "UK imperial";
  unit_database[unit_pos].quantity   = "volume";
  unit_database[unit_pos].multiplier = 1136.5225e-6;
  //unit_database[unit_pos].imperial   = 1;
  unit_database[unit_pos].exponent[UNIT_LENGTH]=3;
  unit_pos++;

  unit_database[unit_pos].nameAs     = "quart_US"; // US quart
  unit_database[unit_pos].nameAp     = "quarts_US";
  unit_database[unit_pos].nameLs     = "quart\\_US";
  unit_database[unit_pos].nameLp     = "quarts\\_US";
  unit_database[unit_pos].nameFs     = "quart_US";
  unit_database[unit_pos].nameFp     = "quarts_US";
  unit_database[unit_pos].comment    = "US customary";
  unit_database[unit_pos].quantity   = "volume";
  unit_database[unit_pos].multiplier = GSL_CONST_MKSA_QUART;
  //unit_database[unit_pos].us         = 1;
  unit_database[unit_pos].exponent[UNIT_LENGTH]=3;
  unit_pos++;

  unit_database[unit_pos].nameAs     = "gallon_UK"; // UK gallon
  unit_database[unit_pos].nameAp     = "gallons_UK";
  unit_database[unit_pos].nameLs     = "gallon\\_UK";
  unit_database[unit_pos].nameLp     = "gallons\\_UK";
  unit_database[unit_pos].nameFs     = "gallon_UK";
  unit_database[unit_pos].nameFp     = "gallons_UK";
  unit_database[unit_pos].comment    = "UK imperial";
  unit_database[unit_pos].quantity   = "volume";
  unit_database[unit_pos].multiplier = GSL_CONST_MKSA_UK_GALLON;
  //unit_database[unit_pos].imperial   = 1;
  unit_database[unit_pos].exponent[UNIT_LENGTH]=3;
  unit_pos++;

  unit_database[unit_pos].nameAs     = "gallon_US"; // US gallon
  unit_database[unit_pos].nameAp     = "gallons_US";
  unit_database[unit_pos].nameLs     = "gallon\\_US";
  unit_database[unit_pos].nameLp     = "gallons\\_US";
  unit_database[unit_pos].nameFs     = "gallon_US";
  unit_database[unit_pos].nameFp     = "gallons_US";
  unit_database[unit_pos].comment    = "US customary";
  unit_database[unit_pos].quantity   = "volume";
  unit_database[unit_pos].multiplier = GSL_CONST_MKSA_US_GALLON;
  //unit_database[unit_pos].us         = 1;
  unit_database[unit_pos].exponent[UNIT_LENGTH]=3;
  unit_pos++;

  unit_database[unit_pos].nameAs     = "bushel_UK"; // UK bushel
  unit_database[unit_pos].nameAp     = "bushels_UK";
  unit_database[unit_pos].nameLs     = "bushel\\_UK";
  unit_database[unit_pos].nameLp     = "bushels\\_UK";
  unit_database[unit_pos].nameFs     = "bushel_UK";
  unit_database[unit_pos].nameFp     = "bushels_UK";
  unit_database[unit_pos].comment    = "UK imperial";
  unit_database[unit_pos].quantity   = "volume";
  unit_database[unit_pos].multiplier = 36.36872e-3;
  //unit_database[unit_pos].imperial   = 1;
  unit_database[unit_pos].exponent[UNIT_LENGTH]=3;
  unit_pos++;

  unit_database[unit_pos].nameAs     = "bushel_US"; // US bushel
  unit_database[unit_pos].nameAp     = "bushels_US";
  unit_database[unit_pos].nameLs     = "bushel\\_US";
  unit_database[unit_pos].nameLp     = "bushels\\_US";
  unit_database[unit_pos].nameFs     = "bushel_US";
  unit_database[unit_pos].nameFp     = "bushels_US";
  unit_database[unit_pos].comment    = "US customary";
  unit_database[unit_pos].quantity   = "volume";
  unit_database[unit_pos].multiplier = 35.23907016688e-3;
  //unit_database[unit_pos].us         = 1;
  unit_database[unit_pos].exponent[UNIT_LENGTH]=3;
  unit_pos++;

  unit_database[unit_pos].nameAs     = "cup_US"; // US cup
  unit_database[unit_pos].nameAp     = "cups_US";
  unit_database[unit_pos].nameLs     = "cup\\_US";
  unit_database[unit_pos].nameLp     = "cups\\_US";
  unit_database[unit_pos].nameFs     = "cup_US";
  unit_database[unit_pos].nameFp     = "cups_US";
  unit_database[unit_pos].comment    = "US customary";
  unit_database[unit_pos].quantity   = "volume";
  unit_database[unit_pos].multiplier = GSL_CONST_MKSA_CUP;
  //unit_database[unit_pos].us         = 1;
  unit_database[unit_pos].exponent[UNIT_LENGTH]=3;
  unit_pos++;

  unit_database[unit_pos].nameAs     = "bath"; // bath
  unit_database[unit_pos].nameAp     = "baths";
  unit_database[unit_pos].nameLs     = unit_database[unit_pos].nameAs;
  unit_database[unit_pos].nameLp     = unit_database[unit_pos].nameAp;
  unit_database[unit_pos].nameFs     = "bath";
  unit_database[unit_pos].nameFp     = "baths";
  unit_database[unit_pos].quantity   = "volume";
  unit_database[unit_pos].multiplier = 22e-3;
  unit_database[unit_pos].ancient    = 1;
  unit_database[unit_pos].exponent[UNIT_LENGTH]=3;
  unit_pos++;

  unit_database[unit_pos].nameAs     = "homer"; // homer
  unit_database[unit_pos].nameAp     = "homers";
  unit_database[unit_pos].nameLs     = unit_database[unit_pos].nameAs;
  unit_database[unit_pos].nameLp     = unit_database[unit_pos].nameAp;
  unit_database[unit_pos].nameFs     = "homer";
  unit_database[unit_pos].nameFp     = "homers";
  unit_database[unit_pos].quantity   = "volume";
  unit_database[unit_pos].multiplier = 220e-3;
  unit_database[unit_pos].ancient    = 1;
  unit_database[unit_pos].exponent[UNIT_LENGTH]=3;
  unit_pos++;

  unit_database[unit_pos].nameAs     = "teaspoon"; // teaspoon
  unit_database[unit_pos].nameAp     = "teaspoons";
  unit_database[unit_pos].nameLs     = unit_database[unit_pos].nameAs;
  unit_database[unit_pos].nameLp     = unit_database[unit_pos].nameAp;
  unit_database[unit_pos].nameFs     = "teaspoon";
  unit_database[unit_pos].nameFp     = "teaspoons";
  unit_database[unit_pos].quantity   = "volume";
  unit_database[unit_pos].multiplier = 5e-6; // 5 mL
  unit_database[unit_pos].exponent[UNIT_LENGTH]=3;
  unit_pos++;

  unit_database[unit_pos].nameAs     = "tablespoon"; // tablespoon
  unit_database[unit_pos].nameAp     = "tablespoons";
  unit_database[unit_pos].nameLs     = unit_database[unit_pos].nameAs;
  unit_database[unit_pos].nameLp     = unit_database[unit_pos].nameAp;
  unit_database[unit_pos].nameFs     = "tablespoon";
  unit_database[unit_pos].nameFp     = "tablespoons";
  unit_database[unit_pos].quantity   = "volume";
  unit_database[unit_pos].multiplier = 15e-6; // 15 mL
  unit_database[unit_pos].exponent[UNIT_LENGTH]=3;
  unit_pos++;

  unit_database[unit_pos].nameAs     = "firkin_UK_ale"; // firkin of ale
  unit_database[unit_pos].nameAp     = "firkins_UK_ale";
  unit_database[unit_pos].nameLs     = "firkin\\_UK\\_ale";
  unit_database[unit_pos].nameLp     = "firkins\\_UK\\_ale";
  unit_database[unit_pos].nameFs     = "firkin_UK_ale";
  unit_database[unit_pos].nameFp     = "firkins_UK_ale";
  unit_database[unit_pos].quantity   = "volume";
  unit_database[unit_pos].multiplier = 40.91481e-3;
  unit_database[unit_pos].exponent[UNIT_LENGTH]=3;
  unit_pos++;

  unit_database[unit_pos].nameAs     = "firkin_UK_wine"; // firkin of wine
  unit_database[unit_pos].nameAp     = "firkins_UK_wine";
  unit_database[unit_pos].nameLs     = "firkin\\_UK\\_wine";
  unit_database[unit_pos].nameLp     = "firkins\\_UK\\_wine";
  unit_database[unit_pos].nameFs     = "firkin_wine";
  unit_database[unit_pos].nameFp     = "firkins_wine";
  unit_database[unit_pos].quantity   = "volume";
  unit_database[unit_pos].multiplier = 318e-3;
  unit_database[unit_pos].exponent[UNIT_LENGTH]=3;
  unit_pos++;

  unit_database[unit_pos].nameAs     = "kilderkin_UK_ale"; // kilderkin of ale
  unit_database[unit_pos].nameAp     = "kilderkins_UK_ale";
  unit_database[unit_pos].nameLs     = "kilderkin\\_UK\\_ale";
  unit_database[unit_pos].nameLp     = "kilderkins\\_UK\\_ale";
  unit_database[unit_pos].nameFs     = "kilderkin_UK_ale";
  unit_database[unit_pos].nameFp     = "kilderkins_UK_ale";
  unit_database[unit_pos].quantity   = "volume";
  unit_database[unit_pos].multiplier = 81.82962e-3;
  unit_database[unit_pos].exponent[UNIT_LENGTH]=3;
  unit_pos++;

  unit_database[unit_pos].nameAs     = "sterad"; // steradians
  unit_database[unit_pos].nameAp     = "sterad";
  unit_database[unit_pos].nameLs     = unit_database[unit_pos].nameAs;
  unit_database[unit_pos].nameLp     = unit_database[unit_pos].nameAp;
  unit_database[unit_pos].nameFs     = "steradian";
  unit_database[unit_pos].nameFp     = "steradians";
  unit_database[unit_pos].quantity   = "solidangle";
  unit_database[unit_pos].multiplier = 1;
  unit_database[unit_pos].si = unit_database[unit_pos].cgs = 1;
  unit_database[unit_pos].exponent[UNIT_ANGLE]=2;
  unit_pos++;

  unit_database[unit_pos].nameAs     = "sqdeg"; // square degrees
  unit_database[unit_pos].nameAp     = "sqdeg";
  unit_database[unit_pos].nameLs     = unit_database[unit_pos].nameAs;
  unit_database[unit_pos].nameLp     = unit_database[unit_pos].nameAp;
  unit_database[unit_pos].nameFs     = "square_degree";
  unit_database[unit_pos].nameFp     = "square_degrees";
  unit_database[unit_pos].quantity   = "solidangle";
  unit_database[unit_pos].multiplier = pow(M_PI/180, 2);
  unit_database[unit_pos].imperial = unit_database[unit_pos].us = unit_database[unit_pos].ancient = 1;
  unit_database[unit_pos].exponent[UNIT_ANGLE]=2;
  unit_pos++;

  unit_database[unit_pos].nameAs     = "Hz"; // hertz
  unit_database[unit_pos].nameAp     = "Hz";
  unit_database[unit_pos].nameLs     = unit_database[unit_pos].nameAs;
  unit_database[unit_pos].nameLp     = unit_database[unit_pos].nameAp;
  unit_database[unit_pos].nameFs     = "hertz";
  unit_database[unit_pos].nameFp     = "hertz";
  unit_database[unit_pos].quantity   = "frequency";
  unit_database[unit_pos].multiplier = 1.0;
  unit_database[unit_pos].maxPrefix  =  24;
  unit_database[unit_pos].notToBeCompounded = 1;
  unit_database[unit_pos].si = unit_database[unit_pos].cgs = unit_database[unit_pos].imperial = unit_database[unit_pos].us = unit_database[unit_pos].ancient = 1;
  unit_database[unit_pos].exponent[UNIT_TIME]=-1;
  unit_pos++;

  unit_database[unit_pos].nameAs     = "Bq"; // becquerel
  unit_database[unit_pos].nameAp     = "Bq";
  unit_database[unit_pos].nameLs     = unit_database[unit_pos].nameAs;
  unit_database[unit_pos].nameLp     = unit_database[unit_pos].nameAp;
  unit_database[unit_pos].nameFs     = "becquerel";
  unit_database[unit_pos].nameFp     = "becquerel";
  unit_database[unit_pos].quantity   = "frequency";
  unit_database[unit_pos].multiplier = 1.0;
  unit_database[unit_pos].maxPrefix  =  24;
  unit_database[unit_pos].exponent[UNIT_TIME]=-1;
  unit_pos++;

  unit_database[unit_pos].nameAs     = "B"; // bytes
  unit_database[unit_pos].nameAp     = "B";
  unit_database[unit_pos].nameLs     = unit_database[unit_pos].nameAs;
  unit_database[unit_pos].nameLp     = unit_database[unit_pos].nameAp;
  unit_database[unit_pos].nameFs     = "byte";
  unit_database[unit_pos].nameFp     = "bytes";
  unit_database[unit_pos].quantity   = "information_content";
  unit_database[unit_pos].multiplier = 8.0;
  unit_database[unit_pos].maxPrefix  =  24;
  unit_database[unit_pos].si = unit_database[unit_pos].cgs = unit_database[unit_pos].imperial = unit_database[unit_pos].us = unit_database[unit_pos].ancient = 1;
  unit_database[unit_pos].exponent[UNIT_BIT]=1;
  unit_pos++;

  unit_database[unit_pos].nameAs     = "Kib"; // kibibits
  unit_database[unit_pos].nameAp     = "Kib";
  unit_database[unit_pos].nameLs     = unit_database[unit_pos].nameAs;
  unit_database[unit_pos].nameLp     = unit_database[unit_pos].nameAp;
  unit_database[unit_pos].nameFs     = "kibibit";
  unit_database[unit_pos].nameFp     = "kibibits";
  unit_database[unit_pos].quantity   = "information_content";
  unit_database[unit_pos].multiplier = 1024.0;
  unit_database[unit_pos].exponent[UNIT_BIT]=1;
  unit_pos++;

  unit_database[unit_pos].nameAs     = "KiB"; // kibibytes
  unit_database[unit_pos].nameAp     = "KiB";
  unit_database[unit_pos].nameLs     = unit_database[unit_pos].nameAs;
  unit_database[unit_pos].nameLp     = unit_database[unit_pos].nameAp;
  unit_database[unit_pos].nameFs     = "kibibyte";
  unit_database[unit_pos].nameFp     = "kibibytes";
  unit_database[unit_pos].quantity   = "information_content";
  unit_database[unit_pos].multiplier = 1024.0 * 8.0;
  unit_database[unit_pos].exponent[UNIT_BIT]=1;
  unit_pos++;

  unit_database[unit_pos].nameAs     = "Mib"; // mebibits
  unit_database[unit_pos].nameAp     = "Mib";
  unit_database[unit_pos].nameLs     = unit_database[unit_pos].nameAs;
  unit_database[unit_pos].nameLp     = unit_database[unit_pos].nameAp;
  unit_database[unit_pos].nameFs     = "mebibit";
  unit_database[unit_pos].nameFp     = "mebibits";
  unit_database[unit_pos].quantity   = "information_content";
  unit_database[unit_pos].multiplier = 1024.0 * 1024.0;
  unit_database[unit_pos].exponent[UNIT_BIT]=1;
  unit_pos++;

  unit_database[unit_pos].nameAs     = "MiB"; // mebibytes
  unit_database[unit_pos].nameAp     = "MiB";
  unit_database[unit_pos].nameLs     = unit_database[unit_pos].nameAs;
  unit_database[unit_pos].nameLp     = unit_database[unit_pos].nameAp;
  unit_database[unit_pos].nameFs     = "mebibyte";
  unit_database[unit_pos].nameFp     = "mebibytes";
  unit_database[unit_pos].quantity   = "information_content";
  unit_database[unit_pos].multiplier = 1024.0 * 1024.0 * 8.0;
  unit_database[unit_pos].exponent[UNIT_BIT]=1;
  unit_pos++;

  unit_database[unit_pos].nameAs     = "Gib"; // gibibits
  unit_database[unit_pos].nameAp     = "Gib";
  unit_database[unit_pos].nameLs     = unit_database[unit_pos].nameAs;
  unit_database[unit_pos].nameLp     = unit_database[unit_pos].nameAp;
  unit_database[unit_pos].nameFs     = "gibibit";
  unit_database[unit_pos].nameFp     = "gibibits";
  unit_database[unit_pos].quantity   = "information_content";
  unit_database[unit_pos].multiplier = 1024.0 * 1024.0 * 1024.0;
  unit_database[unit_pos].exponent[UNIT_BIT]=1;
  unit_pos++;

  unit_database[unit_pos].nameAs     = "GiB"; // gibibytes
  unit_database[unit_pos].nameAp     = "GiB";
  unit_database[unit_pos].nameLs     = unit_database[unit_pos].nameAs;
  unit_database[unit_pos].nameLp     = unit_database[unit_pos].nameAp;
  unit_database[unit_pos].nameFs     = "gibibyte";
  unit_database[unit_pos].nameFp     = "gibibytes";
  unit_database[unit_pos].quantity   = "information_content";
  unit_database[unit_pos].multiplier = 1024.0 * 1024.0 * 1024.0 * 8.0;
  unit_database[unit_pos].exponent[UNIT_BIT]=1;
  unit_pos++;

  unit_database[unit_pos].nameAs     = "N"; // newton
  unit_database[unit_pos].nameAp     = "N";
  unit_database[unit_pos].nameLs     = unit_database[unit_pos].nameAs;
  unit_database[unit_pos].nameLp     = unit_database[unit_pos].nameAp;
  unit_database[unit_pos].nameFs     = "newton";
  unit_database[unit_pos].nameFp     = "newtons";
  unit_database[unit_pos].quantity   = "force";
  unit_database[unit_pos].multiplier = 1;
  unit_database[unit_pos].minPrefix  = -24;
  unit_database[unit_pos].maxPrefix  =  24;
  unit_database[unit_pos].si = 1;
  unit_database[unit_pos].exponent[UNIT_MASS]   = 1;
  unit_database[unit_pos].exponent[UNIT_LENGTH] = 1;
  unit_database[unit_pos].exponent[UNIT_TIME]   =-2;
  unit_pos++;

  unit_database[unit_pos].nameAs     = "dyn"; // dyne
  unit_database[unit_pos].nameAp     = "dyn";
  unit_database[unit_pos].nameLs     = unit_database[unit_pos].nameAs;
  unit_database[unit_pos].nameLp     = unit_database[unit_pos].nameAp;
  unit_database[unit_pos].nameFs     = "dyne";
  unit_database[unit_pos].nameFp     = "dynes";
  unit_database[unit_pos].quantity   = "force";
  unit_database[unit_pos].multiplier = 1e-5;
  unit_database[unit_pos].cgs = 1;
  unit_database[unit_pos].exponent[UNIT_MASS]   = 1;
  unit_database[unit_pos].exponent[UNIT_LENGTH] = 1;
  unit_database[unit_pos].exponent[UNIT_TIME]   =-2;
  unit_pos++;

  unit_database[unit_pos].nameAs     = "lbf"; // pound force
  unit_database[unit_pos].nameAp     = "lbf";
  unit_database[unit_pos].nameLs     = unit_database[unit_pos].nameAs;
  unit_database[unit_pos].nameLp     = unit_database[unit_pos].nameAp;
  unit_database[unit_pos].nameFs     = "pound_force";
  unit_database[unit_pos].nameFp     = "pounds_force";
  unit_database[unit_pos].quantity   = "force";
  unit_database[unit_pos].multiplier = GSL_CONST_MKSA_POUND_FORCE;
  unit_database[unit_pos].imperial = unit_database[unit_pos].us = unit_database[unit_pos].ancient = 1;
  unit_database[unit_pos].exponent[UNIT_MASS]   = 1;
  unit_database[unit_pos].exponent[UNIT_LENGTH] = 1;
  unit_database[unit_pos].exponent[UNIT_TIME]   =-2;
  unit_pos++;

  unit_database[unit_pos].nameAs     = "Pa"; // pascal
  unit_database[unit_pos].nameAp     = "Pa";
  unit_database[unit_pos].nameLs     = unit_database[unit_pos].nameAs;
  unit_database[unit_pos].nameLp     = unit_database[unit_pos].nameAp;
  unit_database[unit_pos].nameFs     = "pascal";
  unit_database[unit_pos].nameFp     = "pascals";
  unit_database[unit_pos].quantity   = "pressure";
  unit_database[unit_pos].multiplier = 1;
  unit_database[unit_pos].minPrefix  = -24;
  unit_database[unit_pos].maxPrefix  =  24;
  unit_database[unit_pos].si = 1;
  unit_database[unit_pos].exponent[UNIT_MASS]   = 1;
  unit_database[unit_pos].exponent[UNIT_LENGTH] =-1;
  unit_database[unit_pos].exponent[UNIT_TIME]   =-2;
  unit_pos++;

  unit_database[unit_pos].nameAs     = "Ba"; // barye
  unit_database[unit_pos].nameAp     = "Ba";
  unit_database[unit_pos].nameLs     = unit_database[unit_pos].nameAs;
  unit_database[unit_pos].nameLp     = unit_database[unit_pos].nameAp;
  unit_database[unit_pos].nameFs     = "barye";
  unit_database[unit_pos].nameFp     = "baryes";
  unit_database[unit_pos].quantity   = "pressure";
  unit_database[unit_pos].multiplier = 0.1;
  unit_database[unit_pos].cgs = 1;
  unit_database[unit_pos].exponent[UNIT_MASS]   = 1;
  unit_database[unit_pos].exponent[UNIT_LENGTH] =-1;
  unit_database[unit_pos].exponent[UNIT_TIME]   =-2;
  unit_pos++;

  unit_database[unit_pos].nameAs     = "atm"; // atmosphere
  unit_database[unit_pos].nameAp     = "atms";
  unit_database[unit_pos].nameLs     = unit_database[unit_pos].nameAs;
  unit_database[unit_pos].nameLp     = unit_database[unit_pos].nameAp;
  unit_database[unit_pos].nameFs     = "atmosphere";
  unit_database[unit_pos].nameFp     = "atmospheres";
  unit_database[unit_pos].quantity   = "pressure";
  unit_database[unit_pos].multiplier = GSL_CONST_MKSA_STD_ATMOSPHERE;
  unit_database[unit_pos].minPrefix  = -24;
  unit_database[unit_pos].maxPrefix  =  24;
  unit_database[unit_pos].ancient    = 1;
  unit_database[unit_pos].exponent[UNIT_MASS]   = 1;
  unit_database[unit_pos].exponent[UNIT_LENGTH] =-1;
  unit_database[unit_pos].exponent[UNIT_TIME]   =-2;
  unit_pos++;

  unit_database[unit_pos].nameAs     = "bar"; // bar
  unit_database[unit_pos].nameAp     = "bars";
  unit_database[unit_pos].nameLs     = unit_database[unit_pos].nameAs;
  unit_database[unit_pos].nameLp     = unit_database[unit_pos].nameAp;
  unit_database[unit_pos].nameFs     = "bar";
  unit_database[unit_pos].nameFp     = "bars";
  unit_database[unit_pos].quantity   = "pressure";
  unit_database[unit_pos].multiplier = 1e5;
  unit_database[unit_pos].minPrefix  = -24;
  unit_database[unit_pos].exponent[UNIT_MASS]   = 1;
  unit_database[unit_pos].exponent[UNIT_LENGTH] =-1;
  unit_database[unit_pos].exponent[UNIT_TIME]   =-2;
  unit_pos++;

  unit_database[unit_pos].nameAs     = "psi"; // psi
  unit_database[unit_pos].nameAp     = "psi";
  unit_database[unit_pos].nameLs     = unit_database[unit_pos].nameAs;
  unit_database[unit_pos].nameLp     = unit_database[unit_pos].nameAp;
  unit_database[unit_pos].nameFs     = "pound_per_square_inch";
  unit_database[unit_pos].nameFp     = "pounds_per_square_inch";
  unit_database[unit_pos].quantity   = "pressure";
  unit_database[unit_pos].multiplier = GSL_CONST_MKSA_PSI;
  unit_database[unit_pos].imperial   = unit_database[unit_pos].us         = 1;
  unit_database[unit_pos].exponent[UNIT_MASS]   = 1;
  unit_database[unit_pos].exponent[UNIT_LENGTH] =-1;
  unit_database[unit_pos].exponent[UNIT_TIME]   =-2;
  unit_pos++;

  unit_database[unit_pos].nameAs     = "inHg"; // inch of mercury
  unit_database[unit_pos].nameAp     = "inHg";
  unit_database[unit_pos].nameLs     = unit_database[unit_pos].nameAs;
  unit_database[unit_pos].nameLp     = unit_database[unit_pos].nameAp;
  unit_database[unit_pos].nameFs     = "inch_of_mercury";
  unit_database[unit_pos].nameFp     = "inches_of_mercury";
  unit_database[unit_pos].quantity   = "pressure";
  unit_database[unit_pos].multiplier = GSL_CONST_MKSA_INCH_OF_MERCURY;
  unit_database[unit_pos].exponent[UNIT_MASS]   = 1;
  unit_database[unit_pos].exponent[UNIT_LENGTH] =-1;
  unit_database[unit_pos].exponent[UNIT_TIME]   =-2;
  unit_pos++;

  unit_database[unit_pos].nameAs     = "inAq"; // inch of water
  unit_database[unit_pos].nameAp     = "inAq";
  unit_database[unit_pos].nameLs     = unit_database[unit_pos].nameAs;
  unit_database[unit_pos].nameLp     = unit_database[unit_pos].nameAp;
  unit_database[unit_pos].nameFs     = "inch_of_water";
  unit_database[unit_pos].nameFp     = "inches_of_water";
  unit_database[unit_pos].quantity   = "pressure";
  unit_database[unit_pos].multiplier = GSL_CONST_MKSA_INCH_OF_WATER;
  unit_database[unit_pos].exponent[UNIT_MASS]   = 1;
  unit_database[unit_pos].exponent[UNIT_LENGTH] =-1;
  unit_database[unit_pos].exponent[UNIT_TIME]   =-2;
  unit_pos++;

  unit_database[unit_pos].nameAs     = "J"; // joule
  unit_database[unit_pos].nameAp     = "J";
  unit_database[unit_pos].nameLs     = unit_database[unit_pos].nameAs;
  unit_database[unit_pos].nameLp     = unit_database[unit_pos].nameAp;
  unit_database[unit_pos].nameFs     = "joule";
  unit_database[unit_pos].nameFp     = "joules";
  unit_database[unit_pos].quantity   = "energy";
  unit_database[unit_pos].multiplier = 1;
  unit_database[unit_pos].minPrefix  = -24;
  unit_database[unit_pos].maxPrefix  =  24;
  unit_database[unit_pos].si = 1;
  unit_database[unit_pos].exponent[UNIT_MASS]   = 1;
  unit_database[unit_pos].exponent[UNIT_LENGTH] = 2;
  unit_database[unit_pos].exponent[UNIT_TIME]   =-2;
  unit_pos++;

  unit_database[unit_pos].nameAs     = "eV"; // electronvolt
  unit_database[unit_pos].nameAp     = "eV";
  unit_database[unit_pos].nameLs     = unit_database[unit_pos].nameAs;
  unit_database[unit_pos].nameLp     = unit_database[unit_pos].nameAp;
  unit_database[unit_pos].nameFs     = "electronvolt";
  unit_database[unit_pos].nameFp     = "electronvolts";
  unit_database[unit_pos].quantity   = "energy";
  unit_database[unit_pos].multiplier = GSL_CONST_MKSA_ELECTRON_VOLT;
  unit_database[unit_pos].minPrefix  = -24;
  unit_database[unit_pos].maxPrefix  =  24;
  unit_database[unit_pos].exponent[UNIT_MASS]   = 1;
  unit_database[unit_pos].exponent[UNIT_LENGTH] = 2;
  unit_database[unit_pos].exponent[UNIT_TIME]   =-2;
  unit_pos++;

  unit_database[unit_pos].nameAs     = "BeV"; // billion electronvolts
  unit_database[unit_pos].nameAp     = "BeV";
  unit_database[unit_pos].nameLs     = unit_database[unit_pos].nameAs;
  unit_database[unit_pos].nameLp     = unit_database[unit_pos].nameAp;
  unit_database[unit_pos].nameFs     = "billion_electronvolts";
  unit_database[unit_pos].nameFp     = "billion_electronvolts";
  unit_database[unit_pos].quantity   = "energy";
  unit_database[unit_pos].multiplier = 1e9 * GSL_CONST_MKSA_ELECTRON_VOLT;
  unit_database[unit_pos].exponent[UNIT_MASS]   = 1;
  unit_database[unit_pos].exponent[UNIT_LENGTH] = 2;
  unit_database[unit_pos].exponent[UNIT_TIME]   =-2;
  unit_pos++;

  unit_database[unit_pos].nameAs     = "erg"; // erg
  unit_database[unit_pos].nameAp     = "erg";
  unit_database[unit_pos].nameLs     = unit_database[unit_pos].nameAs;
  unit_database[unit_pos].nameLp     = unit_database[unit_pos].nameAp;
  unit_database[unit_pos].nameFs     = "erg";
  unit_database[unit_pos].nameFp     = "ergs";
  unit_database[unit_pos].quantity   = "energy";
  unit_database[unit_pos].multiplier = 1e-7;
  unit_database[unit_pos].cgs = 1;
  unit_database[unit_pos].exponent[UNIT_MASS]   = 1;
  unit_database[unit_pos].exponent[UNIT_LENGTH] = 2;
  unit_database[unit_pos].exponent[UNIT_TIME]   =-2;
  unit_pos++;

  unit_database[unit_pos].nameAs     = "cal"; // calorie
  unit_database[unit_pos].nameAp     = "cal";
  unit_database[unit_pos].nameLs     = unit_database[unit_pos].nameAs;
  unit_database[unit_pos].nameLp     = unit_database[unit_pos].nameAp;
  unit_database[unit_pos].nameFs     = "calorie";
  unit_database[unit_pos].nameFp     = "calories";
  unit_database[unit_pos].quantity   = "energy";
  unit_database[unit_pos].multiplier = GSL_CONST_MKSA_CALORIE;
  unit_database[unit_pos].maxPrefix  =  3;
  unit_database[unit_pos].imperial = unit_database[unit_pos].us = unit_database[unit_pos].ancient = 1;
  unit_database[unit_pos].exponent[UNIT_MASS]   = 1;
  unit_database[unit_pos].exponent[UNIT_LENGTH] = 2;
  unit_database[unit_pos].exponent[UNIT_TIME]   =-2;
  unit_pos++;

  unit_database[unit_pos].nameAs     = "kWh"; // Kilowatt hour
  unit_database[unit_pos].nameAp     = "kWh";
  unit_database[unit_pos].nameLs     = unit_database[unit_pos].nameAs;
  unit_database[unit_pos].nameLp     = unit_database[unit_pos].nameAp;
  unit_database[unit_pos].nameFs     = "kilowatt_hour";
  unit_database[unit_pos].nameFp     = "kilowatt_hours";
  unit_database[unit_pos].quantity   = "energy";
  unit_database[unit_pos].multiplier = 3.6e6;
  unit_database[unit_pos].exponent[UNIT_MASS]   = 1;
  unit_database[unit_pos].exponent[UNIT_LENGTH] = 2;
  unit_database[unit_pos].exponent[UNIT_TIME]   =-2;
  unit_pos++;

  unit_database[unit_pos].nameAs     = "BTU"; // British Thermal Unit
  unit_database[unit_pos].nameAp     = "BTU";
  unit_database[unit_pos].nameLs     = unit_database[unit_pos].nameAs;
  unit_database[unit_pos].nameLp     = unit_database[unit_pos].nameAp;
  unit_database[unit_pos].nameFs     = "British Thermal Unit";
  unit_database[unit_pos].nameFp     = "British Thermal Units";
  unit_database[unit_pos].quantity   = "energy";
  unit_database[unit_pos].multiplier = GSL_CONST_MKSA_BTU;
  unit_database[unit_pos].exponent[UNIT_MASS]   = 1;
  unit_database[unit_pos].exponent[UNIT_LENGTH] = 2;
  unit_database[unit_pos].exponent[UNIT_TIME]   =-2;
  unit_pos++;

  unit_database[unit_pos].nameAs     = "therm"; // Therm
  unit_database[unit_pos].nameAp     = "therms";
  unit_database[unit_pos].nameLs     = unit_database[unit_pos].nameAs;
  unit_database[unit_pos].nameLp     = unit_database[unit_pos].nameAp;
  unit_database[unit_pos].nameFs     = "therm";
  unit_database[unit_pos].nameFp     = "therms";
  unit_database[unit_pos].quantity   = "energy";
  unit_database[unit_pos].multiplier = GSL_CONST_MKSA_THERM;
  unit_database[unit_pos].exponent[UNIT_MASS]   = 1;
  unit_database[unit_pos].exponent[UNIT_LENGTH] = 2;
  unit_database[unit_pos].exponent[UNIT_TIME]   =-2;
  unit_pos++;

  unit_database[unit_pos].nameAs     = "W"; // watt
  unit_database[unit_pos].nameAp     = "W";
  unit_database[unit_pos].nameLs     = unit_database[unit_pos].nameAs;
  unit_database[unit_pos].nameLp     = unit_database[unit_pos].nameAp;
  unit_database[unit_pos].nameFs     = "watt";
  unit_database[unit_pos].nameFp     = "watts";
  unit_database[unit_pos].quantity   = "power";
  unit_database[unit_pos].multiplier = 1;
  unit_database[unit_pos].minPrefix  = -24;
  unit_database[unit_pos].maxPrefix  =  24;
  unit_database[unit_pos].si = 1;
  unit_database[unit_pos].exponent[UNIT_MASS]   = 1;
  unit_database[unit_pos].exponent[UNIT_LENGTH] = 2;
  unit_database[unit_pos].exponent[UNIT_TIME]   =-3;
  unit_pos++;

  unit_database[unit_pos].nameAs     = "horsepower"; // horsepower
  unit_database[unit_pos].nameAp     = "horsepower";
  unit_database[unit_pos].nameLs     = unit_database[unit_pos].nameAs;
  unit_database[unit_pos].nameLp     = unit_database[unit_pos].nameAp;
  unit_database[unit_pos].nameFs     = "horsepower";
  unit_database[unit_pos].nameFp     = "horsepower";
  unit_database[unit_pos].quantity   = "power";
  unit_database[unit_pos].multiplier = GSL_CONST_MKSA_HORSEPOWER;
  unit_database[unit_pos].imperial = unit_database[unit_pos].us = unit_database[unit_pos].ancient = 1;
  unit_database[unit_pos].exponent[UNIT_MASS]   = 1;
  unit_database[unit_pos].exponent[UNIT_LENGTH] = 2;
  unit_database[unit_pos].exponent[UNIT_TIME]   =-3;
  unit_pos++;

  unit_database[unit_pos].nameAs     = "Lsun"; // Solar luminosity
  unit_database[unit_pos].nameAp     = "Lsun";
  unit_database[unit_pos].nameLs     = "L_\\odot";
  unit_database[unit_pos].nameLp     = "L_\\odot";
  unit_database[unit_pos].nameFs     = "solar_luminosity";
  unit_database[unit_pos].nameFp     = "solar_luminosities";
  unit_database[unit_pos].alt1       = "Lsolar";
  unit_database[unit_pos].quantity   = "power";
  unit_database[unit_pos].multiplier = 3.839e26;
  unit_database[unit_pos].exponent[UNIT_MASS]   = 1;
  unit_database[unit_pos].exponent[UNIT_LENGTH] = 2;
  unit_database[unit_pos].exponent[UNIT_TIME]   =-3;
  unit_pos++;

  unit_database[unit_pos].nameAs     = "clo"; // clo
  unit_database[unit_pos].nameAp     = "clos";
  unit_database[unit_pos].nameLs     = unit_database[unit_pos].nameAs;
  unit_database[unit_pos].nameLp     = unit_database[unit_pos].nameAp;
  unit_database[unit_pos].nameFs     = "clo";
  unit_database[unit_pos].nameFp     = "clos";
  unit_database[unit_pos].quantity   = "thermal_insulation";
  unit_database[unit_pos].multiplier = 0.154;
  unit_database[unit_pos].tempType   = 1;
  unit_database[unit_pos].exponent[UNIT_MASS]        =-1;
  unit_database[unit_pos].exponent[UNIT_TEMPERATURE] = 1;
  unit_database[unit_pos].exponent[UNIT_TIME]        = 3;
  unit_pos++;

  unit_database[unit_pos].nameAs     = "tog"; // tog
  unit_database[unit_pos].nameAp     = "togs";
  unit_database[unit_pos].nameLs     = unit_database[unit_pos].nameAs;
  unit_database[unit_pos].nameLp     = unit_database[unit_pos].nameAp;
  unit_database[unit_pos].nameFs     = "tog";
  unit_database[unit_pos].nameFp     = "togs";
  unit_database[unit_pos].quantity   = "thermal_insulation";
  unit_database[unit_pos].multiplier = 0.1;
  unit_database[unit_pos].tempType   = 1;
  unit_database[unit_pos].exponent[UNIT_MASS]        =-1;
  unit_database[unit_pos].exponent[UNIT_TEMPERATURE] = 1;
  unit_database[unit_pos].exponent[UNIT_TIME]        = 3;
  unit_pos++;

  unit_database[unit_pos].nameAs     = "Gy"; // gray
  unit_database[unit_pos].nameAp     = "Gy";
  unit_database[unit_pos].nameLs     = unit_database[unit_pos].nameAs;
  unit_database[unit_pos].nameLp     = unit_database[unit_pos].nameAp;
  unit_database[unit_pos].nameFs     = "gray";
  unit_database[unit_pos].nameFp     = "gray";
  unit_database[unit_pos].quantity   = "radiation_dose";
  unit_database[unit_pos].multiplier = 1.0;
  unit_database[unit_pos].exponent[UNIT_LENGTH] = 2;
  unit_database[unit_pos].exponent[UNIT_TIME]   =-2;
  unit_pos++;

  unit_database[unit_pos].nameAs     = "Sv"; // sievert
  unit_database[unit_pos].nameAp     = "Sv";
  unit_database[unit_pos].nameLs     = unit_database[unit_pos].nameAs;
  unit_database[unit_pos].nameLp     = unit_database[unit_pos].nameAp;
  unit_database[unit_pos].nameFs     = "sievert";
  unit_database[unit_pos].nameFp     = "sieverts";
  unit_database[unit_pos].quantity   = "radiation_dose";
  unit_database[unit_pos].multiplier = 1.0;
  unit_database[unit_pos].exponent[UNIT_LENGTH] = 2;
  unit_database[unit_pos].exponent[UNIT_TIME]   =-2;
  unit_pos++;

  unit_database[unit_pos].nameAs     = "kat"; // katal
  unit_database[unit_pos].nameAp     = "kat";
  unit_database[unit_pos].nameLs     = unit_database[unit_pos].nameAs;
  unit_database[unit_pos].nameLp     = unit_database[unit_pos].nameAp;
  unit_database[unit_pos].nameFs     = "katal";
  unit_database[unit_pos].nameFp     = "katals";
  unit_database[unit_pos].quantity   = "catalytic_activity";
  unit_database[unit_pos].multiplier = 1.0;
  unit_database[unit_pos].si = unit_database[unit_pos].cgs = unit_database[unit_pos].imperial = unit_database[unit_pos].us = unit_database[unit_pos].ancient = 1;
  unit_database[unit_pos].exponent[UNIT_MOLE] = 1;
  unit_database[unit_pos].exponent[UNIT_TIME] =-1;
  unit_pos++;

  unit_database[unit_pos].nameAs     = "P"; // poise
  unit_database[unit_pos].nameAp     = "P";
  unit_database[unit_pos].nameLs     = unit_database[unit_pos].nameAs;
  unit_database[unit_pos].nameLp     = unit_database[unit_pos].nameAp;
  unit_database[unit_pos].nameFs     = "poise";
  unit_database[unit_pos].nameFp     = "poises";
  unit_database[unit_pos].quantity   = "viscosity";
  unit_database[unit_pos].multiplier = GSL_CONST_MKSA_POISE;
  unit_database[unit_pos].cgs = 1;
  unit_database[unit_pos].exponent[UNIT_MASS]   = 1;
  unit_database[unit_pos].exponent[UNIT_LENGTH] =-1;
  unit_database[unit_pos].exponent[UNIT_TIME]   =-1;
  unit_pos++;

  unit_database[unit_pos].nameAs     = "kayser"; // kayser
  unit_database[unit_pos].nameAp     = "kaysers";
  unit_database[unit_pos].nameLs     = unit_database[unit_pos].nameAs;
  unit_database[unit_pos].nameLp     = unit_database[unit_pos].nameAp;
  unit_database[unit_pos].nameFs     = "kayser";
  unit_database[unit_pos].nameFp     = "kaysers";
  unit_database[unit_pos].quantity   = "wavenumber";
  unit_database[unit_pos].multiplier = 100;
  unit_database[unit_pos].cgs = 1;
  unit_database[unit_pos].exponent[UNIT_LENGTH] =-1;
  unit_pos++;

  unit_database[unit_pos].nameAs     = "C"; // coulomb
  unit_database[unit_pos].nameAp     = "C";
  unit_database[unit_pos].nameLs     = unit_database[unit_pos].nameAs;
  unit_database[unit_pos].nameLp     = unit_database[unit_pos].nameAp;
  unit_database[unit_pos].nameFs     = "coulomb";
  unit_database[unit_pos].nameFp     = "coulombs";
  unit_database[unit_pos].quantity   = "charge";
  unit_database[unit_pos].multiplier = 1.0;
  unit_database[unit_pos].minPrefix  = -24;
  unit_database[unit_pos].maxPrefix  =  24;
  unit_database[unit_pos].si = unit_database[unit_pos].cgs = unit_database[unit_pos].imperial = unit_database[unit_pos].us = unit_database[unit_pos].ancient = 1;
  unit_database[unit_pos].exponent[UNIT_TIME]    = 1;
  unit_database[unit_pos].exponent[UNIT_CURRENT] = 1;
  unit_pos++;

  unit_database[unit_pos].nameAs     = "V"; // volt
  unit_database[unit_pos].nameAp     = "V";
  unit_database[unit_pos].nameLs     = unit_database[unit_pos].nameAs;
  unit_database[unit_pos].nameLp     = unit_database[unit_pos].nameAp;
  unit_database[unit_pos].nameFs     = "volt";
  unit_database[unit_pos].nameFp     = "volts";
  unit_database[unit_pos].quantity   = "potential";
  unit_database[unit_pos].multiplier = 1.0;
  unit_database[unit_pos].minPrefix  = -24;
  unit_database[unit_pos].maxPrefix  =  24;
  unit_database[unit_pos].si = unit_database[unit_pos].cgs = unit_database[unit_pos].imperial = unit_database[unit_pos].us = unit_database[unit_pos].ancient = 1;
  unit_database[unit_pos].exponent[UNIT_MASS]    = 1;
  unit_database[unit_pos].exponent[UNIT_LENGTH]  = 2;
  unit_database[unit_pos].exponent[UNIT_TIME]    =-3;
  unit_database[unit_pos].exponent[UNIT_CURRENT] =-1;
  unit_pos++;

  unit_database[unit_pos].nameAs     = "ohm"; // ohm
  unit_database[unit_pos].nameAp     = "ohms";
  unit_database[unit_pos].nameLs     = "\\Omega";
  unit_database[unit_pos].nameLp     = "\\Omega";
  unit_database[unit_pos].nameFs     = "ohm";
  unit_database[unit_pos].nameFp     = "ohms";
  unit_database[unit_pos].quantity   = "resistance";
  unit_database[unit_pos].multiplier = 1.0;
  unit_database[unit_pos].minPrefix  = -24;
  unit_database[unit_pos].maxPrefix  =  24;
  unit_database[unit_pos].si = unit_database[unit_pos].cgs = unit_database[unit_pos].imperial = unit_database[unit_pos].us = unit_database[unit_pos].ancient = 1;
  unit_database[unit_pos].exponent[UNIT_MASS]    = 1;
  unit_database[unit_pos].exponent[UNIT_LENGTH]  = 2;
  unit_database[unit_pos].exponent[UNIT_TIME]    =-3;
  unit_database[unit_pos].exponent[UNIT_CURRENT] =-2;
  unit_pos++;

  unit_database[unit_pos].nameAs     = "S"; // siemens
  unit_database[unit_pos].nameAp     = "S";
  unit_database[unit_pos].nameLs     = unit_database[unit_pos].nameAs;
  unit_database[unit_pos].nameLp     = unit_database[unit_pos].nameAp;
  unit_database[unit_pos].nameFs     = "siemens";
  unit_database[unit_pos].nameFp     = "siemens";
  unit_database[unit_pos].quantity   = "conductance";
  unit_database[unit_pos].multiplier = 1.0;
  unit_database[unit_pos].minPrefix  = -24;
  unit_database[unit_pos].maxPrefix  =  24;
  unit_database[unit_pos].si = unit_database[unit_pos].cgs = unit_database[unit_pos].imperial = unit_database[unit_pos].us = unit_database[unit_pos].ancient = 1;
  unit_database[unit_pos].exponent[UNIT_MASS]    =-1;
  unit_database[unit_pos].exponent[UNIT_LENGTH]  =-2;
  unit_database[unit_pos].exponent[UNIT_TIME]    = 3;
  unit_database[unit_pos].exponent[UNIT_CURRENT] = 2;
  unit_pos++;

  unit_database[unit_pos].nameAs     = "mho"; // mho
  unit_database[unit_pos].nameAp     = "mhos";
  unit_database[unit_pos].nameLs     = "\\mho";
  unit_database[unit_pos].nameLp     = "\\mho";
  unit_database[unit_pos].nameFs     = "mho";
  unit_database[unit_pos].nameFp     = "mhos";
  unit_database[unit_pos].quantity   = "conductance";
  unit_database[unit_pos].multiplier = 1.0;
  unit_database[unit_pos].minPrefix  = -24;
  unit_database[unit_pos].maxPrefix  =  24;
  unit_database[unit_pos].exponent[UNIT_MASS]    =-1;
  unit_database[unit_pos].exponent[UNIT_LENGTH]  =-2;
  unit_database[unit_pos].exponent[UNIT_TIME]    = 3;
  unit_database[unit_pos].exponent[UNIT_CURRENT] = 2;
  unit_pos++;

  unit_database[unit_pos].nameAs     = "F"; // farad
  unit_database[unit_pos].nameAp     = "F";
  unit_database[unit_pos].nameLs     = unit_database[unit_pos].nameAs;
  unit_database[unit_pos].nameLp     = unit_database[unit_pos].nameAp;
  unit_database[unit_pos].nameFs     = "farad";
  unit_database[unit_pos].nameFp     = "farad";
  unit_database[unit_pos].quantity   = "capacitance";
  unit_database[unit_pos].multiplier = 1.0;
  unit_database[unit_pos].minPrefix  = -24;
  unit_database[unit_pos].maxPrefix  =  24;
  unit_database[unit_pos].si = unit_database[unit_pos].cgs = unit_database[unit_pos].imperial = unit_database[unit_pos].us = unit_database[unit_pos].ancient = 1;
  unit_database[unit_pos].exponent[UNIT_LENGTH]  =-2;
  unit_database[unit_pos].exponent[UNIT_MASS]    =-1;
  unit_database[unit_pos].exponent[UNIT_TIME]    = 4;
  unit_database[unit_pos].exponent[UNIT_CURRENT] = 2;
  unit_pos++;

  unit_database[unit_pos].nameAs     = "H"; // henry
  unit_database[unit_pos].nameAp     = "H";
  unit_database[unit_pos].nameLs     = unit_database[unit_pos].nameAs;
  unit_database[unit_pos].nameLp     = unit_database[unit_pos].nameAp;
  unit_database[unit_pos].nameFs     = "henry";
  unit_database[unit_pos].nameFp     = "henry";
  unit_database[unit_pos].quantity   = "inductance";
  unit_database[unit_pos].multiplier = 1.0;
  unit_database[unit_pos].minPrefix  = -24;
  unit_database[unit_pos].maxPrefix  =  24;
  unit_database[unit_pos].si = unit_database[unit_pos].cgs = unit_database[unit_pos].imperial = unit_database[unit_pos].us = unit_database[unit_pos].ancient = 1;
  unit_database[unit_pos].exponent[UNIT_LENGTH]  = 2;
  unit_database[unit_pos].exponent[UNIT_MASS]    = 1;
  unit_database[unit_pos].exponent[UNIT_TIME]    =-2;
  unit_database[unit_pos].exponent[UNIT_CURRENT] =-2;
  unit_pos++;

  unit_database[unit_pos].nameAs     = "T"; // tesla
  unit_database[unit_pos].nameAp     = "T";
  unit_database[unit_pos].nameLs     = unit_database[unit_pos].nameAs;
  unit_database[unit_pos].nameLp     = unit_database[unit_pos].nameAp;
  unit_database[unit_pos].nameFs     = "tesla";
  unit_database[unit_pos].nameFp     = "tesla";
  unit_database[unit_pos].quantity   = "magnetic_field";
  unit_database[unit_pos].multiplier = 1.0;
  unit_database[unit_pos].minPrefix  = -24;
  unit_database[unit_pos].maxPrefix  =  24;
  unit_database[unit_pos].si = 1;
  unit_database[unit_pos].exponent[UNIT_MASS]    = 1;
  unit_database[unit_pos].exponent[UNIT_TIME]    =-2;
  unit_database[unit_pos].exponent[UNIT_CURRENT] =-1;
  unit_pos++;

  unit_database[unit_pos].nameAs     = "G"; // gauss
  unit_database[unit_pos].nameAp     = "G";
  unit_database[unit_pos].nameLs     = unit_database[unit_pos].nameAs;
  unit_database[unit_pos].nameLp     = unit_database[unit_pos].nameAp;
  unit_database[unit_pos].nameFs     = "gauss";
  unit_database[unit_pos].nameFp     = "gauss";
  unit_database[unit_pos].quantity   = "magnetic_field";
  unit_database[unit_pos].multiplier = 1e-4;
  unit_database[unit_pos].minPrefix  = -24;
  unit_database[unit_pos].maxPrefix  =  24;
  unit_database[unit_pos].cgs = unit_database[unit_pos].imperial = unit_database[unit_pos].us = unit_database[unit_pos].ancient = 1;
  unit_database[unit_pos].exponent[UNIT_MASS]    = 1;
  unit_database[unit_pos].exponent[UNIT_TIME]    =-2;
  unit_database[unit_pos].exponent[UNIT_CURRENT] =-1;
  unit_pos++;

  unit_database[unit_pos].nameAs     = "Wb"; // weber
  unit_database[unit_pos].nameAp     = "Wb";
  unit_database[unit_pos].nameLs     = unit_database[unit_pos].nameAs;
  unit_database[unit_pos].nameLp     = unit_database[unit_pos].nameAp;
  unit_database[unit_pos].nameFs     = "weber";
  unit_database[unit_pos].nameFp     = "weber";
  unit_database[unit_pos].quantity   = "magnetic_flux";
  unit_database[unit_pos].multiplier = 1.0;
  unit_database[unit_pos].minPrefix  = -24;
  unit_database[unit_pos].maxPrefix  =  24;
  unit_database[unit_pos].si = 1;
  unit_database[unit_pos].exponent[UNIT_LENGTH]  = 2;
  unit_database[unit_pos].exponent[UNIT_MASS]    = 1;
  unit_database[unit_pos].exponent[UNIT_TIME]    =-2;
  unit_database[unit_pos].exponent[UNIT_CURRENT] =-1;
  unit_pos++;

  unit_database[unit_pos].nameAs     = "Mx"; // maxwell
  unit_database[unit_pos].nameAp     = "Mx";
  unit_database[unit_pos].nameLs     = unit_database[unit_pos].nameAs;
  unit_database[unit_pos].nameLp     = unit_database[unit_pos].nameAp;
  unit_database[unit_pos].nameFs     = "maxwell";
  unit_database[unit_pos].nameFp     = "maxwell";
  unit_database[unit_pos].quantity   = "magnetic_flux";
  unit_database[unit_pos].multiplier = 1e-8;
  unit_database[unit_pos].minPrefix  = -24;
  unit_database[unit_pos].maxPrefix  =  24;
  unit_database[unit_pos].cgs = unit_database[unit_pos].imperial = unit_database[unit_pos].us = unit_database[unit_pos].ancient = 1;
  unit_database[unit_pos].exponent[UNIT_LENGTH]  = 2;
  unit_database[unit_pos].exponent[UNIT_MASS]    = 1;
  unit_database[unit_pos].exponent[UNIT_TIME]    =-2;
  unit_database[unit_pos].exponent[UNIT_CURRENT] =-1;
  unit_pos++;

  // Planck Units
  unit_database[unit_pos].nameAs     = "L_planck"; // Planck Length
  unit_database[unit_pos].nameAp     = "L_planck";
  unit_database[unit_pos].nameLs     = "L_P";
  unit_database[unit_pos].nameLp     = "L_P";
  unit_database[unit_pos].nameFs     = "planck_length";
  unit_database[unit_pos].nameFp     = "planck_lengths";
  unit_database[unit_pos].quantity   = "length";
  unit_database[unit_pos].planck     = 1;
  unit_database[unit_pos].multiplier = 1.61625281e-35;
  unit_database[unit_pos].exponent[UNIT_LENGTH]=1;
  unit_pos++;

  unit_database[unit_pos].nameAs     = "M_planck"; // Planck Mass
  unit_database[unit_pos].nameAp     = "M_planck";
  unit_database[unit_pos].nameLs     = "M_P";
  unit_database[unit_pos].nameLp     = "M_P";
  unit_database[unit_pos].nameFs     = "planck_mass";
  unit_database[unit_pos].nameFp     = "planck_masses";
  unit_database[unit_pos].quantity   = "mass";
  unit_database[unit_pos].planck     = 1;
  unit_database[unit_pos].multiplier = 2.1764411e-8;
  unit_database[unit_pos].exponent[UNIT_MASS]=1;
  unit_pos++;

  unit_database[unit_pos].nameAs     = "T_planck"; // Planck Time
  unit_database[unit_pos].nameAp     = "T_planck";
  unit_database[unit_pos].nameLs     = "T_P";
  unit_database[unit_pos].nameLp     = "T_P";
  unit_database[unit_pos].nameFs     = "planck_time";
  unit_database[unit_pos].nameFp     = "planck_times";
  unit_database[unit_pos].quantity   = "time";
  unit_database[unit_pos].planck     = 1;
  unit_database[unit_pos].multiplier = 5.3912427e-44;
  unit_database[unit_pos].exponent[UNIT_TIME]=1;
  unit_pos++;

  unit_database[unit_pos].nameAs     = "Q_planck"; // Planck Charge
  unit_database[unit_pos].nameAp     = "Q_planck";
  unit_database[unit_pos].nameLs     = "Q_P";
  unit_database[unit_pos].nameLp     = "Q_P";
  unit_database[unit_pos].nameFs     = "planck_charge";
  unit_database[unit_pos].nameFp     = "planck_charges";
  unit_database[unit_pos].quantity   = "charge";
  unit_database[unit_pos].planck     = 1;
  unit_database[unit_pos].multiplier = 1.87554587047e-18;
  unit_database[unit_pos].exponent[UNIT_CURRENT]=1;
  unit_database[unit_pos].exponent[UNIT_TIME]   =1;
  unit_pos++;

  unit_database[unit_pos].nameAs     = "Theta_planck"; // Planck Temperature
  unit_database[unit_pos].nameAp     = "Theta_planck";
  unit_database[unit_pos].nameLs     = "\\Theta_P";
  unit_database[unit_pos].nameLp     = "\\Theta_P";
  unit_database[unit_pos].nameFs     = "planck_temperature";
  unit_database[unit_pos].nameFp     = "planck_temperature";
  unit_database[unit_pos].quantity   = "temperature";
  unit_database[unit_pos].planck     = 1;
  unit_database[unit_pos].multiplier = 1.41678571e32;
  unit_database[unit_pos].exponent[UNIT_TEMPERATURE]=1;
  unit_pos++;

  unit_database[unit_pos].nameAs     = "p_planck"; // Planck Momentum
  unit_database[unit_pos].nameAp     = "p_planck";
  unit_database[unit_pos].nameLs     = "p_P";
  unit_database[unit_pos].nameLp     = "p_P";
  unit_database[unit_pos].nameFs     = "planck_momentum";
  unit_database[unit_pos].nameFp     = "planck_momentum";
  unit_database[unit_pos].quantity   = "momentum";
  unit_database[unit_pos].planck     = 1;
  unit_database[unit_pos].multiplier = 6.5248018674330712229902929;
  unit_database[unit_pos].exponent[UNIT_MASS]  = 1;
  unit_database[unit_pos].exponent[UNIT_LENGTH]= 1;
  unit_database[unit_pos].exponent[UNIT_TIME]  =-1;
  unit_pos++;

  unit_database[unit_pos].nameAs     = "E_planck"; // Planck Energy
  unit_database[unit_pos].nameAp     = "E_planck";
  unit_database[unit_pos].nameLs     = "E_P";
  unit_database[unit_pos].nameLp     = "E_P";
  unit_database[unit_pos].nameFs     = "planck_energy";
  unit_database[unit_pos].nameFp     = "planck_energy";
  unit_database[unit_pos].quantity   = "energy";
  unit_database[unit_pos].planck     = 1;
  unit_database[unit_pos].multiplier = 1956085069.7617356777191162109375;
  unit_database[unit_pos].exponent[UNIT_MASS]  = 1;
  unit_database[unit_pos].exponent[UNIT_LENGTH]= 2;
  unit_database[unit_pos].exponent[UNIT_TIME]  =-2;
  unit_pos++;

  unit_database[unit_pos].nameAs     = "F_planck"; // Planck Force
  unit_database[unit_pos].nameAp     = "F_planck";
  unit_database[unit_pos].nameLs     = "F_P";
  unit_database[unit_pos].nameLp     = "F_P";
  unit_database[unit_pos].nameFs     = "planck_force";
  unit_database[unit_pos].nameFp     = "planck_force";
  unit_database[unit_pos].quantity   = "force";
  unit_database[unit_pos].planck     = 1;
  unit_database[unit_pos].multiplier = 1.2102593465942594902618273e+44;
  unit_database[unit_pos].exponent[UNIT_MASS]  = 1;
  unit_database[unit_pos].exponent[UNIT_LENGTH]= 1;
  unit_database[unit_pos].exponent[UNIT_TIME]  =-2;
  unit_pos++;

  unit_database[unit_pos].nameAs     = "P_planck"; // Planck Power
  unit_database[unit_pos].nameAp     = "P_planck";
  unit_database[unit_pos].nameLs     = "P_P";
  unit_database[unit_pos].nameLp     = "P_P";
  unit_database[unit_pos].nameFs     = "planck_power";
  unit_database[unit_pos].nameFp     = "planck_power";
  unit_database[unit_pos].quantity   = "power";
  unit_database[unit_pos].planck     = 1;
  unit_database[unit_pos].multiplier = 3.6282637948422090509290910e+52;
  unit_database[unit_pos].exponent[UNIT_MASS]  = 1;
  unit_database[unit_pos].exponent[UNIT_LENGTH]= 2;
  unit_database[unit_pos].exponent[UNIT_TIME]  =-3;
  unit_pos++;

  unit_database[unit_pos].nameAs     = "I_planck"; // Planck Current
  unit_database[unit_pos].nameAp     = "I_planck";
  unit_database[unit_pos].nameLs     = "I_P";
  unit_database[unit_pos].nameLp     = "I_P";
  unit_database[unit_pos].nameFs     = "planck_current";
  unit_database[unit_pos].nameFp     = "planck_current";
  unit_database[unit_pos].quantity   = "current";
  unit_database[unit_pos].planck     = 1;
  unit_database[unit_pos].multiplier = 3.4788748621352181073707008e+25;
  unit_database[unit_pos].exponent[UNIT_CURRENT]= 1;
  unit_pos++;

  unit_database[unit_pos].nameAs     = "V_planck"; // Planck Voltage
  unit_database[unit_pos].nameAp     = "V_planck";
  unit_database[unit_pos].nameLs     = "V_P";
  unit_database[unit_pos].nameLp     = "V_P";
  unit_database[unit_pos].nameFs     = "planck_voltage";
  unit_database[unit_pos].nameFp     = "planck_voltage";
  unit_database[unit_pos].quantity   = "potential";
  unit_database[unit_pos].planck     = 1;
  unit_database[unit_pos].multiplier = 1.0429417379546963190438953e+27;
  unit_database[unit_pos].exponent[UNIT_MASS]    = 1;
  unit_database[unit_pos].exponent[UNIT_LENGTH]  = 2;
  unit_database[unit_pos].exponent[UNIT_TIME]    =-3;
  unit_database[unit_pos].exponent[UNIT_CURRENT] =-1;
  unit_pos++;

  unit_database[unit_pos].nameAs     = "Z_planck"; // Planck Impedence
  unit_database[unit_pos].nameAp     = "Z_planck";
  unit_database[unit_pos].nameLs     = "Z_P";
  unit_database[unit_pos].nameLp     = "Z_P";
  unit_database[unit_pos].nameFs     = "planck_impedence";
  unit_database[unit_pos].nameFp     = "planck_impedence";
  unit_database[unit_pos].quantity   = "resistance";
  unit_database[unit_pos].planck     = 1;
  unit_database[unit_pos].multiplier = 2.9979282937316497736901511e+01;
  unit_database[unit_pos].exponent[UNIT_MASS]    = 1;
  unit_database[unit_pos].exponent[UNIT_LENGTH]  = 2;
  unit_database[unit_pos].exponent[UNIT_TIME]    =-3;
  unit_database[unit_pos].exponent[UNIT_CURRENT] =-2;
  unit_pos++;

  context->unit_pos = unit_pos;
  if (DEBUG) { sprintf(context->errcontext.tempErrStr, "%d system default units loaded.", unit_pos); ppl_log(&context->errcontext,NULL); }

  return;
 }

