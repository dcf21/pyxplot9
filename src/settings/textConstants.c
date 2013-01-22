// textConstants.c
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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "stringTools/asciidouble.h"
#include "stringTools/strConstants.h"
#include "settings/textConstants.h"
#include "pplConstants.h"

// Contains text messages which pyxplot displays

char ppltxt_version            [SSTR_LENGTH];
char ppltxt_version_underline  [SSTR_LENGTH];
char ppltxt_help               [LSTR_LENGTH];
char ppltxt_welcome            [LSTR_LENGTH];
char ppltxt_invalid            [LSTR_LENGTH];
char ppltxt_valid_set_options  [LSTR_LENGTH];
char ppltxt_set_noword         [LSTR_LENGTH];
char ppltxt_unset_noword       [LSTR_LENGTH];
char ppltxt_set                [LSTR_LENGTH];
char ppltxt_unset              [LSTR_LENGTH];
char ppltxt_show               [LSTR_LENGTH];

void ppltxt_init()
{
sprintf(ppltxt_version, "Pyxplot %s", VERSION);

sprintf(ppltxt_help   , "%s\n\
%s\n\
\n\
Usage: pyxplot <options> <filelist>\n\
  -h, --help:       Display this help.\n\
  -v, --version:    Display version number.\n\
  -q, --quiet:      Turn off initial welcome message.\n\
  -V, --verbose:    Turn on initial welcome message.\n\
  -c, --color:      Use colored highlighting of output.\n\
  -m, --monochrome: Turn off colored highlighting.\n\
\n\
A brief introduction to Pyxplot can be obtained by typing 'man pyxplot'; the\n\
full Users' Guide can be found in the file:\n\
%s%spyxplot.pdf\n\
\n\
Extensive documentation is also available at <http://www.pyxplot.org.uk>, which\n\
also provides contact information for bug reports.\n", ppltxt_version, ppl_strUnderline(ppltxt_version, ppltxt_version_underline), DOCDIR, PATHLINK);

sprintf(ppltxt_welcome, "\n\
                        _       _      PYXPLOT\n\
 _ __  _   ___  ___ __ | | ___ | |_    Version %s\n\
| '_ \\| | | \\ \\/ / '_ \\| |/ _ \\| __|   %s\n\
| |_) | |_| |>  <| |_) | | (_) | |_\n\
| .__/ \\__, /_/\\_\\ .__/|_|\\___/ \\__|   Copyright (C) 2006-2012 Dominic Ford\n\
|_|    |___/     |_|                                 2008-2012 Ross Church\n\
\n\
For documentation and more information, see <http://www.pyxplot.org.uk>.\n\
", VERSION, DATE);

sprintf(ppltxt_invalid, "\n\
 %%s\n\
/|\\ \n\
 |\n\
Error: Unrecognised command.\n\
");

sprintf(ppltxt_valid_set_options, "\n\
'arrow', 'autoscale', 'axescolor', 'axis', 'axisunitstyle', 'backup', 'bar',\n\
'binorigin', 'binwidth', 'boxfrom', 'boxwidth', 'c1format', 'c1label',\n\
'calendar', 'clip', 'colmap', 'colkey', 'contours', 'c<n>range', 'data style',\n\
'display', 'filter', 'fontsize', 'function style', 'grid', 'gridmajcolor',\n\
'gridmincolor', 'key', 'keycolumns', 'label', 'linearscale', 'linewidth',\n\
'logscale', 'multiplot', 'noarrow', 'noaxis', 'nobackup', 'nodisplay',\n\
'nogrid', 'nokey', 'nolabel', 'nologscale', 'nomultiplot', 'nostyle',\n\
'notitle', 'no<m>[xyz]<n>format', 'no<m>[xyz]<n>tics', 'numerics', 'origin',\n\
'output', 'palette', 'papersize', 'pointlinewidth', 'pointsize', 'preamble',\n\
'samples', 'seed', 'size', 'size noratio', 'size ratio', 'size square',\n\
'style', 'terminal', 'textcolor', 'texthalign', 'textvalign', 'title',\n\
'trange', 'unit', 'urange', 'view', 'viewer', 'vrange', 'width',\n\
'[xyz]<n>format', '[xyz]<n>label', '[xyz]<n>range', '<m>[xyz]<n>tics'\n\
");

sprintf(ppltxt_set_noword, "\n\
Set options which Pyxplot recognises are: [] = choose one, <> = optional\n\
%s\n\
", ppltxt_valid_set_options);

sprintf(ppltxt_unset_noword, "\n\
Unset options which Pyxplot recognises are: [] = choose one, <> = optional\n\
\n\
'arrow', 'autoscale', 'axescolor', 'axis', 'axisunitstyle', 'backup', 'bar',\n\
'binorigin', 'binwidth', 'boxfrom', 'boxwidth', 'c1format', 'c1label',\n\
'calendar', 'clip', 'colmap', 'colkey', 'contours', 'c<n>range', 'data style',\n\
'display', 'filter', 'fontsize', 'function style', 'grid', 'gridmajcolor',\n\
'gridmincolor', 'key', 'keycolumns', 'label', 'linewidth', 'logscale',\n\
'multiplot', 'noarrow', 'noaxis', 'nobackup', 'nodisplay', 'nogrid', 'nokey',\n\
'nolabel', 'nologscale', 'nomultiplot', 'notitle', 'no<m>[xyz]<n>tics',\n\
'numerics', 'origin', 'output', 'palette', 'papersize', 'pointlinewidth',\n\
'pointsize', 'preamble', 'samples', 'size', 'style', 'terminal', 'textcolor',\n\
'texthalign', 'textvalign', 'title', 'trange', 'unit', 'urange', 'view',\n\
'viewer', 'vrange', 'width', '[xyz]<n>format', '[xyz]<n>label',\n\
'[xyz]<n>range', '<m>[xyz]<n>tics'\n\
");

sprintf(ppltxt_set, "\n\
Error: Invalid set option '%%s'.\n\
\n\
%s", ppltxt_set_noword);

sprintf(ppltxt_unset, "\n\
Error: Invalid unset option '%%s'.\n\
\n\
%s", ppltxt_unset_noword);

sprintf(ppltxt_show, "\n\
Valid 'show' options are:\n\
\n\
'all', 'arrows', 'axes', 'functions', 'settings', 'labels', 'linestyles',\n\
'units', 'userfunctions', 'variables'\n\
\n\
or any of the following set options:\n\
'arrow', 'autoscale', 'axescolor', 'axis', 'axisunitstyle', 'backup', 'bar',\n\
'binorigin', 'binwidth', 'boxfrom', 'boxwidth', 'c1format', 'c1label',\n\
'calendar', 'clip', 'colmap', 'colkey', 'contours', 'c<n>range', 'data style',\n\
'display', 'filter', 'fontsize', 'function style', 'grid', 'gridmajcolor',\n\
'gridmincolor', 'key', 'keycolumns', 'label', 'linearscale', 'linewidth',\n\
'logscale', 'multiplot', 'numerics', 'origin', 'output', 'palette',\n\
'papersize', 'pointlinewidth', 'pointsize', 'preamble', 'samples', 'seed',\n\
'size', 'size noratio', 'size ratio', 'size square', 'style', 'terminal',\n\
'textcolor', 'texthalign', 'textvalign', 'title', 'trange', 'unit', 'urange',\n\
'view', 'viewer', 'vrange', 'width', '[xyz]<n>format', '[xyz]<n>label',\n\
'[xyz]<n>range', '<m>[xyz]<n>tics'\n\
"); }

