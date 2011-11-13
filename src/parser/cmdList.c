// cmdList.c
//
// The code in this file is part of PyXPlot
// <http://www.pyxplot.org.uk>
//
// Copyright (C) 2006-2012 Dominic Ford <coders@pyxplot.org.uk>
//               2008-2011 Ross Church
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

// The language used in this file is generally refered to as RE++, version 2.
// The atoms of the syntax are summarised below.
//
// =                      If a match fails after this point generate an error rather than continuing
// text@3:var             Match "text", abbreviated to >= 3 letters, and place in variable var
// text@n                 No space after "text", which must be quoted in full
// { ... }                Optionally match ...
// < ...a... | ...b... >  Match exactly one of ...a... or ...b...
// ( ...a... ~ ...b... )  Match ...a..., ...b..., etc., in any order, either 1 or 0 times each
// %q:variable            Match a quoted string ('...' or "...") and place it in variable
// %S:variable            Match an unquoted string and place it in variable
// %f:variable            Match a float and place it in variable
// %d:variable            Match an integer and place it in variable

// List of commands recognised by PyXPlot

// set@2:directive { item@1 %d:editno } projection@3:set_option = < flat@1:projection | gnomonic@1:projection >
// unset@2:directive { item@1 %d:editno } projection@3:set_option =

