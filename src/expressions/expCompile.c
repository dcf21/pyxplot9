// expCompile.c
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

#define _EXPCOMPILE_C 1

#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "coreUtils/errorReport.h"
#include "coreUtils/memAlloc.h"
#include "stringTools/asciidouble.h"
#include "userspace/context.h"
#include "expressions/expCompile_fns.h"

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
// B can be followed by ..C......JK.....QR..U.
// C can be followed by .B.D..GHI..LMNO.......
// D can be followed by .........JK.....Q...U.
// E can be followed by .....F...JK.....QR..U.
// F can be followed by .........JK.........U.
// G can be followed by .....F...JK....PQRS.U.
// H can be followed by ....E.G...............
// I can be followed by .B..E.GH...LMNO.......
// J can be followed by .B..E.GHI..LMNO.......
// K can be followed by .B..E.GHI..LMNO.......
// L can be followed by .........JK.........U.
// M can be followed by .........JK.....QR..U.
// N can be followed by .........JK.....QR..U.
// O can be followed by ....E......L.........V
// P can be followed by .........JK....PQR..U.
// Q can be followed by .....F...JK....PQRS.U.
// R can be followed by ...................T..
// S can be followed by .B..E.GHI..LMNO.......
// T can be followed by .....F...JK....PQRS.U.
// V can be followed by .........JK.........U.

#define MARKUP_MATCH(A) (strncmp(in+scanpos,A,strlen(A))==0)

#define PGE_OVERFLOW \
 { \
  context->tokenBuffLen *= 2; \
  context->tokenBuff     = (pplTokenCode *)realloc(context->tokenBuff, context->tokenBuffLen * sizeof(pplTokenCode)); \
  if (context->tokenBuff == NULL) { *errPos = scanpos; *errType=ERR_OVERFLOW; strcpy(errText, "Algebraic expression too long"); *end = -1; return; } \
  out = context->tokenBuff + outOffset; \
  buffSize = context->tokenBuffLen - outOffset; \
 }

#define NEWSTATE(L,O,P) \
 { \
  int i; \
  state=trialstate; \
  opcode=O; \
  precedence=P; \
  for (i=0;i<L;i++) \
   { \
    out[outpos].state      = state; \
    out[outpos].opcode     = opcode; \
    out[outpos].precedence = precedence; \
    outpos++; \
    if (outpos>=buffSize) PGE_OVERFLOW; \
   } \
  scanpos+=L; \
 }

#define SAMESTATE \
 { \
  out[outpos].state      = state; \
  out[outpos].opcode     = opcode; \
  out[outpos].precedence = precedence; \
  outpos++; \
  if (outpos>=buffSize) PGE_OVERFLOW; \
  scanpos++; \
 }

#define FFWSTATE(L) \
 { \
  while (outpos+L>=buffSize) PGE_OVERFLOW; \
  outpos  += (L); \
  scanpos += (L); \
 }

