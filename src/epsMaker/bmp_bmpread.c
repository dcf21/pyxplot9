// bmp_bmpread.c
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
// Foundation; either version 2 of the License, -1, -1, or (at your option) any later
// version.
//
// You should have received a copy of the GNU General Public License along with
// PyXPlot; if not, write to the Free Software Foundation, Inc., 51 Franklin
// Street, Fifth Floor, Boston, MA  02110-1301, USA

// ----------------------------------------------------------------------------

// This file is edited from code which was kindly contributed to PyXPlot by
// Michael Rutter. It reads in data from Windows and OS/2 bitmap files.

#define _PPL_BMP_BMPREAD_C 1

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "coreUtils/memAlloc.h"
#include "coreUtils/errorReport.h"

#include "epsMaker/bmp_image.h"
#include "epsMaker/bmp_bmpread.h"

void ppl_bmp_bmpread(pplerr_context *ec, FILE *in, bitmap_data *image)
 {
  unsigned char buff[60],encode,*p,c;
  unsigned width,height,depth,dw,excess;
  unsigned long offset,off2,size;
  int i,j,ncols,os2,rle;

  if (DEBUG) ppl_log(ec, "Beginning to decode BMP image file");

  off2=3;

  // Read rest of first header
  if (fread(buff,11,1,in)!=1) { ppl_error(ec, ERR_FILE, -1, -1,"This bitmap file appears to be corrupted"); return; }
  off2 += 11;

  if ((buff[3])||(buff[4])||(buff[5])||(buff[6])) { ppl_error(ec, ERR_FILE, -1, -1,"This bitmap file appears to be corrupted"); return; }

  offset = buff[7] + (buff[8]<<8) + (buff[9]<<16) + (buff[10]<<24);

  // Read info header
  if (fread(buff,12,1,in)!=1) { ppl_error(ec, ERR_FILE, -1, -1,"This bitmap file appears to be corrupted"); return; }
  off2 += 12;

  if ((buff[0]==12)&&(buff[1]==0)) // OS/2 1.x bitmap
   {
    os2=1;
    if ((buff[8]!=1)||(buff[9])) { ppl_error(ec, ERR_FILE, -1, -1,"This OS/2 bitmap file appears to be corrupted"); return; }
    width  = buff[4] + ((unsigned)buff[5]<<8);
    height = buff[6] + ((unsigned)buff[7]<<8);
    depth  = buff[10];
    encode = 0;
    size   = 0;
   }
  else // Windows bitmap
   {
    os2=0;

    // Known valid header lengths at this point are 40, 64, 108 and 124
    // So accept from 40 to 255
    if ((buff[0]<40)||(buff[1])||(buff[2])||(buff[3])) { ppl_error(ec, ERR_FILE, -1, -1,"This Windows bitmap file appears to be corrupted"); return; }
    excess=buff[0]-40;

    if (fread(buff+12,40-12,1,in)!=1) { ppl_error(ec, ERR_FILE, -1, -1,"This bitmap file appears to be corrupted"); return; }
    off2 += 40-12;

    if (excess)
     {
      off2 += excess;
      for (;excess;excess--) fgetc(in); // Can't seek stdin
     }

    width  = buff[4] + ((unsigned)buff[5]<<8) + ((unsigned)buff[ 6]<<16) + ((unsigned)buff[ 7]<<24);
    height = buff[8] + ((unsigned)buff[9]<<8) + ((unsigned)buff[10]<<16) + ((unsigned)buff[11]<<24);

    if ((buff[12]!=1)||(buff[13])||(buff[15])) { ppl_error(ec, ERR_FILE, -1, -1,"This Windows bitmap file appears to be corrupted"); return; }

    depth  = buff[14];
    encode = buff[16];
    size   = buff[20] + (((int)(buff[21]))<<8) + (((int)(buff[22]))<<16) + (((int)(buff[23]))<<24);

    image->XDPI = 0.0254*(buff[24]+(((int)(buff[25]))<<8)+(((int)(buff[26]))<<16)+(((int)(buff[27]))<<24));
    image->YDPI = 0.0254*(buff[28]+(((int)(buff[29]))<<8)+(((int)(buff[30]))<<16)+(((int)(buff[31]))<<24));
    if (image->XDPI<1) image->XDPI=180; // Sensible default resolution in case of zero input
    if (image->YDPI<1) image->YDPI=180;
   }

  if (DEBUG) { sprintf(ec->tempErrStr, "Size %dx%d depth %d bits",width,height,depth); ppl_log(ec, NULL); }

  rle=0;
  if (encode!=0)
   {
    if      ((encode==1)&&(depth== 8)) rle=8;
    else if ((encode==2)&&(depth== 4)) rle=4;
    else if ((encode!=3)||(depth!=16)) { ppl_error(ec, ERR_FILE, -1, -1,"This Windows bitmap file has an invalid encoding"); return; }
   }

  if ((depth!=1)&&(depth!=4)&&(depth!=8)&&(depth!=16)&&(depth!=24)) { sprintf(ec->tempErrStr, "Bitmap colour depth of %d not supported\n",depth); ppl_error(ec, ERR_FILE, -1, -1, NULL); return; }

  if (depth<=8) // We have a palette to read
   {
    ncols = 0;
    if (!os2  ) ncols = buff[32] + (buff[33]<<8) + (buff[34]<<16) + (buff[35]<<24);
    if (!ncols) ncols = 1<<depth;

    if (ncols > (1<<depth)) { sprintf(ec->tempErrStr, "Bitmap image has a palette length of %d, which is not possible with a colour depth of %d", ncols, depth); ppl_error(ec, ERR_FILE, -1, -1, NULL); return; }

    image->pal_len = ncols;
    image->palette = ppl_memAlloc(3*image->pal_len);
    off2 += (4-os2)*image->pal_len;
    if (image->palette == NULL) { ppl_error(ec, ERR_MEMORY, -1, -1,"Out of memory"); return; }
    p = image->palette;
    for (i=0; i<image->pal_len; i+=2)
     {
      // MS Windows uses BGR0, OS/2 uses BGR
      if (fread(buff,8-2*os2,1,in)!=1) { ppl_error(ec, ERR_FILE, -1, -1,"This bitmap file appears to be corrupted"); return; }
      *(p++)=buff[2];
      *(p++)=buff[1];
      *(p++)=buff[0];
      *(p++)=buff[6-os2];
      *(p++)=buff[5-os2];
      *(p++)=buff[4-os2];
     }
    image->type   = BMP_COLOUR_PALETTE;
    image->colour = BMP_COLOUR_PALETTE;
   } else {
    image->type   = BMP_COLOUR_BMP;
    image->colour = BMP_COLOUR_RGB;
   }

  if ((depth==16)&&(encode==3)) // Must read 12 byte pseudopalette
   {
    if (fread(buff+40,12,1,in)!=1) { ppl_error(ec, ERR_FILE, -1, -1,"This bitmap file appears to be corrupted"); return; }
    off2 += 12;
   }

  if      (offset<off2) { ppl_error(ec, ERR_FILE, -1, -1, "This bitmap file appears to be corrupted"); return; }
  else if (offset>off2)
   {
    if (DEBUG) { sprintf(ec->tempErrStr, "%ld bytes of extra data", offset-off2); ppl_log(ec, NULL); }
    for ( ; off2<offset ; off2++) fgetc(in);
   }

  image->height = height;
  image->width  = width;

  dw              = (width*depth+7)/8;
  image->data_len = dw*height;
  image->depth    = depth;

  if (depth==16)
   {
    ppl_bmp_bmp16read(ec, in, buff, image);
    return;
  }

  if (!rle)
   {
    BMP_ALLOC(image->data , image->data_len);
    if (image->data==NULL) { ppl_error(ec, ERR_MEMORY, -1, -1,"Out of memory"); return; }
    for (i=1 ; i<=height ; i++)
     {
      if (fread(image->data+(height-i)*dw,dw,1,in)!=1) { ppl_error(ec, ERR_FILE, -1, -1,"This bitmap file appears to be corrupted"); return; }
      if (dw&3) { if (fread(buff,4-(dw&3),1,in)!=1) { ppl_error(ec, ERR_FILE, -1, -1,"This bitmap file appears to be corrupted"); return; } } // Lines are dword aligned
    }

    if (depth==24) // BMP is BGR not RGB
     {
      p=image->data;
      for(i=0; i<height; i++)
       {
        for(j=0; j<width; j++)
         {
          c=*p;
          *p=*(p+2);
          *(p+2)=c;
          p+=3;
         }
       }
     }
   } else {  // if (rle)
    p = ppl_memAlloc(size);
    if (p==NULL) { ppl_error(ec, ERR_MEMORY, -1, -1,"Out of memory"); image->data = NULL; return; }
    if (fread(p,size,1,in)!=1) { ppl_error(ec, ERR_FILE, -1, -1,"This bitmap file appears to be corrupted"); return; }
    if (ppl_bmp_demsrle(ec,image,p,size) != 0) { image->data = NULL; return; }
   }
  return;
 }