char ppl_cmdList[] = "\
{ < let@3 > } %v:varname \\=~@n:directive:var_set_regex = s@n %r:regex\n\
{ < let@3 > } %v:varname \\=@n:directive:var_set = { < %fi:numeric_value | %q:string_value > }\n\
%v:function_name \\(@n [ %v:argument_name ]:@argument_list, \\)@n [ \\[@n { { < %fu:min | \\*@n:minauto > } < :@n | to@n > { < %fu:max | \\*@n:maxauto > } } \\]@n ]:@range_list \\=@n:directive:func_set = { %e:definition }\n\
arc@3:directive = { item@1 %d:editno } { at@1 } %fu:x ,@n %fu:y radius@1 %fu:r from@1 %fu:angle1 to@1 %fu:angle2 { with@1 ( < linetype@5 | lt@2 > %d:linetype ~ < linewidth@5 | lw@2 > %f:linewidth ~ style@2 %d:style_number ~ < colour@1 | color@1 > < rgb@n %fi:colourR \\:@n %fi:colourG \\:@n %fi:colourB | hsb@n %fi:colourH \\:@n %fi:colourS \\:@n %fi:colourB | cmyk@n %fi:colourC \\:@n %fi:colourM \\:@n %fi:colourY \\:@n %fi:colourK | %e:colour > ~ < fillcolour@1 | fillcolor@1 | fc@2 > < rgb@n %fi:fillcolourR \\:@n %fi:fillcolourG \\:@n %fi:fillcolourB | hsb@n %fi:fillcolourH \\:@n %fi:fillcolourS \\:@n %fi:fillcolourB | cmyk@n %fi:fillcolourC \\:@n %fi:fillcolourM \\:@n %fi:fillcolourY \\:@n %fi:fillcolourK | %e:fillcolour > ) }\n\
< line@2:directive | arrow@2:directive > = { item@1 %d:editno } { from@1 } %fu:x1 ,@n %fu:y1 to@1 %fu:x2 ,@n %fu:y2 { with@1 ( < linetype@5 | lt@2 > %d:linetype ~ < linewidth@5 | lw@2 > %f:linewidth ~ style@2 %d:style_number ~ < colour@1 | color@1 > < rgb@n %fi:colourR \\:@n %fi:colourG \\:@n %fi:colourB | hsb@n %fi:colourH \\:@n %fi:colourS \\:@n %fi:colourB | cmyk@n %fi:colourC \\:@n %fi:colourM \\:@n %fi:colourY \\:@n %fi:colourK | %e:colour > ~ < nohead@2:arrow_style | head@2:arrow_style | twohead@2:arrow_style:twoway | twoway@2:arrow_style > ) }\n\
assert@2:directive = < version@4 < \\>=@n:gtreq | \\<@n:lt > < %S:version | %q:version > | %fi:expr > { %q:message }\n\
< box@2:directive | rectangle@2:directive:box > = { item@1 %d:editno } < from@1 %fu:x1 ,@n %fu:y1 to@1 %fu:x2 ,@n %fu:y2 { rotate@1 %fu:rotation } | at@1 %fu:x1 ,@n %fu:y1 ( width@1 %fu:width ~ height@1 %fu:height ~ rotate@1 %fu:rotation ) > { with@1 ( < linetype@5 | lt@2 > %d:linetype ~ < linewidth@5 | lw@2 > %f:linewidth ~ style@2 %d:style_number ~ < colour@1 | color@1 > < rgb@n %fi:colourR \\:@n %fi:colourG \\:@n %fi:colourB | hsb@n %fi:colourH \\:@n %fi:colourS \\:@n %fi:colourB | cmyk@n %fi:colourC \\:@n %fi:colourM \\:@n %fi:colourY \\:@n %fi:colourK | %e:colour > ~ < fillcolour@1 | fillcolor@1 | fc@2 > < rgb@n %fi:fillcolourR \\:@n %fi:fillcolourG \\:@n %fi:fillcolourB | hsb@n %fi:fillcolourH \\:@n %fi:fillcolourS \\:@n %fi:fillcolourB | cmyk@n %fi:fillcolourC \\:@n %fi:fillcolourM \\:@n %fi:fillcolourY \\:@n %fi:fillcolourK | %e:fillcolour > ) }\n\
break@2:directive = { %s:loopname }\n\
call@2:directive = %v:subroutine_name \\(@n [ < %fi:argument | %q:string_argument > ]:@argument_list, \\)@n\n\
cd@2:directive = [ < %q:directory | %S:directory > ]:path\n\
circle@2:directive = { item@1 %d:editno } { at@1 } %fu:x ,@n %fu:y radius@1 %fu:r { with@1 ( < linetype@5 | lt@2 > %d:linetype ~ < linewidth@5 | lw@2 > %f:linewidth ~ style@2 %d:style_number ~ < colour@1 | color@1 > < rgb@n %fi:colourR \\:@n %fi:colourG \\:@n %fi:colourB | hsb@n %fi:colourH \\:@n %fi:colourS \\:@n %fi:colourB | cmyk@n %fi:colourC \\:@n %fi:colourM \\:@n %fi:colourY \\:@n %fi:colourK | %e:colour > ~ < fillcolour@1 | fillcolor@1 | fc@2 > < rgb@n %fi:fillcolourR \\:@n %fi:fillcolourG \\:@n %fi:fillcolourB | hsb@n %fi:fillcolourH \\:@n %fi:fillcolourS \\:@n %fi:fillcolourB | cmyk@n %fi:fillcolourC \\:@n %fi:fillcolourM \\:@n %fi:fillcolourY \\:@n %fi:fillcolourK | %e:fillcolour > ) }\n\
clear@3:directive =\n\
continue@2:directive = { %s:loopname }\n\
delete@3:directive = { item@1 } [ %d:number ]:deleteno,\n\
do@2:directive = { loopname@1 %s:loopname } { \\{@n:brace { %r:command } }\n\
ellipse@2:directive = { item@1 %d:editno } < from@1 %fu:x1 ,@n %fu:y1 { to@1 } %fu:x2 ,@n %fu:y2 { rotate@1 %fu:rotation } | ( < centre@1 | center@1> %fu:xcentre ,@n %fu:ycentre ~ focus@1 %fu:xfocus ,@n %fu:yfocus ~ < majoraxis@2 %fu:majoraxis | semimajoraxis@6 %fu:semimajoraxis > ~ < minoraxis@2 %fu:minoraxis | semiminoraxis@6 %fu:semiminoraxis > ~ eccentricity@1 %f:eccentricity ~ < < semilatusrectum@5 | slr@2 > %fu:slr | < latusrectum@2 | lr@2 > %fu:lr > ~ rotate@1 %fu:rotation ) > { with@1 ( < linetype@5 | lt@2 > %d:linetype ~ < linewidth@5 | lw@2 > %f:linewidth ~ style@2 %d:style_number ~ < colour@1 | color@1 > < rgb@n %fi:colourR \\:@n %fi:colourG \\:@n %fi:colourB | hsb@n %fi:colourH \\:@n %fi:colourS \\:@n %fi:colourB | cmyk@n %fi:colourC \\:@n %fi:colourM \\:@n %fi:colourY \\:@n %fi:colourK | %e:colour > ~ < fillcolour@1 | fillcolor@1 | fc@2 > < rgb@n %fi:fillcolourR \\:@n %fi:fillcolourG \\:@n %fi:fillcolourB | hsb@n %fi:fillcolourH \\:@n %fi:fillcolourS \\:@n %fi:fillcolourB | cmyk@n %fi:fillcolourC \\:@n %fi:fillcolourM \\:@n %fi:fillcolourY \\:@n %fi:fillcolourK | %e:fillcolour > ) }\n\
\\}@n else@4:directive = ( if@2:if %f:criterion ) ( \\{@n:brace ( %r:command ) )\n\
eps@2:directive = { item@1 %d:editno } < %q:filename | %S:filename > ( at@2 %fu:x ,@n %fu:y ~ rotate@1 %fu:rotation ~ width@1 %fu:width ~ height@1 %fu:height ~ clip@2:clip ~ calcbbox@2:calcbbox )\n\
exec@3:directive: = %q:command\n\
exit@2:directive:quit =\n\
fit@3:directive = [ \\[@n { { < %fu:min | \\*@n:minauto > } < :@n | to@n > { < %fu:max | \\*@n:maxauto > } } \\]@n ]:@range_list %v:fit_function \\(@n [ %v:inputvar ]:@operands, \\)@n { withouterrors@1:withouterrors } < %q:filename | %S:filename > ( every@1 [ { %d:every_item } ]:every_list: ~ index@1 %d:index ~ select@1 %E:select_criterion ~ using@1 { < rows@1:use_rows | columns@1:use_columns > } [ { %E:using_item } ]:using_list: ) via@1 [ %v:fit_variable ]:fit_variables,\n\
< fft@2:directive | ifft@3:directive > = [ \\[@n %fu:min < :@n | to@n > %fu:max < :@n | step@n > %fu:step \\]@n ]:range_list %v:fft_function \\(@n [ %v:inputvar ]:@in_operands, \\)@n { of@1 } < %q:filename ( every@1 [ { %d:every_item } ]:every_list: ~ index@1 %d:index ~ select@1 %E:select_criterion ~ using@1 { < rows@1:use_rows | columns@1:use_columns > } [ { %E:using_item } ]:using_list: ) | %v:input_function \\(@n [ %v:inputvar ]:@out_operands, \\)@n { window@1 < rectangular@1:window | hamming@3:window | hann@3:window | cosine@1:window | lanczos@1:window | bartlett@2:window | triangular@1:window | gauss@1:window | bartletthann@9:window | blackman@2:window > } >\n\
for@2:directive = %v:var_name \\=@n %fu:start_value to@n %fu:final_value ( step@2:step %fu:step_size ) { loopname@1 %s:loopname } { \\{@n:brace { %r:command } }\n\
foreach@4:directive datum@5:df = [ %v:variable ]:variables, in@n:in [ \\[@n { { < %fu:min | \\*@n:minauto > } < :@n | to@n > { < %fu:max | \\*@n:maxauto > } } \\]@n ]:@range_list [ %q:filename ]:filename_list ( every@1 [ { %d:every_item } ]:every_list: ~ index@1 %d:index ~ select@1 %E:select_criterion  ~ using@1 { < rows@1:use_rows | columns@1:use_columns > } [ { < %E:using_item | %Q:using_item > } ]:using_list: ) { loopname@1 %s:loopname } { \\{@n:brace { %r:command } }\n\
foreach@4:directive = %v:var_name in@n:in < \\(@n [ < %fi:value | %q:string | %S:string > ]:item_list, \\)@n | [ %q:filename ]:filename_list > { loopname@1 %s:loopname } { \\{@n:brace { %r:command } }\n\
help@2:directive = %r:topic\n\
history@2:directive = { %d:number_lines }\n\
histogram@5:directive = { \\[@n { { < %fu:min | \\*@n:minauto > } < :@n | to@n > { < %fu:max | \\*@n:maxauto > } } \\]@n } %v:hist_function \\()@2 < %q:filename | %S:filename > ( every@1 [ { %d:every_item } ]:every_list: ~ index@1 %d:index ~ select@1 %E:select_criterion ~ using@1 { < rows@1:use_rows | columns@1:use_columns > } [ { %E:using_item } ]:using_list: ~ binwidth@4 %fu:binwidth ~ binorigin@4 %fu:binorigin ~ bins@n \\(@n [ %fu:x ]:bin_list, \\)@n )\n\
< image@2:directive | jpeg@2:directive:image > = { item@1 %d:editno } = < %q:filename | %S:filename > ( at@2 %fu:x ,@n %fu:y ~ smooth@1:smooth ~ < notransparency@3:notrans | transparent@1 rgb@n %fi:colourR \\:@n %fi:colourG \\:@n %fi:colourB > ~ rotate@1 %fu:rotation ~ < width@1 %fu:width | height@1 %fu:height > )\n\
if@2:directive = %f:criterion ( \\{@n:brace ( %r:command ) )\n\
< list@2:directive | ls@2:directive:list > =\n\
load@2:directive = < %q:filename | %S:filename >\n\
maximise@2:directive = %e:expression via@1 [ %v:fit_variable ]:fit_variables,\n\
minimise@2:directive = %e:expression via@1 [ %v:fit_variable ]:fit_variables,\n\
move@2:directive = { item@1 } %d:moveno to@1 %fu:x ,@n %fu:y { rotate@1 %fu:rotation }\n\
?@n:directive:help = %r:topic\n\
!@n:directive:pling = %r:cmd\n\
piechart@2:directive = { item@1 %d:editno } < %q:filename | [ %e:expression ]:expression_list: > ( every@1 [ { %d:every_item } ]:every_list: ~ index@1 %d:index ~ label@1 { < auto@1:piekeypos | inside@1:piekeypos | key@1:piekeypos | outside@1:piekeypos > } { %Q:label } ~ select@1 %E:select_criterion  ~ using@1 { < rows@1:use_rows | columns@1:use_columns > } [ { %E:using_item } ]:using_list: ~ with@1 ( < linetype@5 | lt@2 > < %d:linetype | %E:linetype_string > ~ < linewidth@5 | lw@2 > < %f:linewidth | %E:linewidth_string > ~ style@2 %d:style_number ~ < colour@1 | color@1 > < rgb@n %fi:colourR \\:@n %fi:colourG \\:@n %fi:colourB | hsb@n %fi:colourH \\:@n %fi:colourS \\:@n %fi:colourB | cmyk@n %fi:colourC \\:@n %fi:colourM \\:@n %fi:colourY \\:@n %fi:colourK | rgb@n %E:colourRexpr \\:@n %E:colourGexpr \\:@n %E:colourBexpr | hsb@n %E:colourHexpr \\:@n %E:colourSexpr \\:@n %E:colourBexpr | cmyk@n %E:colourCexpr \\:@n %E:colourMexpr \\:@n %E:colourYexpr \\:@n %E:colourKexpr | %E:colour > ) ~ format@1 < auto@1:auto_format | %Q:format_string > )\n\
< plot@1:directive = { item@1 %d:editno } { \\3d@2:threedim } | replot@3:directive = { item@1 %d:editno } > [ \\[@n { { < %fu:min | \\*@n:minauto > } < :@n | to@n > { < %fu:max | \\*@n:maxauto > } } \\]@n ]:@range_list [ < %q:filename | { parametric@1:parametric { \\[@n %fu:tmin < :@n | to@n > %fu:tmax \\]@n { \\[@n %fu:vmin < :@n | to@n > %fu:vmax \\]@n } } } [ %e:expression ]:expression_list: > ( axes@1 %a:axis_1 %a:axis_2 { %a:axis_3 } ~ every@1 [ { %d:every_item } ]:every_list: ~ index@1 %d:index ~ label@1 %Q:label ~ select@1 %E:select_criterion { < continuous@1:continuous | discontinuous@1:discontinuous > } ~ < title@1 %q:title | notitle@3:notitle > ~ using@1 { < rows@1:use_rows | columns@1:use_columns > } [ { %E:using_item } ]:using_list: ~ with@1 ( < linetype@5 | lt@2 > < %d:linetype | %E:linetype_string > ~ < linewidth@5 | lw@2 > < %f:linewidth | %E:linewidth_string > ~ < pointsize@7 | ps@2 > < %f:pointsize | %E:pointsize_string > ~ < pointtype@6 | pt@2 > < %d:pointtype | %E:pointtype_string > ~ style@2 %d:style_number ~ < pointlinewidth@6 | plw@3 > < %f:pointlinewidth | %E:pointlinewidth_string > ~ < colour@1 | color@1 > < rgb@n %fi:colourR \\:@n %fi:colourG \\:@n %fi:colourB | hsb@n %fi:colourH \\:@n %fi:colourS \\:@n %fi:colourB | cmyk@n %fi:colourC \\:@n %fi:colourM \\:@n %fi:colourY \\:@n %fi:colourK | rgb@n %E:colourRexpr \\:@n %E:colourGexpr \\:@n %E:colourBexpr | hsb@n %E:colourHexpr \\:@n %E:colourSexpr \\:@n %E:colourBexpr | cmyk@n %E:colourCexpr \\:@n %E:colourMexpr \\:@n %E:colourYexpr \\:@n %E:colourKexpr | %E:colour > ~ < fillcolour@2 | fillcolor@2 | fc@2 > < rgb@n %fi:fillcolourR \\:@n %fi:fillcolourG \\:@n %fi:fillcolourB | hsb@n %fi:fillcolourH \\:@n %fi:fillcolourS \\:@n %fi:fillcolourB | cmyk@n %fi:fillcolourC \\:@n %fi:fillcolourM \\:@n %fi:fillcolourY \\:@n %fi:fillcolourK | rgb@n %E:fillcolourRexpr \\:@n %E:fillcolourGexpr \\:@n %E:fillcolourBexpr | hsb@n %E:fillcolourHexpr \\:@n %E:fillcolourSexpr \\:@n %E:fillcolourBexpr | cmyk@n %E:fillcolourCexpr \\:@n %E:fillcolourMexpr \\:@n %E:fillcolourYexpr \\:@n %E:fillcolourKexpr | %E:fillcolour > ~ < lines@1:style | points@1:style | lp@2:style:linespoints | linespoints@5:style | pl@2:style:linespoints | pointslines@5:style:linespoints | errorbars@6:style:yerrorbars | xerrorbars@1:style | yerrorbars@1:style | zerrorbars@1:style | xyerrorbars@3:style | xzerrorbars@3:style | yzerrorbars@3:style | xyzerrorbars@3:style | errorrange@6:style:yerrorrange | xerrorrange@1:style | yerrorrange@1:style | zerrorrange@1:style | xyerrorrange@3:style | xzerrorrange@3:style | yzerrorrange@3:style | xyzerrorrange@3:style | filledregion@3:style | yerrorshaded@8:style | upperlimits@1:style | lowerlimits@2:style | dots@1:style | impulses@1:style | boxes@1:style | wboxes@1:style | steps@1:style | fsteps@1:style | histeps@1:style | stars@3:style | arrows@3:style:arrows_head | arrows_head@3:style | arrows_nohead@3:style | arrows_twoway@3:style:arrows_twohead | arrows_twohead@3:style | surface@2:style | colourmap@3:style | colmap@4:style:colourmap | contourmap@3:style | contours@3:style:contourmap > ) ) ]:@plot_list,\n\
point@2:directive = { item@1 %d:editno } { at@2 } %fu:x ,@n %fu:y { label@1 < %q:label | %s:label > } { with@1 ( < pointsize@7 | ps@2 > < %f:pointsize | %E:pointsize_string > ~ < pointtype@6 | pt@2 > < %d:pointtype | %E:pointtype_string > ~ style@2 %d:style_number ~ < pointlinewidth@6 | plw@3 > < %f:pointlinewidth | %E:pointlinewidth_string > ~ < colour@1 | color@1 > < rgb@n %fi:colourR \\:@n %fi:colourG \\:@n %fi:colourB | hsb@n %fi:colourH \\:@n %fi:colourS \\:@n %fi:colourB | cmyk@n %fi:colourC \\:@n %fi:colourM \\:@n %fi:colourY \\:@n %fi:colourK | rgb@n %E:colourRexpr \\:@n %E:colourGexpr \\:@n %E:colourBexpr | hsb@n %E:colourHexpr \\:@n %E:colourSexpr \\:@n %E:colourBexpr | cmyk@n %E:colourCexpr \\:@n %E:colourMexpr \\:@n %E:colourYexpr \\:@n %E:colourKexpr | %E:colour > ) }\n\
print@2:directive = [ < %fi:expression | %q:string > ]:@print_list,\n\
pwd@2:directive =\n\
quit@1:directive =\n\
refresh@3:directive =\n\
reset@3:directive =\n\
return@3:directive = < %q:string_return_value | %fi:return_value >\n\
save@2:directive = < %q:filename | %S:filename >\n\
set@2:directive { item@1 %d:editno } %a:axis format@1:set_option:xformat = < auto@1:auto_format | %Q:format_string > { < horizontal@1:orient | vertical@1:orient | rotate@1:orient %fu:rotation > }\n\
set@2:directive { item@1 %d:editno } %a:axis label@1:set_option:xlabel = < %q:label_text | %s:label_text > { rotate@1 %fu:rotation }\n\
set@2:directive { item@1 %d:editno } %a:axis range@1:set_option = < reversed@1:reverse | noreversed@3:noreverse | \\[@n { { < %fu:min | \\*@n:minauto > } < :@n | to@n > { < %fu:max | \\*@n:maxauto > } } \\]@n { < reversed@1:reverse | noreversed@3:noreverse > } >\n\
set@2:directive { item@1 %d:editno } { m@n:minor } { %a:axis } < tics@1:set_option | ticks@1:set_option:tics > = { < axis@2:dir:inwards | border@3:dir:outwards | inwards@2:dir | outwards@3:dir | both@3:dir > } { < autofreq@3:autofreq | %fu:start { ,@n %fu:increment { ,@n %fu:end } } | \\(@n [ { %q:label } %fu:x ]:@tick_list, \\)@n > }\n\
set@2:directive { item@1 %d:editno } arrow@1:set_option = %d:arrow_id { from@1 } { < first@1:x0_system | second@1:x0_system | page@2:x0_system | graph@1:x0_system | axis@n:x0_system %d:x0_axis > } %fu:x0 ,@n { < first@1:y0_system | second@1:y0_system | page@2:y0_system | graph@1:y0_system | axis@n:y0_system %d:y0_axis > } %fu:y0 { ,@n { < first@1:z0_system | second@1:z0_system | page@2:z0_system | graph@1:z0_system | axis@n:z0_system %d:z0_axis > } %fu:z0 } to@1 { < first@1:x1_system | second@1:x1_system | page@2:x1_system | graph@1:x1_system | axis@n:x1_system %d:x1_axis > } %fu:x1 ,@n { < first@1:y1_system | second@1:y1_system | page@2:y1_system | graph@1:y1_system | axis@n:y1_system %d:y1_axis > } %fu:y1 { ,@n { < first@1:z1_system | second@1:z1_system | page@2:z1_system | graph@1:z1_system | axis@n:z1_system %d:z1_axis > } %fu:z1 } { with@1 ( < linetype@5 | lt@2 > %d:linetype ~ < linewidth@5 | lw@2 > %f:linewidth ~ style@2 %d:style_number ~ < colour@1 | color@1 > < rgb@n %fi:colourR \\:@n %fi:colourG \\:@n %fi:colourB | hsb@n %fi:colourH \\:@n %fi:colourS \\:@n %fi:colourB | cmyk@n %fi:colourC \\:@n %fi:colourM \\:@n %fi:colourY \\:@n %fi:colourK | %e:colour > ~ < nohead@2:arrow_style | head@2:arrow_style | twohead@2:arrow_style:twoway | twoway@2:arrow_style > ) }\n\
set@2:directive { item@1 %d:editno } autoscale@2:set_option = { [ %a:axis ]:axes }\n\
set@2:directive { item@1 %d:editno } < axescolour@5:set_option | axescolor@5:set_option:axescolour > = < rgb@n %fi:colourR \\:@n %fi:colourG \\:@n %fi:colourB | hsb@n %fi:colourH \\:@n %fi:colourS \\:@n %fi:colourB | cmyk@n %fi:colourC \\:@n %fi:colourM \\:@n %fi:colourY \\:@n %fi:colourK | %e:colour >\n\
set@2:directive { item@1 %d:editno } axis@1:set_option = [ %a:axis ]:axes ( < invisible@1:invisible | visible@1:visible > ~ < top@2:xorient:on | bottom@2:xorient:off | left@2:yorient:off | right@2:yorient:on | front@2:zorient:off | back@2:zorient:on > ~ < automirrored@2:mirror | mirrored@1:mirror | nomirrored@2:mirror:nomirror | fullmirrored@2:mirror > ~ < atzero@2:atzero | notatzero@4:notatzero > ~ < arrow@2:axisdisp | noarrow@3:axisdisp | twowayarrow@2:axisdisp | reversearrow@2:axisdisp > ~ < notlinked@4:notlinked | linked@1:linked { item@1 %d:linktoid } %a:linkaxis { using@1 %e:usingexp } > )\n\
set@2:directive { item@1 %d:editno } axisunitstyle@5:set_option = < bracketed@1:unitstyle | ratio@1:unitstyle | squarebracketed@1:unitstyle >\n\
set@2:directive                      backup@1:set_option =\n\
set@2:directive { item@1 %d:editno } bar@2:set_option = { < large@1:bar_size_large | small@1:bar_size_small | %f:bar_size > }\n\
set@2:directive { item@1 %d:editno } binorigin@4:set_option = < %fu:bin_origin | auto@4:auto >\n\
set@2:directive { item@1 %d:editno } binwidth@4:set_option = < %fu:bin_width | auto@4:auto >\n\
set@2:directive { item@1 %d:editno } boxfrom@4:set_option = < %fu:box_from | auto@4:auto >\n\
set@2:directive { item@1 %d:editno } boxwidth@1:set_option = < %fu:box_width | auto@4:auto >\n\
set@2:directive { item@1 %d:editno } c1format@1:set_option = < auto@1:auto_format | %Q:format_string > { < horizontal@1:orient | vertical@1:orient | rotate@1:orient %fu:rotation > }\n\
set@2:directive { item@1 %d:editno } c1label@1:set_option = < %q:label_text | %s:label_text > { rotate@1 %fu:rotation }\n\
set@2:directive                      calendar@1:set_option = < < gregorian@1:calendar:Gregorian | julian@1:calendar:Julian | british@1:calendar:British | french@1:calendar:French | papal@1:calendar:Papal | russian@1:calendar:Russian | greek@5:calendar:Greek | muslim@1:calendar:Islamic | islamic@1:calendar:Islamic | jewish@2:calendar:Hebrew | hebrew@1:calendar:Hebrew > | ( input@2 < gregorian@1:calendarin:Gregorian | julian@1:calendarin:Julian | british@1:calendarin:British | french@1:calendarin:French | papal@1:calendarin:Papal | russian@1:calendarin:Russian | greek@5:calendarin:Greek | muslim@1:calendar:Islamic | islamic@1:calendar:Islamic | jewish@2:calendar:Hebrew | hebrew@1:calendar:Hebrew > ~ output@2 < gregorian@1:calendarout:Gregorian | julian@1:calendarout:Julian | british@1:calendarout:British | french@1:calendarout:French | papal@1:calendarout:Papal | russian@1:calendarout:Russian | greek@5:calendarout:Greek | muslim@1:calendar:Islamic | islamic@1:calendar:Islamic | jewish@2:calendar:Hebrew | hebrew@1:calendar:Hebrew > ) >\n\
set@2:directive { item@1 %d:editno } clip@2:set_option =\n\
set@2:directive { item@1 %d:editno } < colkey@4:set_option | colourkey@7:set_option:colkey | colorkey@6:set_option:colkey > = { < left@1:pos | right@1:pos | outside@1:pos:right | top@1:pos | above@1:pos:top | bottom@1:pos | below@1:pos:bottom > }\n\
set@2:directive { item@1 %d:editno } < colmap@4:set_option | colourmap@7:set_option:colmap | colormap@6:set_option:colmap > = < rgb@n %E:colourR \\:@n %E:colourG \\:@n %E:colourB | hsb@n %E:colourH \\:@n %E:colourS \\:@n %E:colourB | cmyk@n %E:colourC \\:@n %E:colourM \\:@n %E:colourY \\:@n %E:colourK > { < mask@1 %E:mask | nomask@1:nomask > }\n\
set@2:directive { item@1 %d:editno } contours@3:set_option = ( < \\(@n [ %fu:contour ]:contour_list, \\)@n | %d:contours > ~ < label@1:label | nolabel@1:nolabel > )\n\
set@2:directive { item@1 %d:editno } c@n < \\1@n:c_number | \\2@n:c_number | \\3@n:c_number | \\4@n:c_number > range@2:set_option:crange = ( < reversed@1:reverse | noreversed@3:noreverse > ~ \\[@n { < %fu:min | \\*@n:minauto > } < :@n | to@n > { < %fu:max | \\*@n:maxauto > } \\]@n ~ < renormalise@3:renormalise | renormalize@3:renormalise | norenormalise@3:norenormalise | norenormalize@3:norenormalise > )\n\
set@2:directive < { item@1 %d:editno } < data@1:dataset_type style@1:set_option | style@2:set_option data@1:dataset_type | function@1:dataset_type style@1:set_option | style@2:set_option function@1:dataset_type > | style@2:set_option:style_numbered %d:style_set_number > = ( < linetype@5 | lt@2 > %d:linetype ~ < linewidth@5 | lw@2 > %f:linewidth ~ < pointsize@7 | ps@2 > %f:pointsize ~ < pointtype@6 | pt@2 > %d:pointtype ~ style@2 %d:style_number ~ < pointlinewidth@6 | plw@3 > %f:pointlinewidth ~ < colour@1 | color@1 > < rgb@n %fi:colourR \\:@n %fi:colourG \\:@n %fi:colourB | hsb@n %fi:colourH \\:@n %fi:colourS \\:@n %fi:colourB | cmyk@n %fi:colourC \\:@n %fi:colourM \\:@n %fi:colourY \\:@n %fi:colourK | %e:colour > ~ < fillcolour@2 | fillcolor@2 | fc@2 > < rgb@n %fi:fillcolourR \\:@n %fi:fillcolourG \\:@n %fi:fillcolourB | hsb@n %fi:fillcolourH \\:@n %fi:fillcolourS \\:@n %fi:fillcolourB | cmyk@n %fi:fillcolourC \\:@n %fi:fillcolourM \\:@n %fi:fillcolourY \\:@n %fi:fillcolourK | %e:fillcolour > ~ < lines@1:style | points@1:style | lp@2:style:linespoints | linespoints@5:style | pl@2:style:linespoints | pointslines@5:style:linespoints | errorbars@6:style:yerrorbars | xerrorbars@1:style | yerrorbars@1:style | zerrorbars@1:style | xyerrorbars@3:style | xzerrorbars@3:style | yzerrorbars@3:style | xyzerrorbars@3:style | errorrange@6:style:yerrorrange | xerrorrange@1:style | yerrorrange@1:style | zerrorrange@1:style | xyerrorrange@3:style | xzerrorrange@3:style | yzerrorrange@3:style | xyzerrorrange@3:style | filledregion@3:style | yerrorshaded@8:style | upperlimits@1:style | lowerlimits@2:style | dots@1:style | impulses@1:style | boxes@1:style | wboxes@1:style | steps@1:style | fsteps@1:style | histeps@1:style | arrows@3:style:arrows_head | arrows_head@3:style | arrows_nohead@3:style | arrows_twoway@3:style:arrows_twohead | arrows_twohead@3:style | surface@2:style | colourmap@3:style | colmap@4:style:colourmap | contourmap@3:style > )\n\
set@2:directive                      display@1:set_option =\n\
set@3:directive                      filter@2:set_option = < %q:filename | %S:filename > < %q:filter | %S:filter >\n\
set@2:directive { item@1 %d:editno } < fountsize@2set_option:fontsize | fontsize@2:set_option > = %f:fontsize\n\
set@2:directive { item@1 %d:editno } grid@1:set_option = [ %a:axis ]:@axes\n\
set@2:directive { item@1 %d:editno } < gridmajcolour@6:set_option | gridmajcolor@6:set_option:gridmajcolour > = < rgb@n %fi:colourR \\:@n %fi:colourG \\:@n %fi:colourB | hsb@n %fi:colourH \\:@n %fi:colourS \\:@n %fi:colourB | cmyk@n %fi:colourC \\:@n %fi:colourM \\:@n %fi:colourY \\:@n %fi:colourK | %e:colour >\n\
set@2:directive { item@1 %d:editno } < gridmincolour@6:set_option | gridmincolor@6:set_option:gridmincolour > = < rgb@n %fi:colourR \\:@n %fi:colourG \\:@n %fi:colourB | hsb@n %fi:colourH \\:@n %fi:colourS \\:@n %fi:colourB | cmyk@n %fi:colourC \\:@n %fi:colourM \\:@n %fi:colourY \\:@n %fi:colourK | %e:colour >\n\
set@2:directive { item@1 %d:editno } key@1:set_option = < below@2:pos | above@2:pos | outside@1:pos | ( < left@1:xpos | right@1:xpos | xcentre@1:xpos | xcenter@1:xpos:xcentre > ~ < top@1:ypos | bottom@2:ypos | ycentre@1:ypos | ycenter@1:ypos:ycentre > ) > { %fu:x_offset ,@n %fu:y_offset }\n\
set@2:directive { item@1 %d:editno } keycolumns@4:set_option = < %d:key_columns | auto@1:auto_columns > \n\
set@2:directive { item@1 %d:editno } label@2:set_option = %d:label_id < %q:label_text | %s:label_text > { at@1 } { < first@1:x_system | second@1:x_system | page@2:x_system | graph@1:x_system | axis@n:x_system %d:x_axis > } %fu:x ,@n { < first@1:y_system | second@1:y_system | page@2:y_system | graph@1:y_system | axis@n:y_system %d:y_axis > } %fu:y { ,@n { < first@1:z_system | second@1:z_system | page@2:z_system | graph@1:z_system | axis@n:z_system %d:z_axis > } %fu:z } ( rotate@1 %fu:rotation ~ gap@1 %fu:gap ~ halign@2 < left@1:halign | centre@1:halign | center@1:halign:centre | right@1:halign > ~ valign@2 < top@1:valign | centre@1:valign | center@1:valign:centre | bottom@1:valign > ~ with@1 < colour@1 | color@1 > < rgb@n %fi:colourR \\:@n %fi:colourG \\:@n %fi:colourB | hsb@n %fi:colourH \\:@n %fi:colourS \\:@n %fi:colourB | cmyk@n %fi:colourC \\:@n %fi:colourM \\:@n %fi:colourY \\:@n %fi:colourK | %e:colour > )\n\
set@2:directive { item@1 %d:editno } < linewidth@5:set_option | lw@2:set_option:linewidth > = %f:linewidth\n\
set@2:directive { item@1 %d:editno } logscale@1:set_option = { [ < %a:axis | t@n:tlog | u@n:ulog | v@n:vlog | c1@n:c1log | c2@n:c2log | c3@n:c3log | c4@n:c4log > ]:axes } { %d:base }\n\
set@2:directive                      multiplot@1:set_option =\n\
set@2:directive { item@1 %d:editno } noarrow@3:set_option = [ %d:arrow_id ]:@arrow_list,\n\
set@2:directive:unset { item@1 %d:editno } noaxis@3:set_option:axis = [ %a:axis ]:axes\n\
set@2:directive                      nobackup@3:set_option =\n\
set@2:directive { item@1 %d:editno } noclip@4:set_option =\n\
set@2:directive { item@1 %d:editno } < nocolkey@4:set_option | nocolourkey@4:set_option:nocolkey | nocolorkey@4:set_option:nocolkey > =\n\
set@2:directive                      nodisplay@3:set_option =\n\
set@2:directive { item@1 %d:editno } nogrid@3:set_option = [ %a:axis ]:@axes\n\
set@2:directive { item@1 %d:editno } nokey@3:set_option =\n\
set@2:directive { item@1 %d:editno } nolabel@4:set_option = [ %d:label_id ]:@label_list,\n\
set@2:directive:unset { item@1 %d:editno } nostyle@3:set_option:style [ %d:id ]:style_ids, =\n\
set@2:directive { item@1 %d:editno } < nologscale@3:set_option | linearscale@3:set_option:nologscale > = { [ < %a:axis | t@n:tlog | u@n:ulog | v@n:vlog | c1@n:c1log | c2@n:c2log | c3@n:c3log | c4@n:c4log > ]:axes }\n\
set@2:directive                      nomultiplot@3:set_option =\n\
set@2:directive { item@1 %d:editno } no@n { m@n:minor } { %a:axis } < tics@1:set_option:notics | ticks@1:set_option:notics > =\n\
set@2:directive { item@1 %d:editno } noc1format@1:set_option =\n\
set@2:directive { item@1 %d:editno } notitle@3:set_option =\n\
set@2:directive { item@1 %d:editno } no@n %a:axis format@1:set_option:noxformat =\n\
set@2:directive                      numerics@2:set_option = ( < sigfig@3 | sf@2 > %d:number_significant_figures ~ errors@2 < explicit@1:errortype:On | nan@1:errortype:Off | quiet@1:errortype:Off | nonan@3:errortype:On | noquiet@3:errortype:On | noexplicit@3:errortype:Off > ~ < complex@1:complex:On | real@1:complex:Off | nocomplex@3:complex:Off | noreal@3:complex:On > ~ display@1 < typeable@1:display | natural@2:display | latex@1:display | tex@1:display:latex > )\n\
set@2:directive { item@1 %d:editno } origin@2:set_option = %fu:x_origin ,@n %fu:y_origin\n\
set@2:directive { item@1 %d:editno } output@1:set_option = < %q:filename | %S:filename >\n\
set@2:directive { item@1 %d:editno } palette@1:set_option = [ < %q:colour | rgb@n %fi:colourR \\:@n %fi:colourG \\:@n %fi:colourB | hsb@n %fi:colourH \\:@n %fi:colourS \\:@n %fi:colourB | cmyk@n %fi:colourC \\:@n %fi:colourM \\:@n %fi:colourY \\:@n %fi:colourK | %e:colour > ]:palette,\n\
set@2:directive                      papersize@3:set_option %fu:x_size ,@n %fu:y_size =\n\
set@2:directive                      papersize@3:set_option = < %q:paper_name | %S:paper_name >\n\
set@2:directive { item@1 %d:editno } < pointlinewidth@6:set_option | plw@3:set_option:pointlinewidth > = %f:pointlinewidth\n\
set@2:directive { item@1 %d:editno } < pointsize@1:set_option | ps@2:set_option:pointsize > = %f:pointsize\n\
set@2:directive { item@1 %d:editno } preamble@2:set_option = < %q:preamble | %r:preamble >\n\
set@2:directive { item@1 %d:editno } samples@2:set_option = ( %d:samples ~ grid@2 < %d:samplesX | \\*@n:samplesXauto > { x@n } < %d:samplesY | \\*@n:samplesYauto > ~ interpolate@3 < inversesquare@1:method | monaghanlattanzio@1:method | nearestneighbour@1:method | nearestneighbor@1:method:nearestneighbour > )\n\
set@2:directive                      seed@2:set_option = %f:seed\n\
set@2:directive { item@1 %d:editno } size@1:set_option = < %fu:width ,@n %fu:height { ,@n %fu:zsize } | ( %fu:width ~ < ratio@1 < %f:ratio | auto@3:noratio > | noratio@1:noratio | square@1:square > ~ < zratio@1 < %f:zratio | auto@3:nozratio > | nozratio@1:nozratio > ) >\n\
set@2:directive                      terminal@1:set_option = ( < x11_singlewindow@1:term:X11_SingleWindow | x11_multiwindow@5:term:X11_MultiWindow | x11_persist@5:term:X11_Persist | postscript@1:term:ps | ps@2:term:ps | eps@1:term:eps | pdf@2:term:pdf | png@2:term:png | gif@1:term:gif | jpg@1:term:jpg | jpeg@1:term:jpg | bmp@1:term:bmp | tiff@1:term:tif | svg@1:term:svg > ~ < colour@1:col:On | color@1:col:On | monochrome@1:col:Off | nocolour@1:col:Off | nocolor@1:col:Off > ~ < enlarge@1:enlarge:On | noenlarge@3:enlarge:Off > ~ < landscape@1:land:On | portrait@2:land:Off > ~ < notransparent@1:trans:Off | nosolid@1:trans:On | transparent@1:trans:On | solid@1:trans:Off > ~ < invert@1:invert:On | noinvert@1:invert:Off > ~ < antialias@1:antiali:On | noantialias@3:antiali:Off > ~ < dpi@3 | resolution@3 > %f:dpi )\n\
set@2:directive { item@1 %d:editno } < textcolour@5:set_option | textcolor@5:set_option:textcolour > = < rgb@n %fi:colourR \\:@n %fi:colourG \\:@n %fi:colourB | hsb@n %fi:colourH \\:@n %fi:colourS \\:@n %fi:colourB | cmyk@n %fi:colourC \\:@n %fi:colourM \\:@n %fi:colourY \\:@n %fi:colourK | %e:colour >\n\
set@2:directive { item@1 %d:editno } texthalign@5:set_option = < left@1:left | centre@1:centre | center@1:centre | right@1:right >\n\
set@2:directive { item@1 %d:editno } textvalign@5:set_option = < top@1:top | centre@1:centre | center@1:centre | bottom@1:bottom >\n\
set@2:directive { item@1 %d:editno } title@2:set_option = < %q:title | %s:title > { %fu:x_offset ,@n %fu:y_offset }\n\
set@2:directive { item@1 %d:editno } trange@2:set_option = { \\[@n:range { %fu:min } < :@n | to@n > { %fu:max } \\]@n } { reverse@1:reverse }\n\
set@2:directive                      unit@1:set_option = ( angle@1 < dimensionless@1:angle:On | nodimensionless@1:angle:Off > ~ display@1 ( < abbreviated@1:abbrev:On | noabbreviated@3:abbrev:Off | full@1:abbrev:Off | nofull@3:abbrev:On > ~ < prefix@1:prefix:On | noprefix@3:prefix:Off > ) ~ scheme@1 < si@2:scheme:SI | cgs@1:scheme:CGS | ancient@1:scheme:ANCIENT | imperial@1:scheme:IMPERIAL | uscustomary@1:scheme:USCustomary | planck@1:scheme:PLANCK | natural@1:scheme:PLANCK > ~ [ of@1 %s:quantity %v:unit ]:preferred_units, ~ preferred@5 %e:preferred_unit ~ nopreferred@7 %e:unpreferred_unit )\n\
set@2:directive { item@1 %d:editno } urange@2:set_option = { \\[@n:range { %fu:min } < :@n | to@n > { %fu:max } \\]@n } { reverse@1:reverse }\n\
set@2:directive { item@1 %d:editno } view@1:set_option = %fu:xy_angle { ,@n } %fu:yz_angle\n\
set@2:directive                      viewer@5:set_option = < auto@4:auto_viewer | %r:viewer >\n\
set@2:directive { item@1 %d:editno } vrange@2:set_option = { \\[@n:range { %fu:min } < :@n | to@n > { %fu:max } \\]@n } { reverse@1:reverse }\n\
set@2:directive { item@1 %d:editno } width@1:set_option:size = %fu:width\n\
set@2:directive:set_error = { item@1 %d:editno } { %s:set_option } %r:restofline\n\
show@2:directive = { item@1 %d:editno } [ %S:setting ]:@setting_list\n\
solve@2:directive = [ %e:left_expression \\=@n %e:right_expression ]:expressions, via@1 [ %v:fit_variable ]:fit_variables,\n\
< spline@3:directive = | interpolate@4 = < akima@1:directive | linear@2:directive | loglinear@2:directive | polynomial@1:directive | spline@2:directive | stepwise@2:directive | 2d@2:directive:interpolate2d { < bmp_r:bmp | bmp_g:bmp | bmp_b:bmp > } > > [ \\[@n { { < %fu:min | \\*@n:minauto > } < :@n | to@n > { < %fu:max | \\*@n:maxauto > } } \\]@n ]:@range_list %v:fit_function \\()@2 < %q:filename | %S:filename > ( every@1 [ { %d:every_item } ]:every_list: ~ index@1 %d:index ~ select@1 %E:select_criterion ~ using@1 { < rows@1:use_rows | columns@1:use_columns > } [ { %E:using_item } ]:using_list: )\n\
subroutine@2:directive = %v:subroutine_name \\(@n [ %v:argument_name ]:@argument_list, \\)@n { \\{@n:brace { %r:command } }\n\
swap@2:directive = %d:item1 %d:item2 \n\
tabulate@2:directive = [ \\[@n { { < %fu:min | \\*@n:minauto > } < :@n | to@n > { < %fu:max | \\*@n:maxauto > } } \\]@n ]:@range_list [ < %q:filename | { parametric@1:parametric { \\[@n %fu:tmin < :@n | to@n > %fu:tmax \\]@n { \\[@n %fu:vmin < :@n | to@n > %fu:vmax \\]@n } } } [ %e:expression ]:expression_list: > ( every@1 [ { %d:every_item } ]:every_list: ~ index@1 %d:index ~ select@1 %E:select_criterion ~ sortby %E:sort_expression ~ using@1 { < rows@1:use_rows | columns@1:use_columns > } [ { %E:using_item } ]:using_list: ) { with@1 ( format@1 %q:format ~ spacing@1 %fu:spacing ) } ]:@tabulate_list,\n\
text@3:directive = { item@1 %d:editno } < %q:string | %s:string > ( at@1 %fu:x ,@n %fu:y ~ rotate@1 %fu:rotation ~ gap@1 %fu:gap ~ halign@2 < left@1:halign | centre@1:halign | center@1:halign:centre | right@1:halign > ~ valign@2 < top@1:valign | centre@1:valign | center@1:valign:centre | bottom@1:valign > ~ with@1 < colour@1 | color@1 > < rgb@n %fi:colourR \\:@n %fi:colourG \\:@n %fi:colourB | hsb@n %fi:colourH \\:@n %fi:colourS \\:@n %fi:colourB | cmyk@n %fi:colourC \\:@n %fi:colourM \\:@n %fi:colourY \\:@n %fi:colourK | %e:colour > )\n\
undelete@3:directive = { item@1 } [ %d:number ]:undeleteno,\n\
unset@3:directive { item@1 %d:editno } { no@n } %a:axis format@1:set_option:xformat =\n\
unset@3:directive { item@1 %d:editno } %a:axis label@1:set_option:xlabel =\n\
unset@3:directive { item@1 %d:editno } %a:axis range@1:set_option =\n\
unset@3:directive { item@1 %d:editno } { no@n } { m@n:minor } { %a:axis } < tics@1:set_option | ticks@1:set_option:tics > =\n\
unset@3:directive { item@1 %d:editno } arrow@2:set_option = [ %d:arrow_id ]:@arrow_list,\n\
unset@2:directive { item@1 %d:editno } autoscale@2:set_option = { [ %a:axis ]:axes }\n\
unset@3:directive { item@1 %d:editno } < axescolour@5:set_option | axescolor@5:set_option:axescolour > =\n\
unset@3:directive { item@1 %d:editno } axis@2:set_option = [ %a:axis ]:axes\n\
unset@3:directive { item@1 %d:editno } axisunitstyle@5:set_option =\n\
unset@3:directive                      backup@1:set_option =\n\
unset@3:directive { item@1 %d:editno } bar@2:set_option =\n\
unset@3:directive                      binorigin@4:set_option =\n\
unset@3:directive                      binwidth@4:set_option =\n\
unset@3:directive { item@1 %d:editno } boxfrom@4:set_option =\n\
unset@3:directive { item@1 %d:editno } boxwidth@1:set_option =\n\
unset@3:directive { item@1 %d:editno } { no@n } c1format@1:set_option =\n\
unset@3:directive { item@1 %d:editno } c1label@1:set_option =\n\
unset@3:directive                      calendar@1:set_option =\n\
unset@3:directive { item@1 %d:editno } clip@2:set_option =\n\
unset@3:directive { item@1 %d:editno } < colkey@4:set_option | colourkey@7:set_option:colkey | colorkey@6:set_option:colkey > =\n\
unset@3:directive { item@1 %d:editno } < colmap@4:set_option | colourmap@7:set_option:colmap | colormap@6:set_option:colmap > =\n\
unset@3:directive { item@1 %d:editno } contours@3:set_option =\n\
unset@3:directive { item@1 %d:editno } c@n < \\1@n:c_number | \\2@n:c_number | \\3@n:c_number | \\4@n:c_number > range@2:set_option:crange =\n\
unset@3:directive                      display@1:set_option =\n\
unset@3:directive                      filter@2:set_option = < %q:filename | %S:filename >\n\
unset@3:directive { item@1 %d:editno } < fountsize@2:set_option:fontsize | fontsize@2:set_option > =\n\
unset@3:directive { item@1 %d:editno } grid@1:set_option =\n\
unset@3:directive { item@1 %d:editno } < gridmajcolour@6:set_option | gridmajcolor@6:set_option:gridmajcolour > =\n\
unset@3:directive { item@1 %d:editno } < gridmincolour@6:set_option | gridmincolor@6:set_option:gridmincolour > =\n\
unset@3:directive { item@1 %d:editno } key@1:set_option =\n\
unset@3:directive { item@1 %d:editno } keycolumns@4:set_option =\n\
unset@3:directive { item@1 %d:editno } label@2:set_option = [ %d:label_id ]:@label_list,\n\
unset@3:directive { item@1 %d:editno } style@2:set_option = < [ %d:id ]:style_ids, | data@1:dataset_type | function@1:dataset_type >\n\
unset@3:directive { item@1 %d:editno } data@1:dataset_type style@1:set_option =\n\
unset@3:directive { item@1 %d:editno } function@1:dataset_type style@1:set_option =\n\
unset@3:directive { item@1 %d:editno } < linewidth@5:set_option | lw@2:set_option:linewidth > =\n\
unset@3:directive { item@1 %d:editno } logscale@1:set_option = { [ < %a:axis | t@n:tlog | u@n:ulog | v@n:vlog | c1@n:c1log | c2@n:c2log | c3@n:c3log | c4@n:c4log > ]:axes }\n\
unset@3:directive                      multiplot@1:set_option =\n\
unset@3:directive { item@1 %d:editno } noarrow@3:set_option:arrow = [ %d:arrow_id ]:@arrow_list,\n\
unset@3:directive { item@1 %d:editno } noaxis@4:set_option:axis = [ %a:axis ]:axes\n\
unset@3:directive { item@1 %d:editno } nobackup@3:set_option:backup =\n\
unset@3:directive { item@1 %d:editno } noclip@4:set_option:clip =\n\
unset@3:directive { item@1 %d:editno } < nocolkey@4:set_option | nocolourkey@4:set_option:nocolkey | nocolorkey@4:set_option:nocolkey > =\n\
unset@3:directive { item@1 %d:editno } nodisplay@3:set_option:display =\n\
unset@3:directive { item@1 %d:editno } nogrid@3:set_option:grid =\n\
unset@3:directive { item@1 %d:editno } nokey@3:set_option:key =\n\
unset@3:directive { item@1 %d:editno } nolabel@4:set_option:label = [ %d:label_id ]:@label_list,\n\
unset@3:directive { item@1 %d:editno } nostyle@4:set_option:style [ %d:id ]:style_ids, =\n\
unset@3:directive { item@1 %d:editno } < nolinewidth@7:set_option:linewidth | nolw@4:set_option:linewidth >\n\
unset@3:directive { item@1 %d:editno } nologscale@3:set_option:logscale = { [ < %a:axis | t@n:tlog | u@n:ulog | v@n:vlog | c1@n:c1log | c2@n:c2log | c3@n:c3log | c4@n:c4log > ]:axes }\n\
unset@3:directive { item@1 %d:editno } nomultiplot@3:set_option:multiplot =\n\
unset@3:directive { item@1 %d:editno } notitle@3:set_option:title =\n\
unset@3:directive                      numerics@2:set_option = { < sigfig@3:set_option:numerics_sigfig | sf@2:set_option:numerics_sigfig | errors@2:set_option:numerics_errors | complex@1:set_option:numerics_complex | real@1:set_option:numerics_complex | nocomplex@3:set_option:numerics_complex | noreal@3:set_option:numerics_complex | display@1:set_option:numerics_display > }\n\
unset@3:directive { item@1 %d:editno } origin@2:set_option =\n\
unset@3:directive { item@1 %d:editno } output@1:set_option =\n\
unset@3:directive { item@1 %d:editno } palette@1:set_option =\n\
unset@3:directive                      papersize@3:set_option =\n\
unset@3:directive { item@1 %d:editno } < pointlinewidth@6:set_option |  plw@3:set_option:pointlinewidth > =\n\
unset@3:directive { item@1 %d:editno } < pointsize@1:set_option | ps@2:set_option:pointsize > =\n\
unset@3:directive { item@1 %d:editno } preamble@2:set_option =\n\
unset@3:directive { item@1 %d:editno } samples@2:set_option =\n\
unset@2:directive                      seed@2:set_option =\n\
unset@3:directive:set { item@1 %d:editno } < axis@1:set_option:noaxis | noaxis@3:set_option > = [ %a:axis ]:axes\n\
unset@3:directive { item@1 %d:editno } size@1:set_option =\n\
unset@3:directive                      terminal@1:set_option =\n\
unset@3:directive { item@1 %d:editno } < textcolour@5:set_option | textcolor@5:set_option:textcolour > =\n\
unset@3:directive { item@1 %d:editno } texthalign@5:set_option =\n\
unset@3:directive { item@1 %d:editno } textvalign@5:set_option =\n\
unset@3:directive { item@1 %d:editno } title@2:set_option =\n\
unset@2:directive { item@1 %d:editno } trange@2:set_option =\n\
unset@3:directive                      unit@1:set_option = { < display@1:set_option:unit_display | scheme@1:set_option:unit_scheme | of@1:set_option:unit_of %s:quantity | preferred@1:set_option:unit_preferred > }\n\
unset@2:directive { item@1 %d:editno } urange@2:set_option =\n\
unset@3:directive { item@1 %d:editno } view@1:set_option =\n\
unset@3:directive                      viewer@1:set_option =\n\
unset@2:directive { item@1 %d:editno } vrange@2:set_option =\n\
unset@3:directive { item@1 %d:editno } width@1:set_option =\n\
unset@3:directive:unset_error = { item@1 %d:editno } { %s:set_option } %r:restofline\n\
while@5:directive = %e:criterion { loopname@1 %s:loopname } { \\{@n:brace { %r:command } }\n\
\\}:n:close_brace while@5:directive = %e:criterion { \\{@n:brace { %r:command } }\n\
";

