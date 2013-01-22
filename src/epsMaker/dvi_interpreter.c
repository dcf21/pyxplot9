// dvi_interpreter.c
//
// The code in this file is part of Pyxplot
// <http://www.pyxplot.org.uk>
//
// Copyright (C) 2006-2013 Dominic Ford <coders@pyxplot.org.uk>
//               2008-2013 Ross Church
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

// Functions for manupulating dvi interpreters

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

#include "coreUtils/errorReport.h"
#include "coreUtils/list.h"
#include "coreUtils/memAlloc.h"
#include "stringTools/strConstants.h"
#include "epsMaker/dvi_interpreter.h"
#include "epsMaker/dvi_font.h"
#include "epsMaker/dvi_read.h"

// Table of the operator functions to allow quick lookup without a long if ... else if ... statement
int (*dviOpTable[58])(pplerr_context *ec, dviInterpreterState *interp, DVIOperator *op);

void makeDviOpTable(pplerr_context *ec)
 {
  dviOpTable[0] = dviInOpSet1234;
  dviOpTable[1] = dviInOpSet1234;
  dviOpTable[2] = dviInOpSet1234;
  dviOpTable[3] = dviInOpSet1234;
  dviOpTable[4] = dviInOpSetRule;
  dviOpTable[5] = dviInOpPut1234;
  dviOpTable[6] = dviInOpPut1234;
  dviOpTable[7] = dviInOpPut1234;
  dviOpTable[8] = dviInOpPut1234;
  dviOpTable[9] = dviInOpPutRule;
  dviOpTable[10] = dviInOpNop;
  dviOpTable[11] = dviInOpBop;
  dviOpTable[12] = dviInOpEop;
  dviOpTable[13] = dviInOpPush;
  dviOpTable[14] = dviInOpPop;
  dviOpTable[15] = dviInOpRight1234;
  dviOpTable[16] = dviInOpRight1234;
  dviOpTable[17] = dviInOpRight1234;
  dviOpTable[18] = dviInOpRight1234;
  dviOpTable[19] = dviInOpW0;
  dviOpTable[20] = dviInOpW1234;
  dviOpTable[21] = dviInOpW1234;
  dviOpTable[22] = dviInOpW1234;
  dviOpTable[23] = dviInOpW1234;
  dviOpTable[24] = dviInOpX0;
  dviOpTable[25] = dviInOpX1234;
  dviOpTable[26] = dviInOpX1234;
  dviOpTable[27] = dviInOpX1234;
  dviOpTable[28] = dviInOpX1234;
  dviOpTable[29] = dviInOpDown1234;
  dviOpTable[30] = dviInOpDown1234;
  dviOpTable[31] = dviInOpDown1234;
  dviOpTable[32] = dviInOpDown1234;
  dviOpTable[33] = dviInOpY0;
  dviOpTable[34] = dviInOpY1234;
  dviOpTable[35] = dviInOpY1234;
  dviOpTable[36] = dviInOpY1234;
  dviOpTable[37] = dviInOpY1234;
  dviOpTable[38] = dviInOpZ0;
  dviOpTable[39] = dviInOpZ1234;
  dviOpTable[40] = dviInOpZ1234;
  dviOpTable[41] = dviInOpZ1234;
  dviOpTable[42] = dviInOpZ1234;
  // Big hole here where we ignore FONTi
  dviOpTable[43] = dviInOpFnt1234;
  dviOpTable[44] = dviInOpFnt1234;
  dviOpTable[45] = dviInOpFnt1234;
  dviOpTable[46] = dviInOpFnt1234;
  dviOpTable[47] = dviInOpSpecial1234;
  dviOpTable[48] = dviInOpSpecial1234;
  dviOpTable[49] = dviInOpSpecial1234;
  dviOpTable[50] = dviInOpSpecial1234;
  dviOpTable[51] = dviInOpFntdef1234;
  dviOpTable[52] = dviInOpFntdef1234;
  dviOpTable[53] = dviInOpFntdef1234;
  dviOpTable[54] = dviInOpFntdef1234;
  dviOpTable[55] = dviInOpPre;
  dviOpTable[56] = dviInOpPost;
  dviOpTable[57] = dviInOpPostPost;
  return;
 }

// Function to interpret an operator. This is a wrapper round the functions below.
int dviInterpretOperator(pplerr_context *ec, dviInterpreterState *interp, DVIOperator *op)
 {
  int (*func)(pplerr_context *ec, dviInterpreterState *interp, DVIOperator *op) = NULL;
  int i=0;
  int err;
  char errStr[SSTR_LENGTH];

  // Deal with the processing of DVI extensions (DIV_XXX/SPECIAL)
  if (interp->special > 0)
   {
    if ((op->op <= DVI_CHARMAX) && (interp->specialLen-->0))
     {
      return dviSpecialChar(ec, interp, op);
     } else {
      // The following function turns the special flag off, and hence op is evaluated below
      if ((err=dviSpecialImplement(ec, interp)) != 0) return err;
     }
   }

  // This if statement extends the lookup table of operator functions
  if (op->op <= DVI_CHARMAX)
   {
    func = dviInOpChar;
   }
  else if (op->op < DVI_FNTNUMMIN)
   {
    i = op->op-DVI_CHARMAX-1;
    func = (dviOpTable[i]);
   }
  else if (op->op < DVI_FNTNUMMAX)
   {
    func = dviInOpFnt;
   }
  else if (op->op <= DVI_POSTPOST)
   {
    i = op->op-DVI_CHARMAX-1-(DVI_FNTNUMMAX-DVI_FNTNUMMIN+1);
    func = (dviOpTable[i]);
   }
  else
   {
    ppl_error(ec, ERR_INTERNAL, -1, -1,"dvi interpreter found an illegal operator");
    return DVIE_CORRUPT;
   }
  if (func==NULL)
   {
    snprintf(errStr, SSTR_LENGTH, "Failed to find handler for dvi operator %d i=%d", op->op, i);
    ppl_error(ec, ERR_INTERNAL, -1, -1,errStr);
    return DVIE_CORRUPT;
   }

  // If we are not typesetting a character and moving right, check if we need to set accumulated text
  if (op->op > DVI_SET1234+3 && interp->currentString != NULL)
   if ((err=dviTypeset(ec, interp))!=0) return err;

  // Call the function to interpret the operator
  return (*func)(ec, interp, op);
 }

