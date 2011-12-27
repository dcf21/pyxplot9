// expEval.c
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

#define _EXPEVAL_C 1

#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <limits.h>

#include <gsl/gsl_math.h>

#include "coreUtils/dict.h"
#include "coreUtils/errorReport.h"
#include "coreUtils/list.h"
#include "expressions/expEval.h"
#include "expressions/expEvalCalculus.h"
#include "expressions/expEvalOps.h"
#include "expressions/expEvalSlice.h"
#include "expressions/fnCall.h"
#include "settings/settingTypes.h"
#include "stringTools/asciidouble.h"
#include "userspace/context.h"
#include "userspace/garbageCollector.h"
#include "userspace/pplObj_fns.h"
#include "userspace/pplObjCmp.h"
#include "userspace/pplObjFunc.h"
#include "userspace/pplObjMethods.h"
#include "userspace/pplObjPrint.h"
#include "userspace/unitsArithmetic.h"
#include "userspace/unitsDisp.h"

#include "pplConstants.h"

static void expEval_stringSubs(ppl_context *context, int Nsubs, int *status, int *errType, char *errText)
 {
  const char allowedFormats[] = "cdieEfgGosSxX%"; // These tokens are allowed after a % format specifier
  char       formatToken[512];
  pplObj    *obj = &context->stack[context->stackPtr];
  int        argP   = -Nsubs;
  char      *format = (char *)((obj+argP-1)->auxil);
  int        outlen = 65536;
  int        inP    = 0;
  int        outP   = 0;
  int        inP2, requiredArgs, l, arg1i, arg2i;
  char      *out    = (char *)malloc(outlen);

  if (out==NULL) { *status=1; *errType=ERR_MEMORY; strcpy(errText, "Out of memory."); return; }

  // Loop over format string looking for tokens
  for ( ; format[inP]!='\0' ; inP++)
   {
    // If output buffer is nearly full, malloc some more
    if (outP>outlen-16384)
     {
      char *outnew=(char *)realloc((void*)out , outlen+=65536);
      if (outnew==NULL) { free(out); *status=1; *errType=ERR_MEMORY; strcpy(errText, "Out of memory."); return; }
      out = outnew;
     }

    if (format[inP]!='%') { out[outP++] = format[inP]; continue; } // Characters other than % tokens are copied straight to output
    inP2=inP+1; // inP2 looks ahead to see experimentally if syntax is right
    requiredArgs = 1; // Normal %f like tokens require 1 argument
    if ((format[inP2]=='+')||(format[inP2]=='-')||(format[inP2]==' ')||(format[inP2]=='#')) inP2++; // optional flag can be <+- #>
    if (format[inP2]=='*') { inP2++; requiredArgs++; }
    else while ((format[inP2]>='0') && (format[inP2]<='9')) inP2++; // length can be * or some digits
    if (format[inP2]=='.') // precision starts with a . and is followed by * or by digits
     {
      inP2++;
      if (format[inP2]=='*') { inP2++; requiredArgs++; }
      else while ((format[inP2]>='0') && (format[inP2]<='9')) inP2++; // length can be * or some digits
     }

    // We do not allow user to specify optional length flag, which could potentially be <hlL>

    for (l=0; allowedFormats[l]!='\0'; l++) if (format[inP2]==allowedFormats[l]) break;
    if (allowedFormats[l]=='%') requiredArgs=0;
    if ((allowedFormats[l]=='\0') || ((allowedFormats[l]=='%')&&(format[inP2-1]!='%')) ) { out[outP++] = format[inP]; continue; } // Have not got correct syntax for a format specifier
    if (requiredArgs > -argP) { free(out); *status=1; *errType=ERR_RANGE; sprintf(errText, "Too few arguments supplied to string substitution operator"); return; } // Have run out of substitution arguments

    // %% token simply produces a literal %
    if (allowedFormats[l]=='%')
     {
      strcpy(out+outP, "%"); // %% just produces a % character
     } else {
      ppl_strSlice(format, formatToken, inP, inP2+1);

      // If token required extra integer arguments, read these now
      if (requiredArgs > 1) { CAST_TO_INT(obj+argP,"%"); arg1i = (int)((obj+argP)->real); argP++; }
      if (requiredArgs > 2) { CAST_TO_INT(obj+argP,"%"); arg2i = (int)((obj+argP)->real); argP++; }

      // Print a string
      if ((allowedFormats[l]=='c') || (allowedFormats[l]=='s') || (allowedFormats[l]=='S'))
       {
        const int tmpbufflen = 65536;
        char *tmpbuff = (char *)malloc(tmpbufflen);
        if (tmpbuff==NULL) { free(out); *status=1; *errType=ERR_MEMORY; sprintf(errText, "Out of memory."); return; }
        pplObjPrint(context, obj+argP, NULL, tmpbuff, tmpbufflen, 0, 0);
        formatToken[inP2-inP] = 's';
        if (requiredArgs==1) snprintf(out+outP, outlen-outP, formatToken, tmpbuff); // Print a string variable
        if (requiredArgs==2) snprintf(out+outP, outlen-outP, formatToken, arg1i, tmpbuff);
        if (requiredArgs==3) snprintf(out+outP, outlen-outP, formatToken, arg1i, arg2i, tmpbuff);
        out[outlen-1]='\0';
        free(tmpbuff);
       }
      else
       {
        pplObj *o = obj+argP;
        CAST_TO_NUM(obj+argP); // Make object into a number
        if ((!gsl_finite(o->real)) || ((context->set->term_current.ComplexNumbers == SW_ONOFF_OFF) && (o->flagComplex!=0))) { strcpy(out+outP, "nan"); }
        else
         {
          if      ((allowedFormats[l]=='d') || (allowedFormats[l]=='i') || (allowedFormats[l]=='o') || (allowedFormats[l]=='x') || (allowedFormats[l]=='X'))
           {
            if      ((o->real>INT_MAX)||(o->imag>INT_MAX)) { strcpy(out+outP,  "inf"); }
            else if ((o->real<INT_MIN)||(o->imag<INT_MIN)) { strcpy(out+outP, "-inf"); }
            else if (o->flagComplex == 0) // Print a real integer
             {
              int arg3i = (int)o->real; // sprintf will expect to be passed an int
              if (requiredArgs==1) snprintf(out+outP, outlen-outP, formatToken, arg3i); // Print an integer variable
              if (requiredArgs==2) snprintf(out+outP, outlen-outP, formatToken, arg1i, arg3i);
              if (requiredArgs==3) snprintf(out+outP, outlen-outP, formatToken, arg1i, arg2i, arg3i);
             }
            else // Print a complex integer
             {
              int arg3i = (int)o->real; // sprintf will expect to be passed an int real part, and an int imag part
              int arg4i = (int)o->imag;
              if (requiredArgs==1) snprintf(out+outP, outlen-outP, formatToken, arg3i); // Print an integer real part
              if (requiredArgs==2) snprintf(out+outP, outlen-outP, formatToken, arg1i, arg3i);
              if (requiredArgs==3) snprintf(out+outP, outlen-outP, formatToken, arg1i, arg2i, arg3i);
              outP += strlen(out+outP);
              out[outP++]='+';
              if (requiredArgs==1) snprintf(out+outP, outlen-outP, formatToken, arg4i); // Print an integer imag part
              if (requiredArgs==2) snprintf(out+outP, outlen-outP, formatToken, arg1i, arg4i);
              if (requiredArgs==3) snprintf(out+outP, outlen-outP, formatToken, arg1i, arg2i, arg4i);
              outP += strlen(out+outP);
              strcpy(out+outP,"i");
             }
           }
          else if ((allowedFormats[l]=='x') || (allowedFormats[l]=='X'))
           {
            if      ((o->real>UINT_MAX)||(o->imag>UINT_MAX)) { strcpy(out+outP,  "inf"); }
            else if ((o->real<0       )||(o->imag<0       )) { strcpy(out+outP, "-inf"); }
            else if (o->flagComplex == 0) // Print a real unsigned integer
             {
              unsigned int arg3u = (unsigned int)o->real; // sprintf will expect to be passed an unsigned int
              if (requiredArgs==1) snprintf(out+outP, outlen-outP, formatToken, arg3u); // Print an integer variable
              if (requiredArgs==2) snprintf(out+outP, outlen-outP, formatToken, arg1i, arg3u);
              if (requiredArgs==3) snprintf(out+outP, outlen-outP, formatToken, arg1i, arg2i, arg3u);
             }
            else // Print a complex unsigned integer
             {
              unsigned int arg3u = (unsigned int)o->real; // sprintf will expect to be passed an int real part, and an int imag part
              unsigned int arg4u = (unsigned int)o->imag;
              if (requiredArgs==1) snprintf(out+outP, outlen-outP, formatToken, arg3u); // Print an integer real part
              if (requiredArgs==2) snprintf(out+outP, outlen-outP, formatToken, arg1i, arg3u);
              if (requiredArgs==3) snprintf(out+outP, outlen-outP, formatToken, arg1i, arg2i, arg3u);
              outP += strlen(out+outP);
              out[outP++]='+';
              if (requiredArgs==1) snprintf(out+outP, outlen-outP, formatToken, arg4u); // Print an integer imag part
              if (requiredArgs==2) snprintf(out+outP, outlen-outP, formatToken, arg1i, arg4u);
              if (requiredArgs==3) snprintf(out+outP, outlen-outP, formatToken, arg1i, arg2i, arg4u);
              outP += strlen(out+outP);
              strcpy(out+outP,"i");
             }
           }
          else // otherwise, sprintf will expect a double
           {
            if (requiredArgs==1) snprintf(out+outP, outlen-outP, formatToken, o->real); // Print a double (real part)
            if (requiredArgs==2) snprintf(out+outP, outlen-outP, formatToken, arg1i, o->real);
            if (requiredArgs==3) snprintf(out+outP, outlen-outP, formatToken, arg1i, arg2i, o->real);
            if (o->flagComplex != 0) // Print a complex double
             {
              outP += strlen(out+outP);
              out[outP++]='+';
              if (requiredArgs==1) snprintf(out+outP, outlen-outP, formatToken, o->imag); // Print the imaginary part
              if (requiredArgs==2) snprintf(out+outP, outlen-outP, formatToken, arg1i, o->imag);
              if (requiredArgs==3) snprintf(out+outP, outlen-outP, formatToken, arg1i, arg2i, o->imag);
              outP += strlen(out+outP);
              strcpy(out+outP,"i");
             }
           }
          if (!o->dimensionless)
           {
            outP += strlen(out+outP);
            snprintf(out+outP, outlen-outP, " %s", ppl_printUnit(context,o,&o->real,&o->imag,0,1,0)); // Print dimensions of this value
            out[outlen-1]='\0';
           }
         }
       }
      argP++;
     }
    outP += strlen(out+outP);
    inP = inP2;
   }
  out[outP]='\0';

  // Return output string
  pplObjStr(obj, 0, 1, out);
  context->stackPtr++;
  return;

  cast_fail:
    *status = 1;
    if (out!=NULL) free(out);
    return;
 }

