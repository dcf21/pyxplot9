// unitsDisp.c
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

// NB: This source file is not included in the PyXPlot Makefile, but is
// included as a part of ppl_userspace.c. This allows some functions to be
// compiled inline for speed.

#define _PPL_UNITS_C 1

#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <math.h>
#include <string.h>

#include <gsl/gsl_complex.h>
#include <gsl/gsl_complex_math.h>
#include <gsl/gsl_math.h>

#include "ppl_error.h"
#include "ppl_settings.h"
#include "ppl_setting_types.h"
#include "ppl_units.h"

#include "ListTools/lt_memory.h"

#include "StringTools/asciidouble.h"

const char *SIprefixes_full  [] = {"yocto","zepto","atto","femto","pico","nano","micro","milli","","kilo","mega","giga","tera","peta","exa","zetta","yotta"};
const char *SIprefixes_abbrev[] = {"y","z","a","f","p","n","u","m","","k","M","G","T","P","E","Z","Y"};
const char *SIprefixes_latex [] = {"y","z","a","f","p","n","\\upmu ","m","","k","M","G","T","P","E","Z","Y"};

double TempTypeMultiplier[8]; // These are filled in by ppl_userspace_init.c
double TempTypeOffset    [8]; // They store the offsets and multiplier for each of the units of temperature

#include "ppl_units_fns.h"

// NB: The LaTeX upmu character is defined by the {upgreek} package

// Display a value with units
char *ppl_units_NumericDisplay(value *in, int N, int typeable, int NSigFigs)
 {
  static char outputA[LSTR_LENGTH], outputB[LSTR_LENGTH];
  double NumberOutReal, NumberOutImag, OoM;
  char *output, *unitstr;
  int i=0;
  if (N==0) output = outputA;
  else      output = outputB;

  if (NSigFigs <= 0) NSigFigs = settings_term_current.SignificantFigures; // If number of significant figures not specified, use user-selected number

  if ((settings_term_current.ComplexNumbers == SW_ONOFF_OFF) && (in->FlagComplex!=0)) return NumericDisplay(GSL_NAN, N, NSigFigs, (typeable==SW_DISPLAY_L));

  if (typeable==0) typeable = settings_term_current.NumDisplay;
  unitstr = ppl_units_GetUnitStr(in, &NumberOutReal, &NumberOutImag, N, 1, typeable);

  if (((settings_term_current.ComplexNumbers == SW_ONOFF_OFF) && (in->FlagComplex!=0)) || (!gsl_finite(NumberOutReal)) || (!gsl_finite(NumberOutImag)))
   {
    if (typeable == SW_DISPLAY_L) output[i++] = '$';
    strcpy(output+i, NumericDisplay(GSL_NAN, N, NSigFigs, (typeable==SW_DISPLAY_L)));
    i+=strlen(output+i);
   }
  else
   {
    OoM = hypot(NumberOutReal , NumberOutImag) * pow(10 , -NSigFigs);

    if (typeable == SW_DISPLAY_L) output[i++] = '$';
    if ((fabs(NumberOutReal) >= OoM) && (fabs(NumberOutImag) > OoM)) output[i++] = '('; // open brackets on complex number
    if (fabs(NumberOutReal) >= OoM) { strcpy(output+i, NumericDisplay(NumberOutReal, N, NSigFigs, (typeable==SW_DISPLAY_L))); i+=strlen(output+i); }
    if ((fabs(NumberOutReal) >= OoM) && (fabs(NumberOutImag) > OoM) && (NumberOutImag > 0)) output[i++] = '+';
    if (fabs(NumberOutImag) > OoM)
     {
      if (fabs(NumberOutImag-1.0)>=OoM) // If this is false, imaginary part is 1, so print +i, not +1i
       {
        if (fabs(NumberOutImag+1.0)>=OoM)
         {
          strcpy(output+i, NumericDisplay(NumberOutImag, N, NSigFigs, (typeable==SW_DISPLAY_L)));
          i+=strlen(output+i);
         }
        else
         { output[i++]='-'; } // Just print -i, not -1i
       }
      if      (typeable != SW_DISPLAY_T)                                       { output[i++] = 'i'; }
      else if ((fabs(NumberOutImag-1.0)>=OoM)&&(fabs(NumberOutImag+1.0)>=OoM)) { strcpy(output+i, "*sqrt(-1)"); i+=strlen(output+i); }
      else                                                                     { strcpy(output+i,  "sqrt(-1)"); i+=strlen(output+i); } // We've not printed 1 or -1, so nothing to multiply with
     }
    if ((fabs(NumberOutReal) >= OoM) && (fabs(NumberOutImag) > OoM)) output[i++] = ')'; // close brackets on complex number
   }

  if (unitstr[0]!='\0')
   {
    if      (typeable == SW_DISPLAY_N) output[i++] = ' ';
    else if (typeable == SW_DISPLAY_L) { output[i++] = '\\'; output[i++] = ','; }
    sprintf(output+i, "%s", unitstr);
    i+=strlen(output+i); // Add unit string as required
   }

  if (typeable == SW_DISPLAY_L) output[i++] = '$';
  output[i++] = '\0'; // null terminate string
  return output;
 }

