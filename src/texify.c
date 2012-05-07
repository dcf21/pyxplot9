// texify.c
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

#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <math.h>
#include <string.h>

#include "coreUtils/memAlloc.h"
#include "expressions/expCompile_fns.h"
#include "settings/settingTypes.h"
#include "stringTools/strConstants.h"
#include "stringTools/asciidouble.h"
#include "userspace/context.h"
#include "userspace/pplObj_fns.h"
#include "userspace/unitsArithmetic.h"
#include "userspace/unitsDisp.h"
#include "pplConstants.h"
#include "texify.h"

#define FFW_TOKEN \
   while ((j<tlen)&&(c->tokenBuff[j]+'@'==o)) { i++; j+=3; }

#define FFW_N(n) \
   { i+=n; j+=3*n; }

#define ENTER_PLAINTEXT \
   if ( *textRm  ) { *textRm  =0; snprintf(out+k, outlen-k, "}"); k+=strlen(out+k); } \
   if ( *mathMode) { *mathMode=0; snprintf(out+k, outlen-k, "$"); k+=strlen(out+k); }

#define ENTER_MATHMODE \
   if (!*mathMode) { *mathMode=1; snprintf(out+k, outlen-k, "$\\displaystyle "); k+=strlen(out+k); } \
   if ( *textRm  ) { *textRm  =0; snprintf(out+k, outlen-k, "}"); k+=strlen(out+k); }

#define ENTER_TEXTRM \
   if (!*mathMode) { *mathMode=1; snprintf(out+k, outlen-k, "$\\displaystyle "); k+=strlen(out+k); } \
   if (!*textRm  ) { *textRm  =1; snprintf(out+k, outlen-k, "\\textrm{"); k+=strlen(out+k); }


static char *LatexVarNamesFr[] = {"alpha"  ,"beta"   ,"gamma"  ,"delta"  ,"epsilon","zeta"   ,"eta"    ,"theta"  ,"iota"   ,"kappa"  ,"lambda" ,"mu"     ,"nu"     ,"xi"     ,"pi"     ,"rho"    ,"sigma"  ,"tau"    ,"upsilon","phi"    ,"chi"    ,"psi"    ,"omega"  ,"Gamma"  ,"Delta"  ,"Theta"  ,"Lambda" ,"Xi"     ,"Pi"     ,"Sigma"  ,"Upsilon","Phi"    ,"Psi"    ,"Omega"  ,"aleph"  , NULL};

static char *LatexVarNamesTo[] = {"\\alpha","\\beta","\\gamma","\\delta","\\epsilon","\\zeta","\\eta","\\theta","\\iota","\\kappa","\\lambda","\\mu","\\nu","\\xi","\\pi","\\rho","\\sigma","\\tau","\\upsilon","\\phi","\\chi","\\psi","\\omega","\\Gamma","\\Delta","\\Theta","\\Lambda","\\Xi","\\Pi","\\Sigma","\\Upsilon","\\Phi","\\Psi","\\Omega","\\aleph",NULL};

