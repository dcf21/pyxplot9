// bmp_jpegread.c
//
// The code in this file is part of Pyxplot
// <http://www.pyxplot.org.uk>
//
// Copyright (C) 2006-2013 Dominic Ford <coders@pyxplot.org.uk>
//               2008-2013 Ross Church
//
//               2009-2012 Michael Rutter
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

// This file is edited from code which was kindly contributed to Pyxplot by
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
  int  comps=0, i, j;

  unsigned int   width=0, height=0, save, sos;
  unsigned int   len, chunk=64*1024;
  unsigned char *buff, *header, *headp, *headendp, type=0, comp=0, *p;

  BMP_ALLOC(buff, chunk);
  header   = (unsigned char *)ppl_memAlloc(HEADLEN);
  headp    = header;
  headendp = header + HEADLEN - 8;

  if (DEBUG) ppl_log(ec, "Beginning to decode JPEG image file");

  if ((buff == NULL)||(header == NULL)) { ppl_error(ec, ERR_MEMORY, -1, -1,"Out of memory"); return; }

  *(headp++)=0xff;
  *(headp++)=0xd8;

  do
   {
    if (fread(buff,3,1,jpeg)!=1) { ppl_error(ec, ERR_FILE, -1, -1,"This JPEG image file appears to be corrupted (A)"); return; }
    type=buff[0];
    len=buff[1]*256+buff[2]-2;
    save=0;
    if (fread(buff,len,1,jpeg)!=1) { ppl_error(ec, ERR_FILE, -1, -1,"This JPEG image file appears to be corrupted (B)"); return; }
    if (DEBUG) { sprintf(ec->tempErrStr,"Entry type %x length 0x%x",(int)type,len+2); ppl_log(ec, NULL); }
    if ((type==0xe0)&&!strcmp((char*)buff,"JFIF"))
     {
      if ( ((buff[7]==1)||(buff[7]==2)) )
       {
        image->XDPI=buff[8]*256+buff[9];
        image->YDPI=buff[10]*256+buff[11];
        if (buff[7]==2) /* units were pixels per cm */
         {
          image->XDPI*=25.4;
          image->YDPI*=25.4;
         }
       }
      if (DEBUG)
       {
        sprintf(ec->tempErrStr, "JPEG version %d.%02d",(int)buff[5],(int)buff[6]); ppl_log(ec, NULL);
        sprintf(ec->tempErrStr, "JFIF thumbnail size %dx%d", (int)buff[12],(int)buff[13]); ppl_log(ec, NULL);
        if (image->XDPI) { sprintf(ec->tempErrStr, "DPI: %.1fx%.1f",image->XDPI, image->YDPI); ppl_log(ec, NULL); }
        sprintf(ec->tempErrStr, "JFIF APP0 entry length 0x%x",len+2); ppl_log(ec, NULL);
       }
     }

    if (DEBUG && ((type==0xe1) && (!strcmp((char*)buff,"Exif"))) )
     {
      sprintf(ec->tempErrStr, "Exif JPEG file"); ppl_log(ec, NULL);
      sprintf(ec->tempErrStr, "Exif APP1 entry length 0x%x",len+2); ppl_log(ec, NULL);
     }

    if ( ((type&0xf0)==0xc0) && (type!=0xc4) && (type!=0xcc) )
     {
      if (comp) { ppl_error(ec, ERR_FILE, -1, -1, "Cannot decode JPEG image file: it contains multiple images?"); return; }
      comp   = type;
      height = 256*buff[1]+buff[2];
      width  = 256*buff[3]+buff[4];
      comps  = buff[5];
      save   = 1;
     }

    if (DEBUG && (type==0xfe)) { buff[len]=0; sprintf(ec->tempErrStr, "JPEG Comment: %s",buff); ppl_log(ec, NULL); }
    if (DEBUG && (type==0xe0)) { sprintf(ec->tempErrStr, "APP0 Marker: %s",buff); ppl_log(ec, NULL); }
    if (DEBUG && (type==0xdd)) { sprintf(ec->tempErrStr, "Restart markers present"); ppl_log(ec, NULL); }

    if ((type==0xdb)||(type==0xc4)||(type==0xda)||(type==0xdd)) save=1;
    if (save)
     {
      *(headp++)=0xff;
      *(headp++)=type;
      *(headp++)=(len+2)>>8;
      *(headp++)=(len+2)&0xff;
      if (headp+len>headendp) { ppl_error(ec, ERR_FILE, -1, -1, "JPEG header storage exhausted"); return ; }
      for (i=0; i<len; i++) *(headp++)=buff[i];
     }
    else if (DEBUG) { sprintf(ec->tempErrStr, "Discarding section of JPEG image file."); ppl_log(ec, NULL); }
   }
  while ((type!=0xda) && fread(buff,1,1,jpeg) && (buff[0]==0xff));

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
      ppl_error(ec, ERR_FILE, -1, -1, "JPEG image detected to have progressive encoding, which Pyxplot does not support. Please convert to baseline JPEG and try again.");
      return;
    default:
      sprintf(ec->tempErrStr, "JPEG image detected to have unsupported compression type SOF%d. Please convert to baseline JPEG and try again.",((int)comp)&0xf);
      ppl_error(ec, ERR_FILE, -1, -1, NULL);
      return;
   }

  if ((comps!=3) && (comps!=1)) { sprintf(ec->tempErrStr,"JPEG image contains %d colour components; Pyxplot only supports JPEG images with one (greyscale) or three (RGB) components", comps); ppl_error(ec, ERR_FILE, -1, -1, NULL); return; }

  if (comps == 3)
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
    image->data_len = i+j;
   }
  else
   {
    i=j;
    while (i==len)
     {
      chunk *= 2;
      BMP_ALLOC(p , chunk); // We didn't malloc enough memory, and ppl_memAlloc can't realloc, so just have to malloc a new chunk :(
      if (p == NULL) { ppl_error(ec, ERR_MEMORY, -1, -1, "Out of memory"); return; }
      memcpy(p, buff, chunk/2);
      buff = p;
      len  = chunk/2;
      i    = fread(buff+len,1,len,jpeg);
     }
    image->data_len=len+i;
   }

  // Check what we have read for integrity and trailing rubbish. Assume that
  // the first two bytes are 0xffd8. Then we need not deal with awkward markers
  // which are not followed by lengths...

  i  =2;
  sos=0;

  while(i<image->data_len)
   {
    if (buff[i]!=0xff)
     {
      if (DEBUG) { sprintf(ec->tempErrStr, "Offset %d",i); ppl_log(ec, NULL); }
      ppl_error(ec, ERR_FILE, -1, -1, "Unexpected data in JPEG - no marker where expected");
      return;
     }
    i++;
    if (DEBUG) { sprintf(ec->tempErrStr, "Check: entry type %x ...",buff[i]); ppl_log(ec, NULL); }
    if (buff[i]==0xd9) break; // End of image
    if (buff[i]==0xda) sos=1;
    if (buff[i]==0xd8) { ppl_error(ec, ERR_FILE, -1, -1, "Unexpected extra SOI in JPEG data."); return; }
    i++;
    len=buff[i]*256+buff[i+1]-2;
    i+=2;
    if (DEBUG) { sprintf(ec->tempErrStr, "... length 0x%x",len+2); ppl_log(ec, NULL); }
    i+=len;
    if (sos) // We now have an entropy-encoded section of unknown length
     {
      j=i;
      // Entropy-encoded sections stop at a marker (0xff), save that they
      // don't for 0xff00, and it is easiest to consider the restart markers
      // 0xffd0 to 0xffd7 to be part of the entropy-encoded section
      while (!((i>=image->data_len) || ((buff[i]==0xff)&&((buff[i+1]!=0)&&((buff[i+1]&0xf8)!=0xd0)))))
        i++;
      if (DEBUG) { sprintf(ec->tempErrStr, "Check: entropy encoded section %d bytes",i-j); ppl_log(ec, NULL); }
      sos=0;
     }
   }

  if (buff[i]!=0xd9) { ppl_error(ec, ERR_FILE, -1, -1, "Error -- JPEG EOI not found, file corrupted/truncated?"); return; }

  if (DEBUG && (i+1<image->data_len))
   { sprintf(ec->tempErrStr, "%lu bytes of trailing garbage removed from JPEG", image->data_len-i-1); ppl_log(ec, NULL); }

  image->data=buff;
  image->data_len=i+1;
  return;
 }
