// expCompile.c
//
// The code in this file is part of PyXPlot
// <http://www.pyxplot.org.uk>
//
// Copyright (C) 2006-2012 Dominic Ford <coders@pyxplot.org.uk>
//               2008-2011 Ross Church
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

#define _EXPCOMPILE_C 1

#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "stringTools/asciidouble.h"

#include "pplConstants.h"

// expMarkup() -- Tokenise expressions

// A -- beginning of expression
// B -- a string literal
// C -- a string substitution operator
// D -- a list of string substitutions
// E -- a bracketed sub-expression
// F -- a unary post-lvalue operator
// G -- a variable name
// H -- a unary pre-lvalue operator
// I -- a unary value-based operator
// J -- a binary operator
// K -- a ternary operator
// L -- a numeric literal
// M -- a list literal
// N -- a dictionary literal
// O -- a dollar sign
// P -- a list of function args
// Q -- an array dereference []
// R -- the dot operator
// S -- assignment operator
// T -- parameter name
// U -- end of expression

// A can be followed by .B..E.GHI..LMNO......
// B can be followed by ..C......JK.....Q...U
// C can be followed by .B.D..GHI..LMNO......
// D can be followed by .........JK.....Q...U
// E can be followed by .........JK.....QR..U
// F can be followed by .........JK.........U
// G can be followed by .....F...JK....PQRS.U
// H can be followed by ......G..............
// I can be followed by .B..E.GH...LMNO......
// J can be followed by .B..E.GHI..LMNO......
// K can be followed by .B..E.GHI..LMNO......
// L can be followed by .........JK.........U
// M can be followed by .........JK.....QR..U
// N can be followed by .........JK.....QR..U
// O can be followed by ....E......L.......T.
// P can be followed by .........JK....PQR..U
// Q can be followed by .........JK....PQ.S.U
// R can be followed by ......G..............
// S can be followed by .B..E.GHI..LMNO......
// T can be followed by .........JK.........U

#define MARKUP_MATCH(A) (strncmp(in+scanpos,A,strlen(A))==0)

#define PGE_OVERFLOW { *errPos = scanpos; strcpy(errText, "Overflow Error: Algebraic expression too long"); *end = -1; return; }

#define NEWSTATE(L) state=trialstate; for (i=0;i<L;i++) { out[scanpos++]=(unsigned char)(state-'@') ; if (scanpos>=ALGEBRA_MAXLENGTH) PGE_OVERFLOW; }
#define SAMESTATE                                       { out[scanpos++]=(unsigned char)(state-'@') ; if (scanpos>=ALGEBRA_MAXLENGTH) PGE_OVERFLOW; }

