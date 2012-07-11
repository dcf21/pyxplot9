// pplObjUnits.h
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

#ifndef _PPLOBJUNITS_H
#define _PPLOBJUNITS_H 1

#define UNITS_MAX_BASEUNITS   24
#define UNITS_MAX           1024

#define UNIT_LENGTH      0
#define UNIT_TIME        1
#define UNIT_MASS        2
#define UNIT_CURRENT     3
#define UNIT_TEMPERATURE 4
#define UNIT_MOLE        5
#define UNIT_ANGLE       6
#define UNIT_BIT         7
#define UNIT_COST        8
#define UNIT_FIRSTUSER   9

typedef struct unit
 {
  char         *nameAs, *nameAp, *nameLs, *nameLp, *nameFs, *nameFp, *alt1, *alt2, *alt3, *alt4, *comment, *quantity;
  double        multiplier, offset;
  unsigned char si, cgs, imperial, us, planck, ancient, userSel, notToBeCompounded, modified, tempType;
  int           maxPrefix;
  int           minPrefix;
  int           userSelPrefix;
  double        exponent[UNITS_MAX_BASEUNITS];
 } unit;

#include "userspace/pplObj.h"

typedef struct PreferredUnit
 {
  int    NUnits;
  int    UnitID[UNITS_MAX_BASEUNITS];
  int    prefix[UNITS_MAX_BASEUNITS];
  double exponent[UNITS_MAX_BASEUNITS];
  pplObj value;
  unsigned char modified;
 } PreferredUnit;

#endif

