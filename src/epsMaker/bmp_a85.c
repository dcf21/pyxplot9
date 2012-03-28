// bmp_a85.c
//
// The code in this file is part of PyXPlot
// <http://www.pyxplot.org.uk>
//
// Copyright (C) 2006-2012 Dominic Ford <coders@pyxplot.org.uk>
//               2008-2012 Ross Church
//
//               2009-2010 Michael Rutter
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

// This file is edited from code which was kindly contributed to PyXPlot by
// Michael Rutter. It efficiently encodes a string of raw image data into
// postscript's ASCII 85 data format, making use of all of the printable ASCII
// characters.

#define _PPL_BMP_A85_C 1

#include <stdlib.h>
#include <stdio.h>
#include "coreUtils/errorReport.h"

// Number of raw bytes to encode per line. Note actual line length will be
// 1.25x this. Sane values are 56 (70 characters) or 60 (75 chars). Must be a
// multiple of 4.

#define LINELEN 56

unsigned int bmp_A85(pplerr_context *ec, FILE* fout, unsigned char* in, int len)
 {
  int           i, j;
  unsigned int  tmp, t, line, length=0;
  unsigned int  pow85[] = {52200625,614125,7225,85,1}; // powers of 85
  unsigned char out[LINELEN/4*5+4], *outp;

  while (len>0)
   {
    line = (len>LINELEN) ? LINELEN : len;
    len  = len-line;
    outp = out;
    for (i=0; i<(line&0xfffc); i+=4) // encode groups of four bytes
     {
      tmp=0;
      for(j=0; j<4; j++)
       {
        tmp  = tmp<<8;
        tmp += *(in++);
       }
      if (tmp)
       {
        for (j=0; j<4; j++)
         {
          t    = tmp/pow85[j];
          tmp -= t*pow85[j];
          *(outp++) = t+'!';
         }
        *(outp++)=tmp+'!';
       }
      else
       *(outp++)='z'; // zero is encoded as "z", not "!"
     }

    if (line&0x3) // Deal with any final group of <4 bytes
     {
      i   = line&0xfffc;
      tmp = 0;
      for (j=0; j<4; j++)
       {
        tmp = tmp<<8;
        if ((i+j)<line) tmp+=*(in++);
       }
      for (j=0; j<((line&0x3)+1); j++)
       {
        t    = tmp/pow85[j];
        tmp -= t*pow85[j];
        *(outp++)=t+'!';
       }
     }

    *outp=0;

	 // Assist things which parse DSC comments by ensuring that any line which
	 // would start %! or %% has a space prefixed.

    if ((out[0]=='%')&&((out[1]=='%')||(out[1]=='!'))) length += fprintf(fout," %s\n",out);
    else                                               length += fprintf(fout,"%s\n" ,out);
   }

  length += fprintf(fout,"~>\n");
  return length;
 }