// -------------------------------------------------------------------
// Routines for printing units
// -------------------------------------------------------------------

#define UNIT_INSCHEME(X)  (  ((X).si       && (settings_term_current.UnitScheme == SW_UNITSCH_SI  )) \
                          || ((X).cgs      && (settings_term_current.UnitScheme == SW_UNITSCH_CGS )) \
                          || ((X).imperial && (settings_term_current.UnitScheme == SW_UNITSCH_IMP )) \
                          || ((X).us       && (settings_term_current.UnitScheme == SW_UNITSCH_US  )) \
                          || ((X).planck   && (settings_term_current.UnitScheme == SW_UNITSCH_PLK )) \
                          || ((X).ancient  && (settings_term_current.UnitScheme == SW_UNITSCH_ANC ))  )

void ppl_units_FindOptimalNextUnit(value *in, unsigned char first, unit **best, double *pow)
 {
  int i,j,k,score,found=0,BestScore=0;
  double power;

  for (i=0; i<ppl_unit_pos; i++)
   {
    if (!TempTypeMatch(in->TempType, ppl_unit_database[i].TempType)) continue; // Don't convert between different temperature units
    for (j=0; j<UNITS_MAX_BASEUNITS; j++)
     {
      if ( (ppl_unit_database[i].exponent[j] == 0) || (in->exponent[j]==0) ) continue;
      power = in->exponent[j] / ppl_unit_database[i].exponent[j];
      score = 0;
      for (k=0; k<UNITS_MAX_BASEUNITS; k++) if (ppl_units_DblEqual(in->exponent[k] , power*ppl_unit_database[i].exponent[k])) score++;

      if (ppl_unit_database[i].NotToBeCompounded && ((!first) || (score<UNITS_MAX_BASEUNITS-1))) continue;

      if (found == 0) // This is first possible unit we've found, and we have nothing to compare it to.
       {
        *best     = ppl_unit_database+i;
        *pow      = power;
        BestScore = score;
        found     = 1;
       }

      // A user-preferred unit always beats a non-user-preferred unit
      if ( (score >= BestScore) && ( (ppl_unit_database[i].UserSel)) && (!((*best)->UserSel)) )
       { *best = ppl_unit_database+i; *pow = power; BestScore = score; continue; }
      if ( (score <= BestScore ) &&(!(ppl_unit_database[i].UserSel)) && ( ((*best)->UserSel)) )
       continue;

      // A unit in the current scheme always beats one not in current scheme
      if (( UNIT_INSCHEME(ppl_unit_database[i])) && (!UNIT_INSCHEME(**best)))
        { *best = ppl_unit_database+i; *pow = power; BestScore = score; continue; }
      if ((!UNIT_INSCHEME(ppl_unit_database[i])) && ( UNIT_INSCHEME(**best)))
        continue;

      // A unit which matches more dimensions wins
      if (score > BestScore)
        { *best = ppl_unit_database+i; *pow = power; BestScore = score; continue; }
      if ((score == BestScore) && (ppl_units_DblEqual(fabs(power), 1.0) && (!ppl_units_DblEqual(fabs(*pow), 1.0))) )
        { *best = ppl_unit_database+i; *pow = power; continue; }
      if ((score == BestScore) && (ppl_units_DblEqual(power, 1.0) && (!ppl_units_DblEqual(*pow, 1.0))) )
        { *best = ppl_unit_database+i; *pow = power; continue; }
      if (score < BestScore)
        continue;
     }
   }
  if (found==0) {*pow = 0; *best = NULL; return;}
  for (j=0; j<UNITS_MAX_BASEUNITS; j++)
   {
    in->exponent[j] -= (*pow) * (*best)->exponent[j];
   }
  return;
 }

