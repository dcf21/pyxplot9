// parser.h
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

#ifndef _PARSER_H
#define _PARSER_H 1

#include "coreUtils/list.h"
#include "userspace/context.h"

#define PN_TYPE_SEQ     21000
#define PN_TYPE_OPT     21001
#define PN_TYPE_REP     21002 // Must have at least one repeat item
#define PN_TYPE_REP2    21003 // Can have zero items
#define PN_TYPE_PER     21004
#define PN_TYPE_ORA     21005
#define PN_TYPE_ITEM    21006
#define PN_TYPE_CONFIRM 21007

typedef struct parserNode {
  int   type;
  int   listLen;
  char *matchString; // ITEMs only
  char *outString;   // "
  int   acLevel;     // "
  char *varName;
  int   outStackPos;
  struct parserNode *firstChild;
  struct parserNode *nextSibling;
 } parserNode;

typedef struct parserAtom {
  int stackOutPos;
  int linePos; // Position of this atom in the string copy of the line, used for error reporting
  int atomLen; // Length of this atom; number of bytes to advance in bytecode to find next parserAtom record
  unsigned char amLiteral;
  unsigned char options[8]; // characters, e.g. 'd' if %d is an allowed kind of value for this variable
 } parserAtom;

typedef struct parserLine {
  char *linetxt;
  void *bytecode;
  int   bytecodelen;
 } parserLine;

typedef struct parserStatus {
  parserLine *inPrep;
  int readingData, readingCode;
 } parserStatus;

#ifndef _PARSERINIT_C
extern list *pplParserCmdList[];
#endif

int ppl_parserInit   (ppl_context *c);
int ppl_parserCompile(ppl_context *c, int ExpandMacros);
int ppl_parserExecute(ppl_context *c);

#endif

