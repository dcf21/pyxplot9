// airyFuncs.c
//
// The code in this file is part of Pyxplot
// <http://www.pyxplot.org.uk>
//
// Copyright (C) 2006-2013 Dominic Ford <coders@pyxplot.org.uk>
//               2008-2013 Ross Church
//
//               2009-2010 Matthew Smith
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

// This file contains an implementation of the Airy functions for general
// complex inputs provided courtesy of Matthew Smith.

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <gsl/gsl_math.h>
#include <gsl/gsl_complex.h>
#include <gsl/gsl_complex_math.h>

#include "defaultObjs/airyFuncs.h"

#define ADDON *dp = gsl_complex_add(*dp,gsl_complex_mul(f,c)); \
              *dq = gsl_complex_add(*dq,gsl_complex_mul(g,c)); \
               c  = gsl_complex_mul(c,zd)

gsl_complex airy_series_y1(gsl_complex z)
 {
  int i;
  gsl_complex c,d;
  c = gsl_complex_mul(z,z);
  z = gsl_complex_mul(c,z);
  c = GSL_COMPLEX_ONE;
  d = GSL_COMPLEX_ZERO;
  for (i=0;i<=45;i+=3)
   {
    d = gsl_complex_add(d,c);
    c = gsl_complex_mul(c,z);
    c = gsl_complex_div_real(c,((double) (i+2)*(i+3)));
   }
  return d;
 }

gsl_complex airy_series_y1_diff(gsl_complex z)
 {
  int i;
  gsl_complex c,d;
  c = gsl_complex_mul(z,z);
  z = gsl_complex_mul(c,z);
  c = gsl_complex_div_real(c,2.0);
  d = GSL_COMPLEX_ZERO;
  for (i=0;i<=45;i+=3)
   {
    d = gsl_complex_add(d,c);
    c = gsl_complex_mul(c,z);
    c = gsl_complex_div_real(c,((double) (i+3)*(i+5)));
   }
  return d;
 }

gsl_complex airy_series_y2(gsl_complex z)
 {
  int i;
  gsl_complex c,d;
  c = z;
  z = gsl_complex_mul(z,z);
  z = gsl_complex_mul(c,z);
  d = GSL_COMPLEX_ZERO;
  for (i=0;i<=45;i+=3)
   {
    d = gsl_complex_add(d,c);
    c = gsl_complex_mul(c,z);
    c = gsl_complex_div_real(c,((double) (i+3)*(i+4)));
   }
  return d;
 }

gsl_complex airy_series_y2_diff(gsl_complex z)
 {
  int i;
  gsl_complex c,d;
  c = z;
  z = gsl_complex_mul(z,z);
  z = gsl_complex_mul(c,z);
  c = GSL_COMPLEX_ONE;
  d = GSL_COMPLEX_ZERO;
  for (i=0;i<=45;i+=3)
   {
    d = gsl_complex_add(d,c);
    c = gsl_complex_mul(c,z);
    c = gsl_complex_div_real(c,((double) (i+1)*(i+3)));
   }
  return d;
 }