// ----------------------------------
// Functions that implement operators
// ----------------------------------

// Typeset a special character
int dviNonAsciiChar(pplerr_context *ec, dviInterpreterState *interp, int c, char move)
 {
  char s[64];
  int err;
  dviStackState *postPos, *dviPos;   // Current positions in dvi and postscript code
  double width, height, depth, italic, size[4];

  postPos = interp->output->currentPosition;
  dviPos = interp->state;

  // First check if we need to move before typesetting
  if (postPos== NULL)
   {
    dviPostscriptMoveto(ec, interp);
    interp->output->currentPosition = dviCloneInterpState(ec, dviPos);
   }
  else if (postPos->h != dviPos->h || postPos->v != dviPos->v)
   {
    dviPostscriptMoveto(ec, interp);
   }

  // Update bounding box
  dviGetCharSize(ec, interp, (char)c, size);
  width  = size[0] / interp->scale; // Convert back into dvi units
  height = size[1] / interp->scale;
  depth  = size[2] / interp->scale;
  italic = size[3] / interp->scale;
  if (DEBUG) { sprintf(ec->tempErrStr, "DVI: width of glyph %g height of glyph %g", width, height); ppl_log(ec, NULL); }
  if ((err=dviUpdateBoundingBox(ec, interp, width+italic, height, depth))!=0) return err;

  // Count the number of characters to write to the ps string
  snprintf(s, 64, "(\\%o) show\n", c);

  // Send the string off to the postscript routine and clean up memory
  if ((err=dviPostscriptAppend(ec, interp, s))!=0) return err;
  //free(interp->currentString);
  interp->currentString = NULL;
  interp->currentStrlen = 0;

  // Adjust the current position
  if (move == DVI_YES)
   {
    interp->state->h += width;
    interp->output->currentPosition->h += width;
   }
  return 0;
 }

// Interpreter functions for various types of dvi operators
int dviInOpChar(pplerr_context *ec, dviInterpreterState *interp, DVIOperator *op)
 {
  int charToTypeset = op->op;
  int err;
  char *s;

  // Typeset non-printable characters separately
  if (  (charToTypeset< 48) ||
       ((charToTypeset> 57) && (charToTypeset<65)) ||
       ((charToTypeset> 90) && (charToTypeset<97)) ||
        (charToTypeset>126)                           )
   {
    // Clear the queue if there's anything on it
    if (interp->currentString != NULL)
     if ((err=dviTypeset(ec, interp))!=0) return err;
    if ((err=dviNonAsciiChar(ec, interp, charToTypeset, DVI_YES))!=0) return err;
    return 0;
   }

  // See if we have anywhere to put the character
  if (interp->currentString == NULL)
   {
    interp->currentString = (char *)ppl_memAlloc(SSTR_LENGTH);
    if (interp->currentString==NULL) { ppl_error(ec, ERR_MEMORY, -1, -1,"Out of memory"); return DVIE_MEMORY; }
    *(interp->currentString) = '\0';
    interp->currentStrlen = LSTR_LENGTH;
   }
  else if (strlen(interp->currentString) == interp->currentStrlen-2) // If the string is full, extend it
   {
    if ((err=dviTypeset(ec, interp))!=0) return err;
    interp->currentString = (char *)ppl_memAlloc(SSTR_LENGTH);
    if (interp->currentString==NULL) { ppl_error(ec, ERR_MEMORY, -1, -1,"Out of memory"); return DVIE_MEMORY; }
    *(interp->currentString) = '\0';
    interp->currentStrlen = LSTR_LENGTH;
   }
  // Write the character to the string
  s = interp->currentString+strlen(interp->currentString); // s now points to the \0
  if      (charToTypeset == 40) snprintf(s, 3, "%s", "\\(");
  else if (charToTypeset == 41) snprintf(s, 3, "%s", "\\)");
  else                          snprintf(s, 2, "%c", charToTypeset);
  return 0;
 }

// DVI_SET1234
int dviInOpSet1234(pplerr_context *ec, dviInterpreterState *interp, DVIOperator *op)
 {
  return dviNonAsciiChar(ec, interp, op->ul[0], DVI_YES);
 }

// DVI_SETRULE
// Set a rule and move right
int dviInOpSetRule(pplerr_context *ec, dviInterpreterState *interp, DVIOperator *op)
 {
  int err=0;

  // Don't set a rule if movements are -ve
  if (op->sl[0]<0 || op->sl[1]<0)
   {
    if (DEBUG) ppl_log(ec, "DVI: silent rule");
    interp->state->h += op->sl[1];
    if ((err=dviPostscriptMoveto(ec, interp))!=0) return err;
   }
  else
   {
    if ((err=dviUpdateBoundingBox(ec, interp, (int)op->sl[1], (int)op->sl[0], 0.0))!=0) return err;
    if ((err=dviPostscriptMoveto (ec, interp)                                     )!=0) return err;
    interp->state->v -= op->sl[0];
    if ((err=dviPostscriptLineto       (ec, interp))!=0) return err;
    interp->state->h += op->sl[1];
    if ((err=dviPostscriptLineto       (ec, interp))!=0) return err;
    interp->state->v += op->sl[0];
    if ((err=dviPostscriptLineto       (ec, interp))!=0) return err;
    if ((err=dviPostscriptClosepathFill(ec, interp))!=0) return err;
   }
  return err;
 }

