// stringList.c
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

// List-based string processing functions

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "stringTools/asciidouble.h"

#include "list.h"

/* ppl_strSplit(): Split up a string into bits separated by whitespace */

list *ppl_strSplit(char *in)
 {
  int pos, start, end;
  char *word;
  char *text_buffer;
  list *out;
  out  = ppl_listInit(0);
  pos  = 0;
  text_buffer = (char *)malloc(strlen(in)+1);
  while (in[pos] != '\0')
   {
    // Scan along to find the next word
    while ((in[pos] <= ' ') && (in[pos] > '\0')) pos++;
    start = pos;

    // Scan along to find the end of this word
    while ((in[pos] >  ' ') && (in[pos] > '\0')) pos++;
    end = pos;

    if (end>start)
     {
      word = ppl_strSlice(in, text_buffer, start, end);
      ppl_listAppendCpy(out, word, strlen(word)+1);
     }
   }
  free(text_buffer);
  return out;
 }

