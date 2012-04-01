// canvasDraw.c
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

#define _CANVASDRAW_C

#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <signal.h>
#include <unistd.h>

#include <gsl/gsl_math.h>

#include "coreUtils/errorReport.h"
#include "coreUtils/list.h"
#include "coreUtils/memAlloc.h"
#include "settings/settingTypes.h"
#include "stringTools/asciidouble.h"
#include "epsMaker/canvasDraw.h"
#include "epsMaker/dvi_read.h"
#include "epsMaker/eps_comm.h"
#include "epsMaker/eps_core.h"
#include "epsMaker/eps_settings.h"
#include "userspace/context.h"
#include "canvasItems.h"
#include "children.h"

// Standard postscript prolog text for EPS and PS files
static char *EPS_PROLOG_TEXT = "\
/BeginEPSF { %def                                     % Prepare for EPS file\n\
 /b4_Inc_state save def                               % Save state for cleanup\n\
 /dict_count countdictstack def                       % Count objects on dict stack\n\
 /op_count count 1 sub def                            % Count objects on operand stack\n\
 userdict begin                                       % Push userdict on dict stack\n\
 /showpage { } def                                    % Redefine showpage, { } = null proc\n\
 0 setgray 0 setlinecap 1 setlinewidth 0 setlinejoin  % Prepare graphics state\n\
 10 setmiterlimit [ ] 0 setdash newpath\n\
 /languagelevel where                                 % If level not equal to 1 then set strokeadjust and overprint to their defaults.\n\
  {pop languagelevel\n\
   1 ne\n\
    {false setstrokeadjust false setoverprint\n\
    } if\n\
  } if\n\
} bind def\n\
\n\
/EndEPSF { %def                                       % Clean up after including an EPS file\n\
 count op_count sub {pop} repeat                      % Clean up stacks\n\
 countdictstack dict_count sub {end} repeat\n\
 b4_Inc_state restore\n\
} bind def\n\
";
static char *PS_PROLOG_TEXT = "\
";

void ppl_canvas_draw(ppl_context *c, unsigned char *unsuccessful_ops)
 {
  int i;

  // By default, we record all operations as having been successful
  for (i=0;i<MULTIPLOT_MAXINDEX; i++) unsuccessful_ops[i]=0;
  return;
 }

// Run LaTeX to turn set of strings for typesetting into eps code
#define FPRINTF_LINECOUNT( X ) \
  fprintf(output, "%s", X); \
  for (i=0; X[i]!='\0'; i++) if (X[i]=='\n') linecount++;

