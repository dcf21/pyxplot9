// expCompile.h
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

#ifndef _EXPCOMPILE_H
#define _EXPCOMPILE_H 1

typedef struct pplExpr {
  int  refCount;
  long srcId; int srcLineN; char *srcFname;
  char *ascii; void *bytecode; int bcLen;
 } pplExpr;

typedef struct pplTokenCode {
  unsigned char  state, opcode, precedence;
  unsigned short depth;
 } pplTokenCode;

typedef struct pplExprPStack {
  unsigned char opType, opcode, precedence;
  int charpos, outpos;
 } pplExprPStack;

typedef struct pplExprBytecode {
  int len, charpos, opcode;
  unsigned char flag;
  union { int i; double d; } auxil;
 } pplExprBytecode;

#endif

