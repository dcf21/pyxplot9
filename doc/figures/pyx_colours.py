# PYX_COLOURS.PY
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

# This python produces figures showing all of the built-in colours defined in
# PyX

import pyx
from pyx import *
from math import *

# Fetch a list of all built-in colours defined in PyX
colours = [ i for i in pyx.color.cmyk.__dict__.keys() if isinstance(pyx.color.cmyk.__dict__[i],pyx.color.cmyk) ]
h_list  = [ pyx.color.cmyk.__dict__[i].hsb().color['h']*2*pi for i in colours ]
s_list  = [ pyx.color.cmyk.__dict__[i].hsb().color['s']      for i in colours ]

# Figure 1: Colours arranges in hue/saturation space

c = canvas.canvas()
outline = path.circle(0,0,7.5) # Draw the hue axis around the edge
c.stroke(outline)

item_list = []
for name,h,s in zip(colours,h_list,s_list): # Convert hue/saturation into x/y coordinates
 x = s*sin(h)*7.5
 y = s*cos(h)*7.5
 item_list.append([x,y,name])

item_list.sort(lambda x,y: cmp(y[0],x[0])) # Draw from right-to-left

boxes = []
positions_l = [ [text.halign.boxleft , text.valign.top] , [text.halign.boxleft , text.valign.bottom] ]
positions_r = [ [text.halign.boxright, text.valign.top] , [text.halign.boxright, text.valign.bottom] ]
for x,y,name in item_list:
 if x<0: positions2 = positions_l + positions_r
 else  : positions2 = positions_r + positions_l
 for position in positions2:
  textbox = c.texrunner.text(x, y, name, position)
  collision = False
  for item in boxes:
   if textbox.bbox().intersects(item.bbox()): # If this colour collides with one already drawn, try moving label
    collision = True
    continue
  if not collision: break
 if not collision:
  shell = path.circle(x,y,0.2)
  c.fill(shell, [ pyx.color.cmyk.__dict__[name] ]) # Draw a blob of the appropriate colour
  boxes.append(textbox)
 else:
  print name

for box in boxes: c.insert(box) # ... and then label it

c.writeEPSfile("pyx_colours.eps")
c.writePDFfile("pyx_colours.pdf")

# --------------
# Figure 2: A list of colours, sorted alphabetically

c = canvas.canvas()

colours2 = colours[:]
colours2.sort()
Clen = 20 # 20 colours in each column
for i in range(len(colours2)):
 name = colours2[i]
 y =  - (i % Clen) * 0.5
 x = int(i / Clen) * 3.5
 shell = path.circle(x,y,0.2)
 c.fill(shell, [ pyx.color.cmyk.__dict__[name] ])
 c.text(x+0.5,y,name, [ text.halign.boxleft, text.valign.middle ] )

c.writeEPSfile("pyx_colours2.eps")
c.writePDFfile("pyx_colours2.pdf")

# ------------
# Figure 3: A list of colours, sorted by hue

c = canvas.canvas()

colours3 = zip(colours,h_list)
colours3.sort(lambda x,y: cmp(y[1],x[1]))
for i in range(len(colours3)):
 name = colours3[i][0]
 y =  - (i % Clen) * 0.5
 x = int(i / Clen) * 3.5
 shell = path.circle(x,y,0.2)
 c.fill(shell, [ pyx.color.cmyk.__dict__[name] ])
 c.text(x+0.5,y,name, [ text.halign.boxleft, text.valign.middle ] )

c.writeEPSfile("pyx_colours3.eps")
c.writePDFfile("pyx_colours3.pdf")