void airy_lookup_ai(double mod, double arg, gsl_complex *a, gsl_complex *b, gsl_complex *z0)
 {
  int sector;
  if (mod>6.0)
   {
    if (mod>7.4)
     {
      sector = (int) floor(arg/0.174532925199432957);
      if (sector==18) sector = 17;
      *z0 = gsl_complex_polar(8.1,M_PI/36+((double) sector)*M_PI/18);
      switch (sector)
       {
        case 0:
        *a = gsl_complex_rect(-0.0000000177089032495511053,-0.0000000360617804208924853);
        *b = gsl_complex_rect(0.0000000464975585245366009,0.000000105770668476231643);
        break;
        case 1:
        *a = gsl_complex_rect(0.000000107088798376430528,0.0000000376445041341328399);
        *b = gsl_complex_rect(-0.000000291595748545378589,-0.000000146296178088760197);
        break;
        case 2:
        *a = gsl_complex_rect(-0.000000843663635658861037,0.0000000316868610357800936);
        *b = gsl_complex_rect(0.00000238658884361740962,0.000000420355834910498508);
        break;
        case 3:
        *a = gsl_complex_rect(0.0000140556717230700034,0.00000320688133083177304);
        *b = gsl_complex_rect(-0.0000358156878450108478,-0.0000205769639137091060);
        break;
        case 4:
        *a = gsl_complex_rect(-0.000117019946243633813,-0.000450917530047182554);
        *b = gsl_complex_rect(-0.000171337835527373982,0.00132059445165182243);
        break;
        case 5:
        *a = gsl_complex_rect(-0.0218589721702940673,-0.00523876961256432212);
        *b = gsl_complex_rect(0.0488267177114627294,0.0415074442551386387);
        break;
        case 6:
        *a = gsl_complex_rect(-1.220790901269847527,-0.236893925052021750);
        *b = gsl_complex_rect(2.591486551263149471,2.404933894000426047);
        break;
        case 7:
        *a = gsl_complex_rect(-22.514625855155557690,-55.623907050365362497);
        *b = gsl_complex_rect(-43.675188242376714544,164.421821533868615198);
        break;
        case 8:
        *a = gsl_complex_rect(1939.251073448379473234,12.475098475038992649);
        *b = gsl_complex_rect(-4052.273683400929841763,-3694.490657407860271925);
        break;
        case 9:
        *a = gsl_complex_rect(-31195.242506641886866766,11129.366826404318139359);
        *b = gsl_complex_rect(82915.787936565613332283,43101.341852362617253660);
        break;
        case 10:
        *a = gsl_complex_rect(246008.113121887508591838,-13409.753242896082955272);
        *b = gsl_complex_rect(-454161.417817375165755840,-524788.703881302459800912);
        break;
        case 11:
        *a = gsl_complex_rect(-560698.611396051473207968,-412629.256090998659764942);
        *b = gsl_complex_rect(-128601.959446731032061736,1955193.832628681094297427);
        break;
        case 12:
        *a = gsl_complex_rect(76998.112421476293620880,691893.869381105492291665);
        *b = gsl_complex_rect(1628946.548655847031109700,-1088969.480171653175994800);
        break;
        case 13:
        *a = gsl_complex_rect(134617.243527793463413968,-206344.398879070883749585);
        *b = gsl_complex_rect(-681561.058089277244582407,-130920.973307774865193561);
        break;
        case 14:
        *a = gsl_complex_rect(-25235.935653472152988056,21451.189074051074287957);
        *b = gsl_complex_rect(78784.750951183600305216,50256.507799831144930004);
        break;
        case 15:
        *a = gsl_complex_rect(958.821788780688620313,-1685.678229384048972271);
        *b = gsl_complex_rect(-5225.659640787960704555,-1662.126603479601054409);
        break;
        case 16:
        *a = gsl_complex_rect(36.914735631845878659,47.309864671438115295);
        *b = gsl_complex_rect(120.556938263078583107,-120.033924619755946995);
        break;
        case 17:
        *a = gsl_complex_rect(-0.411631872053758450,1.154133085728225467);
        *b = gsl_complex_rect(3.438836982285023958,1.020294784165429366);
       }
      if (arg<0.04)
       {
        *z0=gsl_complex_rect(8.3,0.0);
        *a=gsl_complex_rect(1.97486174966769314e-8,0.0);
        *b=gsl_complex_rect(-5.74753973633800903e-8,0.0);
       }
      if (arg>3.0543)
       {
        *z0=gsl_complex_rect(-8.1,0.0);
        *a=gsl_complex_rect(-0.142908147093581424,0.0);
        *b=gsl_complex_rect(0.856218586328624562,0.0);
       }
     }
    else
     {
      sector = (int) floor(arg/0.209439510239319549);
      if (sector==15) sector = 14;
      *z0 = gsl_complex_polar(6.7,M_PI/30+((double) sector)*M_PI/15);
      switch (sector)
       {
        case 0:
        *a = gsl_complex_rect(-0.000000498013167756023292,-0.00000184854213885407739);
        *b = gsl_complex_rect(0.00000106141207289439112,0.00000491032321159520191);
        break;
        case 1:
        *a = gsl_complex_rect(0.00000336764005452339539,0.00000479272362847353000);
        *b = gsl_complex_rect(-0.00000683653066279643336,-0.0000137462950976495803);
        break;
        case 2:
        *a = gsl_complex_rect(-0.0000213017825830797453,-0.0000442950017766455178);
        *b = gsl_complex_rect(0.0000250377490153729240,0.000126060988405383066);
        break;
        case 3:
        *a = gsl_complex_rect(-0.000453121182937822238,0.000798928981097729518);
        *b = gsl_complex_rect(0.00182980866079213440,-0.00154339067057850096);
        break;
        case 4:
        *a = gsl_complex_rect(0.0174497968637443698,0.0227858328776185344);
        *b = gsl_complex_rect(-0.0145310120436193822,-0.0730669102254068242);
        break;
        case 5:
        *a = gsl_complex_rect(0.694446791856491758,0.814918881243262267);
        *b = gsl_complex_rect(-0.397518235624576301,-2.738120635415331876);
        break;
        case 6:
        *a = gsl_complex_rect(-11.764490767924425694,31.325544406980328369);
        *b = gsl_complex_rect(73.649566627644512546,-44.568572652297977178);
        break;
        case 7:
        *a = gsl_complex_rect(-407.494058366312338032,-474.534327075926538419);
        *b = gsl_complex_rect(-104.113703049367037165,1599.301554627228909985);
        break;
        case 8:
        *a = gsl_complex_rect(4356.084334264410458947,2930.518760022207682451);
        *b = gsl_complex_rect(-1280.027816992929934135,-13349.872276666335219657);
        break;
        case 9:
        *a = gsl_complex_rect(-10764.964007299631029460,-11926.966584303603583136);
        *b = gsl_complex_rect(-10459.124120878866095322,39615.596540039307247224);
        break;
        case 10:
        *a = gsl_complex_rect(4946.574049797097648430,15286.215591942850369831);
        *b = gsl_complex_rect(29078.550934092689266044,-28865.665458499383732925);
        break;
        case 11:
        *a = gsl_complex_rect(-359.861527780700026412,-5237.739069198683865596);
        *b = gsl_complex_rect(-12201.342452690017259261,5566.399530314013605794);
        break;
        case 12:
        *a = gsl_complex_rect(207.211780742055496102,590.167329379022769924);
        *b = gsl_complex_rect(1332.979044785335753163,-889.815847689268182778);
        break;
        case 13:
        *a = gsl_complex_rect(-33.011881081169634562,-5.474417285844980876);
        *b = gsl_complex_rect(-1.773154524678477923,86.064325647284144866);
        break;
        case 14:
        *a = gsl_complex_rect(-0.369525268938545475,-0.982363120132267512);
        *b = gsl_complex_rect(-2.640584453161403622,1.000850197648983227);
       }
      if (arg<0.04)
       {
        *z0=gsl_complex_rect(6.9,0.0);
        *a=gsl_complex_rect(9.78611333926603763e-7,0.0);
        *b=gsl_complex_rect(-2.60492608708626216e-6,0.0);
       }
      if (arg>3.0543)
       {
        *z0=gsl_complex_rect(-6.7,0.0);
        *a=gsl_complex_rect(-0.0783124718012559192,0.0);
        *b=gsl_complex_rect(-0.887907965255553545,0.0);
       }
     }
   }
  else
   {
    if (mod>4.6)
     {
      sector = (int) floor(arg/0.261799387799149436);
      if (sector==12) sector = 11;
      *z0 = gsl_complex_polar(5.3,M_PI/24+((double) sector)*M_PI/12);
      switch (sector)
       {
        case 0:
        *a = gsl_complex_rect(-0.00000299476080804093545,-0.0000631787904853607215);
        *b = gsl_complex_rect(-0.00000215173224969956916,0.000148395647117400246);
        break;
        case 1:
        *a = gsl_complex_rect(-0.0000211526279135184427,0.000212253053197331656);
        *b = gsl_complex_rect(0.000140525421643900777,-0.000479067973173668073);
        break;
        case 2:
        *a = gsl_complex_rect(0.00162049409876805045,-0.00119965804791216987);
        *b = gsl_complex_rect(-0.00444923652903960177,0.00150378944888197446);
        break;
        case 3:
        *a = gsl_complex_rect(-0.0128350452818642183,-0.0357139465644490702);
        *b = gsl_complex_rect(-0.00820980981803076405,0.0874268449810398382);
        break;
        case 4:
        *a = gsl_complex_rect(-0.362782027418170725,-0.834518057823886484);
        *b = gsl_complex_rect(-0.329814762520317216,2.062891102183896320);
        break;
        case 5:
        *a = gsl_complex_rect(11.531506362296704366,-12.676590465250343557);
        *b = gsl_complex_rect(-38.696733591956182277,5.095298554790873631);
        break;
        case 6:
        *a = gsl_complex_rect(36.473947017219054415,157.956742301825197010);
        *b = gsl_complex_rect(210.460846217145665009,-300.356420369671554427);
        break;
        case 7:
        *a = gsl_complex_rect(-264.708864377609537581,-478.734565951523681485);
        *b = gsl_complex_rect(-560.542602601722304465,1097.894429871872581481);
        break;
        case 8:
        *a = gsl_complex_rect(282.241916811988637682,468.611849950767167360);
        *b = gsl_complex_rect(670.533293080147449217,-1034.391272631194629515);
        break;
        case 9:
        *a = gsl_complex_rect(-118.557772416653216305,-110.565648036298482925);
        *b = gsl_complex_rect(-154.886211743644654305,332.442288278282390695);
        break;
        case 10:
        *a = gsl_complex_rect(16.745851738821476342,-3.647478660993395055);
        *b = gsl_complex_rect(-14.936631106062903128,-36.055398564922105481);
        break;
        case 11:
        *a = gsl_complex_rect(0.565834486636532210,0.702465032137459288);
        *b = gsl_complex_rect(1.693217682289265054,-1.266250187624501159);
       }
      if (arg<0.04)
       {
        *z0=gsl_complex_rect(5.5,0.0);
        *a=gsl_complex_rect(0.0000336853119085998144,0.0);
        *b=gsl_complex_rect(-0.0000804633913055651434,0.0);
       }
      if (arg>3.0543)
       {
        *z0=gsl_complex_rect(-5.3,0.0);
        *a=gsl_complex_rect(0.182567931068339499,0.0);
        *b=gsl_complex_rect(0.754575419947011041,0.0);
       }
     }
    else
     {
      sector = (int) floor(arg/0.349065850398865915);
      if (sector==9) sector = 8;
      *z0 = gsl_complex_polar(3.9,M_PI/18+((double) sector)*M_PI/9);
      switch (sector)
       {
        case 0:
        *a = gsl_complex_rect(0.000278077618161181118,-0.00136330080544352249);
        *b = gsl_complex_rect(-0.000785309515894828443,0.00271731738202666136);
        break;
        case 1:
        *a = gsl_complex_rect(-0.00431400446443127479,0.00302535550848715075);
        *b = gsl_complex_rect(0.00992381135829497437,-0.00384892534814174421);
        break;
        case 2:
        *a = gsl_complex_rect(0.0231635768410815889,0.0475722413876221875);
        *b = gsl_complex_rect(-0.00488612358065397538,-0.105500920047767831);
        break;
        case 3:
        *a = gsl_complex_rect(0.390152428728285509,0.652075853485570872);
        *b = gsl_complex_rect(0.0586221986844014905,-1.491459402174807467);
        break;
        case 4:
        *a = gsl_complex_rect(-4.926039939271872626,5.848213169295779524);
        *b = gsl_complex_rect(14.674847227356971188,-1.648177173542352832);
        break;
        case 5:
        *a = gsl_complex_rect(-6.720520194312951484,-28.240386236681307535);
        *b = gsl_complex_rect(-36.332727991434027176,41.793639654928228293);
        break;
        case 6:
        *a = gsl_complex_rect(21.097951488433551829,19.939893505140831342);
        *b = gsl_complex_rect(18.029950273188565088,-52.359846500154332147);
        break;
        case 7:
        *a = gsl_complex_rect(-7.532498177950830124,1.339745783114311969);
        *b = gsl_complex_rect(5.911688950003561545,13.522360346905228438);
        break;
        case 8:
        *a = gsl_complex_rect(-0.399255021084886709,-0.620073474688977667);
        *b = gsl_complex_rect(-1.356140170328659265,0.747979061496786733);
       }
      if (arg<0.04)
       {
        *z0=gsl_complex_rect(4.1,0.0);
        *a=gsl_complex_rect(0.000773629663781597187,0.0);
        *b=gsl_complex_rect(-0.00161061146122698678,0.0);
       }
      if (arg>3.0543)
       {
        *z0=gsl_complex_rect(-3.9,0.0);
        *a=gsl_complex_rect(-0.147419905640744195,0.0);
        *b=gsl_complex_rect(-0.747558085535477472,0.0);
       }
      if (mod<3.2)
       {
        if (arg<0.349)
         {
          *z0=gsl_complex_rect(2.7,0.0);
          *a=gsl_complex_rect(0.0111985354510658809,0.0);
          *b=gsl_complex_rect(-0.0193255606923776375,0.0);
         }
        else if (arg<0.698)
         {
          *z0=gsl_complex_rect(2.25,1.3);
          *a=gsl_complex_rect(-0.0151244789566325122,-0.0263137408361838169);
          *b=gsl_complex_rect(0.0147456584151680829,0.0487809489452374034);
         }
        else
         {
          *z0=gsl_complex_rect(1.67,1.99);
          *a=gsl_complex_rect(-0.103601851239231523,-0.0262786874985897695);
          *b=gsl_complex_rect(0.141916105307775135,0.104258228554974092);
         }
       }
     }
   }
  return;
 }

