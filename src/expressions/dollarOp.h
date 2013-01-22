// dollarOp.h
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

#ifndef _DOLLAROP_H
#define _DOLLAROP_H 1

#include "userspace/context.h"
#include "userspace/pplObj.h"

void ppl_dollarOp_config  (ppl_context *c,
  char   **columns_str,
  pplObj  *columns_val,
  int      Ncols,
  char    *filename,
  long     file_linenumber,
  long    *file_linenumbers,
  long     linenumber_count,
  long     block_count,
  long     index_number,
  int      usingRowCol,
  char    *usingExpr,
  char   **colHeads,
  int      NcolHeads,
  pplObj  *colUnits,
  int      NcolUnits);

void ppl_dollarOp_deconfig(ppl_context *c);

void ppl_dollarOp_fetchColByNum(ppl_context *c, pplExpr *inExpr, int inExprCharPos, int colNum);

void ppl_dollarOp_fetchColByName(ppl_context *c, pplExpr *inExpr, int inExprCharPos, char *colName);

#endif

