// settingTypes.h
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

#ifndef _SETTINGTYPES_H
#define _SETTINGTYPES_H 1

// Boolean Switches

#define SW_BOOL_TRUE  10001
#define SW_BOOL_FALSE 10002

#ifndef _SETTINGTYPES_C
extern char *SW_BOOL_STR[];
extern int   SW_BOOL_ACL[];
extern int   SW_BOOL_INT[];
#endif

// On/Off Switches

#define SW_ONOFF_ON  10011
#define SW_ONOFF_OFF 10012

#ifndef _SETTINGTYPES_C
extern char *SW_ONOFF_STR[];
extern int   SW_ONOFF_ACL[];
extern int   SW_ONOFF_INT[];
#endif

// Postscript viewers

#define SW_VIEWER_GV     10020
#define SW_VIEWER_GGV    10021
#define SW_VIEWER_NULL   10022
#define SW_VIEWER_CUSTOM 10023

// Color spaces

#define SW_COLSPACE_RGB  10031
#define SW_COLSPACE_HSB  10032
#define SW_COLSPACE_CMYK 10033

#ifndef _SETTINGTYPES_C
extern char *SW_COLSPACE_STR[];
extern int   SW_COLSPACE_ACL[];
extern int   SW_COLSPACE_INT[];
#endif

// Plot Style Switches

#define SW_STYLE_POINTS         10101
#define SW_STYLE_LINES          10102
#define SW_STYLE_LINESPOINTS    10103
#define SW_STYLE_XERRORBARS     10104
#define SW_STYLE_YERRORBARS     10105
#define SW_STYLE_ZERRORBARS     10106
#define SW_STYLE_XYERRORBARS    10107
#define SW_STYLE_XZERRORBARS    10108
#define SW_STYLE_YZERRORBARS    10109
#define SW_STYLE_XYZERRORBARS   10110
#define SW_STYLE_XERRORRANGE    10111
#define SW_STYLE_YERRORRANGE    10112
#define SW_STYLE_ZERRORRANGE    10113
#define SW_STYLE_XYERRORRANGE   10114
#define SW_STYLE_XZERRORRANGE   10115
#define SW_STYLE_YZERRORRANGE   10116
#define SW_STYLE_XYZERRORRANGE  10117
#define SW_STYLE_FILLEDREGION   10118
#define SW_STYLE_YERRORSHADED   10119
#define SW_STYLE_UPPERLIMITS    10120
#define SW_STYLE_LOWERLIMITS    10121
#define SW_STYLE_DOTS           10122
#define SW_STYLE_IMPULSES       10123
#define SW_STYLE_BOXES          10124
#define SW_STYLE_WBOXES         10125
#define SW_STYLE_STEPS          10126
#define SW_STYLE_FSTEPS         10127
#define SW_STYLE_HISTEPS        10128
#define SW_STYLE_STARS          10129
#define SW_STYLE_ARROWS_HEAD    10130
#define SW_STYLE_ARROWS_NOHEAD  10131
#define SW_STYLE_ARROWS_TWOHEAD 10132
#define SW_STYLE_SURFACE        10133
#define SW_STYLE_COLORMAP       10134
#define SW_STYLE_CONTOURMAP     10135

#ifndef _SETTINGTYPES_C
extern char *SW_STYLE_STR[];
extern int   SW_STYLE_ACL[];
extern int   SW_STYLE_INT[];
#endif

// Systems in which coordinates can be specified in 'set arrow' and 'set label'

#define SW_SYSTEM_FIRST  10201
#define SW_SYSTEM_SECOND 10202
#define SW_SYSTEM_PAGE   10203
#define SW_SYSTEM_GRAPH  10204
#define SW_SYSTEM_AXISN  10205

#ifndef _SETTINGTYPES_C
extern char *SW_SYSTEM_STR[];
extern int   SW_SYSTEM_ACL[];
extern int   SW_SYSTEM_INT[];
#endif

// Arrow types understood by the 'set arrow' command

#define SW_ARROWTYPE_HEAD   10301
#define SW_ARROWTYPE_NOHEAD 10302
#define SW_ARROWTYPE_TWOWAY 10303

#ifndef _SETTINGTYPES_C
extern char *SW_ARROWTYPE_STR[];
extern int   SW_ARROWTYPE_ACL[];
extern int   SW_ARROWTYPE_INT[];
#endif

// Bitmap types understood by the 'image' command

#define SW_BITMAP_BMP 10401
#define SW_BITMAP_GIF 10402
#define SW_BITMAP_JPG 10403
#define SW_BITMAP_PNG 10404

