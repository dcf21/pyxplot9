// dvi_read.h
//
// The code in this file is part of Pyxplot
// <http://www.pyxplot.org.uk>
//
// Copyright (C) 2006-2012 Dominic Ford <coders@pyxplot.org.uk>
//               2008-2012 Ross Church
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

// Functions for manupulating dvi files

#ifndef _PPL_DVI_READ_H
#define _PPL_DVI_READ_H 1

#include <stdlib.h>
#include <stdio.h>

#include "coreUtils/errorReport.h"
#include "coreUtils/list.h"
#include "epsMaker/dvi_font.h"

// Structure to store a DVI operator and the data that goes with it
typedef struct DVIOperator
 {
  unsigned char op;
  unsigned long int ul[10];
  signed long int sl[2];
  char *s[2];
 } DVIOperator;

// Structure to store the state of a dvi interpreter that can be pushed onto the stack
typedef struct dviStackState
 {
  signed long int h,v,w,x,y,z;
 } dviStackState;

// Structure to store a page of postscript
typedef struct postscriptPage
 {
  double *boundingBox;  // The current bounding box
  double *textSizeBox;  // The current text size box (b.box plus oversize text)
  list *text;           // The big list of strings of postscript
 } postscriptPage;

// Structure to store the state of some postscript in the process of being produced
typedef struct postscriptState
 {
  list *pages;                      // List of pages of postscript
  int Npages;                       // The number of pages
  postscriptPage *currentPage;      // The current page
  dviStackState *currentPosition;   // The current position in dvi units
 } postscriptState;

// Structure to store the entire internal state of a dvi interpreter
typedef struct dviInterpreterState
 {
  dviStackState *state;      // h,v,w,x,y,z;  // Positions
  unsigned int f;            // Current font
  dviFontDetails *curFnt;    //       and a pointer to it
  char *currentString;       // The string, if any, currently being rendered
  int currentStrlen;         //       and its current length
  double scale;              // Scale from dvi->ps units
  list *stack;               // The stack
  list *fonts;               // The fonts currently defined
  postscriptState *output;   // The output postscript
  int special;               // A magic flag for special actions
  int specialLen;            // Number of characters in special command
  char *spString;            // String to store special information
  list *colStack;            // Stack of colour items
  double *boundingBox;       // Current bounding box
  double *textSizeBox;       // Current text size box
 } dviInterpreterState;

// Functions allowing dvi interpreters to be manipulated
dviInterpreterState *ReadDviFile(pplerr_context *ec, char *filename, int *status);
dviInterpreterState *dviNewInterpreter(pplerr_context *ec);
int dviInterpretOperator(pplerr_context *ec, dviInterpreterState *interp, DVIOperator *op);
int dviDeleteInterpreter(pplerr_context *ec, dviInterpreterState *interp);
int dviTypeset(pplerr_context *ec, dviInterpreterState *interp);

dviStackState *dviCloneInterpState(pplerr_context *ec, dviStackState *state);

postscriptPage *dviNewPostscriptPage(pplerr_context *ec);
int dviDeletePostscriptPage(pplerr_context *ec, postscriptPage *page);
int dviDeletePostscriptState(pplerr_context *ec, postscriptState *state);

int dviPostscriptMoveto(pplerr_context *ec, dviInterpreterState *interp);
int dviPostscriptAppend(pplerr_context *ec, dviInterpreterState *interp, char *s);

int dviGetCharSize(pplerr_context *ec, dviInterpreterState *interp, unsigned char s, double *size);

// Routines for reading from files
int ReadUChar(pplerr_context *ec, FILE *fp, int *uc);
int ReadLongInt(pplerr_context *ec, FILE *fp, unsigned long int *uli, int n);
int ReadSignedInt(pplerr_context *ec, FILE *fp, signed long int *sli, int n);
double ReadFixWord(pplerr_context *ec, FILE *fp, int *err);

int DisplayDVIOperator(pplerr_context *ec, DVIOperator *op);
int GetDVIOperator(pplerr_context *ec, DVIOperator *op, FILE *fp);

// The following definitions are taken from PyX
#define DVI_CHARMIN       0 // typeset a character and move right (range min)
#define DVI_CHARMAX     127 // typeset a character and move right (range max)
#define DVI_SET1234     128 // typeset a character and move right
#define DVI_SETRULE     132 // typeset a rule and move right
#define DVI_PUT1234     133 // typeset a character
#define DVI_PUTRULE     137 // typeset a rule
#define DVI_NOP         138 // no operation
#define DVI_BOP         139 // beginning of page
#define DVI_EOP         140 // ending of page
#define DVI_PUSH        141 // save the current positions (h, v, w, x, y, z)
#define DVI_POP         142 // restore positions (h, v, w, x, y, z)
#define DVI_RIGHT1234   143 // move right
#define DVI_W0          147 // move right by w
#define DVI_W1234       148 // move right and set w
#define DVI_X0          152 // move right by x
#define DVI_X1234       153 // move right and set x
#define DVI_DOWN1234    157 // move down
#define DVI_Y0          161 // move down by y
#define DVI_Y1234       162 // move down and set y
#define DVI_Z0          166 // move down by z
#define DVI_Z1234       167 // move down and set z
#define DVI_FNTNUMMIN   171 // set current font (range min)
#define DVI_FNTNUMMAX   234 // set current font (range max)
#define DVI_FNT1234     238 // set current font
#define DVI_SPECIAL1234 239 // special (dvi extention)
#define DVI_FNTDEF1234  243 // define the meaning of a font number
#define DVI_PRE         247 // preamble
#define DVI_POST        248 // postamble beginning
#define DVI_POSTPOST    249 // postamble ending

#define DVI_VERSION     2   // dvi version

// true and false
#define DVI_YES 1
#define DVI_NO 0

// Error states

// Errors related to file operations
#define DVIE_NOENT   10
#define DVIE_EOF     11
#define DVIE_ACCESS  12

// Errors related to malformed/corrupted/missing files
#define DVIE_CORRUPT 20
#define DVIE_NOFONT  21

// Internal errors (inconsistent internal state => bug!)
#define DVIE_MEMORY   30
#define DVIE_INTERNAL 31

#endif