// DVI_PUT
int dviInOpPut1234(pplerr_context *ec, dviInterpreterState *interp, DVIOperator *op)
 {
  return dviNonAsciiChar(ec, interp, op->ul[0], DVI_NO);
 }

// DVI_PUTRULE
// Set a rule and don't move right
int dviInOpPutRule(pplerr_context *ec, dviInterpreterState *interp, DVIOperator *op)
 {
  int err=0;

  // Don't set a rule if movements are -ve
  if (op->sl[0]<0 || op->sl[1]<0)
   {
    if (DEBUG) ppl_log(ec, "DVI: silent rule");
   }
  else
   {
    if ((err=dviUpdateBoundingBox(ec, interp, (int)op->sl[1], (int)op->sl[0], 0.0))!=0) return err;
    if ((err=dviPostscriptMoveto (ec, interp)                                     )!=0) return err;
    interp->state->v -= op->sl[0];
    if ((err=dviPostscriptLineto       (ec, interp))!=0) return err;
    interp->state->h += op->sl[1];
    if ((err=dviPostscriptLineto       (ec, interp))!=0) return err;
    interp->state->v += op->sl[0];
    if ((err=dviPostscriptLineto       (ec, interp))!=0) return err;
    if ((err=dviPostscriptClosepathFill(ec, interp))!=0) return err;
    interp->state->h -= op->sl[1];
   }
  return err;
 }

// DVI_NOP
int dviInOpNop(pplerr_context *ec, dviInterpreterState *interp, DVIOperator *op)
 {
  return 0;
 }

// DVI_BOP
int dviInOpBop(pplerr_context *ec, dviInterpreterState *interp, DVIOperator *op)
 {
  // Generate a new page
  postscriptPage *p;
  p = dviNewPostscriptPage(ec);
  if (p == NULL) return DVIE_MEMORY;
  ppl_listAppend(interp->output->pages, (void *)p);
  interp->output->currentPage = p;
  interp->output->Npages++;

  // Check that the stack is empty
  if (ppl_listLen(interp->stack)>0)
   {
    ppl_warning(ec,ERR_INTERNAL,"malformed DVI file: stack not empty at start of new page");
    interp->stack = ppl_listInit(0);
   }

  // There should not be a string in progress on the stack
  if (interp->currentStrlen != 0)
   {
    ppl_warning(ec,ERR_INTERNAL,"error in DVI interpreter: string on stack at newpage");
    interp->currentStrlen = 0;
    //free(interp->currentString);
    interp->currentString = NULL;
   }

  // Set the default state values
  interp->state->h = 0;
  interp->state->v = 0;
  interp->state->w = 0;
  interp->state->x = 0;
  interp->state->y = 0;
  interp->state->z = 0;
  // Leave f as it is undefined after a new page
  return 0;
 }

// DVI_EOP
int dviInOpEop(pplerr_context *ec, dviInterpreterState *interp, DVIOperator *op)
 {
  double *bb;

  // Set appropriate postscript bounding box from dvi bb
  // left bottom right top
  bb = interp->boundingBox;
  // If we have not typeset anything then the bounding box will be empty.
  if (bb == NULL)
   {
    bb = (double *)ppl_memAlloc(4*sizeof(double));
    if (bb==NULL) { ppl_error(ec, ERR_MEMORY, -1, -1, "Out of memory"); return DVIE_MEMORY; }
    bb[0] = interp->state->h;
    bb[1] = interp->state->v;
    bb[2] = interp->state->h;
    bb[3] = interp->state->v;
   }

  // Convert bounding box from DVI to PS units
  bb[0] *= interp->scale;
  bb[1] = 765 - bb[1] * interp->scale;
  bb[2] *= interp->scale;
  bb[3] = 765 - bb[3] * interp->scale;

  // Move pointer to postscript
  interp->output->currentPage->boundingBox = bb;
  interp->boundingBox = NULL;

  if (DEBUG) { sprintf(ec->tempErrStr, "Postscript page: bounding box %f %f %f %f", bb[0], bb[1], bb[2], bb[3]); ppl_log(ec, NULL); }

  // Now repeat for text size box
  bb     = interp->textSizeBox;
  // If we have not typeset anything then the box will be empty.
  if (bb == NULL) {
    bb = (double *)ppl_memAlloc(4*sizeof(double));
    if (bb==NULL) { ppl_error(ec, ERR_MEMORY, -1, -1, "Out of memory"); return DVIE_MEMORY; }
    bb[0] = interp->state->h;
    bb[1] = interp->state->v;
    bb[2] = interp->state->h;
    bb[3] = interp->state->v;
  }

  bb[0] *= interp->scale;
  bb[1]  = 765 - bb[1] * interp->scale;
  bb[2] *= interp->scale;
  bb[3]  = 765 - bb[3] * interp->scale;

  // Move pointer to postscript
  interp->output->currentPage->textSizeBox = bb;
  interp->textSizeBox = NULL;

  if (DEBUG) { sprintf(ec->tempErrStr, "Postscript page: text size box %f %f %f %f", bb[0], bb[1], bb[2], bb[3]); ppl_log(ec, NULL); }

  return 0;
 }

// DVI_PUSH
int dviInOpPush(pplerr_context *ec, dviInterpreterState *interp, DVIOperator *op)
 {
  // Push the current state on to the stack
  dviStackState *new;
  new = dviCloneInterpState(ec, interp->state);
  if (new == NULL) return DVIE_MEMORY;
  ppl_listAppend(interp->stack, (void *)new);
  return 0;
 }