#ifndef _SETTINGTYPES_C
extern char *SW_BITMAP_STR[];
extern int   SW_BITMAP_ACL[];
extern int   SW_BITMAP_INT[];
#endif

// Axis unit styles

#define SW_AXISUNITSTY_BRACKET 10501
#define SW_AXISUNITSTY_RATIO   10502
#define SW_AXISUNITSTY_SQUARE  10503

#ifndef _SETTINGTYPES_C
extern char *SW_AXISUNITSTY_STR[];
extern int   SW_AXISUNITSTY_ACL[];
extern int   SW_AXISUNITSTY_INT[];
#endif

// Terminal Type Switches

#define SW_TERMTYPE_X11S 13001
#define SW_TERMTYPE_X11M 13002
#define SW_TERMTYPE_X11P 13003
#define SW_TERMTYPE_PS   13004
#define SW_TERMTYPE_EPS  13005
#define SW_TERMTYPE_PDF  13006
#define SW_TERMTYPE_PNG  13007
#define SW_TERMTYPE_JPG  13008
#define SW_TERMTYPE_GIF  13009
#define SW_TERMTYPE_BMP  13010
#define SW_TERMTYPE_TIF  13011
#define SW_TERMTYPE_SVG  13012

#ifndef _SETTINGTYPES_C
extern char *SW_TERMTYPE_STR[];
extern int   SW_TERMTYPE_ACL[];
extern int   SW_TERMTYPE_INT[];
#endif

// Key Position Switches

#define SW_KEYPOS_TR 14001
#define SW_KEYPOS_TM 14002
#define SW_KEYPOS_TL 14003
#define SW_KEYPOS_MR 14004
#define SW_KEYPOS_MM 14005
#define SW_KEYPOS_ML 14006
#define SW_KEYPOS_BR 14007
#define SW_KEYPOS_BM 14008
#define SW_KEYPOS_BL 14009
#define SW_KEYPOS_ABOVE 14010
#define SW_KEYPOS_BELOW 14011
#define SW_KEYPOS_OUTSIDE 14012

#ifndef _SETTINGTYPES_C
extern char *SW_KEYPOS_STR[];
extern int   SW_KEYPOS_ACL[];
extern int   SW_KEYPOS_INT[];
#endif

// Color key position switches

#define SW_COLKEYPOS_T 14101
#define SW_COLKEYPOS_B 14102
#define SW_COLKEYPOS_L 14103
#define SW_COLKEYPOS_R 14104

#ifndef _SETTINGTYPES_C
extern char *SW_COLKEYPOS_STR[];
extern int   SW_COLKEYPOS_ACL[];
extern int   SW_COLKEYPOS_INT[];
#endif

// Tick Direction Switches

#define SW_TICDIR_IN   15001
#define SW_TICDIR_OUT  15002
#define SW_TICDIR_BOTH 15003

#ifndef _SETTINGTYPES_C
extern char *SW_TICDIR_STR[];
extern int   SW_TICDIR_ACL[];
extern int   SW_TICDIR_INT[];
#endif

// Tick Label Text Direction Switches

#define SW_TICLABDIR_HORI 15011
#define SW_TICLABDIR_VERT 15012
#define SW_TICLABDIR_ROT  15013

#ifndef _SETTINGTYPES_C
extern char *SW_TICLABDIR_STR[];
extern int   SW_TICLABDIR_ACL[];
extern int   SW_TICLABDIR_INT[];
#endif

// Axis Display Schemes

#define SW_AXISDISP_NOARR 15021
#define SW_AXISDISP_ARROW 15022
#define SW_AXISDISP_TWOAR 15024
#define SW_AXISDISP_BACKA 15026

#ifndef _SETTINGTYPES_C
extern char *SW_AXISDISP_STR[];
extern int   SW_AXISDISP_ACL[];
extern int   SW_AXISDISP_INT[];
#endif

// Axis Display Mirroring Schemes

#define SW_AXISMIRROR_AUTO       15031
#define SW_AXISMIRROR_MIRROR     15032
#define SW_AXISMIRROR_NOMIRROR   15033
#define SW_AXISMIRROR_FULLMIRROR 15034

#ifndef _SETTINGTYPES_C
extern char *SW_AXISMIRROR_STR[];
extern int   SW_AXISMIRROR_ACL[];
extern int   SW_AXISMIRROR_INT[];
#endif

// Graph Projection Schemes

