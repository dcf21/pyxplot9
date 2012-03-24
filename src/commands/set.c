// set.c
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
#include <limits.h>
#include <math.h>
#include <ctype.h>

#include "commands/set.h"

#include "coreUtils/dict.h"
#include "coreUtils/memAlloc.h"
#include "coreUtils/list.h"

#include "expressions/traceback_fns.h"

#include "parser/cmdList.h"
#include "parser/parser.h"

#include "settings/arrows_fns.h"
#include "settings/axes_fns.h"
#include "settings/epsColors.h"
#include "settings/labels_fns.h"
#include "settings/settings.h"
#include "settings/settingTypes.h"
#include "settings/textConstants.h"
#include "settings/withWords_fns.h"

#include "stringTools/asciidouble.h"

#include "userspace/context.h"
#include "userspace/pplObj_fns.h"
#include "userspace/pplObjFunc.h"
#include "userspace/pplObjPrint.h"
#include "userspace/unitsArithmetic.h"
#include "userspace/unitsDisp.h"

#include "canvasItems.h"
#include "pplConstants.h"

#define TBADD(et,pos) ppl_tbAdd(c,pl->srcLineN,pl->srcId,pl->srcFname,0,et,pos,pl->linetxt,"")

void directive_set(ppl_context *c, parserLine *pl, parserOutput *in, int interactive)
 {
  return;
 }