void ppl_bmp_bmp16read(pplerr_context *ec, FILE *in, unsigned char *header, bitmap_data *image)
 {
  unsigned char *palette;
  unsigned char *rowptr,*ptr;
  unsigned int width,height,colour;
  int is15,i,j;

  is15=1;
  width=image->width;
  height=image->height;

  if (header[16]==3)  // Read "palette" and check correct
   {                  // Palette can be 7c00 03e0 001f (15 bit)
                      // or             f800 07e0 001f (16 bit)
                      // as represents RGB bitmasks. However,
                      // Intel byte ordering afflicts the above,
                      // and the entries are stored as dwords,
                      // not words.
    palette = header+40;
    if ((palette[0]!=0) || (palette[4]!=0xe0) || (palette[8]!=0x1f) || (palette[9]!=0)) { ppl_error(ec, ERR_FILE, -1, -1, "This 16-bit bitmap file appears to be corrupted"); return; }

    if (palette[1]&0x80) is15=0;
   } else if (header[30]!=0) { ppl_error(ec, ERR_FILE, -1, -1, "This 16-bit bitmap file has invalid compression type"); return; }

  if (DEBUG)
   {
    if (is15) ppl_log(ec, "15 bit 555");
    else      ppl_log(ec, "16 bit 565");
   }

  image->depth    = 24;
  image->data_len = 3*width*height;
  BMP_ALLOC(image->data , image->data_len);
  if (image->data==NULL) { ppl_error(ec, ERR_MEMORY, -1, -1,"Out of memory"); return; }

  rowptr = ppl_memAlloc(2*width);
  if (rowptr==NULL) { ppl_error(ec, ERR_MEMORY, -1, -1,"Out of memory"); image->data=NULL; return; }

  for (i=1; i<=height; i++)
   {
    if (fread(rowptr,2*width,1,in)!=1) { ppl_error(ec, ERR_FILE, -1, -1,"This bitmap file appears to be corrupted"); return; }
    ptr = image->data+(height-i)*3*width;
    for (j=0; j<width; j++)
     {
      colour   = rowptr[2*j] + (rowptr[2*j+1]<<8);
      *(ptr+2) = (colour&0x1f)<<3; // blue
      if (is15)                    // green
       {
        *(ptr+1) = (colour&0x03e0)>>2;
       } else {
        *(ptr+1) = (colour&0x07e0)>>3;
        colour>>=1;
       }
      *ptr = (colour&0x7c00)>>7;  // red
      ptr += 3;
     }
    if (width&1) { if (fread(rowptr,2,1,in)!=1) { ppl_error(ec, ERR_FILE, -1, -1,"This bitmap file appears to be corrupted"); return; } } // rows are dword aligned
   }
  return;
 }

