// eps_image.c
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

#define _PPL_EPS_IMAGE 1

#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <string.h>
#include <zlib.h>
#include <glob.h>
#include <wordexp.h>

#include "coreUtils/memAlloc.h"
#include "coreUtils/errorReport.h"
#include "settings/settingTypes.h"

#include "epsMaker/bmp_a85.h"
#include "epsMaker/bmp_bmpread.h"
#include "epsMaker/bmp_gifread.h"
#include "epsMaker/bmp_image.h"
#include "epsMaker/bmp_jpegread.h"
#include "epsMaker/bmp_optimise.h"
#include "epsMaker/bmp_pngread.h"

#include "epsMaker/eps_comm.h"
#include "epsMaker/eps_core.h"
#include "epsMaker/eps_image.h"
#include "epsMaker/eps_settings.h"

void eps_image_RenderEPS(EPSComm *x)
 {
  bitmap_data   data;
  FILE         *infile;
  int           ImageType, i, j;
  double        xscale, yscale, r;
  unsigned char buff[10], *imagez;
  char          filename[FNAME_LENGTH];
  uLongf        zlen; // Length of buffer passed to zlib
  static unsigned char transparency_buff[3];

  data.data = data.palette = data.trans = NULL;
  data.type = 0;
  data.XDPI = data.YDPI = 180;
  data.TargetCompression = BMP_ENCODING_FLATE;

  fprintf(x->epsbuffer, "%% Canvas item %d [bitmap image]\n", x->current->id);

  // Expand filename if it contains wildcards
   {
    wordexp_t wordExp;
    glob_t    globData;
    char     *fName = x->current->text;
    if ((wordexp(fName, &wordExp, 0) != 0) || (wordExp.we_wordc <= 0)) { sprintf(x->c->errcontext.tempErrStr, "Could not open file '%s'.", fName); ppl_error(&x->c->errcontext, ERR_FILE, -1, -1, NULL); return; }
    if ((glob(wordExp.we_wordv[0], 0, NULL, &globData) != 0) || (globData.gl_pathc <= 0)) { sprintf(x->c->errcontext.tempErrStr, "Could not open file '%s'.", fName); ppl_error(&x->c->errcontext, ERR_FILE, -1, -1, NULL); wordfree(&wordExp); return; }
    wordfree(&wordExp);
    snprintf(filename, FNAME_LENGTH, "%s", globData.gl_pathv[0]);
    filename[FNAME_LENGTH-1]='\0';
    globfree(&globData);
   }

  // Open input data file
  infile = fopen(filename, "r");
  if (infile==NULL) { sprintf(x->c->errcontext.tempErrStr, "Could not open input file '%s'", filename); ppl_error(&x->c->errcontext, ERR_FILE, -1, -1, x->c->errcontext.tempErrStr); *(x->status) = 1; return; }

  // Use magic to determine file type
  for (i=0; i<3; i++)
   {
    j = fgetc(infile);
    if (j==EOF) { sprintf(x->c->errcontext.tempErrStr, "Could not read any image data from the input file '%s'", filename); ppl_error(&x->c->errcontext, ERR_FILE, -1, -1, x->c->errcontext.tempErrStr); *(x->status) = 1; fclose(infile); return; }
    buff[i] = (unsigned char)j;
   }
  if      ((buff[0]=='G' )&&(buff[1]=='I' )&&(buff[2]=='F' )) ImageType = SW_BITMAP_GIF;
  else if ((buff[0]==0xff)&&(buff[1]==0xd8)&&(buff[2]==0xff)) ImageType = SW_BITMAP_JPG;
  else if ((buff[0]=='B' )&&(buff[1]=='M' )                 ) ImageType = SW_BITMAP_BMP;
  else if ((buff[0]==137 )&&(buff[1]=='P' )&&(buff[2]=='N' )) ImageType = SW_BITMAP_PNG;
  else
   {
    sprintf(x->c->errcontext.tempErrStr, "Could not determine the file type of input file '%s'. The image command only supports bmp, gif, jpeg and png images. The supplied image does not appear to be in any of these formats.", filename);
    ppl_error(&x->c->errcontext, ERR_FILE, -1, -1, x->c->errcontext.tempErrStr);
    *(x->status) = 1;
    fclose(infile);
    return;
   }

  // Read data from file using appropriate input filter
  switch (ImageType)
   {
    case SW_BITMAP_BMP: ppl_bmp_bmpread (&x->c->errcontext , infile , &data); break;
    case SW_BITMAP_GIF: ppl_bmp_gifread (&x->c->errcontext , infile , &data); break;
    case SW_BITMAP_JPG: ppl_bmp_jpegread(&x->c->errcontext , infile , &data); break;
    case SW_BITMAP_PNG: ppl_bmp_pngread (&x->c->errcontext , infile , &data); break;
    default: ppl_error(&x->c->errcontext, ERR_INTERNAL, -1, -1, "Unrecognised image type"); *(x->status) = 1; fclose(infile); return;
   }
  fclose(infile);

  // Check to see whether reading of data failed
  if (data.data == NULL) { *(x->status) = 1; return; }

  // Apply palette optimisations to images if possible
  if ((data.depth ==  8) && (data.type==BMP_COLOUR_PALETTE)) ppl_bmp_palette_check(&x->c->errcontext , &data); // If we did not construct palette, check for trailing unused entries
  if ((data.depth == 24) && (data.type==BMP_COLOUR_BMP    )) ppl_bmp_colour_count(&x->c->errcontext , &data);  // Check full color image to ensure more than 256 colors
  if ((data.depth ==  8) && (data.type==BMP_COLOUR_PALETTE)) ppl_bmp_grey_check(&x->c->errcontext , &data);    // Check paletted images for greyscale conversion
  if ((data.type == BMP_COLOUR_PALETTE) && (data.pal_len <= 16) && (data.depth == 8)) ppl_bmp_compact(&x->c->errcontext , &data); // Compact images with few palette entries

  // If user has specified a transparent color, change transparency properties now
  if (x->current->CustomTransparency)
   {
    data.trans = NULL; // Turn off any transparency which may have been present in the original image
    if (data.colour == BMP_COLOUR_PALETTE)
     {
      for(i=0; i<data.pal_len; i++)
       if ( ((unsigned char)x->current->TransColR == data.palette[3*i  ]) &&
            ((unsigned char)x->current->TransColG == data.palette[3*i+1]) &&
            ((unsigned char)x->current->TransColB == data.palette[3*i+2])    )
        {
         data.trans  = transparency_buff;
         *data.trans = (unsigned char)i;
         break;
        }
     }
    else if (data.colour == BMP_COLOUR_RGB) // RGB
     {
      data.trans    = transparency_buff;
      data.trans[0] = (unsigned char)x->current->TransColR;
      data.trans[1] = (unsigned char)x->current->TransColG;
      data.trans[2] = (unsigned char)x->current->TransColB;
     }
    else if ((data.colour == BMP_COLOUR_GREY) && (x->current->TransColR==x->current->TransColG) && (x->current->TransColR==x->current->TransColB)) // Greyscale
     {
      int magic;

      if      (data.depth == 1) { magic=255   ; } //   2-color greyscale
      else if (data.depth == 2) { magic=255/ 3; } //   4-color greyscale
      else if (data.depth == 4) { magic=255/15; } //  16-color greyscale
      else                      { magic=  1   ; } // 256-color greyscale

      if ((x->current->TransColR % magic)==0)
       {
        data.trans    = transparency_buff;
        data.trans[0] = (unsigned char)(x->current->TransColR / magic);
       }
     }
   }

  // Apply compression to image data
  switch (data.TargetCompression)
   {
    case BMP_ENCODING_NULL: break; // No image compression
    case BMP_ENCODING_DCT: break; // Special case: JPEG data is encoded in DCT, but is already supplied from ppl_bmp_jpegread() in encoded form
    case BMP_ENCODING_FLATE:
      zlen   = data.data_len*1.01+12; // Nasty guess at size of buffer needed.
      imagez = (unsigned char *)ppl_memAlloc(zlen);
      if (imagez == NULL) { ppl_error(&x->c->errcontext, ERR_MEMORY, -1, -1,"Out of memory"); return; }
      if (DEBUG) { ppl_log(&x->c->errcontext, "Calling zlib to compress image data"); }
      j = compress2(imagez,&zlen,data.data,data.data_len,9); // Call zlib to do deflation

      if (j!=0)
       {
        if (DEBUG) { sprintf(x->c->errcontext.tempErrStr, "zlib returned error code %d\n",j); ppl_log(&x->c->errcontext, NULL); }
        data.TargetCompression = BMP_ENCODING_NULL; // Give up trying to compress data
        break;
       }
      if (DEBUG) { sprintf(x->c->errcontext.tempErrStr, "zlib has completed compression. Before flate: %ld bytes. After flate: %ld bytes", data.data_len, (long)zlen); ppl_log(&x->c->errcontext, NULL); }
      if (zlen >= data.data_len)
       {
        if (DEBUG) { ppl_log(&x->c->errcontext, "Using original uncompressed data since zlib made it bigger than it was to start with"); }
        data.TargetCompression = BMP_ENCODING_NULL; // Give up trying to compress data; result was larger than original data size
        break;
       }
      data.data = imagez; // Replace old data with new compressed data
      data.data_len = zlen;
      break;
    default:
      ppl_error(&x->c->errcontext, ERR_INTERNAL, -1, -1, "Unrecognised image compression type requested"); *(x->status) = 1; return;
   }

  // Work out dimensions of image
  if ((x->current->xpos2set) && (x->current->ypos2set)) // Both width and height have been specified
   {
    xscale = x->current->xpos2 * M_TO_PS;
    yscale = x->current->ypos2 * M_TO_PS;
   }
  else if (x->current->xpos2set) // Only width has been set
   {
    xscale = x->current->xpos2 * M_TO_PS;
    yscale = x->current->xpos2 * M_TO_PS * ((double)data.height / (double)data.width);
   }
  else if (x->current->ypos2set) // Only height has been set
   {
    xscale = x->current->ypos2 * M_TO_PS * ((double)data.width / (double)data.height);
    yscale = x->current->ypos2 * M_TO_PS;
   }
  else // Neither height nor width has been set; use DPI information to work out how big image should be
   {
    xscale = ((double)data.width ) / data.XDPI * 72;
    yscale = ((double)data.height) / data.YDPI * 72;
   }

  // Make it into postscript
  fprintf(x->epsbuffer, "gsave\n");
  fprintf(x->epsbuffer, "%.2f %.2f translate\n", x->current->xpos * M_TO_PS, x->current->ypos * M_TO_PS);
  fprintf(x->epsbuffer, "%.2f rotate\n", x->current->rotation * 180 / M_PI);
  fprintf(x->epsbuffer, "%.2f %.2f scale\n", xscale, yscale); // We render image onto a unit square; use scale to make it the size we actually want

  if      (data.colour == BMP_COLOUR_RGB ) fprintf(x->epsbuffer, "/DeviceRGB setcolorspace\n");  // RGB palette
  else if (data.colour == BMP_COLOUR_GREY) fprintf(x->epsbuffer, "/DeviceGray setcolorspace\n"); // Greyscale palette
  else if (data.colour == BMP_COLOUR_PALETTE) // Indexed palette
   {
    fprintf(x->epsbuffer, "[/Indexed /DeviceRGB %d <~\n", data.pal_len-1);
    ppl_bmp_A85(&x->c->errcontext, x->epsbuffer, data.palette, 3*data.pal_len);
    fprintf(x->epsbuffer, "] setcolorspace\n\n");
   }

  fprintf(x->epsbuffer, "<<\n /ImageType %d\n /Width %d\n /Height %d\n /ImageMatrix [%d 0 0 %d 0 %d]\n", ((!x->current->NoTransparency) && (data.trans!=NULL))?4:1, data.width, data.height, data.width, -data.height, data.height);
  fprintf(x->epsbuffer, " /DataSource currentfile /ASCII85Decode filter"); // Image data is stored in currentfile, but need to apply filters to decode it
  if      (data.TargetCompression == BMP_ENCODING_DCT  ) fprintf(x->epsbuffer, " /DCTDecode filter");
  else if (data.TargetCompression == BMP_ENCODING_FLATE) fprintf(x->epsbuffer, " /FlateDecode filter");
  fprintf(x->epsbuffer, "\n /BitsPerComponent %d\n /Decode [0 %d%s]\n", (data.colour==BMP_COLOUR_RGB)?(data.depth/3):(data.depth),
                                                                        (data.type==BMP_COLOUR_PALETTE)?((1<<data.depth)-1):1,
                                                                        (data.colour==BMP_COLOUR_RGB)?" 0 1 0 1":"");
  if ((!x->current->NoTransparency) && (data.trans != NULL)) // If image has transparency, set mask color
   {
    fprintf(x->epsbuffer," /MaskColor [");
    if (data.colour == BMP_COLOUR_RGB) fprintf(x->epsbuffer, "%d %d %d]\n",(int)data.trans[0], (int)data.trans[1], (int)data.trans[2]);
    else                               fprintf(x->epsbuffer, "%d]\n"      ,(int)data.trans[0]);
   }
  if (x->current->smooth) fprintf(x->epsbuffer, " /Interpolate true\n"); // If image has smooth flag set, tell postscript interpretter to interpolate image
  fprintf(x->epsbuffer, ">> image\n");
  ppl_bmp_A85(&x->c->errcontext, x->epsbuffer, data.data, data.data_len);
  fprintf(x->epsbuffer, "grestore\n");

  // Update postscript bounding box
  r = x->current->rotation;
  eps_core_BoundingBox(x, x->current->xpos*M_TO_PS                                  , x->current->ypos*M_TO_PS                                 , 0);
  eps_core_BoundingBox(x, x->current->xpos*M_TO_PS + xscale*cos(r)                  , x->current->ypos*M_TO_PS + xscale*sin(r)                 , 0);
  eps_core_BoundingBox(x, x->current->xpos*M_TO_PS                 + yscale*-sin(r) , x->current->ypos*M_TO_PS                 + yscale*cos(r) , 0);
  eps_core_BoundingBox(x, x->current->xpos*M_TO_PS + xscale*cos(r) + yscale*-sin(r) , x->current->ypos*M_TO_PS + xscale*sin(r) + yscale*cos(r) , 0);

  // Final newline at end of canvas item
  fprintf(x->epsbuffer, "\n");
  return;
 }