void airy_lookup_bi(double mod, double arg, gsl_complex *a, gsl_complex *b, gsl_complex *z0)
 {
  int sector;
  if (mod>6.0)
   {
    if (mod>7.4)
     {
      sector = (int) floor(arg/0.174532925199432957);
      if (sector==18) sector = 17;
      *z0 = gsl_complex_polar(8.1,M_PI/36+((double) sector)*M_PI/18);
      switch (sector)
       {
        case 0:
        *a = gsl_complex_rect(-558529.226580244337773619,1275393.447634993072821797);
        *b = gsl_complex_rect(-1732448.704914136372739047,3515097.016204972926171122);
        break;
        case 1:
        *a = gsl_complex_rect(439508.304244119416649424,-222781.739188122888161954);
        *b = gsl_complex_rect(1311419.354378659708640699,-454799.280543550765239732);
        break;
        case 2:
        *a = gsl_complex_rect(-65161.111802372053136092,11918.613706394351010666);
        *b = gsl_complex_rect(-186715.699308521217501252,-8262.073971492103717457);
        break;
        case 3:
        *a = gsl_complex_rect(3346.406286163157518109,-1960.858563780900306996);
        *b = gsl_complex_rect(10713.234583809513708257,-2346.771879133714761528);
        break;
        case 4:
        *a = gsl_complex_rect(16.627882073362101451,118.857941961930807513);
        *b = gsl_complex_rect(-88.775497062388944113,328.461965674224140787);
        break;
        case 5:
        *a = gsl_complex_rect(-1.872339171752590886,1.609244243294068826);
        *b = gsl_complex_rect(-6.935027712174787454,1.622807859701515936);
        break;
        case 6:
        *a = gsl_complex_rect(0.204271844264402619,-1.189858113961444132);
        *b = gsl_complex_rect(-2.531011694098641311,2.614552835893919778);
        break;
        case 7:
        *a = gsl_complex_rect(55.624155283403016727,-22.513727823137248870);
        *b = gsl_complex_rect(-164.422845362483874729,-43.672729567854732653);
        break;
        case 8:
        *a = gsl_complex_rect(-12.475077336782764614,1939.251053838226351319);
        *b = gsl_complex_rect(3694.490740019415241220,-4052.273683225588960808);
        break;
        case 9:
        *a = gsl_complex_rect(-11129.366827897273281854,-31195.242505853106484354);
        *b = gsl_complex_rect(-43101.341856916666222493,82915.787934907102151989);
        break;
        case 10:
        *a = gsl_complex_rect(13409.753243043921690847,246008.113121715217599676);
        *b = gsl_complex_rect(524788.703881953814630753,-454161.417817336962420697);
        break;
        case 11:
        *a = gsl_complex_rect(412629.256091004048825189,-560698.611395971303468818);
        *b = gsl_complex_rect(-1955193.832628867401099696,-128601.959446594329448509);
        break;
        case 12:
        *a = gsl_complex_rect(-691893.869381172226792260,76998.112421431541688229);
        *b = gsl_complex_rect(1088969.480171678410529483,1628946.548655617333379424);
        break;
        case 13:
        *a = gsl_complex_rect(206344.398879294011493429,134617.243527751576809377);
        *b = gsl_complex_rect(130920.973308133627667224,-681561.058088732256420459);
        break;
        case 14:
        *a = gsl_complex_rect(-21451.189075480655708380,-25235.935652573606099151);
        *b = gsl_complex_rect(-50256.507803544482229015,78784.750948068933798276);
        break;
        case 15:
        *a = gsl_complex_rect(1685.678256936067861969,958.821780279498296088);
        *b = gsl_complex_rect(1662.126644937228196286,-5225.659569331925895489);
        break;
        case 16:
        *a = gsl_complex_rect(-47.310518273460555430,36.914071639720057107);
        *b = gsl_complex_rect(120.035541980043991109,120.554822264227655964);
        break;
        case 17:
        *a = gsl_complex_rect(-1.197232705740973243,-0.398846715021573711);
        *b = gsl_complex_rect(-1.063357695753367352,3.318117262231328193);
       }
      if (arg<0.04)
       {
        *z0=gsl_complex_rect(7.9,0.0);
        *a=gsl_complex_rect(907790.616061993809,0.0);
        *b=gsl_complex_rect(2.52192411395678168e6,0.0);
       }
      if (arg>3.0543)
       {
        *z0=gsl_complex_rect(-8.1,0.0);
        *a=gsl_complex_rect(-0.302303309060702033,0.0);
        *b=gsl_complex_rect(-0.416156639540127581,0.0);
       }
     }
    else
     {
      sector = (int) floor(arg/0.209439510239319549);
      if (sector==15) sector = 14;
      *z0 = gsl_complex_polar(6.7,M_PI/30+((double) sector)*M_PI/15);
      switch (sector)
       {
        case 0:
        *a = gsl_complex_rect(-6718.498016141077682781,31423.076110991688451355);
        *b = gsl_complex_rect(-21499.862164918266666618,79075.350101438181438665);
        break;
        case 1:
        *a = gsl_complex_rect(4614.440624185876042327,-9431.891715788980139568);
        *b = gsl_complex_rect(15566.945504545861683191,-21842.629247584893125288);
        break;
        case 2:
        *a = gsl_complex_rect(-231.266041501962188690,1229.411601495542069767);
        *b = gsl_complex_rect(-1418.971457242574856197,2874.185277325368810183);
        break;
        case 3:
        *a = gsl_complex_rect(-51.703039071181529005,-42.493396840903517232);
        *b = gsl_complex_rect(-82.994675311540956918,-150.842769073588611175);
        break;
        case 4:
        *a = gsl_complex_rect(0.365112412527771787,-2.088477901353265809);
        *b = gsl_complex_rect(3.499709326677641330,-4.359576834211176093);
        break;
        case 5:
        *a = gsl_complex_rect(-0.807480779371119503,0.637530774755938730);
        *b = gsl_complex_rect(2.836355996785682733,-0.509542824443435467);
        break;
        case 6:
        *a = gsl_complex_rect(-31.327128164872260118,-11.765421432328388375);
        *b = gsl_complex_rect(44.566946721399934137,73.645063587926350648);
        break;
        case 7:
        *a = gsl_complex_rect(474.534334475158589384,-407.493960343336156448);
        *b = gsl_complex_rect(-1599.301724054870717152,-104.113509743079282063);
        break;
        case 8:
        *a = gsl_complex_rect(-2930.518758982007634886,4356.084322595529573272);
        *b = gsl_complex_rect(13349.872302253848772773,-1280.027833965680796302);
        break;
        case 9:
        *a = gsl_complex_rect(11926.96658528956161259918428,-10764.964003599848957277);
        *b = gsl_complex_rect(-39615.596546788050096839,-10459.124113435348884149);
        break;
        case 10:
        *a = gsl_complex_rect(-15286.215594653976618077,4946.574047093341911759);
        *b = gsl_complex_rect(28865.665461571287306500,29078.550924526347909082);
        break;
        case 11:
        *a = gsl_complex_rect(5237.739079824331170108,-359.861522847099249784);
        *b = gsl_complex_rect(-5566.399532219090247978,-12201.342422044205071501);
        break;
        case 12:
        *a = gsl_complex_rect(-590.167410569794272249,207.211725322644480679);
        *b = gsl_complex_rect(889.815930383603186378,1332.978801403549967315);
        break;
        case 13:
        *a = gsl_complex_rect(5.474431385915244585,-33.010044174399796237);
        *b = gsl_complex_rect(-86.069038359523344910,-1.772311102281900293);
        break;
        case 14:
        *a = gsl_complex_rect(1.035372887759647344,-0.347508845565481187);
        *b = gsl_complex_rect(-1.048748656729446018,-2.499497840255305692);
       }
      if (arg<0.04)
       {
        *z0=gsl_complex_rect(6.5,0.0);
        *a=gsl_complex_rect(22340.6077183969982,0.0);
        *b=gsl_complex_rect(56062.4958425228607,0.0);
       }
      if (arg>3.0543)
       {
        *z0=gsl_complex_rect(-6.7,0.0);
        *a=gsl_complex_rect(0.341727738606750260,0.0);
        *b=gsl_complex_rect(-0.190098777163749660,0.0);
       }
     }
   }
  else
   {
    if (mod>4.6)
     {
      sector = (int) floor(arg/0.261799387799149436);
      if (sector==12) sector = 11;
      *z0 = gsl_complex_polar(5.3,M_PI/24+((double) sector)*M_PI/12);
      switch (sector)
       {
        case 0:
        *a = gsl_complex_rect(20.245426814435152207,1093.901452950321269133);
        *b = gsl_complex_rect(-127.006310754446323167,2462.151534334939077966);
        break;
        case 1:
        *a = gsl_complex_rect(-94.782225168479687982,-310.063071234672342043);
        *b = gsl_complex_rect(-64.171979214391714810,-730.693286151510822793);
        break;
        case 2:
        *a = gsl_complex_rect(32.650945030599909028,10.426612884358851175);
        *b = gsl_complex_rect(61.927906323985651690,47.517600332057208716);
        break;
        case 3:
        *a = gsl_complex_rect(0.241875100827218364,1.795374658120980130);
        *b = gsl_complex_rect(-1.579062021393483240,3.894637152166555444);
        break;
        case 4:
        *a = gsl_complex_rect(0.848001053842699526,-0.288088612147880764);
        *b = gsl_complex_rect(-2.136098139439629076,-0.170177215249677354);
        break;
        case 5:
        *a = gsl_complex_rect(12.680596901410687529,11.531963736888629092);
        *b = gsl_complex_rect(-5.089096040516567615,-38.689679715697793095);
        break;
        case 6:
        *a = gsl_complex_rect(-157.956991192304654155,36.473600536774768535);
        *b = gsl_complex_rect(300.356656040474685820,210.459875921654205271);
        break;
        case 7:
        *a = gsl_complex_rect(478.734623943236290803,-264.708751953973648146);
        *b = gsl_complex_rect(-1097.894574540610118117,-560.542343421189625366);
        break;
        case 8:
        *a = gsl_complex_rect(-468.611918316635528764,282.241810377874364329);
        *b = gsl_complex_rect(1034.391424753751327680,670.533038203079269517);
        break;
        case 9:
        *a = gsl_complex_rect(110.565823651925420443,-118.557383630953103388);
        *b = gsl_complex_rect(-332.443010743425606638,-154.885522498996482369);
        break;
        case 10:
        *a = gsl_complex_rect(3.649085781057914687,16.742153376032015515);
        *b = gsl_complex_rect(36.064608658094175446,-14.934786509263213107);
        break;
        case 11:
        *a = gsl_complex_rect(-0.760409929247544386,0.516811161929970686);
        *b = gsl_complex_rect(1.367896840330848079,1.549999754654686720);
       }
      if (arg<0.04)
       {
        *z0=gsl_complex_rect(5.1,0.0);
        *a=gsl_complex_rect(819.209658679961277,0.0);
        *b=gsl_complex_rect(1807.33448135136778,0.0);
       }
      if (arg>3.0543)
       {
        *z0=gsl_complex_rect(-5.3,0.0);
        *a=gsl_complex_rect(-0.323716076748792404,0.0);
        *b=gsl_complex_rect(0.405556940883315528,0.0);
       }
     }
    else
     {
      sector = (int) floor(arg/0.349065850398865915);
      if (sector==9) sector = 8;
      *z0 = gsl_complex_polar(3.9,M_PI/18+((double) sector)*M_PI/9);
      switch (sector)
       {
        case 0:
        *a = gsl_complex_rect(16.601467107644055289,55.634582059231978874);
        *b = gsl_complex_rect(21.133773886433404870,108.720649997479224303);
        break;
        case 1:
        *a = gsl_complex_rect(-14.383389979736656919,-5.207676407906256874);
        *b = gsl_complex_rect(-23.765554892203047221,-17.519650020449298055);
        break;
        case 2:
        *a = gsl_complex_rect(-0.023884265619407301,-1.496417360513051860);
        *b = gsl_complex_rect(1.495423695649794520,-2.646791784258125683);
        break;
        case 3:
        *a = gsl_complex_rect(-0.659527602899413950,0.284591312773911343);
        *b = gsl_complex_rect(1.605423336516328350,-0.119224631502940362);
        break;
        case 4:
        *a = gsl_complex_rect(-5.858710599720740606,-4.926966004259099473);
        *b = gsl_complex_rect(1.634837553413198568,14.658256881741154983);
        break;
        case 5:
        *a = gsl_complex_rect(28.242231182049853961,-6.718436965670084911);
        *b = gsl_complex_rect(-41.794996776329057792,-36.327236150152172121);
        break;
        case 6:
        *a = gsl_complex_rect(-19.940775161383171961,21.095312104554362894);
        *b = gsl_complex_rect(52.363924013517555970,18.026029050938499690);
        break;
        case 7:
        *a = gsl_complex_rect(-1.344192502522298750,-7.522944104034740729);
        *b = gsl_complex_rect(-13.543397817730666190,5.908431672902787801);
        break;
        case 8:
        *a = gsl_complex_rect(0.707766208050378964,-0.340021058812675720);
        *b = gsl_complex_rect(-0.845016967250801511,-1.168521092980009461);
       }
      if (arg<0.04)
       {
        *z0=gsl_complex_rect(3.7,0.0);
        *a=gsl_complex_rect(47.5607474995894429,0.0);
        *b=gsl_complex_rect(87.8907272628334109,0.0);
       }
      if (arg>3.0543)
       {
        *z0=gsl_complex_rect(-3.9,0.0);
        *a=gsl_complex_rect(0.372890578319395619,0.0);
        *b=gsl_complex_rect(-0.268298362892145679,0.0);
       }
     }
   }
  return;
 }