void ppl_units_PrefixFix(value *in, unit **UnitList, double *UnitPow, int *UnitPref, int Nunits)
 {
  int     i,j;
  double  NewValueReal, NewValueImag, PrefixBestVal, NewMagnitude, OldMagnitude;
  int     PrefixBestPos, BestPrefix=0;

  // Apply unit multipliers to the value we're going to display
  for (i=0; i<Nunits; i++)
   {
    in->real /= pow(UnitList[i]->multiplier , UnitPow[i]);
    in->imag /= pow(UnitList[i]->multiplier , UnitPow[i]);
    UnitPref[i]=0;
   }

  // Search for alternative dimensionally-equivalent units which give a smaller value
  for (i=0; i<Nunits; i++)
   for (j=0; j<ppl_unit_pos; j++)
    if (TempTypeMatch(UnitList[i]->TempType, ppl_unit_database[j].TempType) && (ppl_units_UnitDimEqual(UnitList[i] , ppl_unit_database + j)))
     {
      OldMagnitude = hypot(in->real , in->imag);
      NewValueReal = in->real * pow(UnitList[i]->multiplier / ppl_unit_database[j].multiplier , UnitPow[i]);
      NewValueImag = in->imag * pow(UnitList[i]->multiplier / ppl_unit_database[j].multiplier , UnitPow[i]);
      NewMagnitude = hypot(NewValueReal , NewValueImag);

      // A user-preferred unit always beats a non-user-preferred unit
      if ( ( (ppl_unit_database[j].UserSel)) && (!(UnitList[i]->UserSel)) )
       { UnitList[i] = ppl_unit_database+j; in->real = NewValueReal; in->imag = NewValueImag; continue; }
      if ( (!(ppl_unit_database[j].UserSel)) && ( (UnitList[i]->UserSel)) )
       continue;

      // A unit in the current scheme always beats one which is not
      if (( UNIT_INSCHEME(ppl_unit_database[j])) && (!UNIT_INSCHEME(*(UnitList[i]))))
        { UnitList[i] = ppl_unit_database+j; in->real = NewValueReal; in->imag = NewValueImag; continue; }
      if ((!UNIT_INSCHEME(ppl_unit_database[j])) && ( UNIT_INSCHEME(*(UnitList[i]))))
        continue;

      // Otherwise, a unit with a smaller display value wins
      if ((NewMagnitude < OldMagnitude) && (NewMagnitude >= 1))
        { UnitList[i] = ppl_unit_database+j; in->real = NewValueReal; in->imag = NewValueImag; }
     }

  // Apply unit multiplier which arise from user-preferred SI prefixes, for example, millimetres
  for (i=0; i<Nunits; i++)
   if (UnitList[i]->UserSel)
    {
     in->real /= pow(10,(UnitList[i]->UserSelPrefix-8)*3 * UnitPow[i]);
     in->imag /= pow(10,(UnitList[i]->UserSelPrefix-8)*3 * UnitPow[i]);
     UnitPref[i] = UnitList[i]->UserSelPrefix-8;
    }

  // Search for an SI prefix we can use to reduce the size of this number
  if (settings_term_current.UnitDisplayPrefix == SW_ONOFF_ON)
   {
    OldMagnitude = hypot(in->real , in->imag);
    PrefixBestPos = -1;
    PrefixBestVal = OldMagnitude;
    for (i=0; i<Nunits; i++) if (ppl_units_DblEqual(UnitPow[i] , 1))
     if (UnitList[i]->UserSel == 0)
      for (j=UnitList[i]->MinPrefix; j<=UnitList[i]->MaxPrefix; j+=3)
       {
        NewMagnitude = OldMagnitude / pow(10,j);
        if ( (NewMagnitude >= 1) && ((NewMagnitude < PrefixBestVal) || (PrefixBestVal<1)) )
         { PrefixBestPos = i; BestPrefix = j; PrefixBestVal = NewMagnitude; }
       }
    if (PrefixBestPos>=0)
     {
      in->real /= pow(10,BestPrefix);
      in->imag /= pow(10,BestPrefix);
      UnitPref[PrefixBestPos] = BestPrefix/3;
     }
   }
  return;
 }