int ppl_bmp_demsrle(pplerr_context *ec, bitmap_data *image, unsigned char *in, unsigned long len)
 {
  unsigned char *out,*c_in,*c_out,code,*end,odd,even;
  unsigned long i,j,delta,size,height,width;
  int eol,rle;

  height = image->height;
  width  = image->width;
  rle    = image->depth;
  if ((rle!=4)&&(rle!=8)) { ppl_error(ec, ERR_FILE, -1, -1,"This bitmap file has an impossible MSRLE image depth"); return 1; }
  if (DEBUG) { sprintf(ec->tempErrStr, "Bitmap image has RLE%d compression\n",rle); ppl_log(ec, NULL); }

  image->depth = 8;
  size = height*width;
  image->data_len = size;

  BMP_ALLOC(out , image->data_len);
  if (out == NULL) { ppl_error(ec, ERR_MEMORY, -1, -1,"Out of memory"); return 1; }

  c_in  = in;
  c_out = out;

  for (i=1; i<=height; i++)
   {
    c_out = out + (height-i)*width;
    end   = c_out + width - 1;
    eol   = 0;
    while ((c_out<=end)&&(eol==0))
     {
      code = *c_in++;
      if (code) // a run
       {
        if (rle==8) odd = even = *c_in++;
        else
         {
          odd  =  *c_in&0xf;
          even = (*c_in&0xf0)>>4;
          c_in++;
         }
        for (j=0; (j<code/2)&&(c_out<=end); j++) { *c_out++=even; *c_out++=odd; }
        if (code&1) *c_out++ = even;
       } else {
        code = *c_in++;
        switch (code)
         {
          case(0): // EOL
            eol = 1;
            break;
          case(1): // EOD
            eol = 2;
            break;
          case(2): // Delta
            delta = *c_in++;    // dx
            for (j=0; (j<delta)&&(c_out<=end); j++) { *c_out++ = 0; }
            delta = *c_in++;    // dy
            if (i+delta>height) { ppl_error(ec, ERR_FILE, -1, -1,"Image overflow whilst decoding RLE in bitmap image file"); return 1; }
            for(j=0; j<delta*width; j++) { *c_out++ =0; }
            i   += delta;
            end += width*delta;
            break;
          default: // Literal
            if (rle==8)
             {
              for (j=0; (j<code)&&(c_out<=end); j++) { *c_out++ = *c_in++; }
              if (code&1) c_in++; // Keep word aligned
             }
            else
             {
              for (j=0; (j<code/2)&&(c_out<=end); j++)
               {
                *c_out++ = (*c_in&0xf0)>>4;
                *c_out++ = (*c_in++)&0xf;
               }
              if (code&1) *c_out++ = ((*c_in++)&0xf0)>>4;
              if ((code+1)&2) c_in++; // Keep word aligned
             }
         }
       }
     }
    if (eol) { sprintf(ec->tempErrStr, "Whilst decoding bitmap image file, encountered bad line length in RLE decode line=%ld\n",i); ppl_error(ec, ERR_FILE, -1, -1, NULL); return 1; }

    // Should now have an EOL or EOD code
    if (*(c_in++) != 0) { sprintf(ec->tempErrStr, "Whilst decoding bitmap image file, encountered bad line length in RLE decode line=%ld\n",i); ppl_error(ec, ERR_FILE, -1, -1, NULL); return 1; }
    if ((*c_in!=0)&&((*c_in!=1)&&(i=height))) { sprintf(ec->tempErrStr, "Whilst decoding bitmap image file, encountered bad line length in RLE decode line=%ld\n",i); ppl_error(ec, ERR_FILE, -1, -1, NULL); return 1; }
    c_in++;
   }

  image->data=out;
  return 0;
 }

