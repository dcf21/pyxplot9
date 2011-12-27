// papersizes.c
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "stringTools/asciidouble.h"
#include "stringTools/strConstants.h"

#include "settings/papersizes.h"

typedef struct PaperSize {
  char   name[32];
  double height;
  double width;
 } PaperSize;

PaperSize PaperSizeList[128] = {
  {"crown"                     , 508   , 381   },
  {"demy"                      , 572   , 445   },
  {"double_demy"               , 889   , 597   },
  {"elephant"                  , 711   , 584   },
  {"envelope_dl"               , 110   , 220   }, // Wider than it is long
  {"executive"                 , 267   , 184   },
  {"foolscap"                  , 330   , 203   },
  {"government_letter"         , 267   , 203   },
  {"international_businesscard",  85.60,  53.98},
  {"large_post"                , 533   , 419   },
  {"letter"                    , 279   , 216   },
  {"legal"                     , 356   , 216   },
  {"ledger"                    , 432   , 279   },
  {"medium"                    , 584   , 457   },
  {"monarch"                   , 267   , 184   },
  {"post"                      , 489   , 394   },
  {"quad_demy"                 ,1143   , 889   },
  {"quarto"                    , 254   , 203   },
  {"royal"                     , 635   , 508   },
  {"statement"                 , 216   , 140   },
  {"tabloid"                   , 432   , 279   },
  {"japanese_shiroku4"         , 379   , 264   },
  {"japanese_shiroku5"         , 262   , 189   },
  {"japanese_shiroku6"         , 188   , 127   },
  {"japanese_kiku4"            , 306   , 227   },
  {"japanese_kiku5"            , 227   , 151   },
  {"us_businesscard"           ,  89   ,  51   },
  {"END"                       ,   0   ,   0   }
 };


void ppl_PaperSizeInit()
 {
  double height , width ;
  double height2, width2;
  int    pos,i,j;

  static char *SeriesNames[] = {"a","swedish_e","c","swedish_g","b","swedish_f","swedish_d","swedish_h"};

  for(pos=0 ; (strcmp(PaperSizeList[pos].name , "END")!=0) ; pos++);

  strcpy(PaperSizeList[pos].name , "4A0");
  height = sqrt(2)*sqrt(4e6/sqrt(2)); // 4A0 has area four square metres
  width  = sqrt(4e6/sqrt(2));
  PaperSizeList[pos].width  = width;
  PaperSizeList[pos].height = height;

  pos++;

  strcpy(PaperSizeList[pos].name , "2A0");
  height = sqrt(2)*sqrt(2e6/sqrt(2)); // 2A0 has area four square metres
  width  = sqrt(2e6/sqrt(2));
  PaperSizeList[pos].width  = width;
  PaperSizeList[pos].height = height;

  pos++;

  for (i=0; i<11; i++)
   {
    height2 = height/sqrt(2);
    width2  = width /sqrt(2);
    sprintf(PaperSizeList[pos].name, "japanese_b%d", i); // Japanese B0 - B11
    PaperSizeList[pos].width  = (width2  + width )/2;
    PaperSizeList[pos].height = (height2 + height)/2;
    pos++;
    for (j=0; j<8; j++)
     {
      sprintf(PaperSizeList[pos].name, "%s%d", SeriesNames[j], i); // Logarithmically-spaced series, including the A, B and C-series.
      PaperSizeList[pos].height = height2 * pow(2.0, ((float)j/16));
      PaperSizeList[pos].width  = width2  * pow(2.0, ((float)j/16));
      pos++;
     }
    height = height2 ; width = width2;
   }
  strcpy(PaperSizeList[pos].name, "END"); // Marker of end of papersize list
  return;
 }

void ppl_PaperSizeByName(char *name, double *height, double *width)
 {
  int i=0;
  while (1)
   {
    if (strcmp(PaperSizeList[i].name, "END")==0) // The requested papersize doesn't exist
     {
      *height = 0.0;
      *width  = 0.0;
      return;
     }
    if (ppl_strCmpNoCase(PaperSizeList[i].name, name)==0) // We've found the requested papersize
     {
      *height = PaperSizeList[i].height;
      *width  = PaperSizeList[i].width;
      return;
     }
    i++;
   }
 }

void ppl_GetPaperName(char *name, double *height, double *width)
 {
  int i=0;
  while (1)
   {
    if (strcmp(PaperSizeList[i].name, "END")==0) // The requested papersize doesn't exist
     {
      strcpy(name, "User-defined papersize");
      return;
     }
    if ((fabs(PaperSizeList[i].height - *height) < 0.5 ) && (fabs(PaperSizeList[i].width - *width) < 0.5 )) // We've found the requested papersize
     {
      strcpy(name, PaperSizeList[i].name);
      return;
     }
    i++;
   }
 }

