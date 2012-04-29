// bmp_jpegread.c
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
// Michael Rutter. It reads in data from JPEG files without performing any
// decompression, since the DCT-compressed data can be rewritten straight out
// to postscript for decompression by the postscript rasterising engine.

#define _PPL_BMP_JPEGREAD_C 1

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <gsl/gsl_const_mksa.h>

#include "coreUtils/memAlloc.h"
#include "coreUtils/errorReport.h"

#include "epsMaker/bmp_image.h"
#include "epsMaker/bmp_jpegread.h"

#define HEADLEN 8*1024

void ppl_bmp_jpegread(pplerr_context *ec, FILE *jpeg, bitmap_data *image)
 {
  int  comps=0;
  long i, j, len, chunk=64*1024;

  unsigned int width=0, height=0, save;
  unsigned char *buff, *header, *headp, *headendp, type=0, comp=0, *p;

  buff     = (unsigned char *)ppl_memAlloc(chunk);
  header   = (unsigned char *)ppl_memAlloc(HEADLEN);
  headp    = header;
  headendp = header + HEADLEN - 8;

  if (DEBUG) ppl_log(ec, "Beginning to decode JPEG image file");

  if ((buff == NULL)||(header == NULL)) { ppl_error(ec, ERR_MEMORY, -1, -1,"Out of memory"); return; }

  if (fread(buff,3,1,jpeg)!=1) { ppl_error(ec, ERR_FILE, -1, -1,"This JPEG image file appears to be corrupted"); return; }
  i=buff[0];
  if (i<0xe0) { ppl_error(ec, ERR_FILE, -1, -1, "In supplied JPEG image, first marker is not APPn. Aborting."); return; }
  len=buff[1]*256+buff[2]-2;

  if (fread(buff,len,1,jpeg)!=1) { ppl_error(ec, ERR_FILE, -1, -1,"This JPEG image file appears to be corrupted"); return; }

  if (DEBUG)
   {
    if (!strcmp((char*)buff,"JFIF")&&(i=0xe0))
     {
      sprintf(ec->tempErrStr,"JPEG version %d.%02d",(int)buff[5],(int)buff[6]);   ppl_log(ec, NULL);
      sprintf(ec->tempErrStr,"Thumbnail size %dx%d",(int)buff[12],(int)buff[13]); ppl_log(ec, NULL);
      sprintf(ec->tempErrStr,"JFIF APP0 entry length 0x%x",(int)len+2);           ppl_log(ec, NULL);
      switch ((int)buff[7])
       {
        case 0:
         ppl_log(ec, "No DPI information available");
         break;
        case 1:
          sprintf(ec->tempErrStr,"DPI specified as %dx%d dots per inch",(256*((int)buff[ 8])+((int)buff[ 9])),
                                                                         (256*((int)buff[10])+((int)buff[11])) );
          ppl_log(ec, NULL);
          image->XDPI = (256*((int)buff[ 8])+((int)buff[ 9]));
          image->YDPI = (256*((int)buff[10])+((int)buff[11]));
          break;
        case 2:
          sprintf(ec->tempErrStr,"DPI specified as %dx%d dots per cm"  ,(256*((int)buff[ 8])+((int)buff[ 9])),
                                                                         (256*((int)buff[10])+((int)buff[11])) );
          ppl_log(ec, NULL);
          image->XDPI = (256*((int)buff[ 8])+((int)buff[ 9])) * 100 * GSL_CONST_MKSA_INCH;
          image->YDPI = (256*((int)buff[10])+((int)buff[11])) * 100 * GSL_CONST_MKSA_INCH;
          break;
        default:
          sprintf(ec->tempErrStr,"DPI specified in unrecognised unit number %d",(int)buff[7]);
          ppl_log(ec, NULL);
          break;
       }
     } else if (!strcmp((char*)buff,"Exif")&&(i=0xe1)) {
      sprintf(ec->tempErrStr,"Exif JPEG file\n");                                 ppl_log(ec, NULL);
      sprintf(ec->tempErrStr,"Exif APP1 entry length 0x%x",(int)len+2);           ppl_log(ec, NULL);
    }
  }

  *(headp++) = 0xff;
  *(headp++) = 0xd8;

  if (DEBUG) { sprintf(ec->tempErrStr, "JFIF APP%d entry discarded", (int)i-0xe0); ppl_log(ec, NULL); }

  while ((type!=0xda) && fread(buff,1,1,jpeg) && (buff[0]=0xff))
   {
    if (fread(buff,3,1,jpeg)!=1) { ppl_error(ec, ERR_FILE, -1, -1,"This JPEG image file appears to be corrupted"); return; }
    type = buff[0];
    len  = buff[1]*256+buff[2]-2;
    save = 0;
    if (fread(buff,len,1,jpeg)!=1) { ppl_error(ec, ERR_FILE, -1, -1,"This JPEG image file appears to be corrupted"); return; }

    if (DEBUG) { sprintf(ec->tempErrStr, "Entry type %x length 0x%x",(int)type,(int)len+2); ppl_log(ec, NULL); }

    if (((type&0xf0)==0xc0) && (type!=0xc4) && (type!=0xcc))
     {
      if (comp) { ppl_error(ec, ERR_FILE, -1, -1, "Cannot decode JPEG image file: it contains multiple images?"); return; }
      comp   = type;
      //prec   = buff[0];
      height = 256*buff[1]+buff[2];
      width  = 256*buff[3]+buff[4];
      comps  = buff[5];
      save   = 1;
     }

    if ((type==0xfe) && (DEBUG)) { buff[len]=0; sprintf(ec->tempErrStr,"JPEG Comment: %s",buff); ppl_log(ec, NULL); }
    if ((type==0xe0) && (DEBUG)) { sprintf(ec->tempErrStr,"APP0 Marker: %s",buff); ppl_log(ec, NULL); }

    if ((type==0xdb)||(type==0xc4)||(type==0xda)||(type==0xdd)) save=1;
    if (save)
     {
      *(headp++) = 0xff;
      *(headp++) = type;
      *(headp++) = (len+2)>>8;
      *(headp++) = (len+2)&0xff;
      if (headp+len>headendp) { ppl_error(ec, ERR_FILE, -1, -1,"Header storage for JPEG image exhausted"); return; }
      for (i=0; i<len; i++) *(headp++) = buff[i];
     }
    else if (DEBUG) ppl_log(ec, "Discarding section of JPEG image file");
   }

  if (DEBUG) { sprintf(ec->tempErrStr, "Image size %dx%d with %d components",width,height,comps); ppl_log(ec, NULL); }

  switch (comp)
   {
    case 0xc0:
      if (DEBUG) ppl_log(ec, "Encoding: baseline JPEG");
      break;
    case 0xc1:
      if (DEBUG) ppl_log(ec, "Encoding: extended sequential, Huffman JPEG");
      break;
    case 0xc2:
      ppl_error(ec, ERR_FILE, -1, -1, "JPEG image detected to have progressive encoding, which PyXPlot does not support. Please convert to baseline JPEG and try again.");
      return;
    default:
      sprintf(ec->tempErrStr, "JPEG image detected to have unsupported compression type SOF%d. Please convert to baseline JPEG and try again.",((int)comp)&0xf);
      ppl_error(ec, ERR_FILE, -1, -1, NULL);
      return;
   }

  if ((comps!=3) && (comps!=1)) { sprintf(ec->tempErrStr,"JPEG image contains %d colour components; PyXPlot only supports JPEG images with one (greyscale) or three (RGB) components", comps); ppl_error(ec, ERR_FILE, -1, -1, NULL); return; }

  if (comps ==3 )
   {
    image->colour = BMP_COLOUR_RGB;
    image->depth  = 24;
   }
  else
   {
    image->colour = BMP_COLOUR_GREY;
    image->depth  = 8;
   }

  image->width             = width;
  image->height            = height;
  image->TargetCompression = BMP_ENCODING_DCT;

  // Now read JPEG image data. Unfortunately, we don't know how much we'll get

  if (DEBUG) { sprintf(ec->tempErrStr, "%d bytes of header read",(int)(headp-header)); ppl_log(ec, NULL); }

  for(i=0; header+i<headp; i++) buff[i] = header[i];
  len = chunk-(headp-header);
  j   = fread(buff+i,1,len,jpeg);

  if (j<len)
   {
    image->data     = buff;
    image->data_len = i+j;
    return;
   }

  i=j;
  while (i==len)
   {
    chunk *= 2;
    p      = (unsigned char *)ppl_memAlloc(chunk); // We didn't malloc enough memory, and ppl_memAlloc can't realloc, so just have to malloc a new chunk :(
    if (p == NULL) { ppl_error(ec, ERR_MEMORY, -1, -1, "Out of memory"); return; }
    memcpy(p, buff, chunk/2);
    buff = p;
    len  = chunk/2;
    i    = fread(buff+len,1,len,jpeg);
   }

  image->data=buff;
  image->data_len=len+i;
  return;
 }