// Main entry point for printing units
char *ppl_units_GetUnitStr(const value *in, double *NumberOutReal, double *NumberOutImag, int N, int DivAllowed, int typeable)
 {
  static char   outputA[LSTR_LENGTH], outputB[LSTR_LENGTH], outputC[LSTR_LENGTH];
  char         *output,*temp;
  value         residual=*in;
  unit         *UnitList[UNITS_MAX_BASEUNITS];
  double        UnitPow [UNITS_MAX_BASEUNITS];
  int           UnitPref[UNITS_MAX_BASEUNITS];
  unsigned char UnitDisp[UNITS_MAX_BASEUNITS];
  double        ExpMax=0;
  int           pos=0, OutputPos=0;
  int           i, j=0, k, l, found, first;
  ListIterator *listiter;
  PreferredUnit *pu;

  if (typeable==0) typeable = settings_term_current.NumDisplay;

  if      (N==0) output = outputA;
  else if (N==1) output = outputB;
  else           output = outputC;

  // Check whether input value is dimensionally equal to any preferred units
  listiter = ListIterateInit(ppl_unit_PreferredUnits);
  while (listiter != NULL)
   {
    pu = (PreferredUnit *)listiter->data;
    if (ppl_units_DimEqual(&pu->value , in) && (in->TempType == pu->value.TempType)) break; // Preferred unit matches dimensions
    listiter = ListIterate(listiter, NULL);
   }

  if (listiter != NULL) // Found a preferred unit to use
   {
    for (i=0; i<pu->NUnits; i++)
     {
      UnitList[i] = ppl_unit_database + pu->UnitID[i];
      UnitPow [i] = pu->exponent[i];
      UnitPref[i] = (pu->prefix[i]>=0) ? pu->prefix[i]-8 : 0;
      UnitDisp[i] = 0;
     }
    pos = pu->NUnits;
    residual.real /= pu->value.real;
    residual.imag /= pu->value.real;
    for (i=0; i<UNITS_MAX_BASEUNITS; i++) residual.exponent[i]=0;
    residual.dimensionless = 1;
   }
  else // Not using a preferred unit
   {
    if (in->dimensionless != 0)
     {
      output[0]='\0';
      if (NumberOutReal != NULL) *NumberOutReal = in->real;
      if (NumberOutImag != NULL) *NumberOutImag = in->imag;
      return output;
     }

    // Find a list of units which multiply together to match dimensions of quantity to display
    while (1)
     {
      if (pos>=UNITS_MAX_BASEUNITS) { ppl_error(ERR_INTERNAL, -1, -1, "Overflow whilst trying to display a unit."); break; }
      ppl_units_FindOptimalNextUnit(&residual, pos==0, UnitList + pos, UnitPow + pos);
      UnitDisp[pos] = 0;
      if (ppl_units_DblEqual(UnitPow[pos],0)!=0) break;
      pos++;
     }

    // Go through list of units and fix prefixes / unit choice to minimise displayed number
    ppl_units_PrefixFix(&residual, UnitList, UnitPow, UnitPref, pos);
   }

  // Display units one by one
  first = 1;
  while (1)
   {
    found = 0;
    for (i=0; i<pos; i++) if ((UnitDisp[i]==0) && ((found==0) || (UnitPow[i]>ExpMax))) { ExpMax=UnitPow[i]; found=1; j=i; }
    if (found==0) break;
    if ((typeable==SW_DISPLAY_T) && first) { strcpy(output+OutputPos, "*unit("); OutputPos+=strlen(output+OutputPos); }
    if (!first) // Print * or /
     {
      if ((!DivAllowed) || (UnitPow[j] > 0) || (ppl_units_DblEqual(UnitPow[j],0)))
          { if (typeable==SW_DISPLAY_L) { output[OutputPos++] = '\\'; output[OutputPos++] = ','; }  else output[OutputPos++] = '*'; }
      else
          { output[OutputPos++] = '/'; }
     }
    if (typeable==SW_DISPLAY_L) { strcpy(output+OutputPos, "\\mathrm{"); OutputPos+=strlen(output+OutputPos); }
    if (UnitPref[j] != 0) // Print SI prefix
     {
      if (settings_term_current.UnitDisplayAbbrev == SW_ONOFF_ON)
       {
        if (typeable!=SW_DISPLAY_L) strcpy(output+OutputPos, SIprefixes_abbrev[UnitPref[j]+8]);
        else                        strcpy(output+OutputPos, SIprefixes_latex [UnitPref[j]+8]);
       }
      else
       { strcpy(output+OutputPos, SIprefixes_full  [UnitPref[j]+8]); }
      OutputPos+=strlen(output+OutputPos);
     }
    if (settings_term_current.UnitDisplayAbbrev == SW_ONOFF_ON)
     {
      if (typeable!=SW_DISPLAY_L)
       {
        if (UnitPow[j] >= 0) strcpy(output+OutputPos, UnitList[j]->nameAp); // Use abbreviated name for unit
        else                 strcpy(output+OutputPos, UnitList[j]->nameAs);
       }
      else
       {
        if (UnitPow[j] >= 0) strcpy(output+OutputPos, UnitList[j]->nameLp); // Use abbreviated LaTeX name for unit
        else                 strcpy(output+OutputPos, UnitList[j]->nameLs);
       }
     } else {
      if (UnitPow[j] >= 0) temp = UnitList[j]->nameFp; // Use full name for unit...
      else                 temp = UnitList[j]->nameFs;

      if (typeable!=SW_DISPLAY_L)
       { strcpy(output+OutputPos, temp); } // ... either as it comes
      else
       {
        for (k=l=0;temp[k]!='\0';k++)
         {
          if (temp[k]=='_') output[OutputPos+(l++)]='\\'; // ... or with escaped underscores for LaTeX
          output[OutputPos+(l++)]=temp[k];
         }
        output[OutputPos+(l++)]='\0';
       }
     }
    OutputPos+=strlen(output+OutputPos);
    if (typeable==SW_DISPLAY_L) { output[OutputPos++]='}'; }
    if ( ((first||(!DivAllowed)) && (!ppl_units_DblEqual(UnitPow[j],1))) || (((!first)||DivAllowed) && (!ppl_units_DblEqual(fabs(UnitPow[j]),1))) ) // Print power
     {
      if (typeable==SW_DISPLAY_L) { output[OutputPos++]='^'; output[OutputPos++]='{'; }
      else                        { output[OutputPos++]='*'; output[OutputPos++]='*'; }
      if ((first)||(!DivAllowed)) sprintf(output+OutputPos, "%s", NumericDisplay(     UnitPow[j] , N, settings_term_current.SignificantFigures, (typeable==SW_DISPLAY_L)));
      else                        sprintf(output+OutputPos, "%s", NumericDisplay(fabs(UnitPow[j]), N, settings_term_current.SignificantFigures, (typeable==SW_DISPLAY_L)));
      OutputPos+=strlen(output+OutputPos);
      if (typeable==SW_DISPLAY_L) { output[OutputPos++]='}'; }
     }
    UnitDisp[j] = 1;
    first = 0;
   }

  // Clean up and return
  if (typeable==SW_DISPLAY_T) output[OutputPos++] = ')';
  output[OutputPos] = '\0';
  if (NumberOutReal != NULL) *NumberOutReal = residual.real;
  if (NumberOutImag != NULL) *NumberOutImag = residual.imag;
  return output;
 }

