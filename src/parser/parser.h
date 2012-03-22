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
#include "expressions/expCompile.h"
#include "userspace/context.h"
#include "userspace/pplObj.h"

#define PN_TYPE_SEQ     21000
#define PN_TYPE_OPT     21001
#define PN_TYPE_REP     21002 // Must have at least one repeat item
#define PN_TYPE_REP2    21003 // Can have zero items
#define PN_TYPE_PER     21004
#define PN_TYPE_ORA     21005
#define PN_TYPE_ITEM    21006
#define PN_TYPE_CONFIRM 21007 // The = token in RE++
#define PN_TYPE_DATABLK 21008
#define PN_TYPE_CODEBLK 21009

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

#define PARSER_TYPE_OPTIONS 8
  
typedef struct parserAtom {
  int stackOutPos;
  int linePos; // Position of this atom in the string copy of the line, used for error reporting
  char options[PARSER_TYPE_OPTIONS]; // characters, e.g. 'd' if %d is an allowed kind of value for this variable
  pplExpr *expr; pplObj *literal;
  struct parserAtom *next;
 } parserAtom;

typedef struct parserLine {
  char       *linetxt;
  int         srcLineN;
  long        srcId;
  char       *srcFname;
  int         stackLen, stackOffset;
  int         containsMacros, refCount;
  parserAtom *firstAtom, *lastAtom;
  struct parserLine *next, *prev;
 } parserLine;

typedef struct parserStatus {
  char         prompt[64];
  parserLine **rootpl;
  parserLine  *pl [MAX_RECURSION_DEPTH];
  parserNode  *stk[MAX_RECURSION_DEPTH][16];
  int          oldStackOffset[MAX_RECURSION_DEPTH][16];
  int          blockDepth;
  int          NinlineDatafiles;
  int          waitingForBrace, outputPos[MAX_RECURSION_DEPTH];
  char         expectingList[LSTR_LENGTH];
  int          eLPos, eLlinePos;
 } parserStatus;

typedef struct parserOutput {
  int    *stkCharPos;
  pplObj *stk;
  int stackLen;
 } parserOutput;

#ifndef _PARSERINIT_C
extern list *pplParserCmdList[];
#endif

int     ppl_parserInit      (ppl_context *c);
void    ppl_parserAtomAdd   (parserLine *in, int stackOutPos, int linePos, char *options, pplExpr *expr, pplObj *literal);
void    ppl_parserAtomFree  (parserAtom **in);
void    ppl_parserLineFree  (parserLine *in);
void    ppl_parserStatInit  (parserStatus **in, parserLine **pl);
void    ppl_parserStatReInit(parserStatus *in);
void    ppl_parserStatAdd   (parserStatus *in, int level, parserLine *pl);
void    ppl_parserStatFree  (parserStatus **in);
void    ppl_parserLineInit  (parserLine **in, int srcLineN, long srcId, char *srcFname, char *line);
int     ppl_parserCompile   (ppl_context *c, parserStatus *s, int srcLineN, long srcId, char *srcFname, char *line, int expandMacros, int blockDepth);
void    ppl_parserLinePrint (ppl_context *c, parserLine *in);
void    ppl_parserExecute   (ppl_context *c, parserLine *in, int interactive, int iterDepth);
void    ppl_parserShell     (ppl_context *c, parserLine *pl, parserOutput *in, int interactive, int iterDepth);

#ifdef HAVE_READLINE
void    ppl_parseAutocompleteSetContext(ppl_context *c);
char  **ppl_rl_completion              (const char *text, int start, int end);
#endif

#endif