void airy_series_taylor(gsl_complex *dp, gsl_complex *dq, gsl_complex zd, gsl_complex z0, double k)
 {
  // If k=0, compute Ai or Bi; if k=1, compute Ai' or Bi'.
  gsl_complex c,f,g,z2,z3;
  c   = GSL_COMPLEX_ONE;
  z2  = gsl_complex_mul(z0,z0);
  z3  = gsl_complex_mul(z2,z0);
  *dp = GSL_COMPLEX_ZERO;
  if (k<0.5)
   {
    *dp = c;
    c   = gsl_complex_mul(c,zd);
   }
  *dq = c;
  c   = gsl_complex_mul(c,zd);
  c   = gsl_complex_div_real(c,2.0-k);
  *dp = gsl_complex_add(*dp,gsl_complex_mul(z0,c));
  c   = gsl_complex_mul(c,zd);
  c   = gsl_complex_div_real(c,3.0-k);
  *dp = gsl_complex_add(*dp,c);
  *dq = gsl_complex_add(*dq,gsl_complex_mul(z0,c));
  c   = gsl_complex_mul(c,zd);
  c   = gsl_complex_div_real(c,4.0-k);
  f   = z2;
  g   = gsl_complex_rect(2.0,0.0);
  ADDON;
  c = gsl_complex_div_real(c,5.0-k);
  f = gsl_complex_mul_real(z0,4.0);
  g = z2;
  ADDON;
  c = gsl_complex_div_real(c,6.0-k);
  f = gsl_complex_add_real(z3,4.0);
  g = gsl_complex_mul_real(z0,6.0);
  ADDON;
  c = gsl_complex_div_real(c,7.0-k);
  f = gsl_complex_mul_real(z2,9.0);
  g = gsl_complex_add_real(z3,10.0);
  ADDON;
  c = gsl_complex_div_real(c,8.0-k);
  f = gsl_complex_mul(z0,gsl_complex_add_real(z3,28.0));
  g = gsl_complex_mul_real(z2,12.0);
  ADDON;
  c = gsl_complex_div_real(c,9.0-k);
  f = gsl_complex_add_real(gsl_complex_mul_real(z3,16.0),28.0);
  g = gsl_complex_mul(z0,gsl_complex_add_real(z3,52.0));
  ADDON;
  c = gsl_complex_div_real(c,10.0-k);
  f = gsl_complex_mul(z2,gsl_complex_add_real(z3,100.0));
  g = gsl_complex_add_real(gsl_complex_mul_real(z3,20.0),80.0);
  ADDON;
  c = gsl_complex_div_real(c,11.0-k);
  f = gsl_complex_mul(z0,gsl_complex_add_real(gsl_complex_mul_real(z3,25.0),280.0));
  g = gsl_complex_mul(z2,gsl_complex_add_real(z3,160.0));
  ADDON;
  c = gsl_complex_div_real(c,12.0-k);
  f = gsl_complex_add_real(gsl_complex_mul(gsl_complex_add_real(z3,260.0),z3),280.0);
  g = gsl_complex_mul(z0,gsl_complex_add_real(gsl_complex_mul_real(z3,30.0),600.0));
  ADDON;
  c = gsl_complex_div_real(c,13.0-k);
  f = gsl_complex_mul(z2,gsl_complex_add_real(gsl_complex_mul_real(z3,36.0),1380.0));
  g = gsl_complex_add_real(gsl_complex_mul(gsl_complex_add_real(z3,380.0),z3),880.0);
  ADDON;
  c = gsl_complex_div_real(c,14.0-k);
  f = gsl_complex_mul(z0,gsl_complex_add_real(gsl_complex_mul(gsl_complex_add_real(z3,560.0),z3),3640.0));
  g = gsl_complex_mul(z2,gsl_complex_add_real(gsl_complex_mul_real(z3,42.0),2520.0));
  ADDON;
  c = gsl_complex_div_real(c,15.0-k);
  f = gsl_complex_add_real(gsl_complex_mul(gsl_complex_add_real(gsl_complex_mul_real(z3,49.0),4760.0),z3),3640.0);
  g = gsl_complex_mul(z0,gsl_complex_add_real(gsl_complex_mul(gsl_complex_add_real(z3,770.0),z3),8680.0));
  ADDON;
  c = gsl_complex_div_real(c,16.0-k);
  f = gsl_complex_mul(z2,gsl_complex_add_real(gsl_complex_mul(gsl_complex_add_real(z3,1064.0),z3),22960.0));
  g = gsl_complex_add_real(gsl_complex_mul(gsl_complex_add_real(gsl_complex_mul_real(z3,56.0),7840.0),z3),12320.0);
  ADDON;
  c = gsl_complex_div_real(c,17.0-k);
  f = gsl_complex_mul(z0,gsl_complex_add_real(gsl_complex_mul(gsl_complex_add_real(gsl_complex_mul_real(z3,64.0),13160.0),z3),58240.0));
  g = gsl_complex_mul(z2,gsl_complex_add_real(gsl_complex_mul(gsl_complex_add_real(z3,1400.0),z3),46480.0));
  ADDON;
  c = gsl_complex_div_real(c,18.0-k);
  f = gsl_complex_add_real(gsl_complex_mul(gsl_complex_add_real(gsl_complex_mul(gsl_complex_add_real(z3,1848.0),z3),99120.0),z3),58240.0);
  g = gsl_complex_mul(z0,gsl_complex_add_real(gsl_complex_mul(gsl_complex_add_real(gsl_complex_mul_real(z3,72.0),20160.0),z3),151200.0));
  ADDON;
  c = gsl_complex_div_real(c,19.0-k);
  f = gsl_complex_mul(z2,gsl_complex_add_real(gsl_complex_mul(gsl_complex_add_real(gsl_complex_mul_real(z3,81.0),31248.0),z3),448560.0));
  g = gsl_complex_add_real(gsl_complex_mul(gsl_complex_add_real(gsl_complex_mul(gsl_complex_add_real(z3,2352.0),z3),179760.0),z3),209440.0);
  ADDON;
  c = gsl_complex_div_real(c,20.0-k);
  f = gsl_complex_mul(z0,gsl_complex_add_real(gsl_complex_mul(gsl_complex_add_real(gsl_complex_mul(gsl_complex_add_real(z3,3000.0),z3),336000.0),z3),1106560.0));
  g = gsl_complex_mul(z2,gsl_complex_add_real(gsl_complex_mul(gsl_complex_add_real(gsl_complex_mul_real(z3,90.0),45360.0),z3),987840.0));
  ADDON;
  c = gsl_complex_div_real(c,21.0-k);
  f = gsl_complex_add_real(gsl_complex_mul(gsl_complex_add_real(gsl_complex_mul(gsl_complex_add_real(gsl_complex_mul_real(z3,100.0),66360.0),z3),2331840.0),z3),1106560.0);
  g = gsl_complex_mul(z0,gsl_complex_add_real(gsl_complex_mul(gsl_complex_add_real(gsl_complex_mul(gsl_complex_add_real(z3,3720.0),z3),562800.0),z3),3082240.0));
  ADDON;
  c = gsl_complex_div_real(c,22.0-k);
  f = gsl_complex_mul(z2,gsl_complex_add_real(gsl_complex_mul(gsl_complex_add_real(gsl_complex_mul(gsl_complex_add_real(z3,4620.0),z3),960960.0),z3),10077760.0));
  g = gsl_complex_add_real(gsl_complex_mul(gsl_complex_add_real(gsl_complex_mul(gsl_complex_add_real(gsl_complex_mul_real(z3,110.0),92400.0),z3),4583040.0),z3),4188800.0);
  ADDON;
  c = gsl_complex_div_real(c,23.0-k);
  f = gsl_complex_mul(z0,gsl_complex_add_real(gsl_complex_mul(gsl_complex_add_real(gsl_complex_mul(gsl_complex_add_real(gsl_complex_mul_real(z3,121.0),129360.0),z3),9387840.0),z3),24344320.0));
  g = gsl_complex_mul(z2,gsl_complex_add_real(gsl_complex_mul(gsl_complex_add_real(gsl_complex_mul(gsl_complex_add_real(z3,5610.0),z3),1515360.0),z3),23826880.0));
  ADDON;
  c = gsl_complex_div_real(c,24.0-k);
  f = gsl_complex_add_real(gsl_complex_mul(gsl_complex_add_real(gsl_complex_mul(gsl_complex_add_real(gsl_complex_mul(gsl_complex_add_real(z3,6820.0),z3),2420880.0),z3),61378240.0),z3),24344320.0);
  g = gsl_complex_mul(z0,gsl_complex_add_real(gsl_complex_mul(gsl_complex_add_real(gsl_complex_mul(gsl_complex_add_real(gsl_complex_mul_real(z3,132.0),174240.0),z3),16964640.0),z3),71998080.0));
  ADDON;
  c = gsl_complex_div_real(c,25.0-k);
  f = gsl_complex_mul(z2,gsl_complex_add_real(gsl_complex_mul(gsl_complex_add_real(gsl_complex_mul(gsl_complex_add_real(gsl_complex_mul_real(z3,144.0),235620.0),z3),31489920.0),z3),256132800.0));
  g = gsl_complex_add_real(gsl_complex_mul(gsl_complex_add_real(gsl_complex_mul(gsl_complex_add_real(gsl_complex_mul(gsl_complex_add_real(z3,8140.0),z3),3640560.0),z3),129236800.0),z3),96342400.0);
  ADDON;
  c = gsl_complex_div_real(c,26.0-k);
  f = gsl_complex_mul(z0,gsl_complex_add_real(gsl_complex_mul(gsl_complex_add_real(gsl_complex_mul(gsl_complex_add_real(gsl_complex_mul(gsl_complex_add_real(z3,9724.0),z3),5525520.0),z3),286686400.0),z3),608608000.0));
  g = gsl_complex_mul(z2,gsl_complex_add_real(gsl_complex_mul(gsl_complex_add_real(gsl_complex_mul(gsl_complex_add_real(gsl_complex_mul_real(z3,156.0),308880.0),z3),53333280.0),z3),643843200.0));
  ADDON;
  c = gsl_complex_div_real(c,27.0-k);
  f = gsl_complex_add_real(gsl_complex_mul(gsl_complex_add_real(gsl_complex_mul(gsl_complex_add_real(gsl_complex_mul(gsl_complex_add_real(gsl_complex_mul_real(z3,169.0),406120.0),z3),92011920.0),z3),1790588800.0),z3),608608000.0);
  g = gsl_complex_mul(z0,gsl_complex_add_real(gsl_complex_mul(gsl_complex_add_real(gsl_complex_mul(gsl_complex_add_real(gsl_complex_mul(gsl_complex_add_real(z3,11440.0),z3),7996560.0),z3),553352800.0),z3),1896294400.0));
  ADDON;
  c = gsl_complex_div_real(c,28.0-k);
  f = gsl_complex_mul(z2,gsl_complex_add_real(gsl_complex_mul(gsl_complex_add_real(gsl_complex_mul(gsl_complex_add_real(gsl_complex_mul(gsl_complex_add_real(z3,13468.0),z3),11651640.0),z3),1105424320.0),z3),7268060800.0));
  g = gsl_complex_add_real(gsl_complex_mul(gsl_complex_add_real(gsl_complex_mul(gsl_complex_add_real(gsl_complex_mul(gsl_complex_add_real(gsl_complex_mul_real(z3,182.0),520520.0),z3),147987840.0),z3),4004000000.0),z3),2504902400.0);
  ADDON;
  c = gsl_complex_div_real(c,29.0-k);
  f = gsl_complex_mul(z0,gsl_complex_add_real(gsl_complex_mul(gsl_complex_add_real(gsl_complex_mul(gsl_complex_add_real(gsl_complex_mul(gsl_complex_add_real(gsl_complex_mul_real(z3,196.0),668668.0),z3),241200960.0),z3),9531121600.0),z3),17041024000.0));
  g = gsl_complex_mul(z2,gsl_complex_add_real(gsl_complex_mul(gsl_complex_add_real(gsl_complex_mul(gsl_complex_add_real(gsl_complex_mul(gsl_complex_add_real(z3,15652.0),z3),16336320.0),z3),1993351360.0),z3),19280060800.0));
  *dp = gsl_complex_add(*dp,gsl_complex_mul(f,c));
  *dq = gsl_complex_add(*dq,gsl_complex_mul(g,c));
  return;
 }