#define SW_PROJ_FLAT 15041
#define SW_PROJ_GNOM 15042

#ifndef _SETTINGTYPES_C
extern char *SW_PROJ_STR[];
extern int   SW_PROJ_ACL[];
extern int   SW_PROJ_INT[];
#endif

// 2D Sampling Methods

#define SW_SAMPLEMETHOD_NEAREST 15061
#define SW_SAMPLEMETHOD_INVSQ   15062
#define SW_SAMPLEMETHOD_ML      15063

#ifndef _SETTINGTYPES_C
extern char *SW_SAMPLEMETHOD_STR[];
extern int   SW_SAMPLEMETHOD_ACL[];
extern int   SW_SAMPLEMETHOD_INT[];
#endif

// Text Horizontal Alignment

#define SW_HALIGN_LEFT  16001
#define SW_HALIGN_CENT  16002
#define SW_HALIGN_RIGHT 16003

#ifndef _SETTINGTYPES_C
extern char *SW_HALIGN_STR[];
extern int   SW_HALIGN_ACL[];
extern int   SW_HALIGN_INT[];
#endif

// Text Vertical Alignment

#define SW_VALIGN_TOP  16011
#define SW_VALIGN_CENT 16012
#define SW_VALIGN_BOT  16013

#ifndef _SETTINGTYPES_C
extern char *SW_VALIGN_STR[];
extern int   SW_VALIGN_ACL[];
extern int   SW_VALIGN_INT[];
#endif

// Colors for displaying on terminals

#define SW_TERMCOL_NOR 17001
#define SW_TERMCOL_RED 17002
#define SW_TERMCOL_GRN 17003
#define SW_TERMCOL_BRN 17004
#define SW_TERMCOL_BLU 17005
#define SW_TERMCOL_MAG 17006
#define SW_TERMCOL_CYN 17007
#define SW_TERMCOL_WHT 17008

#ifndef _SETTINGTYPES_C
extern char *SW_TERMCOL_STR[];
extern int   SW_TERMCOL_ACL[];
extern int   SW_TERMCOL_INT[];
extern char *SW_TERMCOL_TXT[];
#endif

// Schemes in which units can be displayed

#define SW_UNITSCH_SI  18001
#define SW_UNITSCH_CGS 18002
#define SW_UNITSCH_ANC 18003
#define SW_UNITSCH_IMP 18004
#define SW_UNITSCH_US  18005
#define SW_UNITSCH_PLK 18006

#ifndef _SETTINGTYPES_C
extern char *SW_UNITSCH_STR[];
extern int   SW_UNITSCH_ACL[];
extern int   SW_UNITSCH_INT[];
#endif

// Schemes in which we can display numeric results

#define SW_DISPLAY_N 18050
#define SW_DISPLAY_T 18051
#define SW_DISPLAY_L 18052

#ifndef _SETTINGTYPES_C
extern char *SW_DISPLAY_STR[];
extern int   SW_DISPLAY_ACL[];
extern int   SW_DISPLAY_INT[];
#endif

// Calendars which we can use

#define SW_CALENDAR_GREGORIAN 19001
#define SW_CALENDAR_JULIAN    19002
#define SW_CALENDAR_BRITISH   19003
#define SW_CALENDAR_FRENCH    19004
#define SW_CALENDAR_CATHOLIC  19005
#define SW_CALENDAR_RUSSIAN   19006
#define SW_CALENDAR_GREEK     19007
#define SW_CALENDAR_HEBREW    19020
#define SW_CALENDAR_ISLAMIC   19021

#ifndef _SETTINGTYPES_C
extern char *SW_CALENDAR_STR[];
extern int   SW_CALENDAR_ACL[];
extern int   SW_CALENDAR_INT[];
#endif

// Positions for the labels on piecharts

#define SW_PIEKEYPOS_AUTO    19101
#define SW_PIEKEYPOS_INSIDE  19102
#define SW_PIEKEYPOS_KEY     19103
#define SW_PIEKEYPOS_OUTSIDE 19104

#ifndef _SETTINGTYPES_C
extern char *SW_PIEKEYPOS_STR[];
extern int   SW_PIEKEYPOS_ACL[];
extern int   SW_PIEKEYPOS_INT[];
#endif

#include "coreUtils/errorReport.h"

void *ppl_fetchSettingName   (pplerr_context *context, int id, int *id_list, void *name_list, const int name_list_size);
int   ppl_fetchSettingByName (pplerr_context *context, char *name, int *id_list, char **name_list);

#endif
