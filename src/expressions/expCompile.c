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

#include "coreUtils/errorReport.h"
#include "stringTools/asciidouble.h"
#include "userspace/context.h"
#include "expressions/expCompile.h"

#include "pplConstants.h"

// expTokenise() -- Tokenise expressions

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
// T -- dereference name
// U -- end of expression
// V -- parameter name after $

// A can be followed by .B..E.GHI..LMNO.......
// B can be followed by ..C......JK.....Q...U.
// C can be followed by .B.D..GHI..LMNO.......
// D can be followed by .........JK.....Q...U.
// E can be followed by .........JK.....QR..U.
// F can be followed by .........JK.........U.
// G can be followed by .....F...JK....PQRS.U.
// H can be followed by ......G...............
// I can be followed by .B..E.GH...LMNO.......
// J can be followed by .B..E.GHI..LMNO.......
// K can be followed by .B..E.GHI..LMNO.......
// L can be followed by .........JK.........U.
// M can be followed by .........JK.....QR..U.
// N can be followed by .........JK.....QR..U.
// O can be followed by ....E......L.........V
// P can be followed by .........JK....PQR..U.
// Q can be followed by .........JK....PQ.S.U.
// R can be followed by ...................T..
// S can be followed by .B..E.GHI..LMNO.......
// T can be followed by .....F...JK....PQRS.U.
// V can be followed by .........JK.........U.

#define MARKUP_MATCH(A) (strncmp(in+scanpos,A,strlen(A))==0)

#define PGE_OVERFLOW { *errPos = scanpos; strcpy(errText, "Overflow Error: Algebraic expression too long"); *end = -1; return; }

#define NEWSTATE(L,O,P) \
 { \
  int i; \
  state=trialstate; \
  opcode=O; \
  precedence=P; \
  for (i=0;i<L;i++) \
   { \
    out[outpos++]=(unsigned char)(state-'@'); \
    out[outpos++]=opcode; \
    out[outpos++]=precedence; \
    if (outpos>=*outlen) PGE_OVERFLOW; \
   } \
  scanpos+=L; \
 }

#define SAMESTATE \
 { \
  out[outpos++]=(unsigned char)(state-'@'); \
  out[outpos++]=opcode; \
  out[outpos++]=precedence; \
  if (outpos>=*outlen) PGE_OVERFLOW; \
  scanpos++; \
 }

