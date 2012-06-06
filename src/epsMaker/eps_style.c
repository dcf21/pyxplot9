// eps_style.c
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

#define _PPL_EPS_STYLE_C 1

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "epsMaker/eps_style.h"

// Line types
char *eps_LineType(int lt, double lw, double offset)
 {
  static char output[256];
  lt = (lt-1) % 9;
  while (lt<0) lt+=9;
  if      (lt==0) sprintf(output, "0 setlinecap [] %.2f setdash", offset);                                   // solid
  else if (lt==1) sprintf(output, "0 setlinecap [%.2f] %.2f setdash", 2*lw, offset);                         // dashed
  else if (lt==2) sprintf(output, "1 setlinecap [0 %.2f] %.2f setdash", 2*lw, offset);                       // dotted
  else if (lt==3) sprintf(output, "1 setlinecap [0 %.2f %.2f %.2f] %.2f setdash", 2*lw, 2*lw, 2*lw, offset); // dash-dotted
  else if (lt==4) sprintf(output, "0 setlinecap [%.2f %.2f] %.2f setdash", 7*lw, 2*lw, offset);              // long dash
  else if (lt==5) sprintf(output, "1 setlinecap [%.2f %.2f 0 %.2f] %.2f setdash", 7*lw, 2*lw, 2*lw, offset); // long dash - dot
  else if (lt==6) sprintf(output, "1 setlinecap [%.2f %.2f 0 %.2f 0 %.2f] %.2f setdash", 7*lw, 2*lw, 2*lw, 2*lw, offset); // long dash - dot dot
  else if (lt==7) sprintf(output, "1 setlinecap [%.2f %.2f 0 %.2f 0 %.2f 0 %.2f] %.2f setdash", 7*lw, 2*lw, 2*lw, 2*lw, 2*lw, offset); // long dash - dot dot dot
  else if (lt==8) sprintf(output, "0 setlinecap [%.2f %.2f %.2f %.2f] %.2f setdash", 7*lw, 2*lw, 2*lw, 2*lw, offset); // long dash - dash
  return output;
 }