// DVI_POP
int dviInOpPop(pplerr_context *ec, dviInterpreterState *interp, DVIOperator *op)
 {
  // Pop the previous state off the stack
  dviStackState *new;
  new = (dviStackState *)ppl_listPop(interp->stack);
  if (new == NULL)
   {
    ppl_error(ec, ERR_INTERNAL, -1, -1,"corrupt dvi file -- attempt to pop off empty stack");
    return 1;
   }
  interp->state = new;

  // Unset the special flag
  interp->special = 0;
  return 0;
 }

// DVI_RIGHT1234
int dviInOpRight1234(pplerr_context *ec, dviInterpreterState *interp, DVIOperator *op)
 {
  interp->state->h += op->sl[0];
  return 0;
 }

// DVI_W0
int dviInOpW0(pplerr_context *ec, dviInterpreterState *interp, DVIOperator *op)
 {
  interp->state->h += interp->state->w;
  return 0;
 }

// DVI_W1234
int dviInOpW1234(pplerr_context *ec, dviInterpreterState *interp, DVIOperator *op)
 {
  interp->state->w = op->sl[0];
  interp->state->h += interp->state->w;
  return 0;
 }

// DVI_X0
int dviInOpX0(pplerr_context *ec, dviInterpreterState *interp, DVIOperator *op)
 {
  interp->state->h += interp->state->x;
  return 0;
 }

// DVI_X1234
int dviInOpX1234(pplerr_context *ec, dviInterpreterState *interp, DVIOperator *op)
 {
  interp->state->x = op->sl[0];
  interp->state->h += interp->state->x;
  return 0;
 }

// DVI_DOWN1234
int dviInOpDown1234(pplerr_context *ec, dviInterpreterState *interp, DVIOperator *op)
 {
  interp->state->v += op->sl[0];
  return 0;
 }

// DVI_Y0
int dviInOpY0(pplerr_context *ec, dviInterpreterState *interp, DVIOperator *op)
 {
  interp->state->v += interp->state->y;
  return 0;
 }

// DVI_Y1234
int dviInOpY1234(pplerr_context *ec, dviInterpreterState *interp, DVIOperator *op)
 {
  interp->state->y = op->sl[0];
  interp->state->v += interp->state->y;
  return 0;
 }

// DVI_Z0
int dviInOpZ0(pplerr_context *ec, dviInterpreterState *interp, DVIOperator *op)
 {
  interp->state->v += interp->state->z;
  return 0;
 }

// DVI_Z1234
int dviInOpZ1234(pplerr_context *ec, dviInterpreterState *interp, DVIOperator *op)
 {
  interp->state->z = op->sl[0];
  interp->state->v += interp->state->z;
  return 0;
 }

// DVI_FNTi
int dviInOpFnt(pplerr_context *ec, dviInterpreterState *interp, DVIOperator *op)
 {
  int fn;
  fn = op->op - DVI_FNTNUMMIN;
  return dviChngFnt(ec, interp, fn);
 }

// DVI_FNT1234
int dviInOpFnt1234(pplerr_context *ec, dviInterpreterState *interp, DVIOperator *op)
 {
  dviChngFnt(ec, interp, op->ul[0]);
  return 0;
 }

// DVI_SPECIAL1234
int dviInOpSpecial1234(pplerr_context *ec, dviInterpreterState *interp, DVIOperator *op)
 {
  int spesh;
  spesh = op->op - DVI_SPECIAL1234+1;
  interp->special = spesh;
  interp->specialLen = op->ul[0];
  interp->spString = (char *)ppl_memAlloc(SSTR_LENGTH);
  if (interp->spString==NULL) { ppl_error(ec, ERR_MEMORY, -1, -1,"Out of memory"); return DVIE_MEMORY; }
  *(interp->spString) = '\0';
  if (DEBUG) { sprintf(ec->tempErrStr, "DVI: dvi special: %d %lu %d", spesh, op->ul[0], (int)strlen(interp->spString)); ppl_log(ec, NULL); }
  // NOP
  return 0;
 }

// DVI_FNTDEF1234
int dviInOpFntdef1234(pplerr_context *ec, dviInterpreterState *interp, DVIOperator *op)
 {
  // XXX Should perhaps check that we're not re-defining an existing font here
  // XXX Could check that we're not after POST and hence that we need to do this
  dviFontDetails *font;
  int i;

  font = (dviFontDetails *)ppl_memAlloc(sizeof(dviFontDetails));
  if (font==NULL) { ppl_error(ec, ERR_MEMORY, -1, -1, "Out of memory"); return DVIE_MEMORY; }
  ppl_listAppend(interp->fonts, (void *)font);

  // Populate with information from operator
  font->number = op->ul[0];
  font->area = (char *)ppl_memAlloc(op->ul[4]+1);
  if (font->area==NULL) { ppl_error(ec, ERR_MEMORY, -1, -1, "Out of memory"); return DVIE_MEMORY; }
  for (i=0; i<op->ul[4]; i++) font->area[i] = op->s[0][i];
  font->area[op->ul[4]] = '\0';
  font->name = (char *)ppl_memAlloc(op->ul[5]+1);
  if (font->name==NULL) { ppl_error(ec, ERR_MEMORY, -1, -1, "Out of memory"); return DVIE_MEMORY; }
  for (i=0; i<op->ul[5]; i++) font->name[i] = op->s[0][op->ul[4]+i];
  font->name[op->ul[5]] = '\0';
  font->useSize = op->ul[2];
  font->desSize = op->ul[3];

  // parse the TFM file for useful data
  return dviGetTFM(ec, font);
 }

