// settingTypes.c
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

#define _SETTINGTYPES_C 1

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "stringTools/asciidouble.h"
#include "coreUtils/errorReport.h"
#include "settings/settingTypes.h"

char *SW_BOOL_STR[] = {"True"       , "False"      };
int   SW_BOOL_ACL[] = {1            , 1            };
int   SW_BOOL_INT[] = {SW_BOOL_TRUE , SW_BOOL_FALSE , -1};

char *SW_ONOFF_STR[] = {"On"        , "Off"       };
int   SW_ONOFF_ACL[] = {2           , 2           };
int   SW_ONOFF_INT[] = {SW_ONOFF_ON , SW_ONOFF_OFF , -1};

char *SW_COLSPACE_STR[] = {"rgb"           , "hsb"           , "cmyk"           };
int   SW_COLSPACE_ACL[] = {1               , 1               , 1                };
int   SW_COLSPACE_INT[] = {SW_COLSPACE_RGB , SW_COLSPACE_HSB , SW_COLSPACE_CMYK , -1};

char *SW_STYLE_STR[] = {"points"        , "lines"        , "LinesPoints"        , "XErrorBars"        , "YErrorBars"        , "ZErrorBars"        , "XYErrorBars"        , "XZErrorBars"        , "YZErrorBars"        , "XYZErrorBars"        , "XErrorRange"        , "YErrorRange"        , "ZErrorRange"        , "XYErrorRange"        , "XZErrorRange"        , "YZErrorRange"        , "XYZErrorRange"        , "FilledRegion"        , "YErrorShaded"        , "UpperLimits"        , "LowerLimits"        , "dots"        , "impulses"        , "boxes"        , "wboxes"        , "steps"        , "fsteps"        , "histeps"        , "stars"       , "arrows_head"        , "arrows_nohead"        , "arrows_twohead"        , "surface"        , "colormap"        , "contourmap"        };
int   SW_STYLE_ACL[] = {1               , 1              , 6                    , 1                   , 1                   , 1                   , 2                    , 2                    , 2                    , 3                     , 7                    , 7                    , 7                    , 8                     , 8                     , 8                     , 9                      , 3                     , 7                     , 1                    , 2                    , 1             , 1                 , 1              , 1               , 2              , 2               , 2                , 3             , 1                    , 7                      , 8                       , 2                , 3                 , 3                   , -1};
int   SW_STYLE_INT[] = {SW_STYLE_POINTS , SW_STYLE_LINES , SW_STYLE_LINESPOINTS , SW_STYLE_XERRORBARS , SW_STYLE_YERRORBARS , SW_STYLE_ZERRORBARS , SW_STYLE_XYERRORBARS , SW_STYLE_XZERRORBARS , SW_STYLE_YZERRORBARS , SW_STYLE_XYZERRORBARS , SW_STYLE_XERRORRANGE , SW_STYLE_YERRORRANGE , SW_STYLE_ZERRORRANGE , SW_STYLE_XYERRORRANGE , SW_STYLE_XZERRORRANGE , SW_STYLE_YZERRORRANGE , SW_STYLE_XYZERRORRANGE , SW_STYLE_FILLEDREGION , SW_STYLE_YERRORSHADED , SW_STYLE_UPPERLIMITS , SW_STYLE_LOWERLIMITS , SW_STYLE_DOTS , SW_STYLE_IMPULSES , SW_STYLE_BOXES , SW_STYLE_WBOXES , SW_STYLE_STEPS , SW_STYLE_FSTEPS , SW_STYLE_HISTEPS , SW_STYLE_STARS, SW_STYLE_ARROWS_HEAD , SW_STYLE_ARROWS_NOHEAD , SW_STYLE_ARROWS_TWOHEAD , SW_STYLE_SURFACE , SW_STYLE_COLORMAP , SW_STYLE_CONTOURMAP , -1};

char *SW_SYSTEM_STR[] = {"first"         , "second"         , "page"         , "graph"         , "axis"          };
int   SW_SYSTEM_ACL[] = {1               , 1                , 1              , 1               , 1               };
int   SW_SYSTEM_INT[] = {SW_SYSTEM_FIRST , SW_SYSTEM_SECOND , SW_SYSTEM_PAGE , SW_SYSTEM_GRAPH , SW_SYSTEM_AXISN , -1};

char *SW_ARROWTYPE_STR[] = {"head"            , "nohead"            , "twoway"            };
int   SW_ARROWTYPE_ACL[] = {1                 , 1                   , 1                   };
int   SW_ARROWTYPE_INT[] = {SW_ARROWTYPE_HEAD , SW_ARROWTYPE_NOHEAD , SW_ARROWTYPE_TWOWAY , -1};

char *SW_BITMAP_STR[] = {"bmp"        , "gif"        , "jpeg"       , "png"        };
int   SW_BITMAP_ACL[] = {1            , 1            , 1            , 1            };
int   SW_BITMAP_INT[] = {SW_BITMAP_BMP, SW_BITMAP_GIF, SW_BITMAP_JPG, SW_BITMAP_PNG, -1};

