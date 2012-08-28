#!/bin/sh

# pyxplot_timehelper.sh
#
# The code in this file is part of PyXPlot
# <http://www.pyxplot.org.uk>
#
# Copyright (C) 2006-2012 Dominic Ford <coders@pyxplot.org.uk>
#               2008-2012 Ross Church
#
# $Id$
#
# PyXPlot is free software; you can redistribute it and/or modify it under the
# terms of the GNU General Public License as published by the Free Software
# Foundation; either version 2 of the License, or (at your option) any later
# version.
#
# You should have received a copy of the GNU General Public License along with
# PyXPlot; if not, write to the Free Software Foundation, Inc., 51 Franklin
# Street, Fifth Floor, Boston, MA  02110-1301, USA

# ----------------------------------------------------------------------------

# This is an example input filter for PyXPlot which uses some simple regular
# expressions to replace the names of days of the week and months of the year
# with numbers, so that textual dates can be parsed by PyXPlot.

if test "$1" != ""
 then
  if test -e $1
   then
    if test "$2" = ""
     then
      cat $1 | sed -e 's/:/ /g' -e 's:/: :g' -e 's/[Mm]on\(day\)\?/ 1 /g' -e 's/[Tt]ue\(s\(day\)\?\)\?/ 2 /g' -e 's/[Ww]ed\(nesday\)\?/ 3 /g' -e 's/[Tt]hu\(r\(s\(day\)\?\)\?\)\?/ 4 /g' -e 's/[Ff]ri\(day\)\?/ 5 /g' -e 's/[Ss]at\(urday\)\?/ 6 /g' -e 's/[Ss]un\(day\)\?/ 7 /g' -e 's/[Jj]an\(uary\)\?/ 1 /g' -e 's/[Ff]eb\(ruary\)\?/ 2 /g' -e 's/[Mm]ar\(ch\)\?/ 3 /g' -e 's/[Aa]pr\(il\)\?/ 4 /g' -e 's/[Mm]ay/ 5 /g' -e 's/[Jj]une\?/ 6 /g' -e 's/[Jj]uly\?/ 7 /g' -e 's/[Aa]ug\(ust\)\?/ 8 /g' -e 's/[Ss]ep\(t\(ember\)\?\)\?/ 9 /g' -e 's/[Oo]ct\(ober\)\?/ 10 /g' -e 's/[Nn]ov\(ember\)\?/ 11 /g' -e 's/[Dd]ec\(ember\)\?/ 12 /g' -e 's/\[/ [ /g' -e 's/\]/ ] /g'
     else
      echo "pyxplot_timehelper should be passed the name of only one file to process on the commandline." >&2
    fi
   else
    echo "pyxplot_timehelper cannot open input file." >&2
  fi
 else
  echo "pyxplot_timehelper should be passed the name of a file to process on the commandline." >&2
fi  