// Point types
// NB the fact that point types 29-42 depend upon point types 1-14 and 15-28, and that this must be accounted for in canvasDraw.c
char *eps_PointTypes[N_POINTTYPES] = {
       "/pt1 {newpath 2 copy ps75 sub exch ps75 sub exch moveto ps75 2 mul dup rlineto closepath stroke ps75 add exch ps75 sub exch moveto ps75 2 mul dup -1 mul rlineto closepath stroke } bind def",
       "/pt2 {newpath ps75 add exch ps75 add exch moveto ps75 -2 mul 0 rlineto 0 ps75 -2 mul rlineto ps75 2 mul 0 rlineto closepath stroke} bind def",
       "/pt3 {newpath 2 copy exch ps75 1.1 mul add exch moveto ps75 1.1 mul 0 360 arc stroke} bind def",
       "/pt4 {newpath exch ps 1.183 mul add exch ps75 sub moveto ps -1.183 mul ps 2.049 mul 2 copy rlineto -1 mul rlineto closepath stroke} bind def",
       "/pt5 {newpath exch ps75 add exch moveto ps75 -1 mul ps 1.3 mul 2 copy 2 copy rlineto -1 mul rlineto -1 mul exch -1 mul exch rlineto closepath stroke} bind def",
       "/pt6 {newpath exch ps add exch moveto ps -.5 mul ps 0.866 mul rlineto ps -1 mul 0 rlineto ps -.5 mul ps -.866 mul 2 copy rlineto exch -1 mul exch rlineto ps 0 rlineto closepath stroke} bind def",
       "/pt7 {newpath 2 copy ps add moveto /theta 162 def 4 { 2 copy exch theta cos ps mul add exch theta sin ps mul add lineto /theta theta 72 add def } repeat closepath stroke pop pop} bind def",
       "/pt8 {newpath exch ps 1.183 mul add exch ps75 add moveto ps -1.183 mul ps -2.049 mul 2 copy rlineto -1 mul rlineto closepath stroke} bind def",
       "/pt9 {newpath ps 1.183 mul add exch ps75 sub exch moveto ps 2.049 mul ps -1.183 mul 2 copy rlineto exch -1 mul exch rlineto closepath stroke} bind def",
       "/pt10 {newpath ps 1.183 mul add exch ps75 add exch moveto ps -2.049 mul ps -1.183 mul 2 copy rlineto exch -1 mul exch rlineto closepath stroke} bind def",
       "/pt11 {newpath 10 {2 copy} repeat ps add moveto /theta 126 def 5 { exch ps 4 div theta cos mul add exch ps 4 div theta sin mul add lineto /theta theta 36 add def exch ps theta cos mul add exch ps theta sin mul add lineto /theta theta 36 add def } repeat closepath stroke} bind def",
       "/pt12 {newpath moveto ps75 dup rlineto ps -1.5 mul 0 rlineto ps 1.5 mul dup dup neg rlineto neg 0 rlineto closepath stroke} bind def",
       "/pt13 {newpath moveto ps75 dup rlineto 0 ps -1.5 mul rlineto ps 1.5 mul neg dup dup neg rlineto 0 exch rlineto closepath stroke} bind def",
       "/pt14 {newpath 5 {2 copy} repeat /theta 90 def 3 { exch ps theta cos mul add exch ps theta sin mul add moveto lineto stroke /theta theta 120 add def} repeat } bind def",
       "/pt15 {newpath 2 copy exch ps sub exch moveto ps 2 mul 0 rlineto closepath stroke ps sub moveto 0 ps 2 mul rlineto closepath stroke } bind def",
       "/pt16 {newpath ps75 add exch ps75 add exch moveto ps75 -2 mul 0 rlineto 0 ps75 -2 mul rlineto ps75 2 mul 0 rlineto closepath fill} bind def",
       "/pt17 {newpath 2 copy exch ps75 1.1 mul add exch moveto ps75 1.1 mul 0 360 arc fill} bind def",
       "/pt18 {newpath exch ps 1.183 mul add exch ps75 sub moveto ps -1.183 mul ps 2.049 mul 2 copy rlineto -1 mul rlineto closepath fill} bind def",
       "/pt19 {newpath exch ps75 add exch moveto ps75 -1 mul ps 1.3 mul 2 copy 2 copy rlineto -1 mul rlineto -1 mul exch -1 mul exch rlineto closepath fill} bind def",
       "/pt20 {newpath exch ps add exch moveto ps -.5 mul ps 0.866 mul rlineto ps -1 mul 0 rlineto ps -.5 mul ps -.866 mul 2 copy rlineto exch -1 mul exch rlineto ps 0 rlineto closepath fill} bind def",
       "/pt21 {newpath 2 copy ps add moveto /theta 162 def 4 { 2 copy exch theta cos ps mul add exch theta sin ps mul add lineto /theta theta 72 add def } repeat closepath fill pop pop} bind def",
       "/pt22 {newpath exch ps 1.183 mul add exch ps75 add moveto ps -1.183 mul ps -2.049 mul 2 copy rlineto -1 mul rlineto closepath fill} bind def",
       "/pt23 {newpath ps 1.183 mul add exch ps75 sub exch moveto ps 2.049 mul ps -1.183 mul 2 copy rlineto exch -1 mul exch rlineto closepath fill} bind def",
       "/pt24 {newpath ps 1.183 mul add exch ps75 add exch moveto ps -2.049 mul ps -1.183 mul 2 copy rlineto exch -1 mul exch rlineto closepath fill} bind def",
       "/pt25 {newpath 10 {2 copy} repeat ps add moveto /theta 126 def 5 { exch ps 4 div theta cos mul add exch ps 4 div theta sin mul add lineto /theta theta 36 add def exch ps theta cos mul add exch ps theta sin mul add lineto /theta theta 36 add def } repeat closepath fill} bind def",
       "/pt26 {newpath moveto ps75 dup rlineto ps -1.5 mul 0 rlineto ps 1.5 mul dup dup neg rlineto neg 0 rlineto closepath fill} bind def",
       "/pt27 {newpath moveto ps75 dup rlineto 0 ps -1.5 mul rlineto ps 1.5 mul neg dup dup neg rlineto 0 exch rlineto closepath fill} bind def",
       "/pt28 {newpath 5 {2 copy} repeat /theta 270 def 3 { exch ps theta cos mul add exch ps theta sin mul add moveto lineto stroke /theta theta 120 add def} repeat } bind def",
       "/pt29 {newpath 2 copy pt1 pt15} bind def",
       "/pt30 {newpath 2 copy pt16 currentrgbcolor 0 0 0 setrgbcolor 5 3 roll 2 copy pt2 5 2 roll setrgbcolor} bind def",
       "/pt31 {newpath 2 copy pt17 currentrgbcolor 0 0 0 setrgbcolor 5 3 roll 2 copy pt3 5 2 roll setrgbcolor} bind def",
       "/pt32 {newpath 2 copy pt18 currentrgbcolor 0 0 0 setrgbcolor 5 3 roll 2 copy pt4 5 2 roll setrgbcolor} bind def",
       "/pt33 {newpath 2 copy pt19 currentrgbcolor 0 0 0 setrgbcolor 5 3 roll 2 copy pt5 5 2 roll setrgbcolor} bind def",
       "/pt34 {newpath 2 copy pt20 currentrgbcolor 0 0 0 setrgbcolor 5 3 roll 2 copy pt6 5 2 roll setrgbcolor} bind def",
       "/pt35 {newpath 2 copy pt21 currentrgbcolor 0 0 0 setrgbcolor 5 3 roll 2 copy pt7 5 2 roll setrgbcolor} bind def",
       "/pt36 {newpath 2 copy pt22 currentrgbcolor 0 0 0 setrgbcolor 5 3 roll 2 copy pt8 5 2 roll setrgbcolor} bind def",
       "/pt37 {newpath 2 copy pt23 currentrgbcolor 0 0 0 setrgbcolor 5 3 roll 2 copy pt9 5 2 roll setrgbcolor} bind def",
       "/pt38 {newpath 2 copy pt24 currentrgbcolor 0 0 0 setrgbcolor 5 3 roll 2 copy pt10 5 2 roll setrgbcolor} bind def",
       "/pt39 {newpath 2 copy pt25 currentrgbcolor 0 0 0 setrgbcolor 5 3 roll 2 copy pt11 5 2 roll setrgbcolor} bind def",
       "/pt40 {newpath 2 copy pt26 currentrgbcolor 0 0 0 setrgbcolor 5 3 roll 2 copy pt12 5 2 roll setrgbcolor} bind def",
       "/pt41 {newpath 2 copy pt27 currentrgbcolor 0 0 0 setrgbcolor 5 3 roll 2 copy pt13 5 2 roll setrgbcolor} bind def",
       "/pt42 {newpath 11 {2 copy} repeat /theta 90 def 6 { exch ps theta cos mul add exch ps theta sin mul add moveto lineto stroke /theta theta 60 add def} repeat } bind def",
       "/pt43 {newpath 2 copy moveto << /ShadingType 3 /ColorSpace /DeviceRGB /Coords [currentpoint exch ps 0.4 mul sub exch ps 0.3 mul add 0 currentpoint ps] /Function << /FunctionType 2 /Domain [0 1] /C1 [ currentrgbcolor ] /C0 [ 1 1 1 ] /N 1>> >> shfill } bind def",
       "/pt44 {newpath 2 copy moveto << /ShadingType 3 /ColorSpace /DeviceRGB /Coords [currentpoint exch ps 0.4 mul sub exch ps 0.3 mul add 0 currentpoint ps] /Function << /FunctionType 2 /Domain [0 1] /C1 [ currentrgbcolor ] /C0 [ 1 1 1 ] /N 1>> >> shfill currentrgbcolor 5 3 roll 0 0 0 setrgbcolor ps 0 rmoveto currentpoint exch ps sub exch ps 0 360 arc stroke 5 2 roll setrgbcolor} bind def"
 };