// DVI_PRE
int dviInOpPre(pplerr_context *ec, dviInterpreterState *interp, DVIOperator *op)
 {
  unsigned long i, num, den, mag;
  i = op->ul[0];
  num = op->ul[1];
  den = op->ul[2];
  mag = op->ul[3];
  if (i != 2)
   {
    ppl_error(ec, ERR_INTERNAL, -1, -1,"Error interpreting dvi file: not dvi version 2");
    return 1;
   }
  // Convert mag, num and den into points (for ps)
  interp->scale = (double)mag / 1000.0 * (double)num / (double)den / 1.0e3 * 72.0 / 254;
  if (DEBUG) { sprintf(ec->tempErrStr, "DVI: Scale %g V=%lu num=%lu den=%lu mag=%lu", interp->scale,i,num,den,mag); ppl_log(ec, NULL); }
  return 0;
 }

// DVI_POST -- nop
int dviInOpPost(pplerr_context *ec, dviInterpreterState *interp, DVIOperator *op)
 {
  return 0;
 }

// DVI_POSTPOST -- nop
int dviInOpPostPost(pplerr_context *ec, dviInterpreterState *interp, DVIOperator *op)
 {
  return 0;
 }

// ----------------------------------------------------------
// Functions for implementing special operators (DVI_SPECIAL)
// ----------------------------------------------------------

// Accumulate characters output in special mode into a string
int dviSpecialChar(pplerr_context *ec, dviInterpreterState *interp, DVIOperator *op)
 {
  char *s;
  s = interp->spString+strlen(interp->spString);
  snprintf(s, SSTR_LENGTH, "%c", op->op);
  return 0;
 }

// Implement an accumulated special-mode command
int dviSpecialImplement(pplerr_context *ec, dviInterpreterState *interp)
 {
  int err = 0;
  char errString[SSTR_LENGTH];

  if (DEBUG) { sprintf(ec->tempErrStr, "DVI: dvi special. Final string=%s", interp->spString); ppl_log(ec, NULL); }
  // Test for a colour string
  if (strncmp(interp->spString, "color ", 6) == 0)
   {
    if ((err=dviSpecialColourCommand(ec, interp, interp->spString+6))!=0) return err;
   }
  else
   {
    // Unhandled special command (e.g. includegraphics)
    snprintf(errString, SSTR_LENGTH, "ignoring unhandled DVI special string %s", interp->spString);
    ppl_warning(ec, ERR_GENERIC, errString);
   }

  // Clean up
  //free(interp->spString);
  interp->spString = NULL;
  interp->special = 0;
  return err;
 }

// Handle latex colour commands
int dviSpecialColourCommand(pplerr_context *ec, dviInterpreterState *interp, char *command)
 {
  char psText[SSTR_LENGTH];

  // Skip over any leading spaces
  while (command[0] == ' ') command++;

  // See what sort of colour command it is
  if (strncmp(command, "push ", 4)==0)
   {
    // New colour to push onto stack
    if (DEBUG) { sprintf(ec->tempErrStr, "DVI: %s says push", command); ppl_log(ec, NULL); }
    command += 5;
    while (command[0] == ' ') command++;
    if (strncmp(command, "cmyk ", 5)==0)
     {
      command += 5;
      snprintf(psText, SSTR_LENGTH, "%s setcmykcolor\n", command); // CMKY colour
     }
    else if (strncmp(command, "rgb ", 4)==0)
     {
      command += 4;
      snprintf(psText, SSTR_LENGTH, "%s setrgbcolor\n", command); // rgb colour
     }
    else if (strncmp(command, "Black", 5)==0)
     {
      snprintf(psText, SSTR_LENGTH, "0 0 0 setrgbcolor\n"); // black
     }
    else if (strncmp(command, "gray ", 5)==0 || strncmp(command, "grey ", 5)==0) // grey colour
     {
      command += 5;
      snprintf(psText, SSTR_LENGTH, "%s %s %s setrgbcolor\n", command, command, command);
     }
    else
     {
      snprintf(psText, SSTR_LENGTH, "failed to interpret dvi colour %s", command);
      ppl_warning(ec,ERR_INTERNAL,psText);
      return 0;
     }
    return dviSpecialColourStackPush(ec, interp, psText);
   }
  else if (strncmp(command, "pop", 3)==0)
   {
    return dviSpecialColourStackPop(ec, interp); // Pop a colour off the stack
   }
  else
   {
    snprintf(psText, SSTR_LENGTH, "ignoring incomprehensible dvi colour command %s", command);
    ppl_warning(ec,ERR_INTERNAL,psText);
    return 0;
   }
  return 0;
 }

// Pop a colour instruction off the end of the stack
int dviSpecialColourStackPop(pplerr_context *ec, dviInterpreterState *interp)
 {
  int err=0;
  char *item;

  // Lop last colour off the end of the colour stack
  item = (char *)ppl_listPop(interp->colStack);
  if (item==NULL)
   {
    ppl_warning(ec,ERR_INTERNAL,"dvi colour pop from empty stack");
    return 0;
   }

  // Read last colour from stack
  item = (char *)ppl_listLast(interp->colStack);

  // Set colour to item on top of stack
  if (item==NULL)
   {
    // Hit the bottom of the colour stack; returning to default colour. Spit out ASCII x01, and this will be dealt with later
    if ((err=dviPostscriptAppend(ec, interp, "\x01\n"))!=0) return err;
   }
  else
   {
    if ((err=dviPostscriptAppend(ec, interp, item))!=0) return err;
   }
  return 0;
 }

// Push a colour onto the colour stack
int dviSpecialColourStackPush(pplerr_context *ec, dviInterpreterState *interp, char *psText)
 {
  char *s;
  int len, err=0;

  // Stick the string onto the stack
  len = strlen(psText)+1;
  s = (char *)ppl_memAlloc(len);
  if (s==NULL) { ppl_error(ec, ERR_MEMORY, -1, -1, "Out of memory"); return DVIE_MEMORY; }
  snprintf(s, len, "%s", psText);
  ppl_listAppend(interp->colStack, (void *)s);

  // Also append to the postscript stack
  if ((err=dviPostscriptAppend(ec, interp, psText))!=0) return err;
  return 0;
 }