// ------------------------------------------------
// Function to evaluate strings of the form "m/s"
// ------------------------------------------------

int __inline__ UnitNameCmp(const char *in, const char *unit, const unsigned char CaseSensitive)
 {
  int k;
  if (unit==NULL) return 0;
  if (CaseSensitive) for (k=0; ((unit[k]!='\0') && (        unit[k] ==        in[k] )); k++);
  else               for (k=0; ((unit[k]!='\0') && (toupper(unit[k])==toupper(in[k]))); k++);
  if ((unit[k]=='\0') && (!(isalnum(in[k]) || (in[k]=='_')))) return k;
  return 0;
 }

void ppl_units_StringEvaluate(char *in, value *out, int *end, int *errpos, char *errtext)
 {
  int i=0,j=0,k,l,m,p;
  double power=1.0, powerneg=1.0, multiplier;
  ppl_units_zero(out);

  while ((in[i]<=' ')&&(in[i]!='\0')) i++;
  out->real = GetFloat(in+i , &j); // Unit strings can have numbers out the front
  if (j<0) j=0;
  i+=j;
  if (j==0)
   { out->real = 1.0; }
  else
   {
    while ((in[i]<=' ')&&(in[i]!='\0')) i++;
    if      (in[i]=='*')   i++;
    else if (in[i]=='/') { i++; powerneg=-1.0; }
   }

  while (powerneg!=0.0)
   {
    p=0;
    while ((in[i]<=' ')&&(in[i]!='\0')) i++;
    for (j=0; j<ppl_unit_pos; j++)
     {
      multiplier = 1.0;
      if      ((k = UnitNameCmp(in+i, ppl_unit_database[j].nameAp,1))!=0) p=1;
      else if ((k = UnitNameCmp(in+i, ppl_unit_database[j].nameAs,1))!=0) p=1;
      else if ((k = UnitNameCmp(in+i, ppl_unit_database[j].nameFp,0))!=0) p=1;
      else if ((k = UnitNameCmp(in+i, ppl_unit_database[j].nameFs,0))!=0) p=1;
      else if ((k = UnitNameCmp(in+i, ppl_unit_database[j].nameFp,0))!=0) p=1;
      else if ((k = UnitNameCmp(in+i, ppl_unit_database[j].alt1  ,0))!=0) p=1;
      else if ((k = UnitNameCmp(in+i, ppl_unit_database[j].alt2  ,0))!=0) p=1;
      else if ((k = UnitNameCmp(in+i, ppl_unit_database[j].alt3  ,0))!=0) p=1;
      else if ((k = UnitNameCmp(in+i, ppl_unit_database[j].alt4  ,0))!=0) p=1;
      else
       {
        for (l=ppl_unit_database[j].MinPrefix/3+8; l<=ppl_unit_database[j].MaxPrefix/3+8; l++)
         {
          if (l==8) continue;
          for (k=0; ((SIprefixes_full[l][k]!='\0') && (toupper(SIprefixes_full[l][k])==toupper(in[i+k]))); k++);
          if (SIprefixes_full[l][k]=='\0')
           {
            if      ((m = UnitNameCmp(in+i+k, ppl_unit_database[j].nameFp,0))!=0) { p=1; k+=m; multiplier=pow(10,(l-8)*3); break; }
            else if ((m = UnitNameCmp(in+i+k, ppl_unit_database[j].nameFs,0))!=0) { p=1; k+=m; multiplier=pow(10,(l-8)*3); break; }
            else if ((m = UnitNameCmp(in+i+k, ppl_unit_database[j].alt1  ,0))!=0) { p=1; k+=m; multiplier=pow(10,(l-8)*3); break; }
            else if ((m = UnitNameCmp(in+i+k, ppl_unit_database[j].alt2  ,0))!=0) { p=1; k+=m; multiplier=pow(10,(l-8)*3); break; }
            else if ((m = UnitNameCmp(in+i+k, ppl_unit_database[j].alt3  ,0))!=0) { p=1; k+=m; multiplier=pow(10,(l-8)*3); break; }
            else if ((m = UnitNameCmp(in+i+k, ppl_unit_database[j].alt4  ,0))!=0) { p=1; k+=m; multiplier=pow(10,(l-8)*3); break; }
           }
          for (k=0; ((SIprefixes_abbrev[l][k]!='\0') && (SIprefixes_abbrev[l][k]==in[i+k])); k++);
          if (SIprefixes_abbrev[l][k]=='\0')
           {
            if      ((m = UnitNameCmp(in+i+k, ppl_unit_database[j].nameAp,1))!=0) { p=1; k+=m; multiplier=pow(10,(l-8)*3); break; }
            else if ((m = UnitNameCmp(in+i+k, ppl_unit_database[j].nameAs,1))!=0) { p=1; k+=m; multiplier=pow(10,(l-8)*3); break; }
           }
         }
       }
      if (p==0) continue;
      i+=k;
      while ((in[i]<=' ')&&(in[i]!='\0')) i++;
      if (((in[i]=='^') && (i++,1)) || (((in[i]=='*') && (in[i+1]=='*')) && (i+=2,1)))
       {
        power = GetFloat(in+i,&k);
        if (k<=0) { *errpos=i; strcpy(errtext, "Syntax Error: Was expecting a numerical constant here."); return; }
        i+=k;
        while ((in[i]<=' ')&&(in[i]!='\0')) i++;
       }

      if (ppl_unit_database[j].TempType != 0)
       {
        if ((out->TempType >0) && (out->TempType!=ppl_unit_database[j].TempType))
         { *errpos=i; strcpy(errtext, "Unit Error: Attempt to mix Kelvin, oC and oF in the units of a single quantity. Try again with all quantities converted into Kelvin. Type 'help units temperatures' for more details."); return; }
        out->TempType = ppl_unit_database[j].TempType;
       }

      for (k=0; k<UNITS_MAX_BASEUNITS; k++) out->exponent[k] += ppl_unit_database[j].exponent[k] * power * powerneg;
      if (ppl_units_DblEqual(out->exponent[UNIT_TEMPERATURE], 0)) out->TempType = 0; // We've lost our temperature dependence
      out->real *= pow( multiplier * ppl_unit_database[j].multiplier , power*powerneg );
      power = 1.0;
      if      (in[i]=='*') { powerneg= 1.0; i++; }
      else if (in[i]=='/') { powerneg=-1.0; i++; }
      else                 { powerneg= 0.0;      }
      break;
     }
    if (p==0)
     {
      if ((in[i]==')') || (in[i]=='\0'))  { powerneg=0.0; }
      else                                { *errpos=i; strcpy(errtext, "No such unit."); return; }
     }
   }
  j=1;
  for (k=0; k<UNITS_MAX_BASEUNITS; k++) if (ppl_units_DblEqual(out->exponent[k], 0) == 0) j=0;
  out->dimensionless = j;
  if (end != NULL) *end=i;
  return;
 }

