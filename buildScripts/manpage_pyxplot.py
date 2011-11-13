# MANPAGE_PYXPLOT.PY
#
# The code in this file is part of PyXPlot
# <http://www.pyxplot.org.uk>
#
# Copyright (C) 2006-2011 Dominic Ford <coders@pyxplot.org.uk>
#               2008-2011 Ross Church
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

# Generate manpage for PyXPlot

import sys

docpath     = sys.argv[1]
author      = open("AUTHORS","r").read()
description = ""

f = open("README","r")
state = 0
for line in f.readlines():
 if   (line[0:2] == "1."): state = 1
 elif (line[0:2] == "2."): state = 2
 elif (state == 1)       : description += line

sys.stdout.write(r"""
.\" pyxplot.1
.\"
.\" The manpage in this file is part of PyXPlot
.\" <http://www.pyxplot.org.uk>
.\"
.\" Copyright (C) 2006-2011 Dominic Ford <coders@pyxplot.org.uk>
.\"               2008-2011 Ross Church
.\"
.\" $Id$
.\"
.\" PyXPlot is free software; you can redistribute it and/or modify it under the
.\" terms of the GNU General Public License as published by the Free Software
.\" Foundation; either version 2 of the License, or (at your option) any later
.\" version.
.\"
.\" You should have received a copy of the GNU General Public License along with
.\" PyXPlot; if not, write to the Free Software Foundation, Inc., 51 Franklin
.\" Street, Fifth Floor, Boston, MA  02110-1301, USA
.\"
.\" ----------------------------------------------------------------------------

.\" Man page for pyxplot

.TH PYXPLOT 1
.SH NAME
pyxplot \- a commandline data processing, graph plotting, and vector graphics
suite.
.SH SYNOPSIS
.B pyxplot
[file ...]
.SH DESCRIPTION
%s
Full documentation can be found in:
%s
.SH COMMAND LINE OPTIONS
  \-h, \-\-help:       Display this help.
  \-v, \-\-version:    Display version number.
  \-q, \-\-quiet:      Turn off initial welcome message.
  \-V, \-\-verbose:    Turn on initial welcome message.
  \-c, \-\-colour:     Use coloured highlighting of output.
  \-m, \-\-monochrome: Turn off coloured highlighting.
.SH AUTHORS
%s.
.SH CREDITS
Thanks to Dave Ansell, Rachel Holdforth, Stuart Prescott, Michael Rutter and
Matthew Smith, all of whom have made substantial contributions to the
development of PyXPlot.
.SH "SEE ALSO"
.BR pyxplot_watch (1), gnuplot (1)
"""%(description,docpath,author))