void expMarkup(char *in, int *end, int dollarAllowed, unsigned char *out, int *errPos, char *errText)
 {
  const char *allowed[] = {"BEGHILMNO","CJKQU","BDGHILMNO","JKQU","JKQRU","JKU","FJKPQRSU","G","BEGHLMNO","BEGHILMNO","BEGHILMNO","JKU","JKQRU","JKQRU","ELT","JKPQRU","JKPQSU","G","BEGHILMNO","JKU",""};
  char state='A', oldstate, trialstate;
  int scanpos=0, trialpos, i;

  while (state!='U')
   {
    oldstate = state;
    while ((in[scanpos]!='\0') && (in[scanpos]<=' ')) { SAMESTATE; } // Sop up whitespace

    for (trialpos=0; ((trialstate=allowed[(int)(state-'A')][trialpos])!='\0'); trialpos++)
     {
      if      (trialstate=='B') // string literal
       {
        char quoteType;
        if ( (in[scanpos]==(quoteType='\'')) || (in[scanpos]==(quoteType='"')) )
         {
          int j;
          for (j=1; ((in[scanpos+j]!='\0')&&((in[scanpos+j]!=quoteType)||(in[scanpos+j-1]=='\\'))); j++);
          if (in[scanpos+j]==quoteType) { j++; NEWSTATE(j); }
          else                          { *errPos = scanpos; strcpy(errText, "Syntax Error: Mismatched quote"); *end=-1; return; }
         }
       }
      else if (trialstate=='C') // string substitution operator
       {
        if (MARKUP_MATCH("%")) { NEWSTATE(1); }
       }
      else if (   (trialstate=='D')   // a list of string substitutions
               || (trialstate=='E')   // a bracketed sub-expression
               || (trialstate=='P') ) // a list of function arguments
       {
        if (MARKUP_MATCH("("))
         {
          int j;
          ppl_strBracketMatch(in+scanpos,'(',')',NULL,NULL,&j,0);
          if (j>0) { j++; NEWSTATE(j); }
          else     { *errPos = scanpos; strcpy(errText, "Syntax Error: Mismatched ( )"); *end=-1; return; }
         }
       }
      else if ( (trialstate=='F') || // a unary post-lvalue operator
                (trialstate=='H') )  // a unary pre-lvalue operator
       {
        if ( (MARKUP_MATCH("--")) || (MARKUP_MATCH("++")) ) { NEWSTATE(2); }
       }
      else if ( (trialstate=='G') ||  // a variable name
                (trialstate=='T') )   // a parameter name
       {
        if (isalpha(in[scanpos]))
         {
          NEWSTATE(1);
          while ((isalnum(in[scanpos])) || (in[scanpos]=='_')) { SAMESTATE; }
         }
       }
      else if (trialstate=='I') // a unary value-based operator
       {
        if      (MARKUP_MATCH("-"  )) { NEWSTATE(1); }
        else if (MARKUP_MATCH("+"  )) { NEWSTATE(1); }
        else if (MARKUP_MATCH("~"  )) { NEWSTATE(1); }
        else if (MARKUP_MATCH("!"  )) { NEWSTATE(1); }
        else if (MARKUP_MATCH("not")) { NEWSTATE(3); }
        else if (MARKUP_MATCH("NOT")) { NEWSTATE(3); }
       }
      else if (trialstate=='J') // a binary operator
       {
        if      (MARKUP_MATCH("**" )) { NEWSTATE(2); }
        else if (MARKUP_MATCH("*"  )) { NEWSTATE(1); }
        else if (MARKUP_MATCH("/"  )) { NEWSTATE(1); }
        else if (MARKUP_MATCH("%"  )) { NEWSTATE(1); }
        else if (MARKUP_MATCH("+"  )) { NEWSTATE(1); }
        else if (MARKUP_MATCH("-"  )) { NEWSTATE(1); }
        else if (MARKUP_MATCH("<<" )) { NEWSTATE(2); }
        else if (MARKUP_MATCH(">>" )) { NEWSTATE(2); }
        else if (MARKUP_MATCH("<"  )) { NEWSTATE(1); }
        else if (MARKUP_MATCH("<=" )) { NEWSTATE(2); }
        else if (MARKUP_MATCH(">=" )) { NEWSTATE(2); }
        else if (MARKUP_MATCH(">"  )) { NEWSTATE(1); }
        else if (MARKUP_MATCH("==" )) { NEWSTATE(2); }
        else if (MARKUP_MATCH("<>" )) { NEWSTATE(2); }
        else if (MARKUP_MATCH("!=" )) { NEWSTATE(2); }
        else if (MARKUP_MATCH("&"  )) { NEWSTATE(1); }
        else if (MARKUP_MATCH("^"  )) { NEWSTATE(1); }
        else if (MARKUP_MATCH("|"  )) { NEWSTATE(1); }
        else if (MARKUP_MATCH("&&" )) { NEWSTATE(2); }
        else if (MARKUP_MATCH("||" )) { NEWSTATE(2); }
        else if (MARKUP_MATCH(","  )) { NEWSTATE(1); }
       }
      else if (trialstate=='K') // a ternary operator
       {
        if ( (MARKUP_MATCH("?")) || (MARKUP_MATCH(":")) ) { NEWSTATE(1); }
       }
      else if (trialstate=='L') // a numeric literal
       {
        int j=0;
        if (ppl_validFloat(in+scanpos,&j)) { NEWSTATE(j); }
       }
      else if ( (trialstate=='M') || // a list literal
                (trialstate=='Q') )  // an array dereference []
       {
        if (MARKUP_MATCH("["))
         {
          int j;
          ppl_strBracketMatch(in+scanpos,'[',']',NULL,NULL,&j,0);
          if (j>0) { j++; NEWSTATE(j); }
          else     { *errPos = scanpos; strcpy(errText, "Syntax Error: Mismatched [ ]"); *end=-1; return; }
         }
       }
      else if (trialstate=='N') // a dictionary literal
       {
        if (MARKUP_MATCH("("))
         {
          int j;
          ppl_strBracketMatch(in+scanpos,'{','}',NULL,NULL,&j,0);
          if (j>0) { j++; NEWSTATE(j); }
          else     { *errPos = scanpos; strcpy(errText, "Syntax Error: Mismatched { }"); *end=-1; return; }
         }
       }
      else if (trialstate=='O') // a dollar sign
       {
        if ((dollarAllowed)&&(MARKUP_MATCH("$"))) { NEWSTATE(1); }
       }
      else if (trialstate=='R') // the dot operator
       {
        if (MARKUP_MATCH(".")) { NEWSTATE(1); }
       }
      else if (trialstate=='S') // assignment operator
       {
        if      (MARKUP_MATCH("="  )) { NEWSTATE(1); }
        else if (MARKUP_MATCH("+=" )) { NEWSTATE(2); }
        else if (MARKUP_MATCH("-=" )) { NEWSTATE(2); }
        else if (MARKUP_MATCH("*=" )) { NEWSTATE(2); }
        else if (MARKUP_MATCH("/=" )) { NEWSTATE(2); }
        else if (MARKUP_MATCH("%=" )) { NEWSTATE(2); }
        else if (MARKUP_MATCH("&=" )) { NEWSTATE(2); }
        else if (MARKUP_MATCH("^=" )) { NEWSTATE(2); }
        else if (MARKUP_MATCH("|=" )) { NEWSTATE(2); }
        else if (MARKUP_MATCH("<<=")) { NEWSTATE(3); }
        else if (MARKUP_MATCH(">>=")) { NEWSTATE(3); }
       }
      else if (trialstate=='U') // end of expression
       { NEWSTATE(0); }
      if (state != oldstate) break;
     }
    if (state == oldstate) break; // We've got stuck
   }

  if (state=='U') // We reached state U... end of expression
   {
    *errPos = -1;
    *errText='\0';
    out[scanpos]=0;
    *end = scanpos;
    return;
   }
  else
   {
    // Error; we didn't reach state U
    int j=0;
    int B=0,F=0,G=0,J=0,P=0;
    *errPos = scanpos;
    *end    = -1;
    // Now we need to construct an error string
    strcpy(errText,"Syntax Error: At this point, was expecting ");
    i=strlen(errText);
    for (trialpos=0; ((trialstate=allowed[(int)(state-'A')][trialpos])!='\0'); trialpos++)
     {

#define W { if (j!=0) {strcpy(errText+i," or "); i+=strlen(errText+i);} else j=1; }

      if      ((!B)&&((trialstate=='B')||(trialstate=='L')||(trialstate=='M')||(trialstate=='N')||(trialstate=='O')))
                                { W; strcpy(errText+i,"a literal value"); i+=strlen(errText+i); B=1; }
      else if (trialstate=='C') { W; strcpy(errText+i,"a string substitution operator"); i+=strlen(errText+i); }
      else if (trialstate=='D') { W; strcpy(errText+i,"a list of string substitutions"); i+=strlen(errText+i); }
      else if (trialstate=='E') { W; strcpy(errText+i,"a bracketed expression"); i+=strlen(errText+i); }
      else if ((!F)&&((trialstate=='F')||(trialstate=='H')||(trialstate=='I')))
                                { W; strcpy(errText+i,"a unary operator"); i+=strlen(errText+i); F=1; }
      else if ((!G)&&((trialstate=='G')||(trialstate=='T')))
                                { W; strcpy(errText+i,"a variable name"); i+=strlen(errText+i); G=1; }
      else if ((!J)&&((trialstate=='J')||(trialstate=='K')||(trialstate=='S')))
                                { W; strcpy(errText+i,"a binary/ternary operator"); i+=strlen(errText+i); J=1; }
      else if (trialstate=='P') { strcpy(errText+i,"a list of function arguments"); i+=strlen(errText+i); }
      else if ((!P)&&((trialstate=='Q')||(trialstate=='R')))
                                { W; strcpy(errText+i,"an object dereference"); i+=strlen(errText+i); P=1; }
     }
    strcpy(errText+i,"."); i+=strlen(errText+i);
    return;
   }
 }