void airy_ai(gsl_complex in, gsl_complex *out, int *status, char *errText)
 {
  int conjugate,i,max;
  double mod,arg;
  gsl_complex a,b,c,d,dp,dq,zah,zd,z0;
  mod = gsl_complex_abs(in);
  arg = gsl_complex_arg(in);
  if (mod<3.2 && (mod<2.0 || arg>1.05 || arg<-1.05))
   {
    d = gsl_complex_mul_real(airy_series_y1(in),0.355028053887817239);
    d = gsl_complex_sub(d,gsl_complex_mul_real(airy_series_y2(in),0.258819403792806798));
    *status = 0;
    *out    = d;
    return;
   }
  else if (mod<8.8)
   {
    if (arg<0.0)
     {
      conjugate = 1;
      in  = gsl_complex_conjugate(in);
      arg = -arg;
     }
    else
     {
      conjugate = 0;
     }
    airy_lookup_ai(mod,arg,&a,&b,&z0);
    zd = gsl_complex_sub(in,z0);
    airy_series_taylor(&dp,&dq,zd,z0,0.0);
    dp = gsl_complex_mul(a,dp);
    dq = gsl_complex_mul(b,dq);
    d  = gsl_complex_add(dp,dq);
    *status = 0;
    if   (conjugate==1) { *out = gsl_complex_conjugate(d); return; }
    else                { *out = d;                        return; }
   }
  else
   {
    if (log(mod)>440.0)
     {
      *status = 2; // calculation will overflow
      sprintf(errText,"Overflow error within Airy function");
      return;
     }
    if (arg<-2.094395102393195492 || arg>2.094395102393195492)
     {
      zah = gsl_complex_pow_real(gsl_complex_negative(in),1.5);
      max = 29;
      if (mod>10.0) max = 20;
      if (mod>12.0) max = 16;
      if (mod>16.0) max = 12;
      c  = GSL_COMPLEX_ONE;
      dp = GSL_COMPLEX_ZERO;
      dq = GSL_COMPLEX_ZERO;
      for (i=0;i<=max;i+=2)
       {
        dp = gsl_complex_add(dp,c);
        c  = gsl_complex_mul_real(c,((double) (6*i+1)*(6*i+5))/((double) 48*(i+1)));
        c  = gsl_complex_div(c,zah);
        dq = gsl_complex_add(dq,c);
        c  = gsl_complex_mul_real(c,((double) (6*i+7)*(6*i+11))/((double) -48*(i+2)));
        c  = gsl_complex_div(c,zah);
       }
      c = gsl_complex_add_real(gsl_complex_div_real(zah,1.5),0.785398163397448309);
      if (GSL_IMAG(c)>650.0)
       {
        *status = 2; // risk of overflow
        sprintf(errText,"Overflow error within Airy function");
        return;
       }
      dp = gsl_complex_mul(dp,gsl_complex_sin(c));
      dq = gsl_complex_mul(dq,gsl_complex_cos(c));
      d  = gsl_complex_sub(dp,dq);
      d  = gsl_complex_mul(d,gsl_complex_pow_real(gsl_complex_negative(in),-0.25));
      d  = gsl_complex_div_real(d,1.772453850905516027);
     }
    else
     {
      zah = gsl_complex_pow_real(in,1.5);
      max = 29;
      if (mod>10.0) max = 20;
      if (mod>12.0) max = 16;
      if (mod>16.0) max = 12;
      c = GSL_COMPLEX_ONE;
      d = GSL_COMPLEX_ZERO;
      for (i=0;i<=max;i++)
       {
        d = gsl_complex_add(d,c);
        c = gsl_complex_mul_real(c,((double) (6*i+1)*(6*i+5))/((double) -48*(i+1)));
        c = gsl_complex_div(c,zah);
       }
      d = gsl_complex_mul(d,gsl_complex_pow_real(in,-0.25));
      d = gsl_complex_div_real(d,3.544907701811032054);
      c = gsl_complex_div_real(zah,-1.5);
      if (GSL_REAL(c)>650.0)
       {
        *status = 2; // risk of overflow
        sprintf(errText,"Overflow error within Airy function");
        return;
       }
      d = gsl_complex_mul(d,gsl_complex_exp(c));
     }
    *status = 0;
    *out = d;
    return;
   }
 }