// ---------------------------------------
// Functions for manipulating interpreters
// ---------------------------------------

// Produce a new dvi interpreter (for a new dvi file, say)
dviInterpreterState *dviNewInterpreter(pplerr_context *ec)
 {
  dviInterpreterState *interp;
  interp = (dviInterpreterState *)ppl_memAlloc(sizeof(dviInterpreterState));
  if (interp==NULL) { ppl_error(ec, ERR_MEMORY, -1, -1, "Out of memory"); return NULL; }

  // Initially the stack is empty
  interp->stack = ppl_listInit(0);
  if (interp->stack==NULL) { ppl_error(ec, ERR_MEMORY, -1, -1, "Out of memory"); return NULL; }
  interp->output = (postscriptState *)ppl_memAlloc(sizeof(postscriptState));
  if (interp->output==NULL) { ppl_error(ec, ERR_MEMORY, -1, -1, "Out of memory"); return NULL; }

  // No postscript yet
  interp->output->pages = ppl_listInit(0);
  if (interp->output->pages==NULL) { ppl_error(ec, ERR_MEMORY, -1, -1, "Out of memory"); return NULL; }
  interp->output->Npages = 0;
  interp->output->currentPage = NULL;
  interp->output->currentPosition = NULL;

  // Set default positional variables etc.
  interp->state = (dviStackState *)ppl_memAlloc(sizeof(dviStackState));
  if (interp->state==NULL) { ppl_error(ec, ERR_MEMORY, -1, -1, "Out of memory"); return NULL; }
  interp->state->h=0;
  interp->state->v=0;
  interp->state->w=0;
  interp->state->x=0;
  interp->state->y=0;
  interp->state->z=0;
  interp->f=0;
  interp->curFnt = NULL;
  interp->boundingBox = NULL;
  interp->textSizeBox = NULL;
  interp->scale=0.0;

  // No string currently being assembled
  interp->currentString = NULL;
  interp->currentStrlen = 0;

  // No fonts currently
  interp->fonts = ppl_listInit(0);
  if (interp->fonts==NULL) { ppl_error(ec, ERR_MEMORY, -1, -1, "Out of memory"); return NULL; }

  // Nothing special occuring
  interp->special = 0;  // Not in special mode
  interp->spString = NULL;
  interp->colStack = ppl_listInit(0);
  if (interp->colStack==NULL) { ppl_error(ec, ERR_MEMORY, -1, -1, "Out of memory"); return NULL; }

  // Make the big table of operators
  makeDviOpTable(ec);
  return interp;
 }

// Delete an interpreter
int dviDeleteInterpreter(pplerr_context *ec, dviInterpreterState *interp)
 {
  //free(interp->state);
  if (interp->currentStrlen != 0)
   {
    //free(interp->currentString);
    interp->currentStrlen=0;
   }

  // Delete anything on the stack
  interp->stack = ppl_listInit(0);
  if (interp->stack==NULL) { ppl_error(ec, ERR_MEMORY, -1, -1, "Out of memory"); return DVIE_MEMORY; }

  // Delete any remaining postscript output
  if (interp->output != NULL) dviDeletePostscriptState(ec, interp->output);

  // Delete the interpreter shell
  //free(interp);
  return 0;
 }

// Clone an interpreter state, returning a pointer to the new version
// XXX Make the name of this function consistent with usage elsewhere or vice versa
dviStackState *dviCloneInterpState(pplerr_context *ec, dviStackState *orig)
 {
  void *clone;
  clone = ppl_memAlloc(sizeof(dviStackState));
  if (clone != NULL) memcpy(clone, (void *)orig, sizeof(dviStackState));
  return (dviStackState *) clone;
 }

// ---------------------------------------------------------
// Functions for producing and manipulating postscript pages
// ---------------------------------------------------------

// Produce a new page of postscript
postscriptPage *dviNewPostscriptPage(pplerr_context *ec)
 {
  postscriptPage *page;
  page = (postscriptPage *)ppl_memAlloc(sizeof(postscriptPage));
  if (page==NULL) { ppl_error(ec, ERR_MEMORY, -1, -1, "Out of memory"); return NULL; }
  page->boundingBox = NULL;
  page->textSizeBox = NULL;
  //page->position[0] = 0;
  //page->position[1] = 0;
  page->text = ppl_listInit(0);
  if (page->text==NULL) { ppl_error(ec, ERR_MEMORY, -1, -1, "Out of memory"); return NULL; }
  //page->currentPosition = NULL; // No position set initially
  return page;
 }

// Delete a page of postscript output. If we didn't have lt_memory, we'd need to free things at this point.
int dviDeletePostscriptPage(pplerr_context *ec, postscriptPage *page)
 {
  if (page->boundingBox != NULL)
   {
    //free(page->boundingBox);
    page->boundingBox = NULL;
   }
  if (page->textSizeBox != NULL)
   {
    page->textSizeBox = NULL;
   }

  // free page->text
  //free(page);
  return 0;
 }

// Clear a set of postscript pages and state
int dviDeletePostscriptState(pplerr_context *ec, postscriptState *state)
 {
  // free state->pages
  state->Npages = 0;
  state->currentPage = NULL;
  //free(state);
  return 0;
 }

// -------------------------------------------
// Functions for producing postscript commands
// -------------------------------------------