void canvas_CallLaTeX(EPSComm *x)
 {
  int   linecount=1, i, j, pid, LatexStatus, LatexStdIn, LatexOut;
  char  filename[FNAME_LENGTH], *str_buffer;
  FILE *output;
  listIterator *ListIter;
  CanvasTextItem *TempTextItem, *SuspectTextItem=NULL;
  struct timespec waitperiod; // A time.h timespec specifier for a wait of zero seconds
  unsigned char   FirstIter;
  fd_set          readable;
  sigset_t        sigs;

  int  ErrLineNo=0, ErrReadState=0, ErrReadPos=0, NCharsRead=0, ReadErrorState=0, TrialNumber;
  char ErrFilename[FNAME_LENGTH]="", ErrMsg[FNAME_LENGTH]="";
  char TempErrFilename[FNAME_LENGTH], TempErrLineNo[FNAME_LENGTH], TempErrMsg[FNAME_LENGTH];

  if (ppl_listLen(x->TextItems) < 1) return; // We have no text to give to latex, and so don't need to fork

  sigemptyset(&sigs);
  sigaddset(&sigs,SIGCHLD);

  const char TextHeader1[] = "\\documentclass{article}\n\\pagestyle{empty}\n\\usepackage{upgreek}\n";
  const char TextHeader2[] = "\\begin{document}\n\\begin{center}{\\bf This page has been left deliberately blank.}\\end{center}\n\\newpage\n";
  const char ItemHeader [] = "\\vbox{\\hbox{\n";
  const char ItemFooter [] = "\n\\hskip 0pt plus 1filll minus 1filll}\n\\vskip 0pt plus 1filll minus 1filll}\n\\newpage\n";
  const char TextFooter [] = "\\end{document}\n";

  // chdir into temporary directory so that LaTeX's mess goes into /tmp
  if (chdir(x->c->errcontext.session_default.tempdir) < 0) { ppl_error(&x->c->errcontext, ERR_INTERNAL, -1, -1,"Could not chdir into temporary directory."); *(x->status)=1; return; }

  // Make string buffer
  str_buffer = (char *)ppl_memAlloc(LSTR_LENGTH);
  if (str_buffer==NULL) { ppl_error(&x->c->errcontext, ERR_MEMORY,-1,-1,"Out of memory."); return; }

  // Start writing LaTeX document
  sprintf(filename, "%s.tex", x->TeXFilename);
  output = fopen(filename, "w");
  if (output == NULL) { ppl_error(&x->c->errcontext, ERR_INTERNAL, -1, -1, "Could not create temporary LaTeX document"); *(x->status)=1; return; }
  FPRINTF_LINECOUNT(TextHeader1);
  FPRINTF_LINECOUNT(x->c->set->term_current.LatexPreamble);
  FPRINTF_LINECOUNT(TextHeader2);

  // Sequentially print out text strings
  ListIter = ppl_listIterateInit(x->TextItems);
  while (ListIter != NULL)
   {
    TempTextItem = (CanvasTextItem *)ListIter->data;
    TempTextItem->LaTeXstartline = linecount;
    FPRINTF_LINECOUNT(ItemHeader);
    FPRINTF_LINECOUNT(TempTextItem->text);
    FPRINTF_LINECOUNT(ItemFooter);
    TempTextItem->LaTeXendline   = linecount;
    ppl_listIterate(&ListIter);
   }

  // Finish writing LaTeX document
  FPRINTF_LINECOUNT(TextFooter);
  fclose(output);

  // Fork LaTeX process
  pplcsp_forkLaTeX(x->c, filename, &pid, &LatexStdIn, &LatexOut);

  // Wait for latex process's stdout to become readable. Get bored if this takes too long.
  FirstIter = 1;
  LatexStatus = 0;
  while (1)
   {
    for (TrialNumber=1;;)
     {
      waitperiod.tv_sec  = FirstIter ? 15 : 4; // Wait 15 seconds first time around; otherwise wait 4 seconds
      waitperiod.tv_nsec = 0;
      if (ReadErrorState) { waitperiod.tv_nsec = 500000000; waitperiod.tv_sec = 0; } // If we've had an error message, only wait 0.5 sec
      if (DEBUG) { sprintf(x->c->errcontext.tempErrStr,"pselect waiting for %ld seconds and %ld nanoseconds",waitperiod.tv_sec,waitperiod.tv_nsec); ppl_log(&x->c->errcontext, x->c->errcontext.tempErrStr); }
      FD_ZERO(&readable); FD_SET(LatexOut, &readable);
      if (pselect(LatexOut+1, &readable, NULL, NULL, &waitperiod, NULL) == -1)
       {
        if (DEBUG) { sprintf(x->c->errcontext.tempErrStr,"pselect returned -1. errno=%d",errno); ppl_log(&x->c->errcontext, x->c->errcontext.tempErrStr); }
        if ((errno==EINTR) && (TrialNumber<3)) { TrialNumber++; continue; }
        LatexStatus=1;
        break;
       }
      if (DEBUG) { sprintf(x->c->errcontext.tempErrStr,"pipe is %s",(FD_ISSET(LatexOut , &readable))?"readable":"unreadable"); ppl_log(&x->c->errcontext, x->c->errcontext.tempErrStr); }
      break;
     }
    if (LatexStatus) break;
    FirstIter = 0;
    if (!FD_ISSET(LatexOut , &readable)) { LatexStatus=1; if (DEBUG) ppl_log(&x->c->errcontext, "latex's output pipe has not become readable within time limit"); break; }
    else
     {
      if ((NCharsRead = read(LatexOut, str_buffer, LSTR_LENGTH)) > 0)
       {
        for (j=0; j<NCharsRead; j++)
         {
          if (str_buffer[j]=='\n')
           {
            if (ErrReadState==2) { TempErrMsg[ErrReadPos]='\0'; strcpy(ErrFilename, TempErrFilename); strcpy(ErrMsg, TempErrMsg); ErrLineNo = (int)ppl_getFloat(TempErrLineNo, NULL); if (DEBUG) { sprintf(x->c->errcontext.tempErrStr,"Received LaTeX error on line %d of file <%s>",ErrLineNo,ErrFilename); ppl_log(&x->c->errcontext, x->c->errcontext.tempErrStr); sprintf(x->c->errcontext.tempErrStr,"Error message: <%s>",ErrMsg); ppl_log(&x->c->errcontext, x->c->errcontext.tempErrStr); } }
            ErrReadState = ErrReadPos = 0;
           }
          else if (str_buffer[j]==':')
           {
            if      (ErrReadState==0)
             {
              TempErrFilename[ErrReadPos]='\0';
              ErrReadPos=0;
              if (strcmp(TempErrFilename,"! LaTeX Error")==0)
               {
                strcpy(TempErrFilename, filename);
                strcpy(TempErrLineNo  , "-1");
                ErrReadState = 2;
                ReadErrorState = 1;
                if (DEBUG) ppl_log(&x->c->errcontext, "Encountered line beginning \"! LaTeX Error\"");
               }
              else
               { ErrReadState = 1; }
             }
            else if (ErrReadState==1) { TempErrLineNo  [ErrReadPos]='\0'; ErrReadPos=0; ErrReadState++; if (!ppl_validFloat(TempErrLineNo, NULL)) ErrReadState++; else ReadErrorState = 1; }
            else if (ErrReadState==2) { TempErrMsg[ErrReadPos++]=str_buffer[j]; }
           }
          else
           {
            if      (ErrReadState==0) { TempErrFilename[ErrReadPos++] = str_buffer[j]; }
            else if (ErrReadState==1) { TempErrLineNo  [ErrReadPos++] = str_buffer[j]; }
            else if (ErrReadState==2) { TempErrMsg     [ErrReadPos++] = str_buffer[j]; }
           }
          if (ErrReadPos > FNAME_LENGTH-1) ErrReadPos=FNAME_LENGTH-1;
         }
       }
      else { break; } // read function returned zero; indicates EOF
     }
   }

  // Finish talking to LaTeX process
  close(LatexStdIn); // This has the effect of killing latex
  close(LatexOut);
  sigprocmask(SIG_UNBLOCK, &sigs, NULL);

  // Return error message if latex has failed
  if (LatexStatus)
   {
    ppl_strStrip(ErrMsg,ErrMsg);
    if (strcmp(filename, ErrFilename)==0) // Case 1: Error in the LaTeX file which we just generated
     {
      unsigned char ExactHit=0; // Cycle through all text items and see whether error line number is in the middle of one of them
      ListIter = ppl_listIterateInit(x->TextItems);
      while (ListIter != NULL)
       {
        TempTextItem = (CanvasTextItem *)ListIter->data;
        if (TempTextItem->LaTeXstartline > ErrLineNo) break; // We have gone past the line number where the error happened
        SuspectTextItem = TempTextItem;
        ExactHit        = (TempTextItem->LaTeXendline >= ErrLineNo); // If this is zero, then error was after the end of the item we just looked at
        ppl_listIterate(&ListIter);
       }
      if (SuspectTextItem==NULL) sprintf(x->c->errcontext.tempErrStr, "LaTeX error encountered in an unidentifiable canvas item.");
      else if (ExactHit)         sprintf(x->c->errcontext.tempErrStr, "LaTeX error encountered in text string in canvas item %d.", SuspectTextItem->CanvasMultiplotID);
      else                       sprintf(x->c->errcontext.tempErrStr, "LaTeX error encountered at the end of text string in canvas item %d.", SuspectTextItem->CanvasMultiplotID);
      ppl_error(&x->c->errcontext, ERR_GENERAL, -1, -1,x->c->errcontext.tempErrStr);
      sprintf(x->c->errcontext.tempErrStr, "Error was: %s", ErrMsg); // Output the actual error which LaTeX returned to us
      ppl_error(&x->c->errcontext, ERR_PREFORMED, -1, -1,x->c->errcontext.tempErrStr);
      if (SuspectTextItem!=NULL)
       {
        canvas_item *ptr = ((canvas_itemlist *)x->c->canvas_items)->first; // Cycle through all canvas items to find the culprit
        for (ptr = ((canvas_itemlist *)x->c->canvas_items)->first; ((ptr!=NULL)&&(ptr->id!=SuspectTextItem->CanvasMultiplotID)); ptr = ptr->next);
        if (ptr!=NULL)
         {
          sprintf(x->c->errcontext.tempErrStr, "\nInput PyXPlot command was:\n\n"); // Now tell the user what PyXPlot command produced the culprit
          i  = strlen(x->c->errcontext.tempErrStr);
          ppl_canvas_item_textify(x->c, ptr, x->c->errcontext.tempErrStr+i);
          i += strlen(x->c->errcontext.tempErrStr+i);
          strcpy(x->c->errcontext.tempErrStr+i, "\n");
          ppl_error(&x->c->errcontext, ERR_PREFORMED, -1, -1,x->c->errcontext.tempErrStr);
         } // Then output the LaTeX string which produced the error
        sprintf(x->c->errcontext.tempErrStr, "\nOffending input to LaTeX was:\n\n%s\n", SuspectTextItem->text); ppl_error(&x->c->errcontext, ERR_PREFORMED, -1, -1,x->c->errcontext.tempErrStr);
       }
     }
    else if ((ErrFilename[0]=='\0')&&(ErrMsg[0]=='\0')) // Case 2: Error encountered, but we didn't catch error message
     {
      sprintf(x->c->errcontext.tempErrStr, "Unidentified LaTeX error encountered.");
      ppl_error(&x->c->errcontext, ERR_GENERAL, -1, -1,x->c->errcontext.tempErrStr);
     }
    else // Case 3: Error in another file which the user seems to have imported
     {
      sprintf(x->c->errcontext.tempErrStr, "LaTeX error encountered in imported file <%s> on line %d.", ErrFilename, ErrLineNo);
      ppl_error(&x->c->errcontext, ERR_GENERAL, -1, -1,x->c->errcontext.tempErrStr);
      sprintf(x->c->errcontext.tempErrStr, "Error was: %s", ErrMsg);
      ppl_error(&x->c->errcontext, ERR_PREFORMED, -1, -1,x->c->errcontext.tempErrStr);
     }
    if (chdir(x->c->errcontext.session_default.cwd) < 0) { ppl_fatal(&x->c->errcontext,__FILE__,__LINE__,"chdir into cwd failed."); }
    *(x->status) = 1;
    return;
   }

  // Convert dvi into postscript fragments
  sprintf(filename, "%s.dvi", x->TeXFilename);
  x->dvi = ReadDviFile(&x->c->errcontext, filename, x->status);
  if (*(x->status)) return; // DVI interpreter failed

  // Return to user's current working directory after LaTeX has finished making a mess
  if (chdir(x->c->errcontext.session_default.cwd) < 0) { ppl_fatal(&x->c->errcontext,__FILE__,__LINE__,"chdir into cwd failed."); }
  return;
 }