void airy_bi(gsl_complex in, gsl_complex *out, int *status, char *errText)
 {
  int conjugate,i,max;
  double mod,arg;
  gsl_complex a,b,c,d,dp,dq,zah,zd,z0;
  mod = gsl_complex_abs(in);
  arg = gsl_complex_arg(in);
  if (mod<3.2)
   {
    d = gsl_complex_mul_real(airy_series_y1(in), 0.614926627446000735);
    d = gsl_complex_add(d,gsl_complex_mul_real(airy_series_y2(in),0.448288357353826357));
    *status = 0;
    *out = d;
    return;
   }
  else if (mod<8.8)
   {
    if (arg<0.0)
     {
      conjugate = 1;
      in  = gsl_complex_conjugate(in);
      arg = -arg;
     }
    else
     {
      conjugate = 0;
     }
    airy_lookup_bi(mod,arg,&a,&b,&z0);
    zd = gsl_complex_sub(in,z0);
    airy_series_taylor(&dp,&dq,zd,z0,0.0);
    dp = gsl_complex_mul(a,dp);
    dq = gsl_complex_mul(b,dq);
    d  = gsl_complex_add(dp,dq);
    *status = 0;
    if (conjugate==1) { *out = gsl_complex_conjugate(d); return; }
    else              { *out = d;                        return; }
   }
  else
   {
    if (log(mod)>440.0)
     {
      *status = 2; // calculation will overflow
      sprintf(errText,"Overflow error within Airy function");
      return;
     }
    if (arg<-1.63 || arg>1.63)
     {
      zah = gsl_complex_pow_real(gsl_complex_negative(in),1.5);
      max = 29;
      if (mod>10.0) max = 20;
      if (mod>12.0) max = 16;
      if (mod>16.0) max = 12;
      c  = GSL_COMPLEX_ONE;
      dp = GSL_COMPLEX_ZERO;
      dq = GSL_COMPLEX_ZERO;
      for (i=0;i<=max;i+=2)
       {
        dp = gsl_complex_add(dp,c);
        c  = gsl_complex_mul_real(c,((double) (6*i+1)*(6*i+5))/((double) 48*(i+1)));
        c  = gsl_complex_div(c,zah);
        dq = gsl_complex_add(dq,c);
        c  = gsl_complex_mul_real(c,((double) (6*i+7)*(6*i+11))/((double) -48*(i+2)));
        c  = gsl_complex_div(c,zah);
       }
      c = gsl_complex_add_real(gsl_complex_div_real(zah,1.5),0.785398163397448309);
      if (GSL_IMAG(c)>650.0)
       {
        *status = 2; // risk of overflow
        sprintf(errText,"Overflow error within Airy function");
        return;
       }
      dp = gsl_complex_mul(dp,gsl_complex_cos(c));
      dq = gsl_complex_mul(dq,gsl_complex_sin(c));
      d  = gsl_complex_add(dp,dq);
      d  = gsl_complex_mul(d,gsl_complex_pow_real(gsl_complex_negative(in),-0.25));
      d  = gsl_complex_div_real(d,1.772453850905516027);
     }
    else if (arg>-0.51 && arg<0.51)
     {
      zah = gsl_complex_pow_real(in,1.5);
      max = 29;
      if (mod>10.0) max = 20;
      if (mod>12.0) max = 16;
      if (mod>16.0) max = 12;
      c = GSL_COMPLEX_ONE;
      d = GSL_COMPLEX_ZERO;
      for (i=0;i<=max;i++)
       {
        d = gsl_complex_add(d,c);
        c = gsl_complex_mul_real(c,((double) (6*i+1)*(6*i+5))/((double) 48*(i+1)));
        c = gsl_complex_div(c,zah);
       }
      d = gsl_complex_mul(d,gsl_complex_pow_real(in,-0.25));
      d = gsl_complex_div_real(d,1.772453850905516027);
      c = gsl_complex_div_real(zah,1.5);
      if (GSL_REAL(c)>650.0)
       {
        *status = 2; // risk of overflow
        sprintf(errText,"Overflow error within Airy function");
        return;
       }
      d = gsl_complex_mul(d,gsl_complex_exp(c));
     }
    else
     {
      /* The asymptotic expansion is poor here due to Stokes' Phenomenon. */
      c = gsl_complex_polar(1,2.094395102393195492);
      d = gsl_complex_conjugate(c);
      airy_ai(gsl_complex_mul(in,c),&a,status,errText);
      if (*status==2) return;
      airy_ai(gsl_complex_mul(in,d),&b,status,errText);
      if (*status==2) return;
      a = gsl_complex_mul(a,c);
      b = gsl_complex_mul(b,d);
      d = gsl_complex_sub(a,b);
      d = gsl_complex_mul_imag(d,-1.0);
     }
    *status = 0;
    *out = d;
    return;
   }
 }

