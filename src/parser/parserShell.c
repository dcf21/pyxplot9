// parserShell.c
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

#define _PARSERSHELL_C 1

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "coreUtils/errorReport.h"

#include "stringTools/asciidouble.h"
#include "stringTools/strConstants.h"

#include "parser/parser.h"

#include "expressions/traceback.h"

#include "userspace/context.h"
#include "userspace/garbageCollector.h"
#include "userspace/pplObj_fns.h"

#include "pplConstants.h"

#define TBADD(et,pos,lt) ppl_tbAdd(context,in->srcLineN,in->srcId,in->srcFname,0,et,pos,lt)

void ppl_parserShell(ppl_context *c, parserOutput *in, int iterDepth)
 {
  return;
 }