// Make a temporary buffer file into which the eps fragments for objects is temporarily stored
void canvas_MakeEPSBuffer(EPSComm *x)
 {
  x->epsbuffer = tmpfile();
  if (x->epsbuffer == NULL) { ppl_error(&x->c->errcontext, ERR_INTERNAL, -1, -1,"Could not create temporary eps buffer file."); *(x->status)=1; return; }
  return;
 }

// Finally output postscript file, once all of the eps fragments are written to temporary buffer and bounding box is known
void canvas_EPSWrite(EPSComm *x)
 {
  int i;
  double PAGEwidth, PAGEheight, margin_left, margin_top, margin_right, margin_bottom;
  FILE *epsout, *PFAfile;
  char LandscapifyText[FNAME_LENGTH], EnlargementText[FNAME_LENGTH], *PaperName, *PFAfilename;
  listIterator *ListIter;

  // Check that we have a bounding box
  if (!x->bb_set)
   {
    x->bb_left = x->bb_right = x->bb_bottom = x->bb_top = 0.0;
   }

  // Apply enlarge and landscape terminals as required
  LandscapifyText[0] = EnlargementText[0] = '\0';
  if  (x->c->set->term_current.landscape   == SW_ONOFF_ON)                                     canvas_EPSLandscapify(x, LandscapifyText);
  if ((x->c->set->term_current.TermEnlarge == SW_ONOFF_ON) && (x->termtype == SW_TERMTYPE_PS)) canvas_EPSEnlarge    (x, EnlargementText);


  // Work out name to call papersize
  PaperName = x->c->set->term_current.PaperName;
  if (strcmp(PaperName,"User-defined papersize")==0) PaperName = "CustomPageSize";
  // Return to user's current working directory
  if (chdir(x->c->errcontext.session_default.cwd) < 0) { ppl_fatal(&x->c->errcontext, __FILE__,__LINE__,"chdir into cwd failed."); }

  // Work out some sensible margins for page
  PAGEwidth = x->c->set->term_current.PaperWidth.real;
  PAGEheight = x->c->set->term_current.PaperHeight.real;
  margin_left = PAGEwidth / 14;
  margin_top  = PAGEheight / 14;
  margin_left *= M_TO_PS;
  margin_top  *= M_TO_PS;
  margin_right  =   margin_left;
  margin_bottom = 2*margin_top;

  // Open output postscript file for writing
  if ((epsout=fopen(x->EPSFilename,"w"))==NULL) { sprintf(x->c->errcontext.tempErrStr, "Could not open file '%s' for writing.", x->EPSFilename); ppl_error(&x->c->errcontext, ERR_FILE, -1, -1, NULL); *(x->status)=1; return; }

  // Write EPS header
  if (x->c->set->term_current.TermType != SW_TERMTYPE_PS)
   {
    fprintf(epsout, "%s%s\n", "%!PS-Adobe-3.0", " EPSF-3.0");
    fprintf(epsout, "%%%%BoundingBox: %d %d %d %d\n", (int)floor(x->bb_left), (int)floor(x->bb_bottom), (int)ceil(x->bb_right), (int)ceil(x->bb_top));
    fprintf(epsout, "%%%%HiResBoundingBox: %f %f %f %f\n", x->bb_left, x->bb_bottom, x->bb_right, x->bb_top);
   }
  else
   {
    fprintf(epsout, "%s%s\n", "%!PS-Adobe-3.0", "");
    fprintf(epsout, "%%%%BoundingBox: %d %d %d %d\n", (int)floor(margin_left), (int)floor(margin_bottom), (int)ceil(x->bb_right-x->bb_left+margin_left), (int)ceil(x->bb_top-x->bb_bottom+margin_bottom));
    fprintf(epsout, "%%%%HiResBoundingBox: %f %f %f %f\n", margin_left, margin_bottom, x->bb_right-x->bb_left+margin_left, x->bb_top-x->bb_bottom+margin_bottom);
   }
  fprintf(epsout, "%%%%Creator: (PyXPlot %s)\n", VERSION);
  fprintf(epsout, "%%%%Title: (%s)\n", x->title);
  fprintf(epsout, "%%%%CreationDate: (%s)\n", ppl_strStrip(ppl_friendlyTimestring(), x->c->errcontext.tempErrStr));
  if (x->c->set->term_current.TermType == SW_TERMTYPE_PS) fprintf(epsout, "%%%%Pages: 1\n");
  fprintf(epsout, "%%%%DocumentData: Clean7Bit\n");
  if (x->c->set->term_current.landscape == SW_ONOFF_ON) fprintf(epsout, "%%%%Orientation: Landscape\n");
  else                                                fprintf(epsout, "%%%%Orientation: Portrait\n");
  if (x->c->set->term_current.TermType == SW_TERMTYPE_PS)
    fprintf(epsout, "%%%%DocumentMedia: %s %d %d white { }\n", PaperName, (int)(x->c->set->term_current.PaperWidth.real * M_TO_PS), (int)(x->c->set->term_current.PaperHeight.real * M_TO_PS));
  fprintf(epsout, "%%%%DocumentFonts:"); // %%DocumentFonts has a list of all of the fonts that we use
  if (x->dvi != NULL) ListIter = ppl_listIterateInit(x->dvi->fonts);
  else                ListIter = NULL;
  while (ListIter != NULL)
   {
    fprintf(epsout, " %s", ((dviFontDetails *)ListIter->data)->psName);
    ppl_listIterate(&ListIter);
   }
  fprintf(epsout, "\n");
  fprintf(epsout, "%%%%EndComments\n\n");

  // In postscript files, now set up page size
  if (x->c->set->term_current.TermType == SW_TERMTYPE_PS)
   {
    fprintf(epsout, "%%%%BeginDefaults\n");
    fprintf(epsout, "%%%%PageMedia: %s\n", PaperName);
    fprintf(epsout, "%%%%EndDefaults\n");
   }

  // Write EPS prolog
  fprintf(epsout, "%%%%BeginProlog\n");

  // Output all of the fonts which we're going to use
  if (chdir(x->c->errcontext.session_default.tempdir) < 0) { ppl_error(&x->c->errcontext, ERR_INTERNAL, -1, -1,"Could not chdir into temporary directory."); *(x->status)=1; fclose(epsout); return; }
  if (x->dvi != NULL) ListIter = ppl_listIterateInit(x->dvi->fonts);
  else                ListIter = NULL;
  while (ListIter != NULL)
   {
    PFAfilename = ((dviFontDetails *)ListIter->data)->pfaPath;
    if (PFAfilename != NULL)  // PFAfilename==NULL indicates that this is a built-in font that doesn't require a fontdef
     {
      fprintf(epsout, "%%%%BeginFont: %s\n", ((dviFontDetails *)ListIter->data)->psName);
      PFAfile = fopen(PFAfilename,"r");
      if (PFAfile==NULL) { sprintf(x->c->errcontext.tempErrStr, "Could not open pfa file '%s'", PFAfilename); ppl_error(&x->c->errcontext, ERR_FILE, -1, -1, NULL); *(x->status)=1; fclose(epsout); return; }
      while (fgets(x->c->errcontext.tempErrStr, FNAME_LENGTH, PFAfile) != NULL)
       if (fputs(x->c->errcontext.tempErrStr, epsout) == EOF)
        {
         sprintf(x->c->errcontext.tempErrStr, "Error while writing to file '%s'.", x->EPSFilename); ppl_error(&x->c->errcontext, ERR_FILE, -1, -1, NULL);
         *(x->status)=1;
         fclose(PFAfile); fclose(epsout);
         return;
        }
      fclose(PFAfile);
      fprintf(epsout, "\n%%%%EndFont\n");
     }
    ppl_listIterate(&ListIter);
   }
  if (chdir(x->c->errcontext.session_default.cwd) < 0) { ppl_fatal(&x->c->errcontext, __FILE__,__LINE__,"chdir into cwd failed."); }

  // Output macros which PyXPlot needs
  if (x->c->set->term_current.TermType == SW_TERMTYPE_PS) fprintf(epsout, "%s", PS_PROLOG_TEXT);
  fprintf(epsout, "%s", EPS_PROLOG_TEXT);
  fprintf(epsout, "/ps { 1 } def\n/ps75 { ps .75 mul } def\n"); // Pointsize variables

  if (x->PointTypesUsed[28]) { x->PointTypesUsed[14]=x->PointTypesUsed[ 0]=1; } // pt6 depends upon pt4 and pt5
  if (x->PointTypesUsed[29]) { x->PointTypesUsed[15]=x->PointTypesUsed[ 1]=1; } // pt9 depends upon pt7 and pt8
  if (x->PointTypesUsed[30]) { x->PointTypesUsed[16]=x->PointTypesUsed[ 2]=1; } // pt12 depends upon pt10 and pt11
  if (x->PointTypesUsed[31]) { x->PointTypesUsed[17]=x->PointTypesUsed[ 3]=1; } // pt15 depends upon pt13 and pt14
  if (x->PointTypesUsed[32]) { x->PointTypesUsed[18]=x->PointTypesUsed[ 4]=1; } // pt18 depends upon pt16 and pt17
  if (x->PointTypesUsed[33]) { x->PointTypesUsed[19]=x->PointTypesUsed[ 5]=1; } // pt21 depends upon pt19 and pt20
  if (x->PointTypesUsed[34]) { x->PointTypesUsed[20]=x->PointTypesUsed[ 6]=1; } // pt24 depends upon pt22 and pt23
  if (x->PointTypesUsed[35]) { x->PointTypesUsed[21]=x->PointTypesUsed[ 7]=1; } // pt27 depends upon pt25 and pt26
  if (x->PointTypesUsed[36]) { x->PointTypesUsed[22]=x->PointTypesUsed[ 8]=1; } // pt30 depends upon pt28 and pt29
  if (x->PointTypesUsed[37]) { x->PointTypesUsed[23]=x->PointTypesUsed[ 9]=1; } // pt33 depends upon pt31 and pt32
  if (x->PointTypesUsed[38]) { x->PointTypesUsed[24]=x->PointTypesUsed[10]=1; } // pt36 depends upon pt34 and pt35
  if (x->PointTypesUsed[39]) { x->PointTypesUsed[25]=x->PointTypesUsed[11]=1; } // pt33 depends upon pt37 and pt38
  if (x->PointTypesUsed[40]) { x->PointTypesUsed[26]=x->PointTypesUsed[12]=1; } // pt3 depends upon pt1 and pt2
  if (x->PointTypesUsed[41]) { x->PointTypesUsed[27]=x->PointTypesUsed[13]=1; } // pt6 depends upon pt4 and pt5

  if (x->StarTypesUsed [6]) { x->StarTypesUsed [5]=1; } // st7 depends upon st6
  for (i=0; i<N_POINTTYPES; i++) if (x->PointTypesUsed[i]) fprintf(epsout, "%s\n", eps_PointTypes[i]);
  for (i=0; i<N_STARTYPES ; i++) if (x->StarTypesUsed [i]) { fprintf(epsout, "%s\n", eps_StarCore); break; }
  for (i=0; i<N_STARTYPES ; i++) if (x->StarTypesUsed [i]) fprintf(epsout, "%s\n", eps_StarTypes [i]);
  fprintf(epsout, "%%%%EndProlog\n\n");

  // In postscript files, now set up page 1
  if (x->c->set->term_current.TermType == SW_TERMTYPE_PS)
   {
    fprintf(epsout, "<< /PageSize [ %d %d ] >> setpagedevice\n", (int)(x->c->set->term_current.PaperWidth.real * M_TO_PS), (int)(x->c->set->term_current.PaperHeight.real * M_TO_PS));
    fprintf(epsout, "%%%%Page: 1 1\n");
    fprintf(epsout, "%%%%BeginPageSetup\n");
    fprintf(epsout, "/pgsave save def\n");
    fprintf(epsout, "%%%%EndPageSetup\n\n");
   }

  // Now write any global transformations needed by the enlarge and landscape terminals.
  if (x->c->set->term_current.TermType == SW_TERMTYPE_PS) fprintf(epsout, "%.2f %.2f translate\n", margin_left-x->bb_left, margin_bottom-x->bb_bottom);
  if ((LandscapifyText[0]!='\0')||(EnlargementText[0]!='\0'))
   {
    fprintf(epsout, "%% Global transformations to give %soutput%s\n", (LandscapifyText[0]!='\0')?"landscape ":"", (EnlargementText[0]!='\0')?" which fills the page":"");
    fprintf(epsout, "%s", EnlargementText);
    fprintf(epsout, "%s", LandscapifyText);
    fprintf(epsout, "\n");
   }

  // Copy contents of eps buffer into postscript output
  fflush(x->epsbuffer);
  rewind(x->epsbuffer);
  while (fgets(x->c->errcontext.tempErrStr, FNAME_LENGTH, x->epsbuffer) != NULL)
   if (fputs(x->c->errcontext.tempErrStr, epsout) == EOF)
    {
     sprintf(x->c->errcontext.tempErrStr, "Error while writing to file '%s'.", x->EPSFilename); ppl_error(&x->c->errcontext, ERR_FILE, -1, -1, NULL);
     *(x->status)=1;
     fclose(epsout);
     return;
    }

  // Write postscript footer and finish
  if (x->c->set->term_current.TermType == SW_TERMTYPE_PS) fprintf(epsout, "pgsave restore\n"); // End of page
  fprintf(epsout, "showpage\n%%%%EOF\n"); // End of document
  fclose(x->epsbuffer);
  x->epsbuffer=NULL;
  fclose(epsout);
  return;
 }

