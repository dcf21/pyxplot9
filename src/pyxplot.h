// pyxplot.h
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

#ifndef _PYXPLOT_H
#define _PYXPLOT_H 1

#ifndef _PYXPLOT_C
#include <setjmp.h>
extern sigjmp_buf  ppl_sigjmpToMain;
extern sigjmp_buf *ppl_sigjmpFromSigInt;
#endif
void               ppl_sigIntHandle(int signo);

#endif