void ppl_expTokenise(ppl_context *context, char *in, int *end, int dollarAllowed, int collectCommas, int isDict, unsigned char *out, int *outlen, int *errPos, char *errText)
 {
  const char *allowed[] = {"BEGHILMNO","CJKQU","BDGHILMNO","JKQU","JKQRU","JKU","FJKPQRSU","G","BEGHLMNO","BEGHILMNO","BEGHILMNO","JKU","JKQRU","JKQRU","ELV","JKPQRU","JKPQSU","T","BEGHILMNO","FJKPQRSU","","JKU"};
  int nCommaItems=1, nDictItems=0, tertiaryDepth=0;
  char state='A', oldstate, trialstate;
  int scanpos=0, outpos=0, trialpos;
  unsigned char opcode=0, precedence=0;

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
          if (in[scanpos+j]==quoteType) { j++; NEWSTATE(j,0,0); }
          else                          { *errPos = scanpos; strcpy(errText, "Syntax Error: Mismatched quote"); *end=-1; *outlen=0; return; }
         }
       }
      else if (trialstate=='C') // string substitution operator
       {
        if (MARKUP_MATCH("%")) { NEWSTATE(1,0x40,4); }
       }
      else if (   (trialstate=='D')   // a list of string substitutions
               || (trialstate=='E')   // a bracketed sub-expression
               || (trialstate=='P') ) // a list of function arguments
       {
        if (MARKUP_MATCH("("))
         {
          int j,k,l,m,n=1;
          ppl_strBracketMatch(in+scanpos,'(',')',NULL,NULL,&j,0); // Search for a ) to match the (
          if (j<=0) { *errPos = scanpos; strcpy(errText, "Syntax Error: Mismatched ( )"); *end=-1; *outlen=0; return; }
          NEWSTATE(1,0,0); // Record the one character opening bracket
          while ((in[scanpos]!='\0') && (in[scanpos]<=' ')) { SAMESTATE; n++; } // Sop up whitespace
          k=scanpos; l=outpos;
          NEWSTATE(j-n,0,0); // Fast-forward to closing bracket
          n=outpos-l+1;
          if ((in[k]!=')')||(trialstate=='E'))
           {
            ppl_expTokenise(context,in+k,&m,dollarAllowed,(trialstate!='E'),0,out+l,&n,errPos,errText); // Hierarchically tokenise the inside of the brackets
            if (*errPos>=0) { *errPos+=k; return; }
           }
          NEWSTATE(1,out[outpos+1],out[outpos+2]); // Record the one character closing bracket
         }
       }
      else if (trialstate=='F') // a unary post-lvalue operator
       {
        if      (MARKUP_MATCH("--")) { NEWSTATE(2,0xA1,3); }
        else if (MARKUP_MATCH("++")) { NEWSTATE(2,0xA2,3); }
       }
      else if (trialstate=='H')  // a unary pre-lvalue operator
       {
        if      (MARKUP_MATCH("--")) { NEWSTATE(2,0xA3,3); }
        else if (MARKUP_MATCH("++")) { NEWSTATE(2,0xA4,3); }
       }
      else if ( (trialstate=='G') ||  // a variable name
                (trialstate=='T') ||  // a dereference name
                (trialstate=='V')   ) // a $parameter name
       {
        if (isalpha(in[scanpos]))
         {
          NEWSTATE(1,0,0);
          while ((isalnum(in[scanpos])) || (in[scanpos]=='_')) { SAMESTATE; }
         }
       }
      else if (trialstate=='I') // a unary value-based operator
       {
        if      (MARKUP_MATCH("-"  )) { NEWSTATE(1,0x25, 3); }
        else if (MARKUP_MATCH("+"  )) { NEWSTATE(1,0x26, 3); }
        else if (MARKUP_MATCH("~"  )) { NEWSTATE(1,0xA7, 3); }
        else if (MARKUP_MATCH("!"  )) { NEWSTATE(1,0xA8, 3); }
        else if (MARKUP_MATCH("not")) { NEWSTATE(3,0xA8, 3); }
        else if (MARKUP_MATCH("NOT")) { NEWSTATE(3,0xA8, 3); }
       }
      else if (trialstate=='J') // a binary operator
       {
        if      (MARKUP_MATCH("**" )) { NEWSTATE(2,0xC9, 2); }
        else if (MARKUP_MATCH("*"  )) { NEWSTATE(1,0x4A, 5); }
        else if (MARKUP_MATCH("/"  )) { NEWSTATE(1,0x4B, 5); }
        else if (MARKUP_MATCH("%"  )) { NEWSTATE(1,0x4C, 5); }
        else if (MARKUP_MATCH("+"  )) { NEWSTATE(1,0x4D, 6); }
        else if (MARKUP_MATCH("-"  )) { NEWSTATE(1,0x4E, 6); }
        else if (MARKUP_MATCH("<<" )) { NEWSTATE(2,0x4F, 7); }
        else if (MARKUP_MATCH(">>" )) { NEWSTATE(2,0x50, 7); }
        else if (MARKUP_MATCH("<"  )) { NEWSTATE(1,0x51, 8); }
        else if (MARKUP_MATCH("<=" )) { NEWSTATE(2,0x52, 8); }
        else if (MARKUP_MATCH(">=" )) { NEWSTATE(2,0x53, 8); }
        else if (MARKUP_MATCH(">"  )) { NEWSTATE(1,0x54, 8); }
        else if (MARKUP_MATCH("==" )) { NEWSTATE(2,0x55, 9); }
        else if (MARKUP_MATCH("<>" )) { NEWSTATE(2,0x56,10); }
        else if (MARKUP_MATCH("!=" )) { NEWSTATE(2,0x56,10); }
        else if (MARKUP_MATCH("&"  )) { NEWSTATE(1,0x57,11); }
        else if (MARKUP_MATCH("^"  )) { NEWSTATE(1,0x58,12); }
        else if (MARKUP_MATCH("|"  )) { NEWSTATE(1,0x59,13); }
        else if (MARKUP_MATCH("&&" )) { NEWSTATE(2,0x5A,14); }
        else if (MARKUP_MATCH("||" )) { NEWSTATE(2,0x5B,15); }
        else if (MARKUP_MATCH(","  ))
         {
          if ((!collectCommas)||(tertiaryDepth>0)) { NEWSTATE(1,0x5C,18); }
          else
           {
            if (isDict && (nCommaItems>=2-(nDictItems==0))) { *errPos = scanpos; strcpy(errText, "Syntax Error: Expecting : followed by value for dictionary key"); *end=-1; *outlen=0; return; }
            NEWSTATE(1,0x5F,18);
            nCommaItems++;
           }
         }
       }
      else if (trialstate=='K') // a ternary operator
       {
        if      (MARKUP_MATCH("?")) { NEWSTATE(1,0xFD,16); tertiaryDepth++; }
        else if (MARKUP_MATCH(":"))
         {
          if (isDict && (tertiaryDepth==0))
           {
            if (nCommaItems<2-(nDictItems==0)) { *errPos = scanpos; strcpy(errText, "Syntax Error: Expecting , to separate dictionary items"); *end=-1; *outlen=0; return; }
            NEWSTATE(1,0x5F,18);
            nCommaItems=1;
            nDictItems++;
           }
          else
           {
            if (tertiaryDepth<=0) { *errPos = scanpos; strcpy(errText, "Syntax Error: No preceding ? to match with :"); *end=-1; *outlen=0; return; }
            tertiaryDepth--;
            NEWSTATE(1,0xFE,16);
           }
         }
       }
      else if (trialstate=='L') // a numeric literal
       {
        int j=0;
        if (ppl_validFloat(in+scanpos,&j)) { NEWSTATE(j,0,0); }
       }
      else if ( (trialstate=='M') || // a list literal
                (trialstate=='Q') )  // an array dereference []
       {
        if (MARKUP_MATCH("["))
         {
          int j,k,l,m,n;
          ppl_strBracketMatch(in+scanpos,'[',']',NULL,NULL,&j,0); // Search for a ] to match the [
          if (j<=0) { *errPos = scanpos; strcpy(errText, "Syntax Error: Mismatched [ ]"); *end=-1; *outlen=0; return; }
          NEWSTATE(1,0,0); // Record the one character opening bracket
          while ((in[scanpos]!='\0') && (in[scanpos]<=' ')) { SAMESTATE; } // Sop up whitespace
          k=scanpos; l=outpos;
          NEWSTATE(j-1,0,0); // Fast-forward to closing bracket
          n=outpos-l+1;
          if (in[k]!=']')
           {
            ppl_expTokenise(context,in+k,&m,dollarAllowed,1,0,out+l,&n,errPos,errText); // Hierarchically tokenise the inside of the brackets
            if (*errPos>=0) { *errPos+=k; return; }
           }
          NEWSTATE(1,out[outpos+1],out[outpos+2]); // Record the one character closing bracket
         }
       }
      else if (trialstate=='N') // a dictionary literal
       {
        if (MARKUP_MATCH("{"))
         {
          int j,k,l,m,n;
          ppl_strBracketMatch(in+scanpos,'{','}',NULL,NULL,&j,0); // Search for a } to match the {
          if (j<=0) { *errPos = scanpos; strcpy(errText, "Syntax Error: Mismatched { }"); *end=-1; *outlen=0; return; }
          NEWSTATE(1,0,0); // Record the one character opening bracket
          while ((in[scanpos]!='\0') && (in[scanpos]<=' ')) { SAMESTATE; } // Sop up whitespace
          k=scanpos; l=outpos;
          NEWSTATE(j-1,0,0); // Fast-forward to closing bracket
          n=outpos-l+1;
          if (in[k]!='}')
           {
            ppl_expTokenise(context,in+k,&m,dollarAllowed,1,1,out+l,&n,errPos,errText); // Hierarchically tokenise the inside of the brackets
            if (*errPos>=0) { *errPos+=k; return; }
           }
          NEWSTATE(1,out[outpos+1],out[outpos+2]); // Record the one character closing bracket
         }
       }
      else if (trialstate=='O') // a dollar sign
       {
        if ((dollarAllowed)&&(MARKUP_MATCH("$"))) { NEWSTATE(1,0,0); }
       }
      else if (trialstate=='R') // the dot operator
       {
        if (MARKUP_MATCH(".")) { NEWSTATE(1,0,0); }
       }
      else if (trialstate=='S') // assignment operator
       {
        if      (MARKUP_MATCH("="  )) { NEWSTATE(1,0x40,17); }
        else if (MARKUP_MATCH("+=" )) { NEWSTATE(2,0x41,17); }
        else if (MARKUP_MATCH("-=" )) { NEWSTATE(2,0x42,17); }
        else if (MARKUP_MATCH("*=" )) { NEWSTATE(2,0x43,17); }
        else if (MARKUP_MATCH("/=" )) { NEWSTATE(2,0x44,17); }
        else if (MARKUP_MATCH("%=" )) { NEWSTATE(2,0x45,17); }
        else if (MARKUP_MATCH("&=" )) { NEWSTATE(2,0x46,17); }
        else if (MARKUP_MATCH("^=" )) { NEWSTATE(2,0x47,17); }
        else if (MARKUP_MATCH("|=" )) { NEWSTATE(2,0x48,17); }
        else if (MARKUP_MATCH("<<=")) { NEWSTATE(3,0x49,17); }
        else if (MARKUP_MATCH(">>=")) { NEWSTATE(3,0x4A,17); }
       }
      else if (trialstate=='U') // end of expression
       { NEWSTATE(0,0,0); }
      if (state != oldstate) break;
     }
    if (state == oldstate) break; // We've got stuck
   }

  if (state=='U') // We reached state U... end of expression
   {
    *errPos = -1;
    *errText='\0';
    out[outpos]=0;
    if      (isDict)        { out[outpos+1] = nDictItems >>8; out[outpos+2] = nDictItems &255; }
    else if (collectCommas) { out[outpos+1] = nCommaItems>>8; out[outpos+2] = nCommaItems&255; }
    else                    { out[outpos+1] = 0;              out[outpos+2] = 0;               }
    outpos+=3;
    *end = scanpos;
    *outlen = outpos;
    return;
   }
  else
   {
    // Error; we didn't reach state U
    int i=0,j=0;
    int B=0,F=0,G=0,J=0,P=0;
    *errPos = scanpos;
    *end    = -1;
    *outlen = 0;
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

// ppl_tokenPrint() -- a debugging routine to display tokenised data

void ppl_tokenPrint(ppl_context *context, char *in, unsigned char *tdat, int len)
 {
  int i,j;
  for (i=0    ; i<len; i++     ) printf("  %c" ,in[i]);        printf("\n");
  for (i=0,j=0; i<len; i++,j+=3) printf("  %c" ,'@'+tdat[j]);  printf("\n");
  for (i=0,j=1; i<len; i++,j+=3) printf(" %02x",(int)tdat[j]); printf("\n");
  for (i=0,j=2; i<len; i++,j+=3) printf(" %02x",(int)tdat[j]); printf("\n");
  return;
 }  

// ppl_expCompile -- compile a textual expression into reverse Polish bytecode

#define GET_POINTER \
 { \
  if (lastoutpos<0) { *errPos=ipos; strcpy(errText, "Internal error: Could not find variable name preceding assignment operator (1)."); *end=-1; return; } \
  if      (*(unsigned char *)(out+lastoutpos)==3) *(unsigned char *)(out+lastoutpos)=4; \
  else if (*(unsigned char *)(out+lastoutpos)==5) *(unsigned char *)(out+lastoutpos)=6; \
  else if (*(unsigned char *)(out+lastoutpos)==7) *(unsigned char *)(out+lastoutpos)=16; \
  else { printf("%d %d\n",lastoutpos,*(unsigned char *)(out+lastoutpos)); *errPos=ipos; strcpy(errText, "Internal error: Could not find variable name preceding assignment operator (2)."); *end=-1; return; } \
 }

#define POP_STACK \
 { \
  char opType; \
  while (opType = (stackpos>2) ? *(stack-stackpos+3) : '!'  ,  (opType=='o')||(opType=='a')||(opType=='$')) /* while the stack is not empty and has an operator at the top */ \
   { \
    int stackprec = *(stack-stackpos+1); \
    if ( ((!rightAssoc)&&(precedence>=stackprec)) || ((rightAssoc)&&(precedence>stackprec)) ) /* pop operators with higher precedence from stack */ \
     { \
      unsigned char stacko   = *(stack-stackpos+2); /* operator number of the stack object */ \
      unsigned char bytecode = 0; \
      if      (opType=='a' )   bytecode = 12;                /* assignment operator */ \
      else if (opType=='$' )   bytecode = 15;                /* dollar operator */ \
      else if (stacko==0x40)   bytecode = 14;                /* string subst operator */ \
      else if (stacko==0xA3) { bytecode = 13; GET_POINTER; } /* ++ lval operator */ \
      else if (stacko==0xA4) { bytecode = 13; GET_POINTER; } /* -- lval operator */ \
      else                     bytecode = 11;                /* other operator */ \
      if ((stacko!=0xFD)&&(stacko!=0x5F)) /* null operations get ignored (e.g. commas that collect items, and ? waiting for :s) */ \
       { \
        lastoutpos = outpos; \
        *(unsigned char *)(out+outpos++) = bytecode; \
        *(unsigned char *)(out+outpos++) = stacko; \
        if ((opType=='o')&&(stacko==0x40)) \
         { \
          *(int *)(out+outpos-1) = 1; /* string substitution operator with only one subst item (not a bracketed list) */ \
          outpos += sizeof(int)-1; \
         } \
       } \
      stackpos-=3; /* pop stack */ \
     } \
    else break; \
   } \
 }

void ppl_expCompile(ppl_context *context, char *in, int *end, int dollarAllowed, void *out, int *outlen, void *tmp, int tmplen, int *errPos, char *errText)
 {
  unsigned char *stack = ( (unsigned char *)tmp ) + tmplen - 1;
  unsigned char *tdata =   (unsigned char *)tmp;
  int stackpos = 0;
  int stacklen;
  int tpos, ipos;
  int tlen = tmplen;
  int outpos = 0, lastoutpos = -1;

  // First tokenise expression
  ppl_expTokenise(context, in, end, dollarAllowed, 0, 0, tdata, &tlen, errPos, errText);
  if (*errPos >= 0) return;
  stacklen = tmplen - tlen;

  // The stacking-yard algorithm
  for ( tpos=ipos=0; tpos<tlen; )
   {
    char o = tdata[tpos]+'@'; // Get tokenised markup state code
    if (o=='B') // Process a string literal
     {
      int  i;
      char quoteType=in[ipos];
      lastoutpos = outpos;
      *(unsigned char *)(out+outpos++) = 2; // bytecode op 2
      for ( i=ipos+1 ; ((in[i]!=quoteType) || (in[i-1]=='\\')) ; i++ )
       {
        if (in[i]=='\0') { *errPos=i; strcpy(errText, "Internal error: Unexpected end of string."); *end=-1; return; }
        if ((in[i]=='\\') && (in[i-1]!='\\')) continue; // We have a double backslash
        *(char *)(out+outpos++) = in[i];
       }
      *(unsigned char *)(out+outpos++) = '\0';
     }
    else if ( (o=='C') || (o=='H') || (o=='I') || (o=='J') || (o=='K') || (o=='S') ) // Process an operator
     {
      unsigned char rightAssoc = (tdata[tpos+1] & 0x80) != 0;
      unsigned char precedence = tdata[tpos+2];
      POP_STACK;
      if (o=='S') { GET_POINTER; } // Turn variable lookup into pointer lookup now, because assignment operators are binary and operand is guaranteed to be the last thing on the stack
      *(stack-stackpos-0) = (o=='S')?'a':'o'; // push operator onto stack
      *(stack-stackpos-1) = tdata[tpos+1];
      *(stack-stackpos-2) = tdata[tpos+2];
      stackpos+=3;
     }
    else if ( (o=='D') || (o=='E') || (o=='P') ) // Process ( )
     {
      char bracketType=in[ipos];
      if (bracketType=='(') // open (
       {
        *(stack-stackpos-0) = '('; // push bracket onto stack
        *(stack-stackpos-1) = *(stack-stackpos-2) = 0;
        stackpos+=3;
       }
      else // close )
       {
        unsigned char rightAssoc = 0;
        unsigned char precedence = 255;
        POP_STACK; // Pop the stack of all operators (fake operator above with very low precedence)
        if ( (stackpos<3) || (*(stack-stackpos+3)!='(') ) { *errPos=ipos; strcpy(errText, "Internal error: Could not match ) to an (."); *end=-1; return; }
        stackpos-=3; // pop bracket
        if (o=='D') // apply string substitution operator straight away
         {
          if ( (stackpos<3) || (*(stack-stackpos+2)!=0x40)) { *errPos=ipos; strcpy(errText, "Internal error: Could not match string substituion () to a %."); *end=-1; return; }
          stackpos-=3; // pop stack
          lastoutpos = outpos;
          *(unsigned char *)(out+outpos++) = 14; // bytecode 14 -- string substitution operator
          *(int *)(out+outpos) = (int)256*(tdata[tpos+1]) + tdata[tpos+2]; // store the number of string substitutions; stored with closing bracket in bytecode is number of collected ,-separated items
          outpos += sizeof(int);
         }
        else if (o=='P') // make function call
         {
          lastoutpos = outpos;
          *(unsigned char *)(out+outpos++) = 10; // bytecode 10 -- function call
          *(int *)(out+outpos) = (int)256*(tdata[tpos+1]) + tdata[tpos+2]; // store the number of function arguments; stored with closing bracket in bytecode is number of collected ,-separated items
          outpos += sizeof(int);
         }
       }
     }
    else if (o=='F') // a postfix ++ or -- operator
     {
      unsigned char rightAssoc = (tdata[tpos+1] & 0x80) != 0;
      unsigned char precedence = tdata[tpos+2];
      POP_STACK;
      GET_POINTER;
      lastoutpos = outpos;
      *(unsigned char *)(out+outpos++) = 13;
      *(unsigned char *)(out+outpos++) = tdata[tpos+1];
     }
    else if ( (o=='G') || (o=='T') || (o=='V') ) // a variable name or field dereferenced with the . operator
     {
      int  i;
      lastoutpos = outpos;
      if      (o=='G') *(unsigned char *)(out+outpos++) = 3; // foo -- op 3: variable lookup "foo"
      else if (o=='T') *(unsigned char *)(out+outpos++) = 5; // foo.bar -- op 5: dereference "bar"
      else             *(unsigned char *)(out+outpos++) = 2; // $foo -- push "foo" onto stack as a string constant
      for ( i=ipos ; (isalnum(in[i])) ; i++ )
       {
        if (in[i]=='\0') { *errPos=ipos; strcpy(errText, "Internal error: Unexpected end of variable name."); *end=-1; return; }
        *(char *)(out+outpos++) = in[i];
       }
      *(unsigned char *)(out+outpos++) = '\0';
     }
    else if (o=='L') // a numeric literal
     {
      lastoutpos = outpos;
      *(unsigned char *)(out+outpos++) = 1;
      *(double *)(out+outpos) = ppl_getFloat(in+ipos,NULL);
      outpos += sizeof(double);
     }
    else if (o=='M') // list literal
     {
      char bracketType=in[ipos];
      if (bracketType=='[') // open [
       {
        *(stack-stackpos-0) = '['; // push bracket onto stack
        *(stack-stackpos-1) = *(stack-stackpos-2) = 0;
        stackpos+=3;
       }
      else // close ]
       {
        unsigned char rightAssoc = 0;
        unsigned char precedence = 255;
        POP_STACK; // Pop the stack of all operators (fake operator above with very low precedence)
        if ( (stackpos<3) || (*(stack-stackpos+3)!='[') ) { *errPos=ipos; strcpy(errText, "Internal error: Could not match ] to an [."); *end=-1; return; }
        stackpos-=3; // pop bracket
        lastoutpos = outpos;
        *(unsigned char *)(out+outpos++) = 9; // bytecode 9 -- make list
        *(int *)(out+outpos) = (int)256*(tdata[tpos+1]) + tdata[tpos+2]; // store the number of list items; stored with closing bracket in bytecode is number of collected ,-separated items
        outpos += sizeof(int);
       }
     }
    else if (o=='N') // dictionary literal
     {
      char bracketType=in[ipos];
      if (bracketType=='{') // open {
       {
        *(stack-stackpos-0) = '{'; // push bracket onto stack
        *(stack-stackpos-1) = *(stack-stackpos-2) = 0;
        stackpos+=3;
       }
      else // close }
       {
        unsigned char rightAssoc = 0;
        unsigned char precedence = 255;
        POP_STACK; // Pop the stack of all operators (fake operator above with very low precedence)
        if ( (stackpos<3) || (*(stack-stackpos+3)!='{') ) { *errPos=ipos; strcpy(errText, "Internal error: Could not match } to an {."); *end=-1; return; }
        stackpos-=3; // pop bracket
        lastoutpos = outpos;
        *(unsigned char *)(out+outpos++) = 8; // bytecode 8 -- make dict
        *(int *)(out+outpos) = (int)256*(tdata[tpos+1]) + tdata[tpos+2]; // store the number of list items; stored with closing bracket in bytecode is number of collected ,-separated items
        outpos += sizeof(int);
       }
     }
    else if (o=='O') // a dollar operator
     {
      *(stack-stackpos-0) = '$'; // push dollar onto stack
      *(stack-stackpos-1) = *(stack-stackpos-2) = 0;
      stackpos+=3;
     }
    else if (o=='R') // a dot dereference
     {
      // Nop -- the work is done in the following T-state parameter.
     }
    else if (o=='Q') // array dereference or slice
     {
     }
    while ((tpos<tlen) && (tdata[tpos]+'@' == o))
     {
      tpos+=3; ipos++;
      if (tpos>=tlen) break;
      if ((o=='D') && ((in[ipos]=='(')||(in[ipos]==')'))) break; // Empty list ( ) of function arguments
      if ((o=='P') && ((in[ipos]=='(')||(in[ipos]==')'))) break; // Empty list ( ) of function arguments
      if ((o=='M') && ((in[ipos]=='[')||(in[ipos]==']'))) break; // Empty list [ ] is two tokens
      if ((o=='N') && ((in[ipos]=='{')||(in[ipos]=='}'))) break; // Empty dictionary { } is two tokens
     }
   }

  // Clean up stack
  {
   unsigned char rightAssoc = 0;
   unsigned char precedence = 255;
   POP_STACK;
  }

  // If there are still brackets on the stack, we've failed
  if (stackpos!=0)
   {
    *errPos=0; strcpy(errText, "Internal error: Unexpected junk left on stack."); *end=-1; return;
   }

  // Store final return
  lastoutpos = outpos;
  *(unsigned char *)(out+outpos++) = 0;
  *outlen = outpos;
  return;
 }

// Debugging routine to produce a textual representation of reverse Polish bytecode

void ppl_reversePolishPrint(ppl_context *context, void *in, char *out)
 {
  int i=0, j=0;
  char op[32],optype[32],arg[1024];

  while (1)
   {
    int o=(int)(*(unsigned char *)(in+j  ));
    int t=(int)(*(unsigned char *)(in+j+1));
    switch (o)
     {
      case 0:
        strcpy (op,     "return");
        strcpy (optype, "");
        strcpy (arg,    "");
        break;
      case 1:
        strcpy (op,     "push");
        strcpy (optype, "numeric");
        sprintf(arg,    "%.2e", *(double *)(in+j+1));
        j+=1+sizeof(double);
        break;
      case 2:
        strcpy (op,     "push");
        strcpy (optype, "string");
        sprintf(arg,    "\"%s\"", (char *)(in+j+1));
        j+=1+strlen((char *)(in+j+1))+1;
        break;
      case 3:
        strcpy (op,     "lookup");
        strcpy (optype, "value");
        sprintf(arg,    "\"%s\"", (char *)(in+j+1));
        j+=1+strlen((char *)(in+j+1))+1;
        break;
      case 4:
        strcpy (op,     "lookup");
        strcpy (optype, "pointer");
        sprintf(arg,    "\"%s\"", (char *)(in+j+1));
        j+=1+strlen((char *)(in+j+1))+1;
        break;
      case 5:
        strcpy (op,     "deref");
        strcpy (optype, "value");
        sprintf(arg,    "\"%s\"", (char *)(in+j+1));
        j+=1+strlen((char *)(in+j+1))+1;
        break;
      case 6:
        strcpy (op,     "deref");
        strcpy (optype, "pointer");
        sprintf(arg,    "\"%s\"", (char *)(in+j+1));
        j+=1+strlen((char *)(in+j+1))+1;
        break;
      case 7:
        strcpy (op,     "slice");
        strcpy (optype, "value");
        sprintf(arg,    "[%d:%d]", *(int *)(in+j+1), *(int *)(in+j+1+sizeof(int)));
        j+=1+2*sizeof(int);
        break;
      case 16:
        strcpy (op,     "slice");
        strcpy (optype, "pointer");
        sprintf(arg,    "[%d:%d]", *(int *)(in+j+1), *(int *)(in+j+1+sizeof(int)));
        j+=1+2*sizeof(int);
        break;
      case 8:
        strcpy (op,     "make");
        strcpy (optype, "dict");
        sprintf(arg,    "%d items", *(int *)(in+j+1));
        j+=1+sizeof(int);
        break;
      case 9:
        strcpy (op,     "make");
        strcpy (optype, "list");
        sprintf(arg,    "%d items", *(int *)(in+j+1));
        j+=1+sizeof(int);
        break;
      case 10:
        strcpy (op,     "call");
        strcpy (optype, "");
        sprintf(arg,    "%d args", *(int *)(in+j+1));
        j+=1+sizeof(int);
        break;
      case 11:
        strcpy (op,     "op");
        if      (((t>>5)&3)==1) strcpy (optype, "unary");
        else if (((t>>5)&3)==2) strcpy (optype, "binary");
        else if (((t>>5)&3)==3) strcpy (optype, "ternary");
        else                    strcpy (optype, "???");
        if      (t==0x25) strcpy (arg, "-"  );
        else if (t==0x26) strcpy (arg, "+"  );
        else if (t==0xA7) strcpy (arg, "~"  );
        else if (t==0xA8) strcpy (arg, "!"  );
        else if (t==0xC9) strcpy (arg, "**" );
        else if (t==0x4A) strcpy (arg, "*"  );
        else if (t==0x4B) strcpy (arg, "/"  );
        else if (t==0x4C) strcpy (arg, "%"  );
        else if (t==0x4D) strcpy (arg, "+"  );
        else if (t==0x4E) strcpy (arg, "-"  );
        else if (t==0x4F) strcpy (arg, "<<" );
        else if (t==0x50) strcpy (arg, ">>" );
        else if (t==0x51) strcpy (arg, "<"  );
        else if (t==0x52) strcpy (arg, "<=" );
        else if (t==0x53) strcpy (arg, ">=" );
        else if (t==0x54) strcpy (arg, ">"  );
        else if (t==0x55) strcpy (arg, "==" );
        else if (t==0x56) strcpy (arg, "!=" );
        else if (t==0x57) strcpy (arg, "&"  );
        else if (t==0x58) strcpy (arg, "^"  );
        else if (t==0x59) strcpy (arg, "|"  );
        else if (t==0x5A) strcpy (arg, "&&" );
        else if (t==0x5B) strcpy (arg, "||" );
        else if (t==0x5C) strcpy (arg, "swap-pop");
        else if (t==0xFD) strcpy (arg, "nop (?)" );
        else if (t==0xFE) strcpy (arg, "?:" );
        else if (t==0x5F) strcpy (arg, "nop (collect)" );
        else              strcpy (arg, "???");
        j+=2;
        break;
      case 12:
        strcpy (op,     "op");
        strcpy (optype, "assign");
        if      (t==0x40) strcpy (arg, "="  );
        else if (t==0x41) strcpy (arg, "+=" );
        else if (t==0x42) strcpy (arg, "-=" );
        else if (t==0x43) strcpy (arg, "*=" );
        else if (t==0x44) strcpy (arg, "/=" );
        else if (t==0x45) strcpy (arg, "%=" );
        else if (t==0x46) strcpy (arg, "&=" );
        else if (t==0x47) strcpy (arg, "^=" );
        else if (t==0x48) strcpy (arg, "|=" );
        else if (t==0x49) strcpy (arg, "<<=");
        else if (t==0x4A) strcpy (arg, ">>=");
        else              strcpy (arg, "???");
        j+=2;
        break;
      case 13:
        strcpy (op,     "op");
        strcpy (optype, "unary");
        if      (t==0xA1) { strcpy (arg, "-- (post-eval)"); }
        else if (t==0xA2) { strcpy (arg, "++ (post-eval)"); }
        else if (t==0xA3) { strcpy (arg, "-- (pre-eval)"); }
        else              { strcpy (arg, "++ (pre-eval)"); }
        j+=2;
        break;
      case 14:
        strcpy (op,     "op");
        strcpy (optype, "binary");
        sprintf(arg,    "string subst (%d items)", *(int *)(in+j+1));
        j+=1+sizeof(int); 
        break;
      case 15:
        strcpy (op,     "op");
        strcpy (optype, "unary");
        strcpy (arg,    "dollar -- column lookup");
        j+=2;
        break;
      default:
        strcpy (op,     "???");
        strcpy (optype, "");
        strcpy (arg,    "Illegal opcode");
        o=0;
        break;
     }
    sprintf(context->errcontext.tempErrStr,"%10s %10s %s",op,optype,arg);
    ppl_report(&context->errcontext, context->errcontext.tempErrStr);
    if (o==0) break;
   }
  out[i]='\0'; // null terminate string
  return;
 }

