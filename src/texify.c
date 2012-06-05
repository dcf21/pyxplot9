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
#include "mathsTools/dcfmath.h"
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
   while ((j<tlen)&&(tokenBuff[j]+'@'==o)) { i++; j+=3; }

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

void ppl_texify_generic(ppl_context *c, char *in, int inlen, int *end, char *out, int outlen, int *mm_in, int *tr_in)
 {
  unsigned char *tokenBuff;
  const int      allowCommaOperator = (inlen<0);
  const int      equalsAllowed      = 1;
  const int      dollarAllowed      = 1;
  int            errPos=-1, errType, tlen;
  char           errbuff[LSTR_LENGTH];
  int           *stkpos, i, j, k, lastOpMultDiv=0;

  int            mm=0, tr=0;
  int           *mathMode = (mm_in==NULL) ? &mm : mm_in;
  int           *textRm   = (tr_in==NULL) ? &tr : tr_in;

#define MAX_BRACKETS 4096
  int        bracketLevel=0, highestBracketLevel=0, bracketOpenPos[MAX_BRACKETS];

  // First tokenise expression
  ppl_expTokenise(c, in, end, dollarAllowed, equalsAllowed, allowCommaOperator, 0, 0, 0, &tlen, &errPos, &errType, errbuff);
  if (errPos>=0) { *end=0; return; }
  if (tlen  <=0) { *end=0; return; }

  // Backup tokenised version
  tokenBuff = (unsigned char *)malloc(tlen);
  if (tokenBuff==NULL) { *end=0; return; }
  memcpy(tokenBuff, c->tokenBuff, tlen);
  if (inlen>=0) tlen = ppl_min(tlen, 3*inlen);

  // Malloc buffer for storing stack positions of numerical constants
  stkpos = (int *)malloc(tlen*sizeof(int));
  if (stkpos==NULL) { *end=0; return; }
  for (i=0; i<tlen; i++) stkpos[i]=-1;

  // Evaluate numerical constants
  k = c->stackPtr;
  for (i=j=0; j<tlen; i++, j+=3)
   if (tokenBuff[j] == (unsigned char)('L'-'@'))
    {
     if (k > ALGEBRA_STACK-4) { free(stkpos); free(tokenBuff); strcpy(out, "stack overflow"); return; }
     pplObjNum(&c->stack[k], 0, ppl_getFloat(in+i,NULL), 0);
     while ((tokenBuff[j] == (unsigned char)('L'-'@')) && (j<tlen)) { stkpos[i++]=k; j+=3; } // ffw over constant
     k++;
    }

  // Evaluate the unit() function (but not after a ., and only if followed by () )
  for (i=j=0; j<tlen; i++, j+=3)
   if ((tokenBuff[j] == (unsigned char)('G'-'@')) && ((j<1)||((tokenBuff[j-3] != (unsigned char)('R'-'@'))&&(tokenBuff[j-3] != (unsigned char)('G'-'@')))))
    {
     int    ei=i, ej=j, upos, p, q=0, errpos=-1;
     pplObj val;
     if (k > ALGEBRA_STACK-4) { free(stkpos); free(tokenBuff); strcpy(out, "stack overflow"); return; }
     while ((tokenBuff[ej] == (unsigned char)('G'-'@')) && (ej<tlen)) { ej+=3; ei++; } // ffw over function name
     if (tokenBuff[ej] != (unsigned char)('P'-'@')) continue; // we don't have function arguments
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
     if (tokenBuff[ej] != (unsigned char)('P'-'@')) continue; // we don't have closing bracket
     ei++; ej+=3;
     while ((tokenBuff[ej] == (unsigned char)('P'-'@')) && (in[i]<=' ') && (in[i]>='\0') && (ej<tlen)) { ej+=3; ei++; } // ffw over function arguments
     memcpy(&c->stack[k], &val, sizeof(pplObj));

     // Clean up
     for ( ; j<ej ; i++,j+=3 ) { stkpos[i]=k; }
     k++;
    }

  // Evaluate minus signs if literal is right argument
  for (i=j=0; j<tlen; i++, j+=3)
   if ((tokenBuff[j] == (unsigned char)('I'-'@')) && ((j<1)||(tokenBuff[j-3] != (unsigned char)('I'-'@'))))
    if (in[i]=='-')
     {
      int ei=i, ej=j, sp;
      while ((tokenBuff[ej] == (unsigned char)('I'-'@')) && (ej<tlen)) { ej+=3; ei++; } // ffw over minus sign
      if ((ej>=tlen)||(stkpos[ei]<0)) continue; // right argument is not literal or unit() function
      sp = stkpos[ei];
      c->stack[sp].real*=-1;
      if (c->stack[sp].flagComplex) c->stack[sp].imag*=-1;
      for ( ; j<ej ; i++,j+=3 ) { stkpos[i]=sp; }
     }

  // Evaluate ** operator if unit() is left argument
  for (i=j=0; j<tlen; i++, j+=3)
   if ((tokenBuff[j] == (unsigned char)('J'-'@')) && (j>0) && (tokenBuff[j-3] == (unsigned char)('P'-'@')) && (stkpos[i-1]>=0))
    if ((in[i]=='*')&&(in[i+1]=='*'))
     {
      int ei=i, ej=j, sp1, sp2, stat=0, errpos=-1;
      pplObj val;
      while ((tokenBuff[ej] == (unsigned char)('J'-'@')) && (ej<tlen)) { ej+=3; ei++; } // ffw over binary operator
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
   if ((tokenBuff[j] == (unsigned char)('J'-'@')) && (j>0) && (stkpos[i-1]>=0))
    if ( ((in[i]=='*')&&(in[i+1]!='*')) || (in[i]=='/') )
     {
      int ei=i, ej=j, sp1, sp2, stat=0, errpos=-1, gotUnit;
      pplObj val;
      gotUnit = (tokenBuff[j-3] == (unsigned char)('P'-'@'));
      while ((tokenBuff[ej] == (unsigned char)('J'-'@')) && (ej<tlen)) { ej+=3; ei++; } // ffw over binary operator
      if ((ej>=tlen)||(stkpos[ei]<0)) continue; // right argument is not a literal
      gotUnit = gotUnit || (tokenBuff[ej] == (unsigned char)('G'-'@'));
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
    int  opMultDiv=0;
    char o = tokenBuff[j]+'@'; // Get tokenised markup state code
    ENTER_MATHMODE;
    if (stkpos[i]>=0)
     {
      double  real=0, imag=0;
      pplObj *o = &c->stack[stkpos[i]];
      char   *u = NULL;
      if (lastOpMultDiv) u = ppl_printUnit(c, o, &real, &imag, 0, 1, SW_DISPLAY_L);
      if ((real!=1) || (imag!=0))
       {
        snprintf(out+k, outlen-k, "%s", ppl_unitsNumericDisplay(c, o, 0, SW_DISPLAY_L, 0)+1); // Chop off initial $
        k+=strlen(out+k)-1; // Chop off final $
        out[k]='\0';
       }
      else
       {
        snprintf(out+k, outlen-k, "%s", u);
        k+=strlen(out+k);
       }
      while ((j<tlen)&&(stkpos[i]>=0)) { i++; j+=3; }
     }
    else if (o=='B') // Process a string literal
     {
      int  ei, ej, l;
      char quoteType=in[i];
      for (ei=i,ej=j ; ((ej<tlen)&&(tokenBuff[ej]+'@'==o)) ; ei++,ej+=3);
      l = ei-i;
      while ((l>1)&&(in[i+l-1]>='\0')&&(in[i+l-1]<=' ')) l--; // Strip trailing spaces
      if ((quoteType=='\'')||(quoteType=='\"')) { l-=2; i++; }
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
      if (in[i]=='-') { ENTER_TEXTRM; snprintf(out+k, outlen-k, "{\\tiny \\raisebox{0.8pt}{\\kern-0.1pt --\\kern0.5pt --}}"); }
      if (in[i]=='+') { ENTER_TEXTRM; snprintf(out+k, outlen-k, "{\\tiny \\raisebox{0.8pt}{\\kern-0.2pt +\\kern-0.2pt +}}"); }
      k+=strlen(out+k);
      i++; j+=3;
      i++; j+=3;
      while ((in[i]>='\0')&&(in[i]<=' ')&&(tokenBuff[j]+'@'==o)) { FFW_N(1); }
     }
    else if ((o=='G')||(o=='T')||(o=='V')) // variable name
     {
      if ((o=='G')&&((j<1)||(tokenBuff[j-3]+'@'!='R')))
       {
        int       ei=i, ej=j, bi, cp[16], ncp, cbp;
        char      dummyVar[FNAME_LENGTH], dummyVarGreek[FNAME_LENGTH], *fname, *latex;
        dictItem *dptr;
        pplFunc  *fnobj;
        void     *ldptr;
        while ((ej<tlen)&&(tokenBuff[ej]+'@'==o)) { ei++; ej+=3; }
        if ((ej>=tlen)||((tokenBuff[ej]+'@'!='P')&&(tokenBuff[ej]+'@'!='B'))) goto variableName; // function not called with () afterwards; int_d is followed by B, not P
        ppl_dictLookupWithWildcard(c->namespaces[0],in+i,dummyVar,FNAME_LENGTH,&dptr);
        if (dptr==NULL) goto variableName; // function was not in the default namespace
        fname = dptr->key;
        ldptr = ppl_dictLookup(c->namespaces[c->ns_ptr], fname); if (ldptr!=NULL) goto variableName; // function redefined locally
        ldptr = ppl_dictLookup(c->namespaces[1        ], fname); if (ldptr!=NULL) goto variableName; // function redefined globally
        fnobj = (pplFunc *)((pplObj *)dptr->data)->auxil;
        if (fnobj==NULL) goto variableName;
        latex = fnobj->LaTeX;
        if (latex==NULL) goto variableName; // no latex model supplied

        // Find ( at beginning of function arguments
        for (bi=i; isalnum(in[bi])||(in[bi]=='_'); bi++);
        for (    ; (in[bi]>'\0')&&(in[bi]<=' '); bi++);
        if (in[bi]!='(') goto variableName; // could not find (
        ppl_strBracketMatch(in+bi,'(',')',cp,&ncp,&cbp,16); // Search for a ) to match the (
        if (cbp<= 0) goto variableName; // could not find )
        if (ncp>=16) goto variableName; // wrong number of arguments
        if (dummyVar[0]=='\0') { if ((ncp< fnobj->minArgs)||(ncp>fnobj->maxArgs)) goto variableName; } // wrong number of arguments
        else                   { if  (ncp!=fnobj->minArgs)                        goto variableName; }

        // Work through latex model
        ppl_texify_MakeGreek(dummyVar, dummyVarGreek, FNAME_LENGTH, mathMode, textRm);
        for (j=0; latex[j]!='\0'; j++)
         {
          if      (latex[j  ]!='@') snprintf(out+k, outlen-k, "%c", latex[j]);
          else if (latex[j+1]=='?') snprintf(out+k, outlen-k, "%s", dummyVarGreek);
          else if (latex[j+1]=='<')
           {
            snprintf(out+k, outlen-k, "\\left( ");
            k+=strlen(out+k);
            if ((bracketLevel>=0)&&(bracketLevel<MAX_BRACKETS)) bracketOpenPos[bracketLevel] = k;
            bracketLevel++;
            if (bracketLevel > highestBracketLevel) highestBracketLevel = bracketLevel;
           }
          else if (latex[j+1]=='>')
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
          else if ((latex[j+1]>='1')&&(latex[j+1]<='6'))
           {
            int l=(latex[j+1]-'1'), end;
            ppl_texify_generic(c, in+bi+cp[l]+1, cp[l+1]-cp[l]-1, &end, out+k, outlen-k, mathMode, textRm);
            k+=strlen(out+k);
           }
          else if (latex[j+1]=='0')
           {
            int a, end;
            for (a=0; a<ncp; a++)
             {
              if (a!=0) { snprintf(out+k, outlen-k, ","); k+=strlen(out+k); }
              ppl_texify_generic(c, in+bi+cp[a]+1, cp[a+1]-cp[a]-1, &end, out+k, outlen-k, mathMode, textRm);
              k+=strlen(out+k);
             }
           }
          k+=strlen(out+k);
          if (latex[j]=='@') j++; // FFW over two-byte code
         }

        // Carry on work after function
        i = bi+cbp+1;
        j = 3*i;
       }

      // If variable name is not a recognised function name, print name as a variable name
variableName:
      ppl_texify_MakeGreek(in+i, out+k, outlen-k, mathMode, textRm);
      k+=strlen(out+k);
      FFW_TOKEN;
     }
    else if (o=='I') // unary operator
     {

#define MARKUP_MATCH(A) (strncmp(in+i,A,strlen(A))==0)

      if      (MARKUP_MATCH("-"  )) { snprintf(out+k, outlen-k, "-"); FFW_N(1); }
      else if (MARKUP_MATCH("+"  )) { snprintf(out+k, outlen-k, "+"); FFW_N(1); }
      else if (MARKUP_MATCH("~"  )) { snprintf(out+k, outlen-k, "\\sim "); FFW_N(1); }
      else if (MARKUP_MATCH("!"  )) { snprintf(out+k, outlen-k, "!"); FFW_N(1); }
      else if (MARKUP_MATCH("not")) { ENTER_TEXTRM; snprintf(out+k, outlen-k, " not "); FFW_N(3); }
      else if (MARKUP_MATCH("NOT")) { ENTER_TEXTRM; snprintf(out+k, outlen-k, " not "); FFW_N(3); }
      k+=strlen(out+k);
      while ((in[i]>='\0')&&(in[i]<=' ')&&(tokenBuff[j]+'@'==o)) { FFW_N(1); }
     }
    else if (o=='J') // a binary operator
     {
      if      (MARKUP_MATCH("**" )) { snprintf(out+k, outlen-k, "\\,*\\kern-1.5pt *\\,\\,"); FFW_N(2); }
      else if (MARKUP_MATCH("<<" )) { snprintf(out+k, outlen-k, "\\ll "); FFW_N(2); }
      else if (MARKUP_MATCH(">>" )) { snprintf(out+k, outlen-k, "\\gg "); FFW_N(2); }
      else if (MARKUP_MATCH("<=" )) { snprintf(out+k, outlen-k, "\\leq "); FFW_N(2); }
      else if (MARKUP_MATCH(">=" )) { snprintf(out+k, outlen-k, "\\geq "); FFW_N(2); }
      else if (MARKUP_MATCH("==" )) { snprintf(out+k, outlen-k, "=="); FFW_N(2); }
      else if (MARKUP_MATCH("<>" )) { snprintf(out+k, outlen-k, "<>"); FFW_N(2); }
      else if (MARKUP_MATCH("!=" )) { snprintf(out+k, outlen-k, "!="); FFW_N(2); }
      else if (MARKUP_MATCH("&&" )) { ENTER_TEXTRM; snprintf(out+k, outlen-k, "\\,\\&\\&\\,"); FFW_N(2); }
      else if (MARKUP_MATCH("and")) { ENTER_TEXTRM; snprintf(out+k, outlen-k, " and "); FFW_N(3); }
      else if (MARKUP_MATCH("AND")) { ENTER_TEXTRM; snprintf(out+k, outlen-k, " and "); FFW_N(3); }
      else if (MARKUP_MATCH("||" )) { snprintf(out+k, outlen-k, "\\,||\\,"); FFW_N(2); }
      else if (MARKUP_MATCH("or" )) { ENTER_TEXTRM; snprintf(out+k, outlen-k, " or "); FFW_N(2); }
      else if (MARKUP_MATCH("OR" )) { ENTER_TEXTRM; snprintf(out+k, outlen-k, " or "); FFW_N(2); }
      else if (MARKUP_MATCH("*"  )) { snprintf(out+k, outlen-k, "\\times "); FFW_N(1); opMultDiv=1; }
      else if (MARKUP_MATCH("/"  )) { snprintf(out+k, outlen-k, "/"); FFW_N(1); opMultDiv=1; }
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
      while ((in[i]>='\0')&&(in[i]<=' ')&&(tokenBuff[j]+'@'==o)) { FFW_N(1); }
     }
    else if (o=='K') // a terniary operator
     {
      if      (MARKUP_MATCH("?")) { snprintf(out+k, outlen-k, "\\,?\\,"); FFW_N(1); }
      else if (MARKUP_MATCH(":")) { snprintf(out+k, outlen-k, ":"); FFW_N(1); }
      k+=strlen(out+k);
      while ((in[i]>='\0')&&(in[i]<=' ')&&(tokenBuff[j]+'@'==o)) { FFW_N(1); }
     }
    else if ((o=='M')||(o=='Q')) // list literal
     {
      if (in[i]=='[') snprintf(out+k, outlen-k, "\\left[");
      if (in[i]==']') snprintf(out+k, outlen-k, "\\right]");
      k+=strlen(out+k);
      i++; j+=3;
      while ((in[i]>='\0')&&(in[i]<=' ')&&(tokenBuff[j]+'@'==o)) { FFW_N(1); }
     }
    else if (o=='N') // dictionary literal
     {
      if (in[i]=='{') snprintf(out+k, outlen-k, "\\left\\{");
      if (in[i]=='}') snprintf(out+k, outlen-k, "\\right\\}");
      k+=strlen(out+k);
      i++; j+=3;
      while ((in[i]>='\0')&&(in[i]<=' ')&&(tokenBuff[j]+'@'==o)) { FFW_N(1); }
     }
    else if (o=='O') // dollar operator
     {
      if (in[i]=='$') snprintf(out+k, outlen-k, "\\$");
      k+=strlen(out+k);
      i++; j+=3;
      while ((in[i]>='\0')&&(in[i]<=' ')&&(tokenBuff[j]+'@'==o)) { FFW_N(1); }
     }
    else if (o=='R') // dot operator
     {
      if (in[i]=='.') snprintf(out+k, outlen-k, ".");
      k+=strlen(out+k);
      i++; j+=3;
      while ((in[i]>='\0')&&(in[i]<=' ')&&(tokenBuff[j]+'@'==o)) { FFW_N(1); }
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
      while ((in[i]>='\0')&&(in[i]<=' ')&&(tokenBuff[j]+'@'==o)) { FFW_N(1); }
     }
    else // unknown token
     {
      FFW_TOKEN;
     }
    lastOpMultDiv = opMultDiv;
   }

  if (inlen<0) { ENTER_PLAINTEXT; }
  out[k]='\0';
  free(stkpos);
  free(tokenBuff);
  return;
 }