char *SW_AXISUNITSTY_STR[] = {"bracketed"           , "ratio"             , "squarebracketed"    };
int   SW_AXISUNITSTY_ACL[] = {1                     , 1                   , 1                    };
int   SW_AXISUNITSTY_INT[] = {SW_AXISUNITSTY_BRACKET, SW_AXISUNITSTY_RATIO, SW_AXISUNITSTY_SQUARE, -1};

char *SW_TERMTYPE_STR[] = {"X11_SingleWindow", "X11_MultiWindow" , "X11_Persist"    , "ps"           , "eps"           , "pdf"           , "png"           , "jpg"           , "gif"           , "bmp"           , "tif"          , "svg"           };
int   SW_TERMTYPE_ACL[] = {1                 , 5                 , 5                , 1              , 1               , 2               , 2               , 1               , 1               , 1               , 1              , 1               };
int   SW_TERMTYPE_INT[] = {SW_TERMTYPE_X11S  , SW_TERMTYPE_X11M  , SW_TERMTYPE_X11P , SW_TERMTYPE_PS , SW_TERMTYPE_EPS , SW_TERMTYPE_PDF , SW_TERMTYPE_PNG , SW_TERMTYPE_JPG , SW_TERMTYPE_GIF , SW_TERMTYPE_BMP, SW_TERMTYPE_TIF , SW_TERMTYPE_SVG , -1};

char *SW_KEYPOS_STR[] = {"top right"  , "top xcentre" , "top left"   , "ycentre right" , "ycentre xcentre" , "ycentre left" , "bottom right" , "bottom xcentre" , "bottom left" , "above"         , "below"         , "outside"         };
int   SW_KEYPOS_ACL[] = {5            , 5             , 5            , 9               , 9                 , 9              , 8              , 8                , 8             , 1               , 2               , 1                 };
int   SW_KEYPOS_INT[] = {SW_KEYPOS_TR , SW_KEYPOS_TM  , SW_KEYPOS_TL , SW_KEYPOS_MR    , SW_KEYPOS_MM      , SW_KEYPOS_ML   , SW_KEYPOS_BR   , SW_KEYPOS_BM     , SW_KEYPOS_BL  , SW_KEYPOS_ABOVE , SW_KEYPOS_BELOW , SW_KEYPOS_OUTSIDE , -1};

char *SW_COLKEYPOS_STR[] = {"top"          , "bottom"       , "left"         , "right"        };
int   SW_COLKEYPOS_ACL[] = {1              , 1              , 1              , 1              };
int   SW_COLKEYPOS_INT[] = {SW_COLKEYPOS_T , SW_COLKEYPOS_B , SW_COLKEYPOS_L , SW_COLKEYPOS_R , -1};

char *SW_TICDIR_STR[] = {"inwards"    , "outwards"    , "both"         };
int   SW_TICDIR_ACL[] = {1            , 1             , 1              };
int   SW_TICDIR_INT[] = {SW_TICDIR_IN , SW_TICDIR_OUT , SW_TICDIR_BOTH , -1};

char *SW_TICLABDIR_STR[] = {"horizontal"      , "vertical"        , "rotate"         };
int   SW_TICLABDIR_ACL[] = {1                 , 1                 , 1                };
int   SW_TICLABDIR_INT[] = {SW_TICLABDIR_HORI , SW_TICLABDIR_VERT , SW_TICLABDIR_ROT , -1};

char *SW_AXISDISP_STR[] = {"noarrow"         , "arrow"           , "twowayarrow"     , "reversearrow"    };
int   SW_AXISDISP_ACL[] = {1                 , 1                 , 1                 , 1                 };
int   SW_AXISDISP_INT[] = {SW_AXISDISP_NOARR , SW_AXISDISP_ARROW , SW_AXISDISP_TWOAR , SW_AXISDISP_BACKA , -1};

char *SW_AXISMIRROR_STR[] = {"automirrored"     , "mirrored"           , "nomirror"             , "fullmirrored"           };
int   SW_AXISMIRROR_ACL[] = {1                  , 1                    , 1                      , 1                        };
int   SW_AXISMIRROR_INT[] = {SW_AXISMIRROR_AUTO , SW_AXISMIRROR_MIRROR , SW_AXISMIRROR_NOMIRROR , SW_AXISMIRROR_FULLMIRROR , -1};

char *SW_PROJ_STR[] = {"flat"       , "gnomonic"   };
int   SW_PROJ_ACL[] = {1            , 1            };
int   SW_PROJ_INT[] = {SW_PROJ_FLAT , SW_PROJ_GNOM , -1};

char *SW_SAMPLEMETHOD_STR[] = {"NearestNeighbour"      , "InverseSquare"       , "MonaghanLattanzio" };
int   SW_SAMPLEMETHOD_ACL[] = {1                       , 1                     , 1                   };
int   SW_SAMPLEMETHOD_INT[] = {SW_SAMPLEMETHOD_NEAREST , SW_SAMPLEMETHOD_INVSQ , SW_SAMPLEMETHOD_ML  , -1};

