# manpage_pyxplot.py
#
# The code in this file is part of Pyxplot
# <http://www.pyxplot.org.uk>
#
# Copyright (C) 2006-2012 Dominic Ford <coders@pyxplot.org.uk>
#               2008-2012 Ross Church
#
# $Id$
#
# Pyxplot is free software; you can redistribute it and/or modify it under the
# terms of the GNU General Public License as published by the Free Software
# Foundation; either version 2 of the License, or (at your option) any later
# version.
#
# You should have received a copy of the GNU General Public License along with
# Pyxplot; if not, write to the Free Software Foundation, Inc., 51 Franklin
# Street, Fifth Floor, Boston, MA  02110-1301, USA

# ----------------------------------------------------------------------------

# Generate manpage for Pyxplot

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
.\" The manpage in this file is part of Pyxplot
.\" <http://www.pyxplot.org.uk>
.\"
.\" Copyright (C) 2006-2012 Dominic Ford <coders@pyxplot.org.uk>
.\"               2008-2012 Ross Church
.\"
.\" $Id$
.\"
.\" Pyxplot is free software; you can redistribute it and/or modify it under the
.\" terms of the GNU General Public License as published by the Free Software
.\" Foundation; either version 2 of the License, or (at your option) any later
.\" version.
.\"
.\" You should have received a copy of the GNU General Public License along with
.\" Pyxplot; if not, write to the Free Software Foundation, Inc., 51 Franklin
.\" Street, Fifth Floor, Boston, MA  02110-1301, USA
.\"
.\" ----------------------------------------------------------------------------

.\" Man page for pyxplot

.TH PYXPLOT 1
.SH NAME
pyxplot \- a multi-purpose command-line data processing, vector graphics and graph-plotting tool.
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
  \-c, \-\-color:      Use colored highlighting of output.
  \-m, \-\-monochrome: Turn off coloured highlighting.
.SH AUTHORS
%s.
.SH BUGS AND USER FORUMS
To report bugs in Pyxplot, or to meet other Pyxplot users in our forums, please
visit us on sourceforge: <http://sourceforge.net/projects/pyxplot/>.
.SH CREDITS
Matthew Smith, Michael Rutter, Zoltan Voros and John Walker have all
contributed code to Pyxplot.  We welcome bug reports, which can be submitted to
our project page on Sourceforge, and thank the many testers who have already
made significant contributions to the project by helping us to track down bugs.
.SH "SEE ALSO"
.BR pyxplot_watch (1), gnuplot (1)
"""%(description,docpath,author))