static void ppl_texify_MakeGreek(const char *in, char *out, int outlen, int *mathMode, int *textRm)
 {
  int i,ji=0,jg,a,l,m,k=0,kno_=0;

  ENTER_MATHMODE;
  while ((isalnum(in[ji]))||(in[ji]=='_'))
   {
    a=0;
    for (i=0; LatexVarNamesFr[i]!=NULL; i++)
     {
      for (jg=0; LatexVarNamesFr[i][jg]!='\0'; jg++) if (LatexVarNamesFr[i][jg]!=in[ji+jg]) break;
      if (LatexVarNamesFr[i][jg]!='\0') continue;
      if (!isalnum(in[ji+jg])) // We have a Greek letter
       {
        snprintf(out+k, outlen-1-k, "%s", LatexVarNamesTo[i]);
        k += strlen(out+k);
        ji += jg;
        a=1;
        break;
       }
      else if (isdigit(in[ji+jg]))
       {
        l=ji+jg;
        while (isdigit(in[ji+jg])) jg++;
        if (isalnum(in[ji+jg])) continue;
        snprintf(out+k, outlen-1-k, "%s_{", LatexVarNamesTo[i]); // We have Greek letter underscore number
        k += strlen(out+k);
        for (m=l;m<ji+jg;m++) if (k<outlen-1) out[k++]=in[m];
        if (k<outlen-1) out[k++] = '}';
        ji += jg;
        a=1;
        break;
       }
     }
    if ((a==0)&&(ji>1))
     {
      for (jg=0; isdigit(in[ji+jg]); jg++);
      if ( (jg>0) && (!isalnum(in[ji+jg])) && (in[ji+jg]!='_') )
       {
        k=kno_;
        snprintf(out+k, outlen-1-k, "_{"); // We have an underscore number at the end of variable name
        k += strlen(out+k);
        for (m=ji;m<ji+jg;m++) if (k<outlen-1) out[k++]=in[m];
        if (k<outlen-1) out[k++] = '}';
        ji += jg;
        a=1;
       }
     }
    if (a==0)
     {
      for (jg=0; (isalnum(in[ji+jg])); jg++) if (k<outlen-1) out[k++]=in[ji+jg];
      ji+=jg;
     }
    kno_=k;
    if (in[ji]=='_')
     {
      if (k<outlen-1) out[k++]='\\';
      if (k<outlen-1) out[k++]='_';
      ji++;
     }
   }
  out[k++]='\0';
  return;
 }

void ppl_texify_string(char *in, char *out, int inlen, int outlen)
 {
  int i,j,DoubleQuoteLevel=0,QuoteLevel=0;

  for (i=j=0; ((in[i]!='\0')&&((inlen<0)||(i<inlen))); i++)
   {
    if (j>outlen-16) { strcpy(out+j, "..."); j+=strlen(out+j); break; }
    if      (in[i]=='\\') { strcpy(out+j, "$\\backslash$"); j+=strlen(out+j); }
    else if (in[i]=='_' ) { out[j++]='\\'; out[j++]=in[i]; }
    else if (in[i]=='&' ) { out[j++]='\\'; out[j++]=in[i]; }
    else if (in[i]=='%' ) { out[j++]='\\'; out[j++]=in[i]; }
    else if (in[i]=='$' ) { out[j++]='\\'; out[j++]=in[i]; }
    else if (in[i]=='{' ) { out[j++]='\\'; out[j++]=in[i]; }
    else if (in[i]=='}' ) { out[j++]='\\'; out[j++]=in[i]; }
    else if (in[i]=='#' ) { out[j++]='\\'; out[j++]=in[i]; }
    else if (in[i]=='^' ) { strcpy(out+j, "\\verb|^|"); j+=strlen(out+j); }
    else if (in[i]=='~' ) { strcpy(out+j, "$\\sim$"); j+=strlen(out+j); }
    else if (in[i]=='<' ) { strcpy(out+j, "$<$"); j+=strlen(out+j); }
    else if (in[i]=='>' ) { strcpy(out+j, "$>$"); j+=strlen(out+j); }
    else if (in[i]=='|' ) { strcpy(out+j, "$|$"); j+=strlen(out+j); }
    else if ((in[i]=='\"')&&((i==0)||(in[i-1]!='\\'))) { out[j++]=DoubleQuoteLevel?'\'':'`'; out[j++]=DoubleQuoteLevel?'\'':'`'; DoubleQuoteLevel=!DoubleQuoteLevel; }
    else if ((in[i]=='\'')&&((i==0)||(in[i-1]!='\\'))) { out[j++]=QuoteLevel?'\'':'`'; QuoteLevel=!QuoteLevel; }
    else                  { out[j++]=in[i]; }
   }
  out[j++]='\0';
  return;
 }

