# manpage_pyxplot_watch.py
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

# Generate manpage for Pyxplot watcher

import sys

docpath     = sys.argv[1]
author      = open("AUTHORS","r").read()
description = ""

sys.stdout.write(r"""
.\" pyxplot_watch.1
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

.\" Man page for pyxplot_watch

.TH PYXPLOT_WATCH 1
.SH NAME
pyxplot_watch \- a tool which monitors a collection of Pyxplot command scripts
and executes them whenever they are modified.
.SH SYNOPSIS
.B pyxplot_watch
[file ...]
.SH DESCRIPTION
pyxplot_watch is a part of the Pyxplot plotting package; it is a simple tool
for watching Pyxplot command script files, and executing them whenever they are
modified. It is should be followed on the commandline by a list of command
scripts which are to be watched.  Full documentation can be found in:
%s
.SH COMMAND LINE OPTIONS
  \-v, \-\-verbose: Verbose mode; output full activity log to terminal
  \-q, \-\-quiet  : Quiet mode; only output Pyxplot error messages to terminal
  \-h, \-\-help   : Display this help
  \-V, \-\-version: Display version number
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
.BR pyxplot (1), gnuplot (1)
"""%(docpath,author))

