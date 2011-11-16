// ppl_memAlloc.h
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

// Functions for memory management

#ifndef _MEMALLOC_H
#define _MEMALLOC_H 1

#include "coreUtils/errorReport.h"

void  ppl_memAlloc_MemoryInit            ( pplerr_context *ec, void(*mem_error_handler)(pplerr_context *,int, int, int, char *) , void(*mem_log_handler)(pplerr_context *,char *) );
void  ppl_memAlloc_MemoryStop            ();
int   ppl_memAlloc_DescendIntoNewContext ();
int   ppl_memAlloc_AscendOutOfContext    (int context);
void _ppl_memAlloc_SetMemContext         (int context);
int   ppl_memAlloc_GetMemContext         ();
void  ppl_memAlloc_FreeAll               (int context);
void  ppl_memAlloc_Free                  (int context);

void *ppl_memAlloc                       (int size);
void *ppl_memAlloc_incontext             (int size, int context);

// Allocate memory in 128kb blocks (131072 bytes)
#define FM_BLOCKSIZE  131072

// Always align mallocs to 8-byte boundaries; 64-bit processors do double arithmetic twice as fast when word-aligned
#define SYNCSTEP      8

#endif