// Write a text item out from dvi buffer
void canvas_EPSRenderTextItem(EPSComm *x, char **strout, int pageno, double xpos, double ypos, int halign, int valign, char *colstr, double fontsize, double rotate, double *width, double *height)
 {
  postscriptPage *dviPage;
  listIterator *ListIter;
  char *cptr, *out;
  static char *buffer=NULL;
  const long   BUFLEN=131072;
  long         bufpos=0;
  double bb_left, bb_right, bb_top, bb_bottom, xanchor, yanchor;
  double ab_left, ab_right, ab_top, ab_bottom; // The align box is different from the bounding box in that it gives the baseline height of the lowest line of text and the cap height of the highest line of text

  // If colstr is blank, we are painting with null ink
  if (colstr[0]=='\0') return;

  // Write output to text buffer initially
  if (buffer==NULL) buffer = malloc(BUFLEN); // memory leak here!
  if (buffer==NULL) return;
  out=buffer;

  if (x->dvi == NULL) { ppl_error(&x->c->errcontext, ERR_INTERNAL, -1, -1, "Attempting to display a text item before latex has generated it"); return; }

  // Fetch requested page of postscript
  dviPage = (postscriptPage *)ppl_listGetItem(x->dvi->output->pages, pageno+1);
  if (dviPage==NULL) { ppl_error(&x->c->errcontext, ERR_INTERNAL, -1, -1, "Not all text items were rendered by LaTeX"); *(x->status)=1; return; }
  bb_left   = dviPage->boundingBox[0];
  bb_bottom = dviPage->boundingBox[1];
  bb_right  = dviPage->boundingBox[2];
  bb_top    = dviPage->boundingBox[3];
  ab_left   = dviPage->textSizeBox[0];
  ab_bottom = dviPage->textSizeBox[1];
  ab_right  = dviPage->textSizeBox[2];
  ab_top    = dviPage->textSizeBox[3];

  // Return dimensions of text item
  if (width  != NULL) *width  = (bb_right - bb_left)*fontsize;
  if (height != NULL) *height = (bb_top - bb_bottom)*fontsize;

  // Work out where our anchor point is on postscript
  if      (halign == SW_HALIGN_LEFT ) xanchor = ab_left;
  else if (halign == SW_HALIGN_CENT ) xanchor = (ab_left + ab_right)/2.0;
  else if (halign == SW_HALIGN_RIGHT) xanchor = ab_right;
  else                                 { ppl_error(&x->c->errcontext, ERR_INTERNAL, -1, -1, "Illegal halign value passed to canvas_EPSRenderTextItem"); *(x->status)=1; return; }
  if      (valign == SW_VALIGN_TOP  ) yanchor = bb_top;
  else if (valign == SW_VALIGN_CENT ) yanchor = (ab_top + ab_bottom)/2.0;
  else if (valign == SW_VALIGN_BOT  ) yanchor = ab_bottom;
  else                                 { ppl_error(&x->c->errcontext, ERR_INTERNAL, -1, -1, "Illegal valign value passed to canvas_EPSRenderTextItem"); *(x->status)=1; return; }

  // Update bounding box of canvas. For this, use BOUNDING box, not ALIGNMENT box.
  eps_core_PlotBoundingBox(x, xpos*M_TO_PS + (bb_left  - xanchor)*fontsize*cos(rotate) + (bb_bottom - yanchor)*fontsize*-sin(rotate),
                              ypos*M_TO_PS + (bb_left  - xanchor)*fontsize*sin(rotate) + (bb_bottom - yanchor)*fontsize* cos(rotate), 0, 1);
  eps_core_PlotBoundingBox(x, xpos*M_TO_PS + (bb_right - xanchor)*fontsize*cos(rotate) + (bb_bottom - yanchor)*fontsize*-sin(rotate),
                              ypos*M_TO_PS + (bb_right - xanchor)*fontsize*sin(rotate) + (bb_bottom - yanchor)*fontsize* cos(rotate), 0, 1);
  eps_core_PlotBoundingBox(x, xpos*M_TO_PS + (bb_left  - xanchor)*fontsize*cos(rotate) + (bb_top    - yanchor)*fontsize*-sin(rotate),
                              ypos*M_TO_PS + (bb_left  - xanchor)*fontsize*sin(rotate) + (bb_top    - yanchor)*fontsize* cos(rotate), 0, 1);
  eps_core_PlotBoundingBox(x, xpos*M_TO_PS + (bb_right - xanchor)*fontsize*cos(rotate) + (bb_top    - yanchor)*fontsize*-sin(rotate),
                              ypos*M_TO_PS + (bb_right - xanchor)*fontsize*sin(rotate) + (bb_top    - yanchor)*fontsize* cos(rotate), 0, 1);


  // Perform translation such that postscript text appears in the right place on the page
  snprintf(out+bufpos, BUFLEN-bufpos, "gsave\n"); bufpos+=strlen(out+bufpos);
  snprintf(out+bufpos, BUFLEN-bufpos, "%.2f %.2f translate\n", xpos * M_TO_PS, ypos * M_TO_PS); bufpos+=strlen(out+bufpos);
  snprintf(out+bufpos, BUFLEN-bufpos, "%.2f rotate\n", rotate * 180 / M_PI); bufpos+=strlen(out+bufpos);
  snprintf(out+bufpos, BUFLEN-bufpos, "%f %f scale\n", fontsize, fontsize); bufpos+=strlen(out+bufpos);
  snprintf(out+bufpos, BUFLEN-bufpos, "%.2f %.2f translate\n", -xanchor, -yanchor); bufpos+=strlen(out+bufpos);

  // Copy postscript description of page out of dvi buffer
  ListIter = ppl_listIterateInit(dviPage->text);
  while (ListIter!=NULL)
   {
    cptr = (char *)ListIter->data;
    while (*cptr!='\0')
     {
      if (*cptr!='\x01') snprintf(out+bufpos, BUFLEN-bufpos, "%c", *cptr);
      else               snprintf(out+bufpos, BUFLEN-bufpos, "%s", colstr); // ASCII x01 is a magic code to tell us to revert to default colour
      bufpos+=strlen(out+bufpos);
      cptr++;
     }
    ppl_listIterate(&ListIter);
   }

  // Undo translation and we're finished
  snprintf(out+bufpos, BUFLEN-bufpos, "grestore\n"); bufpos+=strlen(out+bufpos);

  // Either return string, or write output to file
  if (strout==NULL) fprintf(x->epsbuffer, "%s", buffer);
  else              *strout=buffer;
  return;
 }