pplObj *ppl_expEval(ppl_context *context, void *in, int *lastOpAssign, int dollarAllowed, int IterDepth, int *errPos, int *errType, char *errText)
 {
  int j=0;
  int initialStackPtr;
  int charpos=0;

  // If at bottom iteration depth, clean up stack now if there is any left-over junk
  if (IterDepth==0) for ( ; context->stackPtr>0 ; ) { context->stackPtr--; ppl_garbageObject(&context->stack[context->stackPtr]); }
  initialStackPtr = context->stackPtr;
  *lastOpAssign=0;

  while (1)
   {
    pplObj *stk     = &context->stack[context->stackPtr];
    int     pos     = j; // Position of start of instruction
    int     len     = *(int *)(in+j            ); // length of bytecode instruction with data
    int     o       = (int)(*(unsigned char *)(in+j+2*sizeof(int)  )); // Opcode number
    charpos = *(int *)(in+j+sizeof(int)); // character position of token (for error reporting)
    j+=2*sizeof(int)+1; // Leave j pointing to first data item after opcode

    if (cancellationFlag) { *errPos=0; *errType=ERR_INTERRUPT; strcpy(errText,"Operation cancelled."); goto cleanup_on_error; }

    switch (o)
     {
      case 0: // Return
        break;
      case 1: // Numeric literal
        *lastOpAssign=0;
        pplObjNum(stk , 0 , *(double *)(in+j) , 0);
        context->stackPtr++;
        break;
      case 2: // String literal
       {
        int l = strlen((char *)(in+j));
        char *out;
        *lastOpAssign=0;
        if ((out = (char *)malloc(l+1))==NULL) { *errPos=charpos; *errType=ERR_MEMORY; strcpy(errText,"Out of memory."); goto cleanup_on_error; }
        strcpy(out , (char *)(in+j));
        pplObjStr(stk , 0 , 1 , out);
        context->stackPtr++;
        break;
       }
      case 3: // Lookup value
       {
        int i , got=0;
        char *key = (char *)(in+j);
        *lastOpAssign=0;
        for (i=context->ns_ptr ; i>=0 ; i=(i>1)?1:i-1)
         {
          pplObj *obj = (pplObj *)ppl_dictLookup(context->namespaces[i] , key);
          if (obj==NULL) continue;
          if ((obj->objType==PPLOBJ_GLOB)||(obj->objType==PPLOBJ_ZOM)) continue;
          pplObjCpy(stk , obj , 0 , 1);
          stk->immutable = stk->immutable || context->namespaces[i]->immutable;
          context->stackPtr++;
          got=1;
          break;
         }
        if (!got) { *errPos=charpos; *errType=ERR_NAMESPACE; sprintf(errText,"No such variable '%s'.",key); goto cleanup_on_error; }
        break;
       }
      case 4: // Lookup value (pointer)
       {
        int i;
        char *key = (char *)(in+j);
        *lastOpAssign=0;
        for (i=context->ns_ptr ; ; i=1)
         {
          pplObj *obj = (pplObj *)ppl_dictLookup(context->namespaces[i] , key);
          if ((obj==NULL)&&(!context->namespaces[i]->immutable))
           {
            pplObjZom(stk , 0); // Create a temporary zombie for now
            ppl_dictAppendCpy(context->namespaces[i] , key , stk , sizeof(pplObj));
            obj = (pplObj *)ppl_dictLookup(context->namespaces[i] , key);
            if (obj==NULL) { *errPos=charpos; *errType=ERR_INTERNAL; sprintf(errText,"Out of memory."); goto cleanup_on_error; }
           }
          if ((obj==NULL)||((context->namespaces[i]->immutable)&&(obj->objType!=PPLOBJ_GLOB))) { *errPos=charpos; *errType=ERR_INTERNAL; sprintf(errText,"Cannot modify variable in immutable namespace."); goto cleanup_on_error; }
          if (obj->objType==PPLOBJ_GLOB) { if (i<2) { *errPos=charpos; *errType=ERR_INTERNAL; sprintf(errText,"Variable declared global in global namespace."); goto cleanup_on_error; } continue; }
          pplObjCpy(stk , obj , 0 , 1);
          context->stackPtr++;
          break;
         }
        break;
       }
      case 5: // Dereference value
       {
        char   *key     = (char *)(in+j);
        pplObj *in      = stk-1;
        pplObj *in_cpy  = NULL;
        pplObj *iter    = in;
        int     t       = iter->objType;
        int     imm     = iter->immutable;
        int   found     = 0;
        *lastOpAssign   = 0;
        // Make copy of object that will be self_this (in, which we therefore don't garbage below)
        in_cpy = (pplObj *)malloc(sizeof(pplObj));
        if (in_cpy == NULL) { *errPos=charpos; *errType=ERR_INTERNAL; sprintf(errText,"Out of memory."); goto cleanup_on_error; }
        memcpy(in_cpy, in, sizeof(pplObj));
        // Loop through module / class instance and its prototypes looking for named method
        for ( ; (t==PPLOBJ_MOD)||(t==PPLOBJ_USER) ; iter=iter->objPrototype , t=iter->objType )
         {
          dict   *d   = (dict *)(iter->auxil);
          pplObj *obj = (pplObj *)ppl_dictLookup(d , key);
          imm = imm || iter->immutable;
          if ((obj==NULL) || (obj->objType==PPLOBJ_ZOM) || (obj->objType==PPLOBJ_GLOB)) { imm=1; continue; } // Named methods of prototypes are immutable
          pplObjCpy(in , obj , 0 , 1);
          in->immutable = in->immutable || imm;
          found = 1;
          break;
         }
        if (!found) // If module or class instance doesn't have named method, see if type has named method
         {
          dict   *d   = pplObjMethods[in->objType];
          pplObj *obj = (pplObj *)ppl_dictLookup(d , key);
          if (obj==NULL) { *errPos=charpos; *errType=ERR_NAMESPACE; sprintf(errText,"No such method '%s'.",key); goto cleanup_on_error; }
          pplObjCpy(in , obj , 0 , 1);
          in->immutable = 1;
         }
        in->self_this = in_cpy;
        break;
       }
      case 6: // Dereference value (pointer)
       {
        char *key = (char *)(in+j);
        int   t   = (stk-1)->objType;
        dict *d   = (dict *)((stk-1)->auxil);
        *lastOpAssign=0;
        if ((stk-1)->immutable) { *errPos=charpos; *errType=ERR_INTERNAL; sprintf(errText,"Cannot modify variable in immutable namespace."); goto cleanup_on_error; }
        if ((t==PPLOBJ_MOD)||(t==PPLOBJ_USER))
         {
          pplObj *obj = (pplObj *)ppl_dictLookup(d , key);
          if (obj==NULL)
           {
            pplObjZom(stk , 0); // Create a temporary zombie for now
            ppl_dictAppendCpy(d , key , stk , sizeof(pplObj));
            obj = (pplObj *)ppl_dictLookup(d , key);
            if (obj==NULL) { *errPos=charpos; *errType=ERR_INTERNAL; sprintf(errText,"Out of memory."); goto cleanup_on_error; }
           }
          ppl_garbageObject(stk-1);
          pplObjCpy(stk-1 , obj , 0 , 1);
          break;
         }
        else
         {
          *errPos=charpos; *errType=ERR_TYPE; sprintf(errText,"Cannot assign methods or variables to this object."); goto cleanup_on_error;
         }
        break;
       }
      case 7: // slice
      case 16: // slice pointer
       {
        int       status = 0;
        const int getPtr = (o==16);
        const int mode   = (int)*(unsigned char *)(in+j);
        const int range  = (mode & 1) == 0;
        const int maxset = (mode & 2) != 0;
        const int minset = (mode & 4) != 0;
        int       Nrange = 1 , i , min=0 , max=0;
        *lastOpAssign=0;
        if (range)
         {
          Nrange = minset + maxset;
          for (i=0; i<Nrange; i++)
           {
            int *out = ((i==0)&&maxset) ? &max : &min;
            if ((stk-i)->objType!=PPLOBJ_NUM) { *errPos=charpos; *errType=ERR_TYPE; sprintf(errText,"Range limits when slicing must be numerical values; supplied limit has type <%s>.",pplObjTypeNames[(stk-1)->objType]); goto cleanup_on_error; }
            if (!(stk-i)->dimensionless) { *errPos=charpos; *errType=ERR_NUMERIC; sprintf(errText,"Range limits when slicing must be dimensionless numbers; supplied limit has units of <%s>.", ppl_printUnit(context, stk-i, NULL, NULL, 0, 1, 0) ); goto cleanup_on_error; }
            if ((stk-i)->flagComplex) { *errPos=charpos; *errType=ERR_NUMERIC; sprintf(errText,"Range limits when slicing must be real numbers; supplied limit is complex."); goto cleanup_on_error; }
            if ( (!gsl_finite((stk-i)->real)) || ((stk-i)->real<INT_MIN) || ((stk-i)->real>INT_MAX) ) { *errPos=charpos; *errType=ERR_NUMERIC; sprintf(errText,"Range limits when slicing must be in the range %d to %d.", INT_MIN, INT_MAX); goto cleanup_on_error; }
            *out = (int)(stk-i)->real;
           }
         }
        if (!range) ppl_sliceItem (context, getPtr, &status, errType, errText);
        else        ppl_sliceRange(context, minset, min, maxset, max, &status, errType, errText);
        if (status) { *errPos = charpos; goto cleanup_on_error; }
       }
      case 8: // Make dict
       {
        int k;
        int len = *(int *)(in+j);
        dict *d = ppl_dictInit(HASHSIZE_LARGE,1);
        *lastOpAssign=0;
        if (d==NULL) { *errPos=charpos; *errType=ERR_INTERNAL; sprintf(errText,"Out of memory."); goto cleanup_on_error; }
        if (context->stackPtr<2*len) { *errPos=charpos; *errType=ERR_INTERNAL; sprintf(errText,"Attempt to make dictionary with too few items on the stack."); goto cleanup_on_error; }
        for (k=0; k<len; k++) if ((stk-2*(len-k))->objType!=PPLOBJ_STR) { *errPos=charpos; *errType=ERR_TYPE; sprintf(errText,"Dictionary keys must be strings; supplied key has type <%s>.",pplObjTypeNames[(stk-2*(len-k))->objType]); goto cleanup_on_error; }
        for (k=0; k<len; k++)
         {
          (stk-2*(len-k)+1)->amMalloced = 1;
          ppl_dictAppendCpy( d , (char *)((stk-2*(len-k))->auxil) , (void *)(stk-2*(len-k)+1) , sizeof(pplObj) );
         }
        for (k=0; k<len; k++) free((stk-2*(len-k))->auxil); // Free key strings
        context->stackPtr -= 2*len; // Don't garbage collect, as objects have gone into dictionary
        pplObjDict(&context->stack[context->stackPtr] , 0 , 1 , d);
        context->stackPtr++;
        break;
       }
      case 9: // Make list
       {
        int k;
        int len = *(int *)(in+j);
        list *l = ppl_listInit(1);
        *lastOpAssign=0;
        if (l==NULL) { *errPos=charpos; *errType=ERR_INTERNAL; sprintf(errText,"Out of memory."); goto cleanup_on_error; }
        if (context->stackPtr<len) { *errPos=charpos; *errType=ERR_INTERNAL; sprintf(errText,"Attempt to make list with too few items on the stack."); goto cleanup_on_error; }
        for (k=0; k<len; k++)
         {
          (stk-len+k)->amMalloced = 1;
          ppl_listAppendCpy( l , (void *)(stk-len+k) , sizeof(pplObj) );
         }
        context->stackPtr -= len; // Don't garbage collect, as objects have gone into list
        pplObjList(&context->stack[context->stackPtr] , 0 , 1 , l);
        context->stackPtr++;
        break;
       }
      case 10: // Execute function call
       {
        int nArgs = *(int *)(in+j) , stat=0;
        *lastOpAssign=0;
        ppl_fnCall(context, nArgs, charpos, dollarAllowed, IterDepth, &stat, errPos, errType, errText);
        if (stat) goto cleanup_on_error;
        break;
       }
      case 11: // Operator
       {
        int t = (int)*(unsigned char *)(in+j);
        *lastOpAssign=0;
        pplObjNum(stk, 0, 0, 0);
        switch (t)
         {
          case 0x25: // -
           {
            CAST_TO_NUM(stk-1);
            (stk-1)->real *= -1;
            if ((stk-1)->flagComplex) (stk-1)->imag *=-1;
            break;
           }
          case 0x26: // +
           {
            CAST_TO_NUM(stk-1);
            break;
           }
          case 0xA7: // ~
           {
            CAST_TO_INT(stk-1,"~");
            (stk-1)->real = ~(int)((stk-1)->real);
            break;
           }
          case 0xA8: // !
           {
            CAST_TO_BOOL(stk-1);
            (stk-1)->real = !(stk-1)->real;
            break;
           }
          case 0xC9: // **
           {
            int status=0;
            CAST_TO_NUM(stk-1); CAST_TO_NUM(stk-2);
            ppl_uaPow(context, stk-2, stk-1, stk, &status, errType, errText);
            if (status) { *errPos=charpos; goto cleanup_on_error; }
            memcpy(stk-2, stk, sizeof(pplObj));
            context->stackPtr--;
            break;
           }
          case 0x4A: // *
           {
            int status=0;
            ppl_opMul(context, stk-2, stk-1, stk, 1, &status, errType, errText);
            if (status) { *errPos=charpos; goto cleanup_on_error; }
            ppl_garbageObject(stk-2);
            ppl_garbageObject(stk-1);
            memcpy(stk-2, stk, sizeof(pplObj));
            context->stackPtr--;
            break;
           }
          case 0x4B: // div
           {
            int status=0;
            ppl_opDiv(context, stk-2, stk-1, stk, 1, &status, errType, errText);
            if (status) { *errPos=charpos; goto cleanup_on_error; }
            ppl_garbageObject(stk-2);
            ppl_garbageObject(stk-1);
            memcpy(stk-2, stk, sizeof(pplObj));
            context->stackPtr--;
            break;
           }
          case 0x4C: // mod
           {
            int status=0;
            CAST_TO_NUM(stk-1); CAST_TO_NUM(stk-2);
            ppl_uaMod(context, stk-2, stk-1, stk, &status, errType, errText);
            if (status) { *errPos=charpos; goto cleanup_on_error; }
            memcpy(stk-2, stk, sizeof(pplObj));
            context->stackPtr--;
            break;
           }
          case 0x4D: // add
           {
            int status=0;
            ppl_opAdd(context, stk-2, stk-1, stk, 1, &status, errType, errText);
            if (status) { *errPos=charpos; goto cleanup_on_error; }
            ppl_garbageObject(stk-2);
            ppl_garbageObject(stk-1);
            memcpy(stk-2, stk, sizeof(pplObj));
            context->stackPtr--;
            break;
           }
          case 0x4E: // sub
           {
            int status=0;
            ppl_opSub(context, stk-2, stk-1, stk, 1, &status, errType, errText);
            if (status) { *errPos=charpos; goto cleanup_on_error; }
            ppl_garbageObject(stk-2);
            ppl_garbageObject(stk-1);
            memcpy(stk-2, stk, sizeof(pplObj));
            context->stackPtr--;
            break;
           }
          case 0x4F: // <<
           {
            CAST_TO_INT(stk-1,"<<"); CAST_TO_INT(stk-2,"<<");
            (stk-2)->real = ((int)(stk-2)->real) << ((int)(stk-1)->real);
            context->stackPtr--;
            break;
           }
          case 0x50: // >>
           {
            CAST_TO_INT(stk-1,">>"); CAST_TO_INT(stk-2,">>");
            (stk-2)->real = ((int)(stk-2)->real) >> ((int)(stk-1)->real);
            context->stackPtr--;
            break;
           }
          case 0x57: // &
           {
            CAST_TO_INT(stk-1,"&"); CAST_TO_INT(stk-2,"&");
            (stk-2)->real = ((int)(stk-2)->real) & ((int)(stk-1)->real);
            context->stackPtr--;
            break;
           }
          case 0x58: // ^
           {
            CAST_TO_INT(stk-1,"^"); CAST_TO_INT(stk-2,"^");
            (stk-2)->real = ((int)(stk-2)->real) ^ ((int)(stk-1)->real);
            context->stackPtr--;
            break;
           }
          case 0x59: // |
           {
            CAST_TO_INT(stk-1,"|"); CAST_TO_INT(stk-2,"|");
            (stk-2)->real = ((int)(stk-2)->real) | ((int)(stk-1)->real);
            context->stackPtr--;
            break;
           }
          case 0x5C: // swap-pop
           {
            ppl_garbageObject(stk-2);
            memcpy(stk-2, stk-1, sizeof(pplObj));
            context->stackPtr--;
            break;
           }
          case 0x51: // <
          case 0x52: // <=
          case 0x53: // >=
          case 0x54: // >
          case 0x55: // ==
          case 0x56: // !=
           {
            int stat=0 , cmp = pplObjCmp(context, stk-2, stk-1, &stat, errType, errText);
            if (stat) { *errPos=charpos; goto cleanup_on_error; }
            if      (t==0x51) stat = (cmp == -1);
            else if (t==0x52) stat = (cmp == -1) || (cmp==0);
            else if (t==0x53) stat = (cmp ==  1) || (cmp==0);
            else if (t==0x54) stat = (cmp ==  1);
            else if (t==0x55) stat = (cmp ==  0);
            else              stat = (cmp !=  0);
            ppl_garbageObject(stk-2);
            ppl_garbageObject(stk-1);
            pplObjBool(stk-2,0,stat);
            context->stackPtr--;
            break;
           }
          default:
           *errPos=charpos; *errType=ERR_INTERNAL; sprintf(errText,"Unknown operator with id=%d.",t); goto cleanup_on_error;
         }
        break;
       }
      case 12: // assignment operator
       {
        int     t  = (int)*(unsigned char *)(in+j);
        pplObj *o  = stk-2;
        pplObj *in = stk-1;
        pplObj *tmp= stk;
        int     t2 = in->objType;
        int status = 0;
        *lastOpAssign=1;
        if (context->stackPtr < 2) { *errPos=charpos; *errType=ERR_INTERNAL; sprintf(errText,"Too few items on stack for assignment operator."); goto cleanup_on_error; }
        if (o->self_lval == NULL)  { *errPos=charpos; *errType=ERR_TYPE; sprintf(errText,"Assignment operators can only be applied to variables."); goto cleanup_on_error; }
        if (o->immutable) { *errPos=charpos; *errType=ERR_TYPE; sprintf(errText,"Cannot assign to an immutable object."); goto cleanup_on_error; }

#define ASSIGN \
         { \
          if (o->self_dval==NULL) \
           { \
            int om = o->self_lval->amMalloced; \
            o->self_lval->amMalloced = 0; \
            ppl_garbageObject(o->self_lval); \
            pplObjCpy(o->self_lval, tmp, om, 1); \
           } \
          else /* Assign vector or matrix element */ \
           { \
            if ((t2!=PPLOBJ_NUM) || (tmp->flagComplex)) { *errPos=charpos; *errType=ERR_TYPE; sprintf(errText,"Vectors and matrices can only contain real numbers."); goto cleanup_on_error; } \
            if (!ppl_unitsDimEqual(o,tmp)) { *errPos=charpos; *errType=ERR_UNIT; sprintf(errText,"Cannot insert element with dimensions <%s> into vector with dimensions <%s>.", ppl_printUnit(context,tmp,NULL,NULL,0,1,0), ppl_printUnit(context,o,NULL,NULL,1,1,0)); goto cleanup_on_error; } \
            *o->self_dval = tmp->real; \
           } \
          ppl_garbageObject(o); \
          memcpy(o , tmp , sizeof(pplObj)); /* swap-pop */ \
          context->stackPtr--; \
        }

        if (t==0x40) // =
         {
          pplObj *tmp = in;
          ASSIGN;
         }
        else if (t==0x41) // +=
         {
          ppl_opAdd(context, o, in, tmp, 0, &status, errType, errText);
          if (status) { *errPos=charpos; goto cleanup_on_error; }
          ppl_garbageObject(in);
          ASSIGN;
         }
        else if (t==0x42) // -=
         {
          ppl_opSub(context, o, in, tmp, 0, &status, errType, errText);
          if (status) { *errPos=charpos; goto cleanup_on_error; }
          ppl_garbageObject(in);
          ASSIGN;
         }
        else if (t==0x43) // *=
         {
          ppl_opMul(context, o, in, tmp, 0, &status, errType, errText);
          if (status) { *errPos=charpos; goto cleanup_on_error; }
          ppl_garbageObject(in);
          ASSIGN;
         }
        else if (t==0x44) // /=
         {
          ppl_opDiv(context, o, in, tmp, 0, &status, errType, errText);
          if (status) { *errPos=charpos; goto cleanup_on_error; }
          ppl_garbageObject(in);
          ASSIGN;
         }
        else
         {
          if (o->objType != PPLOBJ_NUM) { *errPos=charpos; *errType=ERR_TYPE; sprintf(errText,"The fused operator-assignment operators can only be applied to numeric variables."); goto cleanup_on_error; }
          CAST_TO_NUM(in);
          pplObjNum(tmp, 0, 0, 0);
          switch (t)
           {
            case 0x45: // %=
              ppl_uaMod(context, o, in, tmp, &status, errType, errText);
              break;
            case 0x46: // &=
              CAST_TO_INT(in,"&="); CAST_TO_INT(o,"&=");
              tmp->real = ((int)o->real) & ((int)in->real);
              break;
            case 0x47: // ^=
              CAST_TO_INT(in,"^="); CAST_TO_INT(o,"^=");
              tmp->real = ((int)o->real) ^ ((int)in->real);
              break;
            case 0x48: // |=
              CAST_TO_INT(in,"|="); CAST_TO_INT(o,"|=");
              tmp->real = ((int)o->real) | ((int)in->real);
              break;
            case 0x49: // <<=
              CAST_TO_INT(in,"<<="); CAST_TO_INT(o,"<<=");
              tmp->real = ((int)o->real) << ((int)in->real);
              break;
            case 0x4A: // >>=
              CAST_TO_INT(in,">>="); CAST_TO_INT(o,">>=");
              tmp->real = ((int)o->real) >> ((int)in->real);
              break;
            case 0x4B: // **=
              ppl_uaPow(context, o, in, tmp, &status, errType, errText);
              break;
            default:
             *errPos=charpos; *errType=ERR_INTERNAL; sprintf(errText,"Unknown fused operator-assignment operator with id=%d.",t); goto cleanup_on_error;
           }
          if (status) { *errPos=charpos; goto cleanup_on_error; }
          ppl_garbageObject(in);
          ASSIGN;
          break;
         }
        break;
       }
      case 13: // increment and decrement operators
       {
        int     t = (int)*(unsigned char *)(in+j);
        pplObj *o = stk-1;
        *lastOpAssign=1;
        if (context->stackPtr < 1)    { *errPos=charpos; *errType=ERR_INTERNAL; sprintf(errText,"Too few items on stack for -- or ++ operator."); goto cleanup_on_error; }
        if (o->self_lval == NULL)     { *errPos=charpos; *errType=ERR_TYPE;     sprintf(errText,"The -- and ++ operators can only be applied to variables."); goto cleanup_on_error; }
        if (o->objType != PPLOBJ_NUM) { *errPos=charpos; *errType=ERR_TYPE;     sprintf(errText,"The -- and ++ operators can only be applied to numeric variables."); goto cleanup_on_error; }
        if (o->immutable) { *errPos=charpos; *errType=ERR_TYPE; sprintf(errText,"Cannot modify an immutable object."); goto cleanup_on_error; }
        switch (t)
         {
          case 0x21: // -- (post-eval)
          case 0x23: // -- (pre-eval)
            if (o->self_dval != NULL) (*o->self_dval)--;
            else                        o->self_lval->real--;
            if (t==0x23) o->real = (o->self_dval != NULL) ? *o->self_dval : o->self_lval->real;
            break;
          case 0x22: // ++ (post-eval)
          case 0x24: // ++ (pre-eval)
            if (o->self_dval != NULL) (*o->self_dval)++;
            else                        o->self_lval->real++;
            if (t==0x24) o->real = (o->self_dval != NULL) ? *o->self_dval : o->self_lval->real;
            break;
          default:
           *errPos=charpos; *errType=ERR_INTERNAL; sprintf(errText,"Unknown increment/decrement operator with id=%d.",t); goto cleanup_on_error;
         }            
        break;
       }
      case 14: // string substitution
       {
        int status = 0;
        int Nsubs  = *(int *)(in+j);
        int i;
        if (context->stackPtr < Nsubs+1) { *errPos=charpos; *errType=ERR_INTERNAL; sprintf(errText,"Too few items on stack for string substitution operator."); goto cleanup_on_error; }
        if ((stk-Nsubs-1)->objType != PPLOBJ_STR) { *errPos=charpos; *errType=ERR_TYPE; sprintf(errText,"Attempt to apply string substitution operator to a non-string."); goto cleanup_on_error; }
        expEval_stringSubs(context, Nsubs, &status, errType, errText);
        if (status) {  *errPos=charpos; goto cleanup_on_error; }
        context->stackPtr--;
        for (i=0; i<Nsubs+1; i++) { context->stackPtr--; ppl_garbageObject(&context->stack[context->stackPtr]); }
        memcpy(&context->stack[context->stackPtr] , stk , sizeof(pplObj));
        context->stackPtr++;
        break;
       }
      case 17: // branch if false
       {
        int branch;
        *lastOpAssign=0;
        CAST_TO_BOOL(stk-1);
        branch = (stk-1)->real == 0;
        if (branch) { len = *(int *)(in+j+1)-pos; if (*(unsigned char *)(in+j)) context->stackPtr--; }
        else        { context->stackPtr--; }
        break;
       }
      case 18: // branch if true
       {
        int branch;
        *lastOpAssign=0;
        CAST_TO_BOOL(stk-1);
        branch = (stk-1)->real != 0;
        if (branch) { len = *(int *)(in+j+1)-pos; if (*(unsigned char *)(in+j)) context->stackPtr--; }
        else        { context->stackPtr--; }
        break;
       }
      case 19: // goto
       {
        *lastOpAssign=0;
        len = *(int *)(in+j)-pos;
        break;
       }
      case 20: // make boolean
       {
        *lastOpAssign=0;
        CAST_TO_BOOL(stk-1);
        break;
       }
      default:
       *errPos=charpos; *errType=ERR_INTERNAL; sprintf(errText,"Illegal bytecode opcode passed to expEval."); goto cleanup_on_error;
     }
    if (o==0) break;
    j = pos+len;
   }
  if (context->stackPtr <= 0) { *errPos=0; *errType=ERR_INTERNAL; sprintf(errText,"Unexpected empty stack at end of evaluation."); goto cleanup_on_error; }
  if (context->stackPtr != initialStackPtr+1) ppl_warning(&context->errcontext, ERR_INTERNAL, "Unexpected junk on stack in expEval.");
  return &context->stack[context->stackPtr-1];

  cast_fail:         *errPos = charpos;
  cleanup_on_error:
    for ( ; context->stackPtr>initialStackPtr ; ) { context->stackPtr--; ppl_garbageObject(&context->stack[context->stackPtr]); }
    return NULL;
 }