void ppl_expTokenise(ppl_context *context, char *in, int *end, int dollarAllowed, int equalsAllowed, int allowCommaOperator, int collectCommas, int isDict, int outOffset, int *outlen, int *errPos, int *errType, char *errText)
 {
  const char    *allowed[] = {"BEHILMNOG","CJKQRU","BDHILMNOG","JKQU","FJKQRU","JKU","SFJKPQRU","EG","BEHLMNOG","BEHILMNOG","BEHILMNOG","JKU","JKQRU","JKQRU","ELV","JKPQRU","SFJKPQRU","T","BEHILMNOG","SFJKPQRU","","JKU"};
  int            nCommaItems=1, nDictItems=0, tertiaryDepth=0;
  char           state='A', trialstate;
  int            scanpos=0, outpos=0, oldpos, trialpos;
  unsigned char  opcode=0, precedence=0;
  pplTokenCode  *out;
  int            buffSize;
  int            expOp = 0; // Was last named function one which acts on an expression -- e.g. unit() or int_dx()?
  int            extraArg = 0; // Was last named function one which has a variable name encoded in it -- i.e. int_dx() or diff_dx()?

  if ( (outOffset==0) && (context->tokenBuffLen > ALGEBRA_MAXLEN) )
   {
    free(context->tokenBuff);
    context->tokenBuff    = NULL;
    context->tokenBuffLen = 0;
   }

  if (context->tokenBuff == NULL)
   {
    context->tokenBuff    = (pplTokenCode *)malloc(ALGEBRA_MAXLEN * sizeof(pplTokenCode));
    context->tokenBuffLen = ALGEBRA_MAXLEN;
   }

  if (context->tokenBuff == NULL)
   {
    *errPos = 0; *errType=ERR_MEMORY; strcpy(errText, "Out of memory"); *end = -1; return;
   }

  out = context->tokenBuff + outOffset;
  buffSize = context->tokenBuffLen - outOffset;

  while (state!='U')
   {
    while ((in[scanpos]!='\0') && (in[scanpos]<=' ')) { SAMESTATE; } // Sop up whitespace
    oldpos = scanpos;

    for (trialpos=0; ((trialstate=allowed[(int)(state-'A')][trialpos])!='\0'); trialpos++)
     {
      if ((expOp)&&(trialstate!='P')) continue; // int_dx and diff_dx MUST be followed by arguments; they have already put one on the stack.
      if      (trialstate=='B') // string literal
       {
        char quoteType;
        int  raw=0;
        if ( (in[scanpos]=='r') && ((in[scanpos+1]=='\'') || (in[scanpos+1]=='"')) ) { raw=1; NEWSTATE(1,0,0); } // r'hello' is raw string
        if ( (in[scanpos]=='e') && ((in[scanpos+1]=='\'') || (in[scanpos+1]=='"')) ) { raw=0; NEWSTATE(1,0,0); } // e'hello' is string with extended escapes
        if ( (in[scanpos]==(quoteType='\'')) || (in[scanpos]==(quoteType='"')) )
         {
          int j=1, tripleQuote=0;
          if ((in[scanpos+1]==quoteType)&&(in[scanpos+2]==quoteType)) { j=3; tripleQuote=1; }
          for ( ; (in[scanpos+j]!='\0') &&
                  ( ((!tripleQuote) && (in[scanpos+j]!=quoteType)) ||
                    (  tripleQuote  && ((in[scanpos+j]!=quoteType)||(in[scanpos+j+1]!=quoteType)||(in[scanpos+j+2]!=quoteType)) )
                  )
                ; j++)
            if ((!raw)&&(in[scanpos+j]=='\\')&&(in[scanpos+j+1]!='\0')) j++;
          if (in[scanpos+j]==quoteType) { if (tripleQuote) { j+=3; } else { j++; } NEWSTATE(j,0,0); }
          else                          { *errPos = scanpos; *errType=ERR_SYNTAX; strcpy(errText, "Mismatched quote."); *end=-1; *outlen=0; return; }
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
          int j,k,l,m,n=1,cp[3],cpl=3,extraStrArg=0;
          ppl_strBracketMatch(in+scanpos,'(',')',cp,&cpl,&j,cpl); // Search for a ) to match the (
          if (j<=0) { *errPos = scanpos; *errType=ERR_SYNTAX; strcpy(errText, "Mismatched ( )"); *end=-1; *outlen=0; return; }
          NEWSTATE(1,0,0); // Record the one character opening bracket
          while ((in[scanpos]!='\0') && (in[scanpos]<=' ')) { SAMESTATE; n++; } // Sop up whitespace
          if ((trialstate=='P')&&(expOp)&&(in[scanpos]!='\'')&&(in[scanpos]!='"')) // Function call acting on expression: save expression as string even though it's not quoted
           {
            int explen = cp[1]+(cpl!=1)-n;
            if (expOp>1) explen=j-n; // texify receives all arguments as a string
            if (cpl<1) { *errPos=scanpos; *errType=ERR_INTERNAL; strcpy(errText, "ppl_strBracketMatch returned fewer than two results."); *end=-1; *outlen=0; return; }
            trialstate='B'; NEWSTATE(explen,0,0); trialstate='P';
            n+=explen;
            extraStrArg=(explen>0);
           }
          k=scanpos; l=outpos; j-=n;
          if ((in[k]!=')')||(trialstate=='E'))
           {
            ppl_expTokenise(context,in+k,&m,dollarAllowed,1,1,(trialstate!='E'),0,l+outOffset,&n,errPos,errType,errText); // Hierarchically tokenise the inside of the brackets
            if (*errPos>=0) { *errPos+=k; return; }
            if (m!=j) { *errPos = m+k; *errType=ERR_SYNTAX; strcpy(errText, "Unexpected trailing matter at end of expression."); *end=-1; *outlen=0; return; }
           }
          else out[outpos].depth = 0;
          FFWSTATE(j); // Fast-forward to closing bracket
          if (extraStrArg) out[outpos].depth++;
          if (extraArg)    out[outpos].depth++;
          NEWSTATE(1,0,0); // Record the one character closing bracket
          extraArg=0;
          expOp=0;
         }
       }
      else if (trialstate=='F') // a unary post-lvalue operator
       {
        if      (MARKUP_MATCH("--")) { NEWSTATE(2,0x21,3); }
        else if (MARKUP_MATCH("++")) { NEWSTATE(2,0x22,3); }
       }
      else if (trialstate=='H')  // a unary pre-lvalue operator
       {
        if      (MARKUP_MATCH("--")) { NEWSTATE(2,0x23,3); }
        else if (MARKUP_MATCH("++")) { NEWSTATE(2,0x24,3); }
       }
      else if ( (trialstate=='G') ||  // a variable name
                (trialstate=='T') ||  // a dereference name
                (trialstate=='V')   ) // a $parameter name
       {
        if (isalpha(in[scanpos]))
         {
          const char tmp = trialstate;
          if      ((strncmp(in+scanpos,"unit",4)==0) && (!isalnum(in[scanpos+4])) && (in[scanpos+4]!='_')) // unit() function
           {
            expOp = 1;
            NEWSTATE(4,0,0);
           }
          else if (strncmp(in+scanpos,"int_d",5)==0) // int_d() function
           {
            if (!isalpha(in[scanpos+5])) { *errPos=scanpos; *errType=ERR_SYNTAX; strcpy(errText, "System function int_d should be followed by a variable name to integrate over."); *end=-1; *outlen=0; return; }
            NEWSTATE(5,0,0);
            trialstate='B'; NEWSTATE(1,0,0);
            while ((isalnum(in[scanpos])) || (in[scanpos]=='_')) { SAMESTATE; }
            trialstate=tmp; NEWSTATE(0,0,0);
            expOp = 1;
            extraArg=1;
           }
          else if (strncmp(in+scanpos,"diff_d",6)==0) // diff_d() function
           {
            if (!isalpha(in[scanpos+6])) { *errPos=scanpos; *errType=ERR_SYNTAX; strcpy(errText, "System function diff_d should be followed by a variable name to differentiate with respect to."); *end=-1; *outlen=0; return; }
            NEWSTATE(6,0,0);
            trialstate='B'; NEWSTATE(1,0,0);
            while ((isalnum(in[scanpos])) || (in[scanpos]=='_')) { SAMESTATE; }
            trialstate=tmp; NEWSTATE(0,0,0);
            expOp = 1;
            extraArg=1;
           }
          else
           {
            NEWSTATE(1,0,0);
            while ((isalnum(in[scanpos])) || (in[scanpos]=='_')) { SAMESTATE; }
            expOp = 0;
           }
         }
       }
      else if (trialstate=='I') // a unary value-based operator
       {
        if      (MARKUP_MATCH("-"  )) { NEWSTATE(1,0x25, 3); }
        else if (MARKUP_MATCH("+"  )) { NEWSTATE(1,0x26, 3); }
        else if (MARKUP_MATCH("~"  )) { NEWSTATE(1,0xA7, 3); }
        else if (MARKUP_MATCH("!"  )) { NEWSTATE(1,0xA8, 3); }
        else if (MARKUP_MATCH("not")&&(!isalpha(in[scanpos+3]))) { NEWSTATE(3,0xA8, 3); }
        else if (MARKUP_MATCH("NOT")&&(!isalpha(in[scanpos+3]))) { NEWSTATE(3,0xA8, 3); }
       }
      else if (trialstate=='J') // a binary operator
       {
        if      (MARKUP_MATCH("**" )) { NEWSTATE(2,0xC9, 2); }
        else if (MARKUP_MATCH("<<" )) { NEWSTATE(2,0x4F, 7); }
        else if (MARKUP_MATCH(">>" )) { NEWSTATE(2,0x50, 7); }
        else if (MARKUP_MATCH("<=" )) { NEWSTATE(2,0x52, 8); }
        else if (MARKUP_MATCH(">=" )) { NEWSTATE(2,0x53, 8); }
        else if (MARKUP_MATCH("==" )) { NEWSTATE(2,0x55, 9); }
        else if (MARKUP_MATCH("<>" )) { NEWSTATE(2,0x56,10); }
        else if (MARKUP_MATCH("!=" )) { NEWSTATE(2,0x56,10); }
        else if (MARKUP_MATCH("&&" )) { NEWSTATE(2,0x5A,14); }
        else if (MARKUP_MATCH("and")&&(!isalpha(in[scanpos+3]))) { NEWSTATE(3,0x5A,14); }
        else if (MARKUP_MATCH("AND")&&(!isalpha(in[scanpos+3]))) { NEWSTATE(3,0x5A,14); }
        else if (MARKUP_MATCH("||" )) { NEWSTATE(2,0x5B,15); }
        else if (MARKUP_MATCH("or" )&&(!isalpha(in[scanpos+2]))) { NEWSTATE(2,0x5B,15); }
        else if (MARKUP_MATCH("OR" )&&(!isalpha(in[scanpos+2]))) { NEWSTATE(2,0x5B,15); }
        else if (MARKUP_MATCH("*"  )) { NEWSTATE(1,0x4A, 5); }
        else if (MARKUP_MATCH("/"  )) { NEWSTATE(1,0x4B, 5); }
        else if (MARKUP_MATCH("%"  )) { NEWSTATE(1,0x4C, 5); }
        else if (MARKUP_MATCH("+"  )) { NEWSTATE(1,0x4D, 6); }
        else if (MARKUP_MATCH("-"  )) { NEWSTATE(1,0x4E, 6); }
        else if (MARKUP_MATCH("<"  )) { NEWSTATE(1,0x51, 8); }
        else if (MARKUP_MATCH(">"  )) { NEWSTATE(1,0x54, 8); }
        else if (MARKUP_MATCH("&"  )) { NEWSTATE(1,0x57,11); }
        else if (MARKUP_MATCH("^"  )) { NEWSTATE(1,0x58,12); }
        else if (MARKUP_MATCH("|"  )) { NEWSTATE(1,0x59,13); }
        else if((MARKUP_MATCH(","  ) && (collectCommas || allowCommaOperator || (tertiaryDepth>0))))
         {
          if ((!collectCommas)||(tertiaryDepth>0)) { NEWSTATE(1,0x5C,18); }
          else
           {
            if (isDict && (nCommaItems>=2-(nDictItems==0))) { *errPos = scanpos; *errType=ERR_SYNTAX; strcpy(errText, "Expecting : followed by value for dictionary key"); *end=-1; *outlen=0; return; }
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
            if (nCommaItems<2-(nDictItems==0)) { *errPos = scanpos; *errType=ERR_SYNTAX; strcpy(errText, "Expecting , to separate dictionary items"); *end=-1; *outlen=0; return; }
            NEWSTATE(1,0x5F,18);
            nCommaItems=1;
            nDictItems++;
           }
          else
           {
            if (tertiaryDepth>0)
             {
              tertiaryDepth--;
              NEWSTATE(1,0xFE,16);
             }
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
        expOp=0;
        if (MARKUP_MATCH("["))
         {
          int j,k,l,m=0,n=1;
          int slice=0,minSet=0,maxSet=0;
          ppl_strBracketMatch(in+scanpos,'[',']',NULL,NULL,&j,0); // Search for a ] to match the [
          if (j<=0) { *errPos = scanpos; *errType=ERR_SYNTAX; strcpy(errText, "Mismatched [ ]"); *end=-1; *outlen=0; return; }
          NEWSTATE(1,0,0); // Record the one character opening bracket
          if ((trialstate=='Q') && (in[scanpos]==':')) // slice of the form [:x]
           {
            SAMESTATE; n++; slice=1;
            while ((in[scanpos]!='\0') && (in[scanpos]<=' ')) { SAMESTATE; n++; } // Sop up whitespace
           }
          k=scanpos; l=outpos; j-=n;
          if ((in[k]!=']') || ((trialstate=='Q')&&(!slice))) // [] is allowed as a list literal, but not as an array dereference
           {
            if (slice) maxSet=1;
            ppl_expTokenise(context,in+k,&m,dollarAllowed,1,1,trialstate!='Q',0,l+outOffset,&n,errPos,errType,errText); // Hierarchically tokenise the inside of the brackets
            if (*errPos>=0) { *errPos+=k; return; }
            if ((trialstate=='Q')&&(!slice)&&(in[k+m]==':'))
             {
              slice = minSet = 1;
              FFWSTATE(m); j-=m; // Fast-forward over first expression in range
              trialstate='J'; NEWSTATE(1,0x5F,18); j--; // Treat : separating ranges as a , operator (collect items on stack)
              while ((in[scanpos]!='\0') && (in[scanpos]<=' ')) { SAMESTATE; j--; } // Sop up whitespace
              trialstate='Q';
              k=scanpos; l=outpos; m=0;
              if (j>0)
               {
                maxSet=1;
                ppl_expTokenise(context,in+k,&m,dollarAllowed,1,1,trialstate!='Q',0,l+outOffset,&n,errPos,errType,errText); // Hierarchically tokenise the inside of the brackets
                if (*errPos>=0) { *errPos+=k; return; }
               }
             }
            if (m!=j) { *errPos = m+k; *errType=ERR_SYNTAX; strcpy(errText, "Unexpected trailing matter at end of expression"); *end=-1; *outlen=0; return; }
           }
          else out[outpos].depth = 0;
          FFWSTATE(j); // Fast-forward to closing bracket
          if (trialstate=='Q')
           {
            out[outpos].opcode     = (((minSet<<1) + maxSet)<<1) + !slice;
            out[outpos].precedence = 0;
           }
          NEWSTATE(1, out[outpos].opcode, out[outpos].precedence); // Record the one character closing bracket
         }
       }
      else if (trialstate=='N') // a dictionary literal
       {
        if (MARKUP_MATCH("{"))
         {
          int j,k,l,m,n=1;
          ppl_strBracketMatch(in+scanpos,'{','}',NULL,NULL,&j,0); // Search for a } to match the {
          if (j<=0) { *errPos = scanpos; *errType=ERR_SYNTAX; strcpy(errText, "Mismatched { }"); *end=-1; *outlen=0; return; }
          NEWSTATE(1,0,0); // Record the one character opening bracket
          while ((in[scanpos]!='\0') && (in[scanpos]<=' ')) { SAMESTATE; n++;} // Sop up whitespace
          k=scanpos; l=outpos; j-=n;
          if (in[k]!='}')
           {
            ppl_expTokenise(context,in+k,&m,dollarAllowed,1,1,1,1,l+outOffset,&n,errPos,errType,errText); // Hierarchically tokenise the inside of the brackets
            if (*errPos>=0) { *errPos+=k; return; }
            if (m!=j) { *errPos = m+k; *errType=ERR_SYNTAX; strcpy(errText, "Unexpected trailing matter at end of expression"); *end=-1; *outlen=0; return; }
           }
          else out[outpos].depth = 0;
          FFWSTATE(j); // Fast-forward to closing bracket
          NEWSTATE(1,0,0); // Record the one character closing bracket
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
        if     ((MARKUP_MATCH("="  )&&(in[scanpos+1]!='=')&&equalsAllowed)) { NEWSTATE(1,0x40,17); } // Match = but not ==
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
        else if (MARKUP_MATCH("**=")) { NEWSTATE(3,0x4B,17); }
       }
      else if (trialstate=='U') // end of expression
       { NEWSTATE(0,0,0); }
      if (scanpos != oldpos) break;
     }
    if (scanpos == oldpos) break; // We've got stuck
   }

  if (state=='U') // We reached state U... end of expression
   {
    *errPos = -1;
    *errType = 0;
    *errText='\0';
    out[outpos].state  = '@';
    out[outpos].opcode = 0;
    if      (isDict)        { out[outpos].depth = nDictItems; }
    else if (collectCommas) { out[outpos].depth = nCommaItems; }
    else                    { out[outpos].depth = 0; }
    outpos++;
    *end    = scanpos;
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
    *errType = ERR_SYNTAX;
    strcpy(errText,"At this point, was expecting ");
    i=strlen(errText);
    for (trialpos=0; ((trialstate=allowed[(int)(state-'A')][trialpos])!='\0'); trialpos++)
     {
      if ((expOp)&&(trialstate!='P')) continue; // int_dx and diff_dx MUST be followed by arguments; they have already put one on the stack.

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
      else if (trialstate=='P') { W; strcpy(errText+i,"a list of function arguments"); i+=strlen(errText+i); }
      else if ((!P)&&((trialstate=='Q')||(trialstate=='R')))
                                { W; strcpy(errText+i,"an object dereference"); i+=strlen(errText+i); P=1; }
     }
    strcpy(errText+i,"."); i+=strlen(errText+i);
    return;
   }
 }

// ppl_tokenPrint() -- a debugging routine to display tokenised data

void ppl_tokenPrint(ppl_context *context, char *in, int len)
 {
  pplTokenCode  *tdat = context->tokenBuff;
  int            i;
  for (i=0; i<len; i++) printf("  %c" ,in[i]);                   printf("\n");
  for (i=0; i<len; i++) printf("  %c" ,tdat[i].state);           printf("\n");
  for (i=0; i<len; i++) printf(" %02x",(int)tdat[i].opcode);     printf("\n");
  for (i=0; i<len; i++) printf(" %02x",(int)tdat[i].precedence); printf("\n");
  for (i=0; i<len; i++) printf(" %02x",(int)tdat[i].depth);      printf("\n");
  return;
 }

// ppl_expCompile -- compile a textual expression into reverse Polish bytecode

// GET_POINTER(): In the expression x=3, the variable x need not already be defined, so look it up in a special way

#define GET_POINTER \
 { \
  if (lastoutpos<0) { *errPos=tpos; *errType=ERR_INTERNAL; strcpy(errText, "Could not find variable name preceding assignment operator."); *end=-1; return; } \
  if      (out[lastoutpos].opcode==3) out[lastoutpos].opcode=4; \
  else if (out[lastoutpos].opcode==5) out[lastoutpos].opcode=6; \
  else if (out[lastoutpos].opcode==7) \
   { \
    int f = out[lastoutpos].auxil.i; \
    if (!(f&1)) { *errPos=tpos; *errType=ERR_SYNTAX; strcpy(errText, "Cannot apply an assignment operator to the output of a slice operator."); *end=-1; return; } \
    out[lastoutpos].opcode=16; \
   } \
 }

#define POP_STACK \
 { \
  char opType; \
  while (opType = (stackpos>0) ? (stack[stackpos-1].opType) : '!'  ,  (opType=='o')||(opType=='a')||(opType=='$')) /* while the stack is not empty and has an operator at the top */ \
   { \
    int stackprec = stack[stackpos-1].precedence; \
    if ((stackprec==2)&&(precedence==3)) stackprec=4; /* Magic treatment of x**-1 */ \
    if ( ((!rightAssoc)&&(precedence>=stackprec)) || ((rightAssoc)&&(precedence>stackprec)) ) /* pop operators with higher precedence from stack */ \
     { \
      const unsigned char stacko   = stack[stackpos-1].opcode; /* operator number of the stack object */ \
      const int           charpos  = stack[stackpos-1].charpos; \
      int                 bytecode = 0; \
      if      (opType=='a' ) bytecode = 12; /* assignment operator */ \
      else if (opType=='$' ) bytecode = 15; /* dollar operator */ \
      else if (stacko==0x40) bytecode = 14; /* string subst operator */ \
      else if (stacko==0x23) bytecode = 13; /* ++ lval operator */ \
      else if (stacko==0x24) bytecode = 13; /* -- lval operator */ \
      else                   bytecode = 11; /* other operator */ \
      if      (stacko==0x5A) { int o=stack[stackpos-1].outpos; BYTECODE_OP(20); BYTECODE_ENDOP; out[o].auxil.i=outpos; } /* || operator */ \
      else if (stacko==0x5B) { int o=stack[stackpos-1].outpos; BYTECODE_OP(20); BYTECODE_ENDOP; out[o].auxil.i=outpos; } /* && operator */ \
      else if (stacko==0xFE) { int o=stack[stackpos-1].outpos;                                  out[o].auxil.i=outpos; } /* ?: operator, closing : */ \
      else if (stacko!=0x5F) /* null operations get ignored (e.g. commas that collect items) */ \
       { \
        const int tpos = charpos; \
        BYTECODE_OP(bytecode); \
        out[outpos].auxil.i = stacko; \
        if ((opType=='o')&&(stacko==0x40)) out[outpos].auxil.i = 1; /* string substitution operator with only one subst item (not a bracketed list) */ \
        BYTECODE_ENDOP; \
       } \
      stackpos--; /* pop stack */ \
     } \
    else break; \
   } \
 }

#define BYTECODE_OP_POS(X,P) \
 { \
  lastoutpos = outpos; \
  out[outpos].len = 0; /* Set length of instruction equals zero for the moment */ \
  out[outpos].charpos = P; /* Store character position of token for error reporting */ \
  out[outpos].opcode = X; \
 }

#define BYTECODE_OP(X) \
 { \
  BYTECODE_OP_POS(X,tpos); \
 }

#define BYTECODE_ENDOP \
 { \
  outpos++; \
  out[lastoutpos].len = outpos - lastoutpos; /* Write the length of the bytecode instruction we've just written */ \
 }

void ppl_expCompile(ppl_context *context, int srcLineN, long srcId, char *srcFname, char *in, int *end, int dollarAllowed, int equalsAllowed, int allowCommaOperator, pplExpr **outExpr, int *errPos, int *errType, char *errText)
 {
  pplExprPStack   *stack;
  pplTokenCode    *tdata;
  int              stackpos = 0;
  int              stacklen;
  int              tpos;
  int              tlen;
  int              outpos = 0, lastoutpos = -1;
  pplExprBytecode *out;
  int              outlen = 512;

  // malloc output structure
  *outExpr = (pplExpr *)calloc(1,sizeof(pplExpr));
  if (*outExpr==NULL) { *errPos=0; *errType=ERR_MEMORY; strcpy(errText, "Out of memory."); *end=-1; return; }
  (*outExpr)->refCount = 1;
  (*outExpr)->srcId    = srcId;
  (*outExpr)->srcLineN = srcLineN;
  (*outExpr)->srcFname = (char *)malloc(strlen(srcFname)+1);
  out = (*outExpr)->bytecode = malloc(outlen * sizeof(pplExprBytecode));
  if (((*outExpr)->bytecode==NULL)||((*outExpr)->srcFname==NULL)) { *errPos=0; *errType=ERR_MEMORY; strcpy(errText, "Out of memory."); *end=-1; return; }
  strcpy((*outExpr)->srcFname , srcFname);

  // First tokenise expression
  ppl_expTokenise(context, in, end, dollarAllowed, equalsAllowed, allowCommaOperator, 0, 0, 0, &tlen, errPos, errType, errText);
  if (*errPos >= 0) return;
  tdata = context->tokenBuff;
  //ppl_tokenPrint(context, in, tlen);

  // Make stack
  if (context->parserStackLen > 1024)
   {
    free(context->parserStack);
    context->parserStack    = NULL;
    context->parserStackLen = 0;
   }

  if (context->parserStack == NULL)
   {
    context->parserStackLen = 1024;
    context->parserStack    = (pplExprPStack *)malloc(context->parserStackLen * sizeof(pplExprPStack));
   }

  if (context->parserStack == NULL)
   {
    *errPos = 0; *errType=ERR_MEMORY; strcpy(errText, "Out of memory"); *end = -1; return;
   }

  stack    = context->parserStack;
  stacklen = context->parserStackLen;

  // Copy ASCII expression into expression object
  (*outExpr)->ascii = (char *)malloc(*end+1);
  if ((*outExpr)->ascii==NULL) { *errPos=0; *errType=ERR_MEMORY; strcpy(errText, "Out of memory."); *end=-1; return; }
  strncpy((*outExpr)->ascii, in, *end);
  (*outExpr)->ascii[*end]='\0';

  // The stacking-yard algorithm
  for ( tpos=0; tpos<tlen; )
   {
    char o = tdata[tpos].state; // Get tokenised markup state code

    // Expand space for bytecode as necessary
    if (outlen - outpos < 256)
     {
      outlen+=2048;
      out = (*outExpr)->bytecode = realloc(out, outlen*sizeof(pplExprBytecode));
      if (out==NULL) { *errPos=0; *errType=ERR_MEMORY; strcpy(errText, "Out of memory."); *end=-1; return; }
     }

    if (o=='B') // Process a string literal
     {
      int  i;
      char quoteType=in[tpos];
      char *oc = (char *)(out+outpos+1);
      int   ol = 0;
      BYTECODE_OP(2); // bytecode op 2
      if ((quoteType=='r') && ((in[tpos+1]=='\'') || (in[tpos+1]=='\"')) ) // r'hello' raw string
       {
        int offset=2, tripleQuote=0;
        quoteType=in[tpos+1];
        if ((in[tpos+2]==quoteType)&&(in[tpos+3]==quoteType)) { offset=4; tripleQuote=1; }
        for ( i=tpos+offset ;
              ( ((!tripleQuote) &&  (in[i]!=quoteType)) ||
                (  tripleQuote  && ((in[i]!=quoteType)||(in[i+1]!=quoteType)||(in[i+2]!=quoteType)) ) );
              i++
            )
         {
          if (in[i]=='\0') { *errPos=i; *errType=ERR_INTERNAL; strcpy(errText, "Unexpected end of string."); *end=-1; return; }
          oc[ol++] = in[i];
         }
       }
      else if ((quoteType=='e') && ((in[tpos+1]=='\'') || (in[tpos+1]=='\"')) ) // e'hello' string with extended escaping
       {
        int offset=2, tripleQuote=0;
        quoteType=in[tpos+1];
        if ((in[tpos+2]==quoteType)&&(in[tpos+3]==quoteType)) { offset=4; tripleQuote=1; }
        for ( i=tpos+offset ;
              ( ((!tripleQuote) &&  (in[i]!=quoteType)) ||
                (  tripleQuote  && ((in[i]!=quoteType)||(in[i+1]!=quoteType)||(in[i+2]!=quoteType)) ) );
              i++
            )
         {
          if (in[i]=='\0') { *errPos=i; *errType=ERR_INTERNAL; strcpy(errText, "Unexpected end of string."); *end=-1; return; }
          if (in[i]=='\\')
           switch (in[i+1])
            {
             case '?' : oc[ol++]='\?'; i++; break;
             case '\'': oc[ol++]='\''; i++; break;
             case '\"': oc[ol++]='\"'; i++; break;
             case '\\': oc[ol++]='\\'; i++; break;
             case 'a' : oc[ol++]='\a'; i++; break;
             case 'b' : oc[ol++]='\b'; i++; break;
             case 'f' : oc[ol++]='\f'; i++; break;
             case 'n' : oc[ol++]='\n'; i++; break;
             case 'r' : oc[ol++]='\r'; i++; break;
             case 't' : oc[ol++]='\t'; i++; break;
             case 'v' : oc[ol++]='\v'; i++; break;
             default  : oc[ol++] = in[i]; break;
            }
          else { oc[ol++] = in[i]; }
         }
       }
      else if ((quoteType!='\'')&&(quoteType!='"')) // Unquoted string
       {
        while ((tpos<tlen) && (tdata[tpos].state == o))
         {
          oc[ol++] = in[tpos++];
          if (tpos>=tlen) break;
         }
        if ((ol>0)&&(oc[ol-1]==',')) ol--;
       }
      else // a string of the form 'hello' with only quote characters escaped
       {
        int offset=1, tripleQuote=0;
        if ((in[tpos+1]==quoteType)&&(in[tpos+2]==quoteType)) { offset=3; tripleQuote=1; }

        for ( i=tpos+offset ;
              ( ((!tripleQuote) &&  (in[i]!=quoteType)) ||
                (  tripleQuote  && ((in[i]!=quoteType)||(in[i+1]!=quoteType)||(in[i+2]!=quoteType)) ) );
              i++
            )
         {
          if (in[i]=='\0') { *errPos=i; *errType=ERR_INTERNAL; strcpy(errText, "Unexpected end of string."); *end=-1; return; }
          if (in[i]=='\\')
           switch (in[i+1])
            {
             case '\'': oc[ol++]='\''; i++; break;
             case '\"': oc[ol++]='\"'; i++; break;
             case '\\': oc[ol++]='\\'; i++; break;
             default  : oc[ol++] = in[i]; break;
            }
          else { oc[ol++] = in[i]; }
         }
       }
      oc[ol++] = '\0';
      outpos += 1 + (ol/sizeof(pplExprBytecode));
      BYTECODE_ENDOP;
     }
    else if ( (o=='C') || (o=='H') || (o=='I') || (o=='J') || (o=='K') || (o=='S') ) // Process an operator
     {
      unsigned char opcode     = tdata[tpos].opcode;
      int           precedence = tdata[tpos].precedence;
      int           rightAssoc = ((opcode & 0x80) != 0) || (o=='S'); // All assignment operators (S) are right-left associative
      POP_STACK;

      // : operator part of ? : ... pop the ? from the stack now
      if ((o=='K')&&(opcode==0xFE))
       {
        int o,precedence=999;
        POP_STACK;
        if ( (stackpos<1) || (stack[stackpos-1].opType!='t') ) { *errPos=tpos; *errType=ERR_INTERNAL; strcpy(errText, "Could not match : to a ? in the ternary operator."); *end=-1; return; }
        o = stack[stackpos-1].outpos;
        out[o].auxil.i = outpos+1;
        stackpos--; // pop ? from stack
       }

      if      ((o=='S')&&(opcode==0x40)) { GET_POINTER; } // Turn variable lookup into pointer lookup now, because assignment operators are binary and operand is guaranteed to be the last thing on the stack
      else if ((o=='J')&&(opcode==0x5A)) { BYTECODE_OP(17); out[outpos].flag = 0; stack[stackpos].outpos=outpos; BYTECODE_ENDOP; } // && operator compiles to if false goto else pop
      else if ((o=='J')&&(opcode==0x5B)) { BYTECODE_OP(18); out[outpos].flag = 0; stack[stackpos].outpos=outpos; BYTECODE_ENDOP; } // || operator compiles to if true  goto else pop
      else if ((o=='K')&&(opcode==0xFD)) { BYTECODE_OP(17); out[outpos].flag = 1; stack[stackpos].outpos=outpos; BYTECODE_ENDOP; } // ? operator -- if false goto
      else if ((o=='K')&&(opcode==0xFE)) { BYTECODE_OP(19);                       stack[stackpos].outpos=outpos; BYTECODE_ENDOP; } // : operator -- goto

      if (stackpos>stacklen-4) { *errPos = tpos; *errType=ERR_OVERFLOW; strcpy(errText, "Stack overflow whilst parsing algebraic expression."); *end=-1; return; }
      stack[stackpos].charpos = tpos;
      if      (o=='S')       stack[stackpos].opType = 'a'; // push operator onto stack
      else if (opcode==0xFD) stack[stackpos].opType = 't';
      else                   stack[stackpos].opType = 'o';
      stack[stackpos].opcode = opcode;
      stack[stackpos].precedence = precedence;
      stackpos++;
     }
    else if ( (o=='D') || (o=='E') || (o=='P') ) // Process ( )
     {
      char bracketType=in[tpos];
      if (bracketType=='(') // open (
       {
        if (stackpos>stacklen-4) { *errPos = tpos; *errType=ERR_OVERFLOW; strcpy(errText, "Stack overflow whilst parsing algebraic expression."); *end=-1; return; }
        stack[stackpos].charpos    = tpos;
        stack[stackpos].opType     = '('; // push bracket onto stack
        stack[stackpos].opcode     = 0;
        stack[stackpos].precedence = 0;
        stackpos++;
       }
      else // close )
       {
        int rightAssoc = 0;
        int precedence = 255;
        int startpos;
        POP_STACK; // Pop the stack of all operators (fake operator above with very low precedence)
        if ( (stackpos<1) || (stack[stackpos-1].opType!='(') ) { *errPos=tpos; *errType=ERR_INTERNAL; strcpy(errText, "Could not match ) to an (."); *end=-1; return; }
        startpos = stack[stackpos-1].charpos;
        stackpos--; // pop bracket
        if (o=='D') // apply string substitution operator straight away
         {
          if ( (stackpos<1) || (stack[stackpos-1].opcode!=0x40)) { *errPos=tpos; *errType=ERR_INTERNAL; strcpy(errText, "Could not match string substituion () to a %."); *end=-1; return; }
          stackpos--; // pop stack
          BYTECODE_OP_POS(14,startpos); // bytecode 14 -- string substitution operator
          out[outpos].auxil.i = (int)tdata[tpos].depth; // store the number of string substitutions; stored with closing bracket in bytecode is number of collected ,-separated items
          BYTECODE_ENDOP;
         }
        else if (o=='P') // make function call
         {
          BYTECODE_OP_POS(10,startpos); // bytecode 10 -- function call
          out[outpos].auxil.i = (int)tdata[tpos].depth; // store the number of function arguments; stored with closing bracket in bytecode is number of collected ,-separated items
          BYTECODE_ENDOP;
         }
       }
     }
    else if (o=='F') // a postfix ++ or -- operator
     {
      int rightAssoc = (tdata[tpos].opcode & 0x80) != 0;
      int precedence =  tdata[tpos].precedence;
      POP_STACK;
      BYTECODE_OP(13);
      out[outpos].auxil.i = tdata[tpos].opcode;
      BYTECODE_ENDOP;
     }
    else if ( (o=='G') || (o=='T') || (o=='V') ) // a variable name or field dereferenced with the . operator
     {
      char *strout = (char *)(out+outpos+1);
      int   slen   = 0;
      if      (o=='G') BYTECODE_OP(3) // foo -- op 3: variable lookup "foo"
      else if (o=='T') BYTECODE_OP(5) // foo.bar -- op 5: dereference "bar"
      else             BYTECODE_OP(2) // $foo -- push "foo" onto stack as a string constant
      while ((tpos<tlen) && (tdata[tpos].state == o))
       {
        if ((!isalnum(in[tpos])) && (in[tpos]!='_')) break;
        strout[slen++] = in[tpos++];
        if (tpos>=tlen) break;
       }
      strout[slen++] = '\0';
      outpos += 1 + (slen/sizeof(pplExprBytecode));
      BYTECODE_ENDOP;
     }
    else if (o=='L') // a numeric literal
     {
      BYTECODE_OP(1);
      out[outpos].auxil.d = ppl_getFloat(in+tpos,NULL);
      BYTECODE_ENDOP;
     }
    else if (o=='M') // list literal
     {
      char bracketType=in[tpos];
      if (bracketType=='[') // open [
       {
        if (stackpos>stacklen-4) { *errPos = tpos; *errType=ERR_OVERFLOW; strcpy(errText, "Stack overflow whilst parsing algebraic expression."); *end=-1; return; }
        stack[stackpos].charpos    = tpos;
        stack[stackpos].opType     = '['; // push bracket onto stack
        stack[stackpos].opcode     = 0;
        stack[stackpos].precedence = 0;
        stackpos++;
       }
      else // close ]
       {
        unsigned char rightAssoc = 0;
        unsigned char precedence = 255;
        int startpos;
        POP_STACK; // Pop the stack of all operators (fake operator above with very low precedence)
        if ( (stackpos<1) || (stack[stackpos-1].opType!='[') ) { *errPos=tpos; *errType=ERR_INTERNAL; strcpy(errText, "Could not match ] to an [."); *end=-1; return; }
        startpos = stack[stackpos-1].charpos;
        stackpos--; // pop bracket
        BYTECODE_OP_POS(9,startpos); // bytecode 9 -- make list
        out[outpos].auxil.i = tdata[tpos].depth; // store the number of list items; stored with closing bracket in bytecode is number of collected ,-separated items
        BYTECODE_ENDOP;
       }
     }
    else if (o=='N') // dictionary literal
     {
      char bracketType=in[tpos];
      if (bracketType=='{') // open {
       {
        if (stackpos>stacklen-64) { *errPos = tpos; *errType=ERR_OVERFLOW; strcpy(errText, "Stack overflow whilst parsing algebraic expression."); *end=-1; return; }
        stack[stackpos].charpos    = tpos;
        stack[stackpos].opType     = '{'; // push bracket onto stack
        stack[stackpos].opcode     = 0;
        stack[stackpos].precedence = 0;
        stackpos++;
       }
      else // close }
       {
        int rightAssoc = 0;
        int precedence = 255;
        int startpos;
        POP_STACK; // Pop the stack of all operators (fake operator above with very low precedence)
        if ( (stackpos<1) || (stack[stackpos-1].opType!='{') ) { *errPos=tpos; *errType=ERR_INTERNAL; strcpy(errText, "Could not match } to an {."); *end=-1; return; }
        startpos = stack[stackpos-1].charpos;
        stackpos--; // pop bracket
        BYTECODE_OP_POS(8,startpos); // bytecode 8 -- make dict
        out[outpos].auxil.i = tdata[tpos].depth; // store the number of list items; stored with closing bracket in bytecode is number of collected ,-separated items
        BYTECODE_ENDOP;
       }
     }
    else if (o=='O') // a dollar operator
     {
      if (stackpos>stacklen-4) { *errPos = tpos; *errType=ERR_OVERFLOW; strcpy(errText, "Stack overflow whilst parsing algebraic expression."); *end=-1; return; }
      stack[stackpos].charpos    = tpos;
      stack[stackpos].opType     = '$'; // push bracket onto stack
      stack[stackpos].opcode     = 0;
      stack[stackpos].precedence = 0;
      stackpos++;
     }
    else if (o=='R') // a dot dereference
     {
      // Nop -- the work is done in the following T-state parameter.
     }
    else if (o=='Q') // array dereference or slice
     {
      char bracketType=in[tpos];
      if (bracketType=='[') // open dereference brackets
       {
        if (stackpos>stacklen-4) { *errPos = tpos; *errType=ERR_OVERFLOW; strcpy(errText, "Stack overflow whilst parsing algebraic expression."); *end=-1; return; }
        stack[stackpos].charpos    = tpos;
        stack[stackpos].opType     = '<'; // push slice onto stack
        stack[stackpos].opcode     = 0;
        stack[stackpos].precedence = 0;
        stackpos++;
       }
      else // close ]
       {
        int rightAssoc = 0;
        int precedence = 255;
        int startpos;
        POP_STACK; // Pop the stack of all operators (fake operator above with very low precedence)
        if ( (stackpos<1) || (stack[stackpos-1].opType!='<') ) { *errPos=tpos; *errType=ERR_INTERNAL; strcpy(errText, "Could not match ] to an [."); *end=-1; return; }
        startpos = stack[stackpos-1].charpos;
        stackpos--; // pop bracket
        BYTECODE_OP_POS(7,startpos); // bytecode 7 -- slice object
        out[outpos].auxil.i = tdata[tpos].opcode; // flags to distinguish [x], [x:], [:x], [x:y] and [:]
        BYTECODE_ENDOP;
       }
     }
    while ((tpos<tlen) && (tdata[tpos].state == o))
     {
      tpos++;
      if (tpos>=tlen) break;
      if ((o=='D') && ((in[tpos]=='(')||(in[tpos]==')'))) break; // Empty list ( ) of function arguments
      if ((o=='E') && ((in[tpos]=='(')||(in[tpos]==')'))) break; // Empty list ( ) of function arguments
      if ((o=='P') && ((in[tpos]=='(')||(in[tpos]==')'))) break; // Empty list ( ) of function arguments
      if ((o=='M') && ((in[tpos]=='[')||(in[tpos]==']'))) break; // Empty list [ ] is two tokens
      if ((o=='Q') && ((in[tpos]=='[')||(in[tpos]==']'))) break; // Empty list [ ] is two tokens
      if ((o=='N') && ((in[tpos]=='{')||(in[tpos]=='}'))) break; // Empty dictionary { } is two tokens
     }
   }

  // Clean up stack
  {
   int rightAssoc = 0;
   int precedence = 255;
   POP_STACK;
  }

  // If there are still brackets on the stack, we've failed
  if (stackpos!=0)
   {
    *errPos=0; *errType=ERR_INTERNAL; strcpy(errText, "Unexpected junk left on stack."); *end=-1; return;
   }

  // Store final return
  BYTECODE_OP(0);
  BYTECODE_ENDOP;
  (*outExpr)->bcLen = outpos * sizeof(pplExprBytecode);

  // Optimise gotos that point directly at other gotos
   {
    int optimised;
    do
     {
      int i;
      for (optimised=i=0 ; ; i+=*(int *)(out+i))
       {
        int o = out[i].opcode; // Opcode number
        if ((o==17)||(o==18))
         {
          int o2    = out[i ].flag;
          int to    = out[i ].auxil.i;
          int to_o  = out[to].opcode; // Opcode branching to
          int to_o2 = out[to].flag;
          if ((to_o!=17)&&(to_o!=18)) to_o2=-1; // this shouldn't be necessary, but optimisation of the below code makes valgrind unhappy
          if ((o==17)&&(o2==0)&&(to_o==17)&&(to_o2==0)) { out[i].auxil.i = out[to].auxil.i; optimised=1; }
          if ((o==18)&&(o2==0)&&(to_o==18)&&(to_o2==0)) { out[i].auxil.i = out[to].auxil.i; optimised=1; }
          if ((o==17)         &&(to_o==19)            ) { out[i].auxil.i = out[to].auxil.i; optimised=1; }
          if ((o==18)         &&(to_o==19)            ) { out[i].auxil.i = out[to].auxil.i; optimised=1; }
         }
        else if (o==0) break;
       }
     }
    while (optimised);
   }

  //ppl_reversePolishPrint(context, *outExpr);
  return;
 }

// Debugging routine to produce a textual representation of reverse Polish bytecode

void ppl_reversePolishPrint(ppl_context *context, pplExpr *expIn)
 {
  int   j=0;
  char  op[32],optype[32],arg[1024];
  pplExprBytecode *in = expIn->bytecode;

  while (1)
   {
    int pos     = j; // Position of start of instruction
    int len     = in[j].len    ; // length of bytecode instruction with data
    int charpos = in[j].charpos; // character position of token (for error reporting)
    int o       = in[j].opcode ; // Opcode number
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
        sprintf(arg,    "%.2e", in[j].auxil.d);
        break;
      case 2:
        strcpy (op,     "push");
        strcpy (optype, "string");
        sprintf(arg,    "\"%s\"", (char *)&(in[j+1]));
        break;
      case 3:
        strcpy (op,     "lookup");
        strcpy (optype, "value");
        sprintf(arg,    "\"%s\"", (char *)&(in[j+1]));
        break;
      case 4:
        strcpy (op,     "lookup");
        strcpy (optype, "pointer");
        sprintf(arg,    "\"%s\"", (char *)&(in[j+1]));
        break;
      case 5:
        strcpy (op,     "deref");
        strcpy (optype, "value");
        sprintf(arg,    "\"%s\"", (char *)&(in[j+1]));
        break;
      case 6:
        strcpy (op,     "deref");
        strcpy (optype, "pointer");
        sprintf(arg,    "\"%s\"", (char *)&(in[j+1]));
        break;
      case 7:
       {
        int f = in[j].auxil.i;
        strcpy (op,     "slice");
        strcpy (optype, "value");
        if       (f & 1)     strcpy(arg,    "[--]"   );
        else if ((f & 6)==6) strcpy(arg,    "[--:--]");
        else if ((f & 6)==4) strcpy(arg,    "[--:]"  );
        else if ((f & 6)==2) strcpy(arg,    "[:--]"  );
        else                 strcpy(arg,    "[:]"    );
        break;
       }
      case 16:
        strcpy (op,     "slice");
        strcpy (optype, "pointer");
        strcpy (arg,    "[--]"   );
        break;
      case 8:
        strcpy (op,     "make");
        strcpy (optype, "dict");
        sprintf(arg,    "%d items", in[j].auxil.i);
        break;
      case 9:
        strcpy (op,     "make");
        strcpy (optype, "list");
        sprintf(arg,    "%d items", in[j].auxil.i);
        break;
      case 10:
        strcpy (op,     "call");
        strcpy (optype, "");
        sprintf(arg,    "%d args", in[j].auxil.i);
        break;
      case 11:
       {
        int t = in[j].auxil.i;
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
        else if (t==0x5A) strcpy (arg, "&& (error: should have been replaced by an if)" );
        else if (t==0x5B) strcpy (arg, "|| (error: should have been replaced by an if)" );
        else if (t==0x5C) strcpy (arg, "swap-pop");
        else if (t==0xFD) strcpy (arg, "? -- error: should have been compiled into a conditional" );
        else if (t==0xFE) strcpy (arg, "?: -- error: should have been compiled into a conditional" );
        else if (t==0x5F) strcpy (arg, "nop (collect -- error: should have been optimised out)" );
        else              strcpy (arg, "??? (error: unknown opcode)");
        break;
       }
      case 12:
       {
        int t = in[j].auxil.i;
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
        else if (t==0x4B) strcpy (arg, "**=");
        else              strcpy (arg, "???");
        break;
       }
      case 13:
       {
        int t = in[j].auxil.i;
        strcpy (op,     "op");
        strcpy (optype, "unary");
        if      (t==0x21) { strcpy (arg, "-- (post-eval)"); }
        else if (t==0x22) { strcpy (arg, "++ (post-eval)"); }
        else if (t==0x23) { strcpy (arg, "-- (pre-eval)"); }
        else              { strcpy (arg, "++ (pre-eval)"); }
        break;
       }
      case 14:
        strcpy (op,     "op");
        strcpy (optype, "binary");
        sprintf(arg,    "string subst (%d items)", in[j].auxil.i);
        break;
      case 15:
        strcpy (op,     "op");
        strcpy (optype, "unary");
        strcpy (arg,    "dollar -- column lookup");
        break;
      case 17:
        strcpy (op,     "branch if");
        strcpy (optype, "false");
        sprintf(arg,    "to %d (%s)", in[j].auxil.i, in[j].flag ? "pop conditional" : "pop conditional; push FALSE on branch");
        break;
      case 18:
        strcpy (op,     "branch if");
        strcpy (optype, "true");
        sprintf(arg,    "to %d (%s)", in[j].auxil.i, in[j].flag ? "pop conditional" : "pop conditional; push TRUE on branch");
        break;
      case 19:
        strcpy (op,     "branch");
        strcpy (optype, "goto");
        sprintf(arg,    "%d", in[j].auxil.i);
        break;
      case 20:
        strcpy (op,     "make");
        strcpy (optype, "boolean");
        strcpy (arg,    "");
        break;
      default:
        strcpy (op,     "???");
        strcpy (optype, "");
        strcpy (arg,    "Illegal opcode");
        o=0;
        break;
     }
    sprintf(context->errcontext.tempErrStr,"%4d %4d %10s %10s %s",pos,charpos,op,optype,arg);
    ppl_report(&context->errcontext, NULL);
    if (o==0) break;
    j = pos+len;
   }
  return;
 }

void pplExpr_free(pplExpr *inExpr)
 {
  if (inExpr==NULL) return;
  if ( __sync_sub_and_fetch(&inExpr->refCount,1) > 0) return;
  if (inExpr->ascii!=NULL) free(inExpr->ascii);
  if (inExpr->srcFname!=NULL) free(inExpr->srcFname);
  if (inExpr->bytecode!=NULL) free(inExpr->bytecode);
  free(inExpr);
  return;
 }

pplExpr *pplExpr_cpy(pplExpr *i)
 {
  pplExpr *o;
  if (i==NULL) return NULL;
  o = (pplExpr *)malloc(sizeof(pplExpr));
  if (o==NULL) return NULL;
  memcpy(o, i, sizeof(pplExpr));
  o->refCount = 1;
  if (i->ascii   !=NULL) { if ((o->ascii   =malloc(strlen(i->ascii   )+1))==NULL) return NULL; strcpy(o->ascii   , i->ascii   ); }
  if (i->srcFname!=NULL) { if ((o->srcFname=malloc(strlen(i->srcFname)+1))==NULL) return NULL; strcpy(o->srcFname, i->srcFname); }
  if (i->bytecode!=NULL) { if ((o->bytecode=malloc(i->bcLen             ))==NULL) return NULL; memcpy(o->bytecode, i->bytecode, i->bcLen); }
  return o;
 }

pplExpr *pplExpr_tmpcpy(pplExpr *i)
 {
  pplExpr *o;
  if (i==NULL) return NULL;
  o = (pplExpr *)ppl_memAlloc(sizeof(pplExpr));
  if (o==NULL) return NULL;
  memcpy(o, i, sizeof(pplExpr));
  o->refCount = 1;
  if (i->ascii   !=NULL) { if ((o->ascii   =ppl_memAlloc(strlen(i->ascii   )+1))==NULL) return NULL; strcpy(o->ascii   , i->ascii   ); }
  if (i->srcFname!=NULL) { if ((o->srcFname=ppl_memAlloc(strlen(i->srcFname)+1))==NULL) return NULL; strcpy(o->srcFname, i->srcFname); }
  if (i->bytecode!=NULL) { if ((o->bytecode=ppl_memAlloc(i->bcLen             ))==NULL) return NULL; memcpy(o->bytecode, i->bytecode, i->bcLen); }
  return o;
 }