// Convert a portrait EPS file to landscape by changing its bounding box and adding a linear transformation to the top
void canvas_EPSLandscapify(EPSComm *x, char *transform)
 {
  double width, height;
  width  = x->bb_right - x->bb_left;
  height = x->bb_top   - x->bb_bottom;
  sprintf(transform, "90 rotate\n%f %f translate\n", -x->bb_left, -x->bb_top);
  x->bb_left   = 0.0;
  x->bb_bottom = 0.0;
  x->bb_right  = height;
  x->bb_top    = width;
  return;
 }

// Enlarge an EPS file to fill a page by changing its bounding box and adding a linear transformation to the top
void canvas_EPSEnlarge(EPSComm *x, char *transform)
 {
  double EPSwidth, EPSheight, PAGEwidth, PAGEheight;
  double margin_left, margin_right, margin_top, margin_bottom;
  double scaling_x, scaling_y, translate_x, translate_y;

  // Read dimensions of page
  PAGEwidth  = x->c->set->term_current.PaperWidth .real;
  PAGEheight = x->c->set->term_current.PaperHeight.real;

  // Calculate dimensions of EPS image
  EPSwidth  = x->bb_right - x->bb_left;
  EPSheight = x->bb_top   - x->bb_bottom;
  if ((EPSwidth<=0.0)||(EPSheight<=0.0)) return; // Don't attempt to enlarge pages of size zero

  // Work out some sensible margins for page
  margin_left = PAGEwidth / 14;
  margin_top  = PAGEheight / 14;
  if (margin_left > 15e-3) margin_left = 15e-3; // mm
  if (margin_top  > 15e-3) margin_top  = 15e-3; // mm
  margin_right  =   margin_left;
  margin_bottom = 2*margin_top;

  // Work out what scaling factors will make page fit
  scaling_x = (PAGEwidth  - margin_left - margin_right ) / EPSwidth  * M_TO_PS;
  scaling_y = (PAGEheight - margin_top  - margin_bottom) / EPSheight * M_TO_PS;
  if ((!gsl_finite(scaling_x))||(!gsl_finite(scaling_y))) return; // Don't attempt to enlarge pages of size zero
  if (scaling_y < scaling_x) scaling_x = scaling_y; // Lowest scaling factor is biggest enlargement we can do

  // Work out how to translate origin to (0,0)
  translate_x =                          margin_left * M_TO_PS  -  x->bb_left * scaling_x;
  translate_y = PAGEheight * M_TO_PS  -  margin_top  * M_TO_PS  -  x->bb_top  * scaling_x;

  // Write out transformation
  sprintf(transform, "%f %f translate\n%f %f scale\n", translate_x, translate_y, scaling_x, scaling_x);
  x->bb_left   = margin_left * M_TO_PS;
  x->bb_right  = margin_left * M_TO_PS  +  EPSwidth * scaling_x;
  x->bb_top    = (PAGEheight - margin_top) * M_TO_PS;
  x->bb_bottom = (PAGEheight - margin_top) * M_TO_PS  -  EPSheight * scaling_x;
  return;
 }

