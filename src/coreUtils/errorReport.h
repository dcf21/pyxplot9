// ppl_error.h
//
// The code in this file is part of PyXPlot
// <http://www.pyxplot.org.uk>
//
// Copyright (C) 2006-2011 Dominic Ford <coders@pyxplot.org.uk>
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

extern char ppl_error_source[];
extern char ppl_tempErrStr[];

void ppl_error_setstreaminfo(int linenumber,char *filename);
void ppl_error(int ErrType, int HighlightPos1, int HighlightPos2, char *msg);
void ppl_fatal(char *file, int line, char *msg);
void ppl_warning(int ErrType, char *msg);
void ppl_report(char *msg);
void ppl_log(char *msg);

#endif