double eps_PointSize[N_POINTTYPES] = {0.75 , 1.0 , 1.0 , 0.75 , 1.183 , 0.75 , 1.3 , 0.75 , 1.183 , 0.75 , 1.3 , 1.0 , 1.0 , 1.183 , 1.183 , 1.183 , 1.2 , 1.0 , 1.0 , 1.0 , 1.0 ,  1.0 , 1.0 , 1.183 , 1.183 , 1.183 , 1.2 , 1.0 , 1.0};

// -------------------------------------------------
// Star types
// -------------------------------------------------

char *eps_StarTypes [N_STARTYPES] = {
// STAR TYPE 1: ALPHA ORIONIS
"/st1 {\n\
   % Inner and outer radii of main and sharp points\n\
   /lmout ps def\n\
   /lmin ps3d def\n\
   /lsout ps2 def\n\
   /lsin ps6d def\n\
   % Shading type\n\
   /shade { alphaShade } bind def\n\
\n\
   % First do the six main points\n\
   blw setlinewidth\n\
   /theta angle 90 add def\n\
   /lout ps def\n\
   /lin ps3d def\n\
   star closepath stroke\n\
\n\
   % Now do the six outer points\n\
   /theta angle 90 add def\n\
   % Theta gets a bit confusing later on, so\n\
   /thetasave theta def\n\
   tlw setlinewidth\n\
   6 {\n\
     dopoint\n\
     % Reset and increment angle for next point\n\
     /thetasave thetasave 60 add def\n\
     /theta thetasave def\n\
   } repeat\n\
\n\
   % Shading of one half of the main point\n\
   /theta angle 90 add def\n\
   /lout ps def\n\
   /lin ps3d def\n\
   6 {\n\
     gsave\n\
       2 copy lout theta cpos moveto  % Tip of first point\n\
       2 copy lineto\n\
       2 copy lin theta 30 add cpos lineto closepath clip\n\
       2 copy\n\
       8 {\n\
         ps30d theta 60 add cpos 2 copy moveto\n\
         2 copy ps theta cpos lineto closepath stroke\n\
       } repeat\n\
       pop pop\n\
     grestore\n\
     /theta theta 60 add def\n\
   } repeat\n\
\n\
   % Do the lines within the main star\n\
   tlw setlinewidth\n\
   /theta angle 120 add def\n\
   3 {\n\
     2 { 2 copy } repeat ps3d theta cpos moveto\n\
     ps3d -1 mul theta cpos lineto stroke\n\
     /theta theta 60 add def\n\
   } repeat\n\
\n\
   pop pop   % Tidy up stack\n\
} bind def\n\
",

// STAR TYPE 2: BETA ORIONIS
"/st2 {\n\
   % Inner and outer radii of main and sharp points\n\
   /lmout ps def\n\
   /lmin ps3d def\n\
   /lsout ps15 def\n\
   /lsin ps6d def\n\
   % Shading type\n\
   /shade { betaShade } bind def\n\
\n\
   % First do the six main points\n\
   blw setlinewidth\n\
   /theta angle 90 add def\n\
   /lout lmout def\n\
   /lin  lmin def\n\
   star closepath stroke\n\
\n\
   % Now do the six outer points\n\
   /theta angle 90 add def\n\
   % Theta gets a bit confusing later on, so\n\
   /thetasave theta def\n\
   tlw setlinewidth\n\
   6 {\n\
     dopoint\n\
     % Reset and increment angle for next point\n\
     /thetasave thetasave 60 add def\n\
     /theta thetasave def\n\
   } repeat\n\
\n\
   % Internal shading of one half of the main point\n\
   /theta angle 90 add def\n\
   /lout lmout def\n\
   /lin lmin def\n\
   6 {\n\
     gsave\n\
       2 copy lout theta cpos moveto  % Tip of first point\n\
       2 copy lineto\n\
       2 copy lin theta 30 add cpos lineto closepath clip\n\
       2 copy\n\
       10 {\n\
         ps40d theta 60 add cpos 2 copy moveto\n\
         2 copy ps theta cpos lineto closepath stroke\n\
       } repeat\n\
       pop pop\n\
     grestore\n\
     /theta theta 60 add def\n\
   } repeat\n\
\n\
   % Do the lines within the main star\n\
   tlw setlinewidth\n\
   /theta angle 90 add def\n\
   3 {\n\
     2 copy ps15 ps15 -2 mul theta radLine\n\
     /theta theta 30 add def\n\
     2 copy ps3d ps3d -2 mul theta radLine\n\
     /theta theta 30 add def\n\
   } repeat\n\
\n\
   pop pop   % Tidy up stack\n\
} bind def\n\
",

// STAR TYPE 3: EPSILON ORIONIS
"/st3 {\n\
   % Inner and outer radii of main and sharp points\n\
   /lmout ps def\n\
   /lmin ps3d def\n\
   /lsout ps15 def\n\
   /lsin ps6d def\n\
   % Shading type\n\
   /shade { epsilonShade } bind def\n\
\n\
   % First do the six main points\n\
   blw setlinewidth\n\
   /theta angle 90 add def\n\
   /lout lmout def\n\
   /lin  lmin def\n\
   star closepath stroke\n\
\n\
   % Now do the six outer points\n\
   /theta angle 90 add def\n\
   % Theta gets a bit confusing later on, so\n\
   /thetasave theta def\n\
   tlw setlinewidth\n\
   6 {\n\
     dopoint\n\
     % Reset and increment angle for next point\n\
     /thetasave thetasave 60 add def\n\
     /theta thetasave def\n\
   } repeat\n\
\n\
   % Internal shading of one half of the main point\n\
   /theta angle 90 add def\n\
   /lout lmout def\n\
   /lin lmin def\n\
   6 {\n\
     gsave\n\
       2 copy lout theta cpos moveto  % Tip of first point\n\
       2 copy lineto\n\
       2 copy lin theta 30 add cpos lineto closepath clip\n\
       2 copy\n\
       8 {\n\
         ps30d theta 60 add cpos 2 copy moveto\n\
         2 copy ps theta cpos lineto closepath stroke\n\
       } repeat\n\
       pop pop\n\
     grestore\n\
     /theta theta 60 add def\n\
   } repeat\n\
\n\
   % Do the lines within the main star\n\
   tlw setlinewidth\n\
   /theta angle 90 add def\n\
   3 {\n\
     2 copy ps ps4d add dup -2 mul theta radLine\n\
     /theta theta 30 add def\n\
     2 copy ps3d ps3d -2 mul theta radLine\n\
     /theta theta 30 add def\n\
   } repeat\n\
\n\
   pop pop   % Tidy up stack\n\
} bind def\n\
",

// STAR TYPE 4: Q ORIONIS
"/st4 {                    % q Orionis\n\
   % First do the six main points\n\
   tlw setlinewidth\n\
   /lmin ps3d def\n\
   /lmout ps def\n\
   /lsin ps6d def\n\
   /lsout ps15 def\n\
   /shade { qShade } bind def\n\
\n\
   /theta angle 90 add def\n\
   /lout lmout def\n\
   /lin lmin def\n\
   star closepath stroke\n\
\n\
   % Shading inside the main star\n\
   tlw setlinewidth\n\
   /theta angle 90 add def\n\
   gsave\n\
     star closepath clip\n\
     2 copy 2 copy ps15 theta 45 add cpos\n\
     4 2 roll ps15 theta 45 sub cpos\n\
     theta angle 90 sub def\n\
     31 {\n\
        ps15d theta sin mul sub exch ps15d theta cos mul sub exch  4 2 roll\n\
        ps15d theta sin mul sub exch ps15d theta cos mul sub exch  4 2 roll\n\
        4 copy moveto lineto closepath stroke\n\
     } repeat\n\
     4 { pop } repeat\n\
  grestore\n\
\n\
   % Do the thicker lines on one side of the main star\n\
   blw setlinewidth\n\
   /theta angle 90 add def\n\
   3 {\n\
     2 copy ps3d theta 30 add cpos moveto\n\
     2 copy ps theta cpos lineto stroke\n\
     /theta theta 60 add def\n\
   } repeat\n\
   3 {\n\
     2 copy ps3d theta 30 sub cpos moveto\n\
     2 copy ps theta cpos lineto stroke\n\
     /theta theta 60 add def\n\
   } repeat\n\
     %2 copy ps3d theta 30 add cpos moveto 2 copy ps theta cpos lineto stroke\n\
\n\
   % Now do the six outer points\n\
   /theta angle 90 add def\n\
   % Theta gets a bit confusing later on, so\n\
   /thetasave theta def\n\
   tlw setlinewidth\n\
   6 {\n\
     dopoint\n\
     % Reset and increment angle for next point\n\
     /thetasave thetasave 60 add def\n\
   } repeat\n\
\n\
   % Do the lines within the main star\n\
   tlw setlinewidth\n\
   /theta angle 120 add def\n\
   3 {\n\
     2 copy lmin lmin -2 mul theta radLine\n\
     /theta theta 60 add def\n\
   } repeat\n\
   /theta angle 90 add def\n\
   3 {\n\
     2 copy ps125 ps125 -2 mul theta radLine\n\
     /theta theta 60 add def\n\
   } repeat\n\
\n\
   pop pop   % Tidy up stack\n\
} bind def\n\
",

// STAR TYPE 5: M ORIONIS
"/st5 {\n\
   % First do the six main points\n\
   blw setlinewidth\n\
   /theta angle 90 add def\n\
   /lmin ps3d def\n\
   /lmout ps def\n\
   /lsin ps6d def\n\
   /lsout ps15 def\n\
   /shade { mShade } bind def\n\
\n\
   /lin lmin def\n\
   /lout lmout def\n\
   star closepath stroke\n\
\n\
   % Now do the six outer points\n\
   /theta angle 90 add def\n\
   % Theta gets a bit confusing later on, so\n\
   /thetasave theta def\n\
   tlw setlinewidth\n\
   6 {\n\
     dopoint\n\
     % Reset and increment angle for next point\n\
     /thetasave thetasave 60 add def\n\
   } repeat\n\
\n\
   % Do the lines within the main star\n\
   tlw setlinewidth\n\
   /theta angle 120 add def\n\
   3 {\n\
     2 { 2 copy } repeat ps3d theta cpos moveto\n\
     ps3d -1 mul theta cpos lineto stroke\n\
     /theta theta 60 add def\n\
   } repeat\n\
\n\
   pop pop   % Tidy up stack\n\
} bind def\n\
",

// STAR TYPE 6: TAU ORIONIS
"/st6 {\n\
   % First do the six main points\n\
   blw setlinewidth\n\
   /theta angle 90 add def\n\
   /lmin ps3d def\n\
   /lmout ps def\n\
   /lsin ps6d def\n\
   /lsout ps15 def\n\
   /shade { tauShade } bind def\n\
\n\
   /lin lmin def\n\
   /lout lmout def\n\
   star closepath stroke\n\
\n\
   % Now do the six outer points\n\
   /theta angle 90 add def\n\
   % Theta gets a bit confusing later on, so\n\
   /thetasave theta def\n\
   tlw setlinewidth\n\
   6 {\n\
     dopoint\n\
     % Reset and increment angle for next point\n\
     /thetasave thetasave 60 add def\n\
   } repeat\n\
\n\
   % Do the lines within the main star\n\
   tlw setlinewidth\n\
   /theta angle 120 add def\n\
   3 {\n\
     2 { 2 copy } repeat ps3d theta cpos moveto\n\
     ps3d -1 mul theta cpos lineto stroke\n\
     /theta theta 60 add def\n\
   } repeat\n\
\n\
   pop pop   % Tidy up stack\n\
} bind def\n\
",

// STAR TYPE 7: PI ORIONIS
"/st7 {\n\
   % Do the lines additional to tau (st6)\n\
   /theta angle 90 add def\n\
   6 {\n\
     2 copy ps theta cpos moveto\n\
     2 copy ps15 theta cpos lineto closepath stroke\n\
     /theta theta 60 add def\n\
   } repeat\n\
\n\
   st6\n\
} bind def\n\
",

// STAR TYPE 8: GAMMA ORIONIS
"/st8 {\n\
   % First do the six main points\n\
   tlw setlinewidth\n\
   /theta angle 90 add def\n\
   /lout ps def\n\
   /lin ps3d def\n\
   star closepath stroke\n\
\n\
   % Do the thicker lines on one side of the main star\n\
   blw setlinewidth\n\
   /theta angle 90 add def\n\
   3 {\n\
     2 copy ps3d theta 30 add cpos moveto\n\
     2 copy ps theta cpos lineto stroke\n\
     /theta theta 60 add def\n\
   } repeat\n\
   3 {\n\
     2 copy ps3d theta 30 sub cpos moveto\n\
     2 copy ps theta cpos lineto stroke\n\
     /theta theta 60 add def\n\
   } repeat\n\
     %2 copy ps3d theta 30 add cpos moveto 2 copy ps theta cpos lineto stroke\n\
\n\
   % Now do the six outer points\n\
   /theta angle 90 add def\n\
   % Theta gets a bit confusing later on, so\n\
   /thetasave theta def\n\
   tlw setlinewidth\n\
   6 {\n\
     gsave\n\
       % Clip out the inside of the main points\n\
       newpath\n\
       /lout ps def\n\
       /lin ps3d def\n\
       2 copy lout theta cpos moveto\n\
       point\n\
       % Then take an arc round to include the tip of the outer point\n\
       2 copy ps15 theta cpos lineto\n\
       2 copy ps15 theta theta 60 sub arcn\n\
       closepath clip\n\
\n\
       % draw the outer point\n\
       newpath\n\
       /theta theta 60 sub def\n\
       /lout ps6d def\n\
       /lin ps125 def\n\
       2 copy lout theta cpos moveto\n\
       point\n\
       closepath stroke\n\
\n\
       % Now put a mask round the smaller-angle part of the outer point\n\
       /lout ps125 def\n\
       /lin ps6d def\n\
       gsave\n\
         newpath\n\
         /theta theta 90 sub def\n\
         2 copy lout theta cpos moveto point\n\
         2 copy ps15 theta theta 60 sub arcn closepath clip\n\
         % Shade in the space between main and outer points (phew)\n\
         newpath\n\
         gammaShade\n\
       grestore\n\
\n\
       % Then repeat for the other side of the outer point\n\
       % Put a mask round the larger-angle part of the outer point\n\
       gsave\n\
         newpath\n\
         2 copy lout theta cpos moveto point\n\
         2 copy ps15 theta theta 60 sub arcn closepath clip\n\
         % Shade in the space between main and outer points\n\
         newpath\n\
         /theta theta 30 sub def\n\
         gammaShade\n\
       grestore\n\
\n\
     grestore\n\
     % Reset and increment angle for next point\n\
     /thetasave thetasave 60 add def\n\
     /theta thetasave def\n\
   } repeat\n\
\n\
   % Do the lines within the main star\n\
   tlw setlinewidth\n\
   /theta angle 120 add def\n\
   3 {\n\
     2 { 2 copy } repeat ps3d theta cpos moveto\n\
     ps3d -1 mul theta cpos lineto stroke\n\
     /theta theta 60 add def\n\
   } repeat\n\
   /theta angle 90 add def\n\
   3 {\n\
     2 { 2 copy } repeat ps125 theta cpos moveto\n\
     ps125 -1 mul theta cpos lineto stroke\n\
     /theta theta 60 add def\n\
   } repeat\n\
\n\
   pop pop   % Tidy up stack\n\
} bind def\n\
",

// STAR TYPE 9: GENERIC STAR
"/st9 {newpath 10 {2 copy} repeat ps 10 div add moveto /theta 126 def 5 { exch ps 40 div theta cos mul add exch ps 40 div theta sin mul add lineto /theta theta 36 add def exch ps 10 div theta cos mul add exch ps 10 div theta sin mul add lineto /theta theta 36 add def } repeat closepath fill} bind def"
      };