void ppl_texify_generic(ppl_context *c, char *in, int *end, char *out, int outlen)
 {
  const int  allowCommaOperator = 1;
  const int  equalsAllowed      = 1;
  const int  dollarAllowed      = 1;
  int        errPos=-1, errType, tlen;
  char       errbuff[LSTR_LENGTH];
  int       *stkpos, i, j, k;

  int        mm=0, tr=0;
  int       *mathMode = &mm;
  int       *textRm   = &tr;

#define MAX_BRACKETS 4096
  int        bracketLevel=0, highestBracketLevel=0, bracketOpenPos[MAX_BRACKETS];

  // First tokenise expression
  ppl_expTokenise(c, in, end, dollarAllowed, equalsAllowed, allowCommaOperator, 0, 0, 0, &tlen, &errPos, &errType, errbuff);
  if (errPos>=0) { *end=0; return; }
  if (tlen  <=0) { *end=0; return; }

  // Malloc buffer for storing stack positions of numerical constants
  stkpos = (int *)malloc(tlen*sizeof(int));
  if (stkpos==NULL) { *end=0; return; }
  for (i=0; i<tlen; i++) stkpos[i]=-1;

  // Evaluate numerical constants
  k = c->stackPtr;
  for (i=j=0; j<tlen; i++, j+=3)
   if (c->tokenBuff[j] == (unsigned char)('L'-'@'))
    {
     if (k > ALGEBRA_STACK-4) { free(stkpos); strcpy(out, "stack overflow"); return; }
     pplObjNum(&c->stack[k], 0, ppl_getFloat(in+i,NULL), 0);
     while ((c->tokenBuff[j] == (unsigned char)('L'-'@')) && (j<tlen)) { stkpos[i++]=k; j+=3; } // ffw over constant
     k++;
    }

  // Evaluate the unit() function (but not after a ., and only if followed by () )
  for (i=j=0; j<tlen; i++, j+=3)
   if ((c->tokenBuff[j] == (unsigned char)('G'-'@')) && ((j<1)||((c->tokenBuff[j-3] != (unsigned char)('R'-'@'))&&(c->tokenBuff[j-3] != (unsigned char)('G'-'@')))))
    {
     int    ei=i, ej=j, upos, p, q=0, errpos=-1;
     pplObj val;
     if (k > ALGEBRA_STACK-4) { free(stkpos); strcpy(out, "stack overflow"); return; }
     while ((c->tokenBuff[ej] == (unsigned char)('G'-'@')) && (ej<tlen)) { ej+=3; ei++; } // ffw over function name
     if (c->tokenBuff[ej] != (unsigned char)('P'-'@')) continue; // we don't have function arguments
     if ((ei-i<4)||(strncmp(in+i,"unit",4)!=0)) continue; // function isn't called unit
     for (p=i+4; p<ei; p++) if ((isalnum(in[p]))||(in[p]=='_')) break;
     if (p<ei) continue; // function has trailing matter
     upos = ei+1; // position of unit name
     val.refCount   = 1;
     val.amMalloced = 0;
     ppl_unitsStringEvaluate(c, in+upos, &val, &q, &errpos, c->errcontext.tempErrStr);
     if (errpos>=0) continue;
     if (in[upos+q]!=')') continue;
     ei = ei+(1+q);
     ej = ej+(1+q)*3;
     if (c->tokenBuff[ej] != (unsigned char)('P'-'@')) continue; // we don't have closing bracket
     while ((c->tokenBuff[ej] == (unsigned char)('P'-'@')) && (ej<tlen)) { ej+=3; ei++; } // ffw over function arguments
     memcpy(&c->stack[k], &val, sizeof(pplObj));

     // Clean up
     for ( ; j<ej ; i++,j+=3 ) { stkpos[i]=k; }
     k++;
    }

  // Evaluate minus signs if literal is right argument
  for (i=j=0; j<tlen; i++, j+=3)
   if ((c->tokenBuff[j] == (unsigned char)('I'-'@')) && ((j<1)||(c->tokenBuff[j-3] != (unsigned char)('I'-'@'))))
    if (in[i]=='-')
     {
      int ei=i, ej=j, sp;
      while ((c->tokenBuff[ej] == (unsigned char)('I'-'@')) && (ej<tlen)) { ej+=3; ei++; } // ffw over minus sign
      if ((ej>=tlen)||(stkpos[ei]<0)) continue; // right argument is not literal or unit() function
      sp = stkpos[ei];
      c->stack[sp].real*=-1;
      if (c->stack[sp].flagComplex) c->stack[sp].imag*=-1;
      for ( ; j<ej ; i++,j+=3 ) { stkpos[i]=sp; }
     }

  // Evaluate ** operator if unit() is left argument
  for (i=j=0; j<tlen; i++, j+=3)
   if ((c->tokenBuff[j] == (unsigned char)('J'-'@')) && (j>0) && (c->tokenBuff[j-3] == (unsigned char)('P'-'@')) && (stkpos[i-1]>=0))
    if ((in[i]=='*')&&(in[i+1]=='*'))
     {
      int ei=i, ej=j, sp1, sp2, stat=0, errpos=-1;
      pplObj val;
      while ((c->tokenBuff[ej] == (unsigned char)('J'-'@')) && (ej<tlen)) { ej+=3; ei++; } // ffw over binary operator
      if ((ej>=tlen)||(stkpos[ei]<0)) continue; // right argument is not a literal
      sp1 = stkpos[i-1];
      sp2 = stkpos[ei];
      val.refCount   = 1;
      val.amMalloced = 0;
      pplObjNum(&val,0,0,0);
      ppl_uaPow(c, &c->stack[sp1], &c->stack[sp2], &val, &stat, &errpos, c->errcontext.tempErrStr);
      if ((stat)||(errpos>=0)) continue;
      memcpy(&c->stack[sp1], &val, sizeof(pplObj));
      for ( ; j<ej           ; i++,j+=3 ) { stkpos[i]=sp1; }
      for ( ; stkpos[i]==sp2 ; i++,j+=3 ) { stkpos[i]=sp1; }
     }

  // Evaluate * and / operators if unit() is either argument
  for (i=j=0; j<tlen; i++, j+=3)
   if ((c->tokenBuff[j] == (unsigned char)('J'-'@')) && (j>0) && (stkpos[i-1]>=0))
    if ( ((in[i]=='*')&&(in[i+1]!='*')) || (in[i]=='/') )
     {
      int ei=i, ej=j, sp1, sp2, stat=0, errpos=-1, gotUnit;
      pplObj val;
      gotUnit = (c->tokenBuff[j-3] == (unsigned char)('P'-'@'));
      while ((c->tokenBuff[ej] == (unsigned char)('J'-'@')) && (ej<tlen)) { ej+=3; ei++; } // ffw over binary operator
      if ((ej>=tlen)||(stkpos[ei]<0)) continue; // right argument is not a literal
      gotUnit = gotUnit || (c->tokenBuff[ej] == (unsigned char)('G'-'@'));
      if (!gotUnit) continue; // neither side is a unit()
      sp1 = stkpos[i-1];
      sp2 = stkpos[ei];
      val.refCount   = 1;
      val.amMalloced = 0;
      pplObjNum(&val,0,0,0);
      if (in[i]=='*') ppl_uaMul(c, &c->stack[sp1], &c->stack[sp2], &val, &stat, &errpos, c->errcontext.tempErrStr);
      else            ppl_uaDiv(c, &c->stack[sp1], &c->stack[sp2], &val, &stat, &errpos, c->errcontext.tempErrStr);
      if ((stat)||(errpos>=0)) continue;
      memcpy(&c->stack[sp1], &val, sizeof(pplObj));
      for ( ; j<ej           ; i++,j+=3 ) { stkpos[i]=sp1; }
      for ( ; stkpos[i]==sp2 ; i++,j+=3 ) { stkpos[i]=sp1; }
     }

  // Produce tex output
  for (i=j=k=0; j<tlen; )
   {
    char o = c->tokenBuff[j]+'@'; // Get tokenised markup state code
    ENTER_MATHMODE;
    if (stkpos[i]>=0)
     {
      snprintf(out+k, outlen-k, "%s", ppl_unitsNumericDisplay(c, &c->stack[stkpos[i]], 0, SW_DISPLAY_L, 0)+1); // Chop off initial $
      k+=strlen(out+k)-1; // Chop off final $
      out[k]='\0';
      while ((j<tlen)&&(stkpos[i]>=0)) { i++; j+=3; }
     }
    else if (o=='B') // Process a string literal
     {
      int  ei, ej, l;
      char quoteType=in[i];
      for (ei=i,ej=j ; ((ej<tlen)&&(c->tokenBuff[ej]+'@'==o)) ; ei++,ej+=3);
      if ((quoteType=='\'')||(quoteType=='\"')) { l=(ei-i-2); i++; }
      else                                      { l=(ei-i  );      }
      ENTER_TEXTRM;
      if      (quoteType=='\'') snprintf(out+k, outlen-k, "`");
      else if (quoteType=='\"') snprintf(out+k, outlen-k, "``");
      k+=strlen(out+k);
      ppl_texify_string(in+i, out+k, l, outlen-k-1);
      k+=strlen(out+k);
      if      (quoteType=='\'') snprintf(out+k, outlen-k, "'");
      else if (quoteType=='\"') snprintf(out+k, outlen-k, "''");
      k+=strlen(out+k);
      ENTER_MATHMODE;
      i=ei; j=ej;
     }
    else if (o=='C') // string substitution operator
     {
      snprintf(out+k, outlen-k, "\\%%");
      k+=strlen(out+k);
      FFW_TOKEN;
     }
    else if ((o=='D')||(o=='E')||(o=='P')) // open or close brackets
     {
      if (in[i]=='(')
       {
        snprintf(out+k, outlen-k, "\\left( ");
        k+=strlen(out+k);
        if ((bracketLevel>=0)&&(bracketLevel<MAX_BRACKETS)) bracketOpenPos[bracketLevel] = k;
        bracketLevel++;
        if (bracketLevel > highestBracketLevel) highestBracketLevel = bracketLevel;
       }
      else if (in[i]==')')
       {
        snprintf(out+k, outlen-k, "\\right) ");
        k+=strlen(out+k);
        bracketLevel--;
        if ((bracketLevel>=0)&&(bracketLevel<MAX_BRACKETS))
         {
          int j = (highestBracketLevel - bracketLevel) % 3;
          if      (j==1) { out[k-2]=')' ; out[k-1]=' '; out[bracketOpenPos[bracketLevel]-2]='(' ; out[bracketOpenPos[bracketLevel]-1]=' '; }
          else if (j==2) { out[k-2]=']' ; out[k-1]=' '; out[bracketOpenPos[bracketLevel]-2]='[' ; out[bracketOpenPos[bracketLevel]-1]=' '; }
          else           { out[k-2]='\\'; out[k-1]='}'; out[bracketOpenPos[bracketLevel]-2]='\\'; out[bracketOpenPos[bracketLevel]-1]='{'; }
         }
       }
      i++; j+=3;
     }
    else if ((o=='F')||(o=='H')) // -- or ++ operators
     {
      if (in[i]=='-') snprintf(out+k, outlen-k, "--");
      if (in[i]=='+') snprintf(out+k, outlen-k, "++");
      k+=strlen(out+k);
      i++; j+=3;
      i++; j+=3;
     }
    else if ((o=='G')||(o=='T')||(o=='V')) // variable name
     {
      ppl_texify_MakeGreek(in+i, out+k, outlen-k, mathMode, textRm);
      k+=strlen(out+k);
      FFW_TOKEN;
     }
    else if (o=='I') // unary operator
     {

#define MARKUP_MATCH(A) (strncmp(in+i,A,strlen(A))==0)

      if      (MARKUP_MATCH("-"  )) { snprintf(out+k, outlen-k, "-"); FFW_N(1); }
      else if (MARKUP_MATCH("+"  )) { snprintf(out+k, outlen-k, "+"); FFW_N(1); }
      else if (MARKUP_MATCH("~"  )) { snprintf(out+k, outlen-k, "\\sim"); FFW_N(1); }
      else if (MARKUP_MATCH("!"  )) { snprintf(out+k, outlen-k, "!"); FFW_N(1); }
      else if (MARKUP_MATCH("not")) { ENTER_TEXTRM; snprintf(out+k, outlen-k, "not"); FFW_N(3); }
      else if (MARKUP_MATCH("NOT")) { ENTER_TEXTRM; snprintf(out+k, outlen-k, "not"); FFW_N(3); }
      k+=strlen(out+k);
     }
    else if (o=='J') // a binary operator
     {
      if      (MARKUP_MATCH("**" )) { snprintf(out+k, outlen-k, "\\,*\\kern-1.5pt *\\,\\,"); FFW_N(2); }
      else if (MARKUP_MATCH("<<" )) { snprintf(out+k, outlen-k, "\\ll"); FFW_N(2); }
      else if (MARKUP_MATCH(">>" )) { snprintf(out+k, outlen-k, "\\gg"); FFW_N(2); }
      else if (MARKUP_MATCH("<=" )) { snprintf(out+k, outlen-k, "\\leq"); FFW_N(2); }
      else if (MARKUP_MATCH(">=" )) { snprintf(out+k, outlen-k, "\\geq"); FFW_N(2); }
      else if (MARKUP_MATCH("==" )) { snprintf(out+k, outlen-k, "=="); FFW_N(2); }
      else if (MARKUP_MATCH("<>" )) { snprintf(out+k, outlen-k, "<>"); FFW_N(2); }
      else if (MARKUP_MATCH("!=" )) { snprintf(out+k, outlen-k, "!="); FFW_N(2); }
      else if (MARKUP_MATCH("&&" )) { ENTER_TEXTRM; snprintf(out+k, outlen-k, "\\,\\&\\&\\,"); FFW_N(2); }
      else if (MARKUP_MATCH("||" )) { snprintf(out+k, outlen-k, "\\,||\\,"); FFW_N(2); }
      else if (MARKUP_MATCH("*"  )) { snprintf(out+k, outlen-k, "\\times"); FFW_N(1); }
      else if (MARKUP_MATCH("/"  )) { snprintf(out+k, outlen-k, "/"); FFW_N(1); }
      else if (MARKUP_MATCH("%"  )) { ENTER_TEXTRM; snprintf(out+k, outlen-k, "\\%%"); FFW_N(1); }
      else if (MARKUP_MATCH("+"  )) { snprintf(out+k, outlen-k, "+"); FFW_N(1); }
      else if (MARKUP_MATCH("-"  )) { snprintf(out+k, outlen-k, "-"); FFW_N(1); }
      else if (MARKUP_MATCH("<"  )) { snprintf(out+k, outlen-k, "<"); FFW_N(1); }
      else if (MARKUP_MATCH(">"  )) { snprintf(out+k, outlen-k, ">"); FFW_N(1); }
      else if (MARKUP_MATCH("&"  )) { ENTER_TEXTRM; snprintf(out+k, outlen-k, "\\,\\&\\,"); FFW_N(1); }
      else if (MARKUP_MATCH("^"  )) { snprintf(out+k, outlen-k, "\\verb|^|"); FFW_N(1); }
      else if (MARKUP_MATCH("|"  )) { snprintf(out+k, outlen-k, "\\,|\\,"); FFW_N(1); }
      else if (MARKUP_MATCH(","  )) { snprintf(out+k, outlen-k, ","); FFW_N(1); }
      k+=strlen(out+k);
     }
    else if (o=='K') // a terniary operator
     {
      if      (MARKUP_MATCH("?")) { snprintf(out+k, outlen-k, "\\,?\\,"); FFW_N(1); }
      else if (MARKUP_MATCH(":")) { snprintf(out+k, outlen-k, ":"); FFW_N(1); }
      k+=strlen(out+k);
     }
    else if ((o=='M')||(o=='Q')) // list literal
     {
      if (in[i]=='[') snprintf(out+k, outlen-k, "\\left[");
      if (in[i]==']') snprintf(out+k, outlen-k, "\\right]");
      k+=strlen(out+k);
      i++; j+=3;
     }
    else if (o=='N') // dictionary literal
     {
      if (in[i]=='{') snprintf(out+k, outlen-k, "\\left\\{");
      if (in[i]=='}') snprintf(out+k, outlen-k, "\\right\\}");
      k+=strlen(out+k);
      i++; j+=3;
     }
    else if (o=='O') // dollar operator
     {
      if (in[i]=='$') snprintf(out+k, outlen-k, "\\$");
      k+=strlen(out+k);
      i++; j+=3;
     }
    else if (o=='R') // dot operator
     {
      if (in[i]=='.') snprintf(out+k, outlen-k, ".");
      k+=strlen(out+k);
      i++; j+=3;
     }
    else if (o=='S') // assignment operator
     {
      if      (MARKUP_MATCH("="  )) { snprintf(out+k, outlen-k, "="); FFW_N(1); }
      else if (MARKUP_MATCH("+=" )) { snprintf(out+k, outlen-k, "\\,\\,+\\kern-2.5pt ="); FFW_N(2); }
      else if (MARKUP_MATCH("-=" )) { snprintf(out+k, outlen-k, "\\,\\,-\\kern-2.5pt ="); FFW_N(2); }
      else if (MARKUP_MATCH("*=" )) { snprintf(out+k, outlen-k, "\\,\\,\\times\\kern-2.5pt ="); FFW_N(2); }
      else if (MARKUP_MATCH("/=" )) { snprintf(out+k, outlen-k, "\\,/\\kern-1.5pt ="); FFW_N(2); }
      else if (MARKUP_MATCH("%=" )) { snprintf(out+k, outlen-k, "\\,\\,\\textrm{\\%%}\\kern-2pt ="); FFW_N(2); }
      else if (MARKUP_MATCH("&=" )) { snprintf(out+k, outlen-k, "\\,\\,\\textrm{\\&}\\kern-2pt ="); FFW_N(2); }
      else if (MARKUP_MATCH("^=" )) { snprintf(out+k, outlen-k, "\\,\\,\\verb|^|\\kern-2pt ="); FFW_N(2); }
      else if (MARKUP_MATCH("|=" )) { snprintf(out+k, outlen-k, "\\,\\,|\\kern-2.3pt ="); FFW_N(2); }
      else if (MARKUP_MATCH("<<=")) { snprintf(out+k, outlen-k, "\\ll="); FFW_N(3); }
      else if (MARKUP_MATCH(">>=")) { snprintf(out+k, outlen-k, "\\gg="); FFW_N(3); }
      else if (MARKUP_MATCH("**=")) { snprintf(out+k, outlen-k, "\\,*\\kern-1.5pt *\\kern-1.5pt="); FFW_N(3); }
      k+=strlen(out+k);
     }
    else // unknown token
     {
      FFW_TOKEN;
     }
   }

  ENTER_PLAINTEXT;
  out[k]='\0';
  free(stkpos);
  return;
 }