void airy_ai_diff(gsl_complex in, gsl_complex *out, int *status, char *errText)
 {
  int conjugate,i,max;
  double mod,arg;
  gsl_complex a,b,c,d,dp,dq,zah,zd,z0;
  mod = gsl_complex_abs(in);
  arg = gsl_complex_arg(in);
  if (mod<3.2 && (mod<2.0 || arg>1.05 || arg<-1.05))
   {
    d = gsl_complex_mul_real(airy_series_y1_diff(in),0.355028053887817239);
    d = gsl_complex_sub(d,gsl_complex_mul_real(airy_series_y2_diff(in),0.258819403792806798));
    *status = 0;
    *out = d;
    return;
   }
  else if (mod<8.8)
   {
    if (arg<0.0) {
    conjugate = 1;
    in = gsl_complex_conjugate(in);
    arg = -arg;
    } else {
    conjugate = 0;
    }
    airy_lookup_ai(mod,arg,&a,&b,&z0);
    zd = gsl_complex_sub(in,z0);
    airy_series_taylor(&dp,&dq,zd,z0,1.0);
    dp = gsl_complex_mul(a,dp);
    dq = gsl_complex_mul(b,dq);
    d  = gsl_complex_add(dp,dq);
    *status = 0;
    if (conjugate==1) { *out = gsl_complex_conjugate(d); return; }
    else              { *out = d;                        return; }
   }
  else
   {
    if (log(mod)>440.0)
     {
      *status = 2; // calculation will overflow
      sprintf(errText,"Overflow error within Airy function");
      return;
     }
    if (arg<-2.094395102393195492 || arg>2.094395102393195492)
     {
      zah = gsl_complex_pow_real(gsl_complex_negative(in),1.5);
      max = 29;
      if (mod>10.0) max = 20;
      if (mod>12.0) max = 16;
      if (mod>16.0) max = 12;
      c  = GSL_COMPLEX_ONE;
      dp = GSL_COMPLEX_ZERO;
      dq = GSL_COMPLEX_ZERO;
      for (i=0;i<=max;i+=2)
       {
        dp = gsl_complex_add(dp,c);
        c  = gsl_complex_mul_real(c,((double) (6*i-1)*(6*i+7))/((double) 48*(i+1)));
        c  = gsl_complex_div(c,zah);
        dq = gsl_complex_add(dq,c);
        c  = gsl_complex_mul_real(c,((double) (6*i+5)*(6*i+13))/((double) -48*(i+2)));
        c  = gsl_complex_div(c,zah);
       }
      c = gsl_complex_add_real(gsl_complex_div_real(zah,1.5),0.785398163397448309);
      if (GSL_IMAG(c)>650.0)
       {
        *status = 2; // risk of overflow
        sprintf(errText,"Overflow error within Airy function");
        return;
       }
      dp = gsl_complex_mul(dp,gsl_complex_cos(c));
      dq = gsl_complex_mul(dq,gsl_complex_sin(c));
      d  = gsl_complex_add(dp,dq);
      d  = gsl_complex_mul(d,gsl_complex_pow_real(gsl_complex_negative(in),+0.25));
      d  = gsl_complex_div_real(d,-1.772453850905516027);
     }
    else
     {
      zah = gsl_complex_pow_real(in,1.5);
      max = 29;
      if (mod>10.0) max = 20;
      if (mod>12.0) max = 16;
      if (mod>16.0) max = 12;
      c = GSL_COMPLEX_ONE;
      d = GSL_COMPLEX_ZERO;
      for (i=0;i<=max;i++)
       {
        d = gsl_complex_add(d,c);
        c = gsl_complex_mul_real(c,((double) (6*i-1)*(6*i+7))/((double) -48*(i+1)));
        c = gsl_complex_div(c,zah);
       }
      d = gsl_complex_mul(d,gsl_complex_pow_real(in,0.25));
      d = gsl_complex_div_real(d,-3.544907701811032054);
      c = gsl_complex_div_real(zah,-1.5);
      if (GSL_REAL(c)>650.0)
       {
        *status = 2; // risk of overflow
        sprintf(errText,"Overflow error within Airy function");
        return;
       }
      d = gsl_complex_mul(d,gsl_complex_exp(c));
     }
    *status = 0;
    *out = d;
    return;
   }
 }

