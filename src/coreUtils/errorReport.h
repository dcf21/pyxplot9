// ppl_error.h
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

// Functions for returning messages to the user

#ifndef _PPL_ERROR_H
#define _PPL_ERROR_H 1

#define ERR_INTERNAL 100
#define ERR_GENERAL  101
#define ERR_SYNTAX   102
#define ERR_NUMERIC  103
#define ERR_FILE     104
#define ERR_MEMORY   105
#define ERR_STACKED  106
#define ERR_PREFORMED 107

#include "stringTools/strConstants.h"

typedef struct pplerr_context_s
 {
  int       error_input_linenumber;
  char      error_input_filename[FNAME_LENGTH];
  char      error_source[16]; // Identifier of the process producing log messages
  char      tempErrStr[LSTR_LENGTH];
 } pplerr_context;

void ppl_error_setstreaminfo(pplerr_context *context, int linenumber,char *filename);
void ppl_error(pplerr_context *context, int ErrType, int HighlightPos1, int HighlightPos2, char *msg);
void ppl_fatal(pplerr_context *context, char *file, int line, char *msg);
void ppl_warning(pplerr_context *context, int ErrType, char *msg);
void ppl_report(pplerr_context *context, char *msg);
void ppl_log(pplerr_context *context, char *msg);

#endif