// Append a string to the set of postscript strings
int dviPostscriptAppend(pplerr_context *ec, dviInterpreterState *interp, char *new)
 {
  char *s;  // A temporary string pointer
  int len;

  // Grab the new string and copy it into place
  len = strlen(new)+1;
  s = (char *)ppl_memAlloc(len);
  if (s==NULL) { ppl_error(ec, ERR_MEMORY, -1, -1, "Out of memory"); return DVIE_MEMORY; }
  strncpy(s, new, len);
  ppl_listAppend(interp->output->currentPage->text, (void *)s);
  return 0;
 }

// Write some postscript to move to the current co-ordinates
int dviPostscriptMoveto(pplerr_context *ec, dviInterpreterState *interp)
 {
  int err=0;
  char s[SSTR_LENGTH];
  double x, y;
  x = interp->state->h * interp->scale;
  y = 765 - interp->state->v * interp->scale;
  snprintf(s, SSTR_LENGTH, "newpath\n%f %f moveto\n", x, y);
  if ((err=dviPostscriptAppend(ec, interp, s))!=0) return err;

  // If we don't have a current position make one, else set the current ps position to the dvi one
  if (interp->output->currentPosition == NULL)
   {
    interp->output->currentPosition = dviCloneInterpState(ec, interp->state);
    if (interp->output->currentPosition == NULL) return DVIE_MEMORY;
   }
  else
   {
    interp->output->currentPosition->h = interp->state->h;
    interp->output->currentPosition->v = interp->state->v;
   }
  return 0;
 }

// Write some postscript to draw a line to the current co-ordinates
int dviPostscriptLineto(pplerr_context *ec, dviInterpreterState *interp)
 {
  int err=0;
  char s[SSTR_LENGTH];
  double x, y;
  x = interp->state->h * interp->scale;
  y = 765 - interp->state->v * interp->scale;
  snprintf(s, SSTR_LENGTH, "%f %f lineto\n", x, y);
  if ((err=dviPostscriptAppend(ec, interp, s))!=0) return err;

  // If we don't have a current position make one, else set the current ps position to the dvi one
  if (interp->output->currentPosition == NULL)
   {
    ppl_error(ec, ERR_INTERNAL, -1, -1,"postscript error: lineto command issued with NULL current state!");
    return DVIE_INTERNAL;
   }
  else
   {
    interp->output->currentPosition->h = interp->state->h;
    interp->output->currentPosition->v = interp->state->v;
   }
  return 0;
 }

// Write some postscript to close a path
int dviPostscriptClosepathFill(pplerr_context *ec, dviInterpreterState *interp)
 {
  int err=0;
  char s[SSTR_LENGTH];
  //double x, y;
  //x = interp->state->h * interp->scale;
  //y = 765 - interp->state->v * interp->scale;
  snprintf(s, SSTR_LENGTH, "closepath fill\n");
  if ((err=dviPostscriptAppend(ec, interp, s))!=0) return err;
  if (interp->output->currentPosition == NULL)
   {
    ppl_error(ec, ERR_INTERNAL, -1, -1,"postscript error: closepath command issued with NULL current state!");
    return DVIE_INTERNAL;
   }
  else
   {
    //free(interp->output->currentPosition);
    interp->output->currentPosition = NULL;
   }
  return 0;
 }

// Typeset the current buffered text
int dviTypeset(pplerr_context *ec, dviInterpreterState *interp)
 {
  // This subroutine does the bulk of the actual postscript work, typesetting runs of characters
  dviStackState *postPos, *dviPos;   // Current positions in dvi and postscript code
  char *s;
  double width, height, depth, italic;
  double size[4];               // Width, height, depth
  int chars, err=0;

  postPos = interp->output->currentPosition;
  dviPos = interp->state;

  // First check if we need to move before typesetting
  if (postPos== NULL)
   {
    if ((err=dviPostscriptMoveto(ec, interp))!=0) return err;
    interp->output->currentPosition = dviCloneInterpState(ec, dviPos);
    if (interp->output->currentPosition == NULL) return DVIE_MEMORY;
   }
  else if (postPos->h != dviPos->h || postPos->v != dviPos->v)
   {
    if ((err=dviPostscriptMoveto(ec, interp))!=0) return err;
   }

  s =  interp->currentString;
  width  = 0.0;
  height = 0.0;
  depth  = 0.0;
  italic = 0.0;
  while (*s != '\0')
   {
    if ((err=dviGetCharSize(ec, interp, (unsigned char) *s, size))!=0) return err;
    width += size[0];
    height = size[1]>height ? size[1] : height;
    depth  = size[2]>depth  ? size[2] : depth;
    italic = size[3];
    s++;
   }

  // Convert back into dvi units
  width  /= interp->scale;
  height /= interp->scale;
  depth  /= interp->scale;
  italic /= interp->scale;
  if (DEBUG) { sprintf(ec->tempErrStr, "DVI: width of glyph %g height of glyph %g", width, height); ppl_log(ec, NULL); }

  // Only need to consider extra italic width for the final glyph
  if ((err=dviUpdateBoundingBox(ec, interp, width+italic, height, depth))!=0) return err;

  // Count the number of characters to write to the ps string
  chars = strlen(interp->currentString)+9;
  s = (char *)ppl_memAlloc(chars);
  if (s==NULL) { ppl_error(ec, ERR_MEMORY, -1, -1, "Out of memory"); return DVIE_MEMORY; }
  snprintf(s, chars, "(%s) show\n", interp->currentString);

  // Send the string off to the postscript routine and clean up memory
  if ((err=dviPostscriptAppend(ec, interp, s))!=0) return err;
  //free(s);
  //free(interp->currentString);
  interp->currentString = NULL;
  interp->currentStrlen = 0;

  // Adjust the current position
  interp->state->h += width;
  interp->output->currentPosition->h += width;
  return 0;
 }