void airy_bi_diff(gsl_complex in, gsl_complex *out, int *status, char *errText)
 {
  int conjugate,i,max;
  double mod,arg;
  gsl_complex a,b,c,d,dp,dq,zah,zd,z0;
  mod = gsl_complex_abs(in);
  arg = gsl_complex_arg(in);
  if (mod<3.2)
   {
    d = gsl_complex_mul_real(airy_series_y1_diff(in), 0.614926627446000735);
    d = gsl_complex_add(d,gsl_complex_mul_real(airy_series_y2_diff(in),0.448288357353826357));
    *status = 0;
    *out = d;
    return;
   }
  else if (mod<8.8)
   {
    if (arg<0.0)
     {
      conjugate = 1;
      in = gsl_complex_conjugate(in);
      arg = -arg;
     }
    else
     {
      conjugate = 0;
     }
    airy_lookup_bi(mod,arg,&a,&b,&z0);
    zd = gsl_complex_sub(in,z0);
    airy_series_taylor(&dp,&dq,zd,z0,1.0);
    dp = gsl_complex_mul(a,dp);
    dq = gsl_complex_mul(b,dq);
    d  = gsl_complex_add(dp,dq);
    *status = 0;
    if (conjugate==1) { *out = gsl_complex_conjugate(d); return; }
    else              { *out = d;                        return; }
   }
  else
   {
    if (log(mod)>440.0)
     {
      *status = 2; // calculation will overflow
      sprintf(errText,"Overflow error within Airy function");
      return;
     }
    if (arg<-1.63 || arg>1.63)
     {
      zah = gsl_complex_pow_real(gsl_complex_negative(in),1.5);
      max = 29;
      if (mod>10.0) max = 20;
      if (mod>12.0) max = 16;
      if (mod>16.0) max = 12;
      c  = GSL_COMPLEX_ONE;
      dp = GSL_COMPLEX_ZERO;
      dq = GSL_COMPLEX_ZERO;
      for (i=0;i<=max;i+=2)
       {
        dp = gsl_complex_add(dp,c);
        c  = gsl_complex_mul_real(c,((double) (6*i-1)*(6*i+7))/((double) 48*(i+1)));
        c  = gsl_complex_div(c,zah);
        dq = gsl_complex_add(dq,c);
        c  = gsl_complex_mul_real(c,((double) (6*i+5)*(6*i+13))/((double) -48*(i+2)));
        c  = gsl_complex_div(c,zah);
       }
      c = gsl_complex_add_real(gsl_complex_div_real(zah,1.5),0.785398163397448309);
      if (GSL_IMAG(c)>650.0)
       {
        *status = 2; // risk of overflow
        sprintf(errText,"Overflow error within Airy function");
        return;
       }
      dp = gsl_complex_mul(dp,gsl_complex_sin(c));
      dq = gsl_complex_mul(dq,gsl_complex_cos(c));
      d  = gsl_complex_sub(dq,dp);
      d  = gsl_complex_mul(d,gsl_complex_pow_real(gsl_complex_negative(in),+0.25));
      d  = gsl_complex_div_real(d,-1.772453850905516027);
     }
    else if (arg>-0.51 && arg<0.51)
     {
      zah = gsl_complex_pow_real(in,1.5);
      max = 29;
      if (mod>10.0) max = 20;
      if (mod>12.0) max = 16;
      if (mod>16.0) max = 12;
      c = GSL_COMPLEX_ONE;
      d = GSL_COMPLEX_ZERO;
      for (i=0;i<=max;i++)
       {
        d = gsl_complex_add(d,c);
        c = gsl_complex_mul_real(c,((double) (6*i-1)*(6*i+7))/((double) 48*(i+1)));
        c = gsl_complex_div(c,zah);
       }
      d = gsl_complex_mul(d,gsl_complex_pow_real(in,0.25));
      d = gsl_complex_div_real(d,1.772453850905516027);
      c = gsl_complex_div_real(zah,1.5);
      if (GSL_REAL(c)>650.0)
       {
        *status = 2; // risk of overflow
        sprintf(errText,"Overflow error within Airy function");
        return;
       }
      d = gsl_complex_mul(d,gsl_complex_exp(c));
     }
    else
     {
      c = gsl_complex_polar(1,2.094395102393195492);
      d = gsl_complex_conjugate(c);
      airy_ai_diff(gsl_complex_mul(in,c),&a,status,errText);
      if (*status==2) return;
      airy_ai_diff(gsl_complex_mul(in,d),&b,status,errText);
      if (*status==2) return;
      a = gsl_complex_mul(a,d);
      b = gsl_complex_mul(b,c);
      d = gsl_complex_sub(a,b);
      d = gsl_complex_mul_imag(d,-1.0);
     }
    *status = 0;
    *out = d;
    return;
   }
 }