char *SW_HALIGN_STR[] = {"left"         , "centre"       , "right"         };
int   SW_HALIGN_ACL[] = {1              , 1              , 1               };
int   SW_HALIGN_INT[] = {SW_HALIGN_LEFT , SW_HALIGN_CENT , SW_HALIGN_RIGHT , -1};

char *SW_VALIGN_STR[] = {"top"         , "centre"       , "bottom"      };
int   SW_VALIGN_ACL[] = {1             , 1              , 1             };
int   SW_VALIGN_INT[] = {SW_VALIGN_TOP , SW_VALIGN_CENT , SW_VALIGN_BOT , -1};

char *SW_TERMCOL_STR[] = {"normal"       , "red"          , "green"        , "amber"        , "blue"         , "magenta"      , "cyan"         , "white"        };
int   SW_TERMCOL_ACL[] = {1              , 1              , 1              , 2              , 1              , 1              , 1              , 1              };
int   SW_TERMCOL_INT[] = {SW_TERMCOL_NOR , SW_TERMCOL_RED , SW_TERMCOL_GRN , SW_TERMCOL_BRN , SW_TERMCOL_BLU , SW_TERMCOL_MAG , SW_TERMCOL_CYN , SW_TERMCOL_WHT , -1};
char *SW_TERMCOL_TXT[] = {"\x1b[0m"      , "\x1b[01;31m"  , "\x1b[01;32m"  , "\x1b[01;33m"  , "\x1b[01;34m"  , "\x1b[01;35m"  , "\x1b[01;36m"  , "\x1b[01;37m"  };

char *SW_UNITSCH_STR[] = {"si"          , "cgs"          , "ancient"      , "imperial"     , "USCustomary" , "planck"       };
int   SW_UNITSCH_INT[] = {SW_UNITSCH_SI , SW_UNITSCH_CGS , SW_UNITSCH_ANC , SW_UNITSCH_IMP , SW_UNITSCH_US , SW_UNITSCH_PLK , -1};
int   SW_UNITSCH_ACL[] = {1             , 1              , 1              , 1              , 1             , 1              , -1};

char *SW_DISPLAY_STR[] = {"natural"     , "typeable"     , "latex"        };
int   SW_DISPLAY_ACL[] = {1             , 1              , 1              };
int   SW_DISPLAY_INT[] = {SW_DISPLAY_N  , SW_DISPLAY_T   , SW_DISPLAY_L   , -1};

char *SW_CALENDAR_STR[] = {"Gregorian"          , "Julian"          , "British"          , "French"          , "Papal"             , "Russian"          , "Greek"          , "Hebrew"          , "Islamic"          };
int   SW_CALENDAR_INT[] = {SW_CALENDAR_GREGORIAN, SW_CALENDAR_JULIAN, SW_CALENDAR_BRITISH, SW_CALENDAR_FRENCH, SW_CALENDAR_CATHOLIC, SW_CALENDAR_RUSSIAN, SW_CALENDAR_GREEK, SW_CALENDAR_HEBREW, SW_CALENDAR_ISLAMIC, -1};
int   SW_CALENDAR_ACL[] = {1                    , 1                 , 1                  , 1                 , 1                   , 1                  , 5                , 1                 , 1                  , -1};

char *SW_PIEKEYPOS_STR[] = {"auto"           , "inside"           , "key"           , "outside"           };
int   SW_PIEKEYPOS_INT[] = {SW_PIEKEYPOS_AUTO, SW_PIEKEYPOS_INSIDE, SW_PIEKEYPOS_KEY, SW_PIEKEYPOS_OUTSIDE, -1};
int   SW_PIEKEYPOS_ACL[] = {1                , 1                  , 1               , 1                   , -1};

void *ppl_fetchSettingName(pplerr_context *context, int id, int *id_list, void *name_list, const int name_list_size)
 {
  int first;
  static int latch=0;
  static char *dummyout = "";
  first = *id_list;
  while(1)
   {
    if (*id_list == id) return name_list;
    if (*id_list == -1)
     {
      if (latch==1) return dummyout; // Prevent recursive calling
      latch=1;
      sprintf(context->tempErrStr, "Setting with illegal value %d; should have had a value of type %d.", id, first);
      ppl_fatal(context,__FILE__, __LINE__, context->tempErrStr);
     }
    id_list++; name_list+=name_list_size;
   }
  if (latch==1) return dummyout;
  latch=1;
  sprintf(context->tempErrStr, "Setting has illegal value %d.", id);
  ppl_fatal(context,__FILE__, __LINE__, context->tempErrStr);
  return NULL;
 }

int ppl_fetchSettingByName(pplerr_context *context, char *name, int *id_list, char **name_list)
 {
  while(1)
   {
    if (*id_list == -1) return -1;
    if (ppl_strCmpNoCase(name, *name_list) == 0) return *id_list;
    id_list++; name_list++;
   }
  return -1;
 }