// Change to a new font
int dviChngFnt(pplerr_context *ec, dviInterpreterState *interp, int fn)
 {
  listIterator *listIter;
  int len, err=0;
  char *s;
  dviFontDetails *font;
  double size;

  interp->f = fn;

  // Find the font in the list
  interp->curFnt = NULL;
  listIter = ppl_listIterateInit(interp->fonts);
  while (listIter != NULL)
   {
    if (((dviFontDetails *)listIter->data)->number == fn)
     {
      interp->curFnt = (dviFontDetails *)listIter->data;
      break;
     }
    ppl_listIterate(&listIter);
   }

  // See whether we found the font, and if not, throw an error
  if (interp->curFnt == NULL)
   {
    ppl_error(ec, ERR_INTERNAL, -1, -1,"Corrupt DVI file: failed to find current font");
    return DVIE_CORRUPT;
   }

  // Produce an appropriate postscript command
  font = (dviFontDetails *)listIter->data;
  len = strlen(font->psName) + 20;
  s = (char *)ppl_memAlloc(len);
  if (s==NULL) { ppl_error(ec, ERR_MEMORY, -1, -1, "Out of memory"); return DVIE_MEMORY; }
  size = font->useSize*interp->scale;
  if (DEBUG) { sprintf(ec->tempErrStr, "DVI: Font useSize %d size %g changed to %d", font->useSize, size, (int)ceil(size-.5)); ppl_log(ec, NULL); }
  snprintf(s, len, "/%s %d selectfont\n", font->psName, (int)ceil(size-.5));
  if ((err=dviPostscriptAppend(ec, interp, s))!=0) return err;
  //free(s);
  return 0;
 }

// Get the size (width, height, depth) of a glyph
int dviGetCharSize(pplerr_context *ec, dviInterpreterState *interp, unsigned char s, double *size)
 {
  dviTFM *tfm;               // Details of this font
  int chnum;                 // Character number in this font
  TFMcharInfo *chin;         // Character info for this character
  int wi, hi, di, ii;        // Indices
  dviFontDetails *font;      // Font information (for tfm and use size)
  double scale;              // Font use size * unit scaling

  font  = (dviFontDetails *)interp->curFnt;
  tfm   = font->tfm;
  chnum = s - tfm->bc;
  chin  = tfm->charInfo+chnum;
  scale = font->useSize * interp->scale;

  wi = (int)chin->wi;
  hi = (int)chin->hi;
  di = (int)chin->di;
  ii = (int)chin->ii;
  size[0] = tfm->width[wi]  * scale;
  size[1] = tfm->height[hi] * scale;
  size[2] = tfm->depth[di]  * scale;
  size[3] = tfm->italic[ii] * scale;

  // The following loop adds a small quantity of padding as the supplied sizes appear to be somewhat insufficient
  size[1] += 0.05*scale;
  size[2] += 0.05*scale;

  if (DEBUG) { sprintf(ec->tempErrStr, "DVI: Character %d chnum %d has indices %d %d %d %d width %g height %g depth %g italic %g useSize %g desSize %g", s, chnum, wi, di, hi, ii, size[0], size[1], size[2], size[3], font->useSize*interp->scale, font->desSize*interp->scale); ppl_log(ec, NULL); }
  return 0;
 }

// Update a bounding box with the position and size of the current object to be typeset
int dviUpdateBoundingBox(pplerr_context *ec, dviInterpreterState *interp, double width, double height, double depth)
 {
  double *bb;
  double bbObj[4];      // Bounding box of the object that we are typeseting

  // left bottom right top
  // DVI counts down from the top (and this is all DVI units)
  bbObj[0] = interp->state->h;
  bbObj[1] = interp->state->v + depth;
  bbObj[2] = interp->state->h + width;
  bbObj[3] = interp->state->v - height;

  // Check to see if we already have a bounding box
  if (interp->boundingBox == NULL)
   {
    bb = (double *)ppl_memAlloc(4*sizeof(double));
    if (bb==NULL) { ppl_error(ec, ERR_MEMORY, -1, -1, "Out of memory"); return DVIE_MEMORY; }
    bb[0] = bbObj[0];
    bb[1] = bbObj[1];
    bb[2] = bbObj[2];
    bb[3] = bbObj[3];
    interp->boundingBox = bb;
   }
  else
   {
    // Check against current bounding box
    bb = interp->boundingBox;
    bb[0] = bb[0] < bbObj[0] ? bb[0] : bbObj[0];
    bb[1] = bb[1] > bbObj[1] ? bb[1] : bbObj[1];
    bb[2] = bb[2] > bbObj[2] ? bb[2] : bbObj[2];
    bb[3] = bb[3] < bbObj[3] ? bb[3] : bbObj[3];
   }

  // Now repeat the process for the text size box
  // This is the baseline and 70% of the use height, which we take to be the cap height
  bbObj[1] = interp->state->v;
  bbObj[3] = interp->state->v - interp->curFnt->useSize * 0.70;

  // Check to see if we already have a text size box
  if (interp->textSizeBox == NULL)
   {
    bb = (double *)ppl_memAlloc(4*sizeof(double));
    if (bb==NULL) { ppl_error(ec, ERR_MEMORY, -1, -1, "Out of memory"); return DVIE_MEMORY; }
    bb[0] = bbObj[0];
    bb[1] = bbObj[1];
    bb[2] = bbObj[2];
    bb[3] = bbObj[3];
    interp->textSizeBox = bb;
   }
  else
   {
    // Check against current bounding box
    bb = interp->textSizeBox;
    bb[0] = bb[0] < bbObj[0] ? bb[0] : bbObj[0];
    bb[1] = bb[1] > bbObj[1] ? bb[1] : bbObj[1];
    bb[2] = bb[2] > bbObj[2] ? bb[2] : bbObj[2];
    bb[3] = bb[3] < bbObj[3] ? bb[3] : bbObj[3];
   }

  return 0;
 }