double eps_StarSize [N_STARTYPES] = {1.0,1.0,1.0,1.0,1.0,1.0,1.0,1.0,1.0};

char *eps_StarCore = "\n\
/ps2 { ps 2 mul } bind def\n\
/ps95 { ps .95 mul } bind def\n\
/ps15 { ps 1.5 mul } bind def\n\
/ps125 { ps 1.25 mul } bind def\n\
\n\
/ps10d { ps 10 div } bind def\n\
/ps15d { ps 15 div } bind def\n\
/ps20d { ps 20 div } bind def\n\
/ps30d { ps 30 div } bind def\n\
/ps40d { ps 40 div } bind def\n\
/ps8d  { ps 8 div } bind def\n\
/ps6d  { ps 6 div } bind def\n\
/ps4d  { ps 4 div } bind def\n\
/ps3d  { ps 3 div } bind def\n\
/ps2d  { ps 2 div } bind def\n\
/lw  1.5  def             % Standard line width\n\
/blw { ps 50 div lw mul } bind def\n\
/tlw { blw 3 div } bind def     % Thin line width\n\
\n\
% Subroutine library\n\
% x y l theta cpos: leave x+lcos theta, y+l sin theta on the stack\n\
/cpos { 2 copy cos mul 3 1 roll sin mul exch 4 3 roll add 3 1 roll add } bind def\n\
% x y inner length theta radLine: draws a radial line, length length from centre x,y at angle theta to vertical from inner radius \n\
/radLine {\n\
   dup 3 1 roll 6 2 roll cpos 2 copy moveto\n\
   4 2 roll cpos lineto closepath stroke\n\
} bind def\n\
\n\
% From outer point, go in to inner vertex and out to next outer point\n\
% call with centre on stack; leave stack unchanged\n\
/point {\n\
   /theta theta 30 add def\n\
   2 copy lin theta cpos lineto   % Inner vertex\n\
   /theta theta 30 add def\n\
   2 copy lout theta cpos lineto   % Outer point \n\
} bind def\n\
\n\
% Before calling star set lin=radius of inner vertices, \n\
%                         lout=radius of outer vertices\n\
%                         theta=angle of outer points to the vertical\n\
% Leaves you with a path that looks like a star\n\
/star { 2 copy\n\
   lout theta cpos moveto  % Tip of first point\n\
   6 { point } repeat\n\
} bind def\n\
\n\
% Apply some shading\n\
\n\
/qShade {\n\
   /theta theta 30 sub def\n\
   10 {\n\
     % Inner shading (at radius ps3, length ps4d)\n\
     2 copy ps3d ps4d theta radLine\n\
     % Middle shading\n\
     2 copy ps75 ps10d theta radLine\n\
     % Outer shading \n\
     2 copy ps ps10d theta radLine\n\
     /theta theta 3 add def\n\
   } repeat\n\
} bind def\n\
/gammaShade {\n\
   /theta theta 30 sub def\n\
   10 {\n\
     % Inner shading (at radius ps3, length ps4d)\n\
     2 copy ps3d ps4d theta radLine\n\
     % Outer shading (at radius ps, length ps3d)\n\
     2 copy ps125 ps10d theta radLine\n\
     /theta theta 3 add def\n\
   } repeat\n\
   % Middle shading with longer lines\n\
   /theta theta 30 sub def\n\
   3 {\n\
     2 copy ps95 ps8d theta radLine\n\
     /theta theta 30 7 div add def\n\
   } repeat\n\
   2 copy ps95 ps10d sub ps10d 3 mul theta radLine\n\
   /theta theta 30 7 div add def\n\
   3 {\n\
     2 copy ps95 ps8d theta radLine\n\
     /theta theta 30 7 div add def\n\
   } repeat\n\
} bind def\n\
\n\
/alphaShade {\n\
   qShade\n\
   /theta theta 30 sub def\n\
   15 {\n\
     2 copy ps125 ps10d theta radLine\n\
     /theta theta 2 add def\n\
   } repeat\n\
} bind def\n\
\n\
/betaShade {\n\
   qShade\n\
   /theta theta 30 sub def\n\
   7 {\n\
     2 copy ps125 ps10d theta radLine\n\
     /theta theta 2 add def\n\
   } repeat\n\
   2 copy ps125 ps10d sub ps10d 3 mul theta radLine\n\
   /theta theta 2 add def\n\
   7 {\n\
     2 copy ps125 ps10d theta radLine\n\
     /theta theta 2 add def\n\
   } repeat\n\
} bind def\n\
\n\
/epsilonShade {\n\
   /theta theta 30 sub def\n\
   4 {\n\
     2 copy ps ps8d theta radLine\n\
     /theta theta 10 3 div add def\n\
   } repeat\n\
   2 copy ps ps8d sub ps8d 3 mul theta radLine\n\
   /theta theta 10 3 div add def\n\
   4 {\n\
     2 copy ps ps8d theta radLine\n\
     /theta theta 10 3 div add def\n\
   } repeat\n\
} bind def\n\
\n\
/mShade {\n\
   /theta theta 30 sub def\n\
   6 {\n\
     % Inner shading (at radius ps3, length ps4d)\n\
     2 copy ps3d ps2d theta radLine\n\
     /theta theta 5 add def\n\
   } repeat\n\
} bind def\n\
\n\
/tauShade {\n\
   /theta theta 30 sub def\n\
   5 {\n\
     % Inner shading (at radius ps3, length ps4d)\n\
     2 copy ps3d ps4d theta radLine\n\
     % Outer shading (at radius ps, length ps3d)\n\
     2 copy ps   ps4d theta radLine\n\
     /theta theta 6 add def\n\
   } repeat\n\
} bind def\n\
\n\
% Two sorts of points, 'main' (inner, wide point) and 'sharp' (outer, thin point)\n\
% Both have inner and outer radii (so lmin, lmout, lsin, lsout)\n\
% This routine draws the outer point and puts the shading in place\n\
/dopoint {\n\
   /theta thetasave def\n\
   gsave newpath\n\
     % Clip out the inside of the main points\n\
     /lout lmout def\n\
     /lin  lmin def\n\
     2 copy lout theta cpos moveto\n\
     point\n\
     % Then take an arc round to include the tip of the outer point\n\
     2 copy lsout theta cpos lineto\n\
     2 copy lsout theta theta 60 sub arcn\n\
     closepath clip\n\
\n\
     % draw the outer point\n\
     newpath\n\
     /theta theta 60 sub def\n\
     /lout lsin def\n\
     /lin lsout def\n\
     2 copy lout theta cpos moveto\n\
     point\n\
     closepath stroke\n\
\n\
     % Now put a mask round the smaller-angle part of the outer point\n\
     /lout lsout def\n\
     /lin lsin def\n\
     gsave\n\
       newpath\n\
       /theta theta 90 sub def\n\
       2 copy lout theta cpos moveto point\n\
       2 copy lsout theta theta 60 sub arcn closepath clip\n\
       % Shade in the space between main and outer points (phew)\n\
       newpath\n\
       shade\n\
     grestore\n\
\n\
     % Then repeat for the other side of the outer point\n\
     % Put a mask round the larger-angle part of the outer point\n\
     gsave\n\
       newpath\n\
       2 copy lout theta cpos moveto point\n\
       2 copy lsout theta theta 60 sub arcn closepath clip\n\
       % Shade in the space between main and outer points\n\
       newpath\n\
       /theta theta 30 sub def\n\
       shade\n\
     grestore\n\
\n\
   grestore\n\
} bind def\n\
";

