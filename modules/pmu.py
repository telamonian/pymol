#A* -------------------------------------------------------------------
#B* This file contains source code for the PyMOL computer program
#C* copyright 1998-2000 by Warren Lyford Delano of DeLano Scientific. 
#D* -------------------------------------------------------------------
#E* It is unlawful to modify or remove this copyright notice.
#F* -------------------------------------------------------------------
#G* Please see the accompanying LICENSE file for further information. 
#H* -------------------------------------------------------------------
#I* Additional authors of this source file include:
#-* 
#-* 
#-*
#Z* -------------------------------------------------------------------

import pm
import math
import string
import glob

from pmp import *

def cbag(s):
   pm.color("magenta","("+s+")")
   pm.color("oxygen","(elem O and "+s+")")
   pm.color("nitrogen","(elem N and "+s+")")
   pm.color("sulfer","(elem S and "+s+")")
   pm.color("hydrogen","(elem H and "+s+")")
   pm.color("carbon","(elem C and "+s+")")

def cbac(s):
   pm.color("magenta","("+s+")")
   pm.color("oxygen","(elem O and "+s+")")
   pm.color("nitrogen","(elem N and "+s+")")
   pm.color("sulfer","(elem S and "+s+")")
   pm.color("hydrogen","(elem H and "+s+")")
   pm.color("cyan","(elem C and "+s+")")

def cbay(s):
   pm.color("magenta","("+s+")")
   pm.color("oxygen","(elem O and "+s+")")
   pm.color("nitrogen","(elem N and "+s+")")
   pm.color("sulfer","(elem S and "+s+")")
   pm.color("hydrogen","(elem H and "+s+")")
   pm.color("yellow","(elem C and "+s+")")

def cbas(s):
   pm.color("magenta","("+s+")")
   pm.color("oxygen","(elem O and "+s+")")
   pm.color("nitrogen","(elem N and "+s+")")
   pm.color("sulfer","(elem S and "+s+")")
   pm.color("hydrogen","(elem H and "+s+")")
   pm.color("salmon","(elem C and "+s+")")

def cbap(s):
   pm.color("magenta","("+s+")")
   pm.color("oxygen","(elem O and "+s+")")
   pm.color("nitrogen","(elem N and "+s+")")
   pm.color("sulfer","(elem S and "+s+")")
   pm.color("hydrogen","(elem H and "+s+")")
   pm.color("purple","(elem C and "+s+")")

def cbaw(s):
   pm.color("magenta","("+s+")")
   pm.color("oxygen","(elem O and "+s+")")
   pm.color("nitrogen","(elem N and "+s+")")
   pm.color("sulfer","(elem S and "+s+")")
   pm.color("hydrogen","(elem H and "+s+")")
   pm.color("hydrogen","(elem C and "+s+")")


def mrock(fir,las,dsp,pha,loop):
	global pmp_nest,pmp_cmd,pmp_cont,pymol
	n = las - fir
	ang = pha * math.pi
	if loop:
		step = 2*math.pi/(n+1)
		last = -(math.sin(ang+step*n)*dsp);
	else:
		last=0
		step = 2*math.pi/n	
	a = 0
	while a<=n:
		deg = (math.sin(ang)*dsp)
		cmd = "mdo %d:turn y,%8.3f;turn y,%8.3f" % (fir+a,last,deg)
		last = -deg;
		print cmd
		pmp_nest=pmp_nest+1
		pmp_cmd[pmp_nest] = cmd;
		pmp_cont[pmp_nest] = '';
		exec(pymol,globals(),globals())
		pmp_nest=pmp_nest-1
		ang = ang + step
		a = a + 1

def mroll(fir,las,loop):
	global pmp_nest,pmp_cmd,pmp_cont,pymol
	n = las - fir
	if loop:
		step = 2*math.pi/(n+1)
	else:
		step = 2*math.pi/n	
	a = 0
	deg = (180*step/math.pi)
	while a<=n:
		cmd = "mdo %d:turn y,%8.3f" % (fir+a,deg)
		print cmd
		pmp_nest=pmp_nest+1
		pmp_cmd[pmp_nest] = cmd;
		pmp_cont[pmp_nest] = '';
		exec(pymol,globals(),globals())
		pmp_nest=pmp_nest-1
		a = a + 1

def hbond(a,b,cutoff=3.3):
   st = "(%s and (%s around %4.2f) and elem N,O),(%s and (%s around %4.2f) and elem N,O),%4.2f" % (a,b,cutoff,b,a,cutoff,cutoff)
   pm.dist("hbond",st)
        
def mload(*args):
   nam = "mov"
   if len(args)>1:
      nam = args[1]
   for a in glob.glob(args[0]):
      pm.load(a,nam)
   