// Function for making preferred unit structures
void MakePreferredUnit(PreferredUnit **output, char *instr, int OutputContext, int *errpos, char *errtext)
 {
  int end, outpos, PrefixOut, i, j, k, l, m, p;
  double power=1.0, powerneg=1.0;
  value UnitVal;

  *output = NULL;
  *errpos = -1;
  end = strlen(instr);
  ppl_units_StringEvaluate(instr, &UnitVal, &end, errpos, errtext);
  if (*errpos>=0) return; // Error in parsing unit expression
  *output = (PreferredUnit *)lt_malloc(sizeof(PreferredUnit));
  if (*output==NULL) { errpos=0; sprintf(errtext, "Out of memory."); }
  (*output)->value = UnitVal;
  (*output)->modified = 1;

  // Read list of units from preferred unit string
  outpos=i=0;
  while ((powerneg!=0.0) && (outpos<UNITS_MAX_BASEUNITS))
   {
    PrefixOut=-1;
    p=0;
    while ((instr[i]<=' ')&&(instr[i]!='\0')) i++;
    for (j=0; j<ppl_unit_pos; j++)
     {
      if      ((k = UnitNameCmp(instr+i, ppl_unit_database[j].nameAp,1))!=0) p=1;
      else if ((k = UnitNameCmp(instr+i, ppl_unit_database[j].nameAs,1))!=0) p=1;
      else if ((k = UnitNameCmp(instr+i, ppl_unit_database[j].nameFp,0))!=0) p=1;
      else if ((k = UnitNameCmp(instr+i, ppl_unit_database[j].nameFs,0))!=0) p=1;
      else if ((k = UnitNameCmp(instr+i, ppl_unit_database[j].nameFp,0))!=0) p=1;
      else if ((k = UnitNameCmp(instr+i, ppl_unit_database[j].alt1  ,0))!=0) p=1;
      else if ((k = UnitNameCmp(instr+i, ppl_unit_database[j].alt2  ,0))!=0) p=1;
      else if ((k = UnitNameCmp(instr+i, ppl_unit_database[j].alt3  ,0))!=0) p=1;
      else if ((k = UnitNameCmp(instr+i, ppl_unit_database[j].alt4  ,0))!=0) p=1;
      else
       {
        for (l=ppl_unit_database[j].MinPrefix/3+8; l<=ppl_unit_database[j].MaxPrefix/3+8; l++)
         {
          if (l==8) continue;
          for (k=0; ((SIprefixes_full[l][k]!='\0') && (toupper(SIprefixes_full[l][k])==toupper(instr[i+k]))); k++);
          if (SIprefixes_full[l][k]=='\0')
           {
            if      ((m = UnitNameCmp(instr+i+k, ppl_unit_database[j].nameFp,0))!=0) { p=1; k+=m; PrefixOut=l; break; }
            else if ((m = UnitNameCmp(instr+i+k, ppl_unit_database[j].nameFs,0))!=0) { p=1; k+=m; PrefixOut=l; break; }
            else if ((m = UnitNameCmp(instr+i+k, ppl_unit_database[j].alt1  ,0))!=0) { p=1; k+=m; PrefixOut=l; break; }
            else if ((m = UnitNameCmp(instr+i+k, ppl_unit_database[j].alt2  ,0))!=0) { p=1; k+=m; PrefixOut=l; break; }
            else if ((m = UnitNameCmp(instr+i+k, ppl_unit_database[j].alt3  ,0))!=0) { p=1; k+=m; PrefixOut=l; break; }
            else if ((m = UnitNameCmp(instr+i+k, ppl_unit_database[j].alt4  ,0))!=0) { p=1; k+=m; PrefixOut=l; break; }
           }
          for (k=0; ((SIprefixes_abbrev[l][k]!='\0') && (SIprefixes_abbrev[l][k]==instr[i+k])); k++);
          if (SIprefixes_abbrev[l][k]=='\0')
           {
            if      ((m = UnitNameCmp(instr+i+k, ppl_unit_database[j].nameAp,1))!=0) { p=1; k+=m; PrefixOut=l; break; }
            else if ((m = UnitNameCmp(instr+i+k, ppl_unit_database[j].nameAs,1))!=0) { p=1; k+=m; PrefixOut=l; break; }
           }
         }
       }
      if (p==0) continue;
      i+=k;
      while ((instr[i]<=' ')&&(instr[i]!='\0')) i++;
      if (((instr[i]=='^') && (i++,1)) || (((instr[i]=='*') && (instr[i+1]=='*')) && (i+=2,1)))
       {
        power = GetFloat(instr+i,&k);
        if (k<=0) { *errpos=i; strcpy(errtext, "Syntax Error: Was expecting a numerical constant here."); return; }
        i+=k;
        while ((instr[i]<=' ')&&(instr[i]!='\0')) i++;
       }
      (*output)->UnitID  [outpos] = j;
      (*output)->prefix  [outpos] = PrefixOut;
      (*output)->exponent[outpos] = power*powerneg;
      outpos++;
      power = 1.0;
      if      (instr[i]=='*') { powerneg= 1.0; i++; }
      else if (instr[i]=='/') { powerneg=-1.0; i++; }
      else                    { powerneg= 0.0;      }
      break;
     }
    if (p==0)
     {
      if ((instr[i]==')') || (instr[i]=='\0'))  { powerneg=0.0; }
      else                                      { *errpos=i; strcpy(errtext, "No such unit."); return; }
     }
   }

  (*output)->NUnits = outpos;
  return;
 }

