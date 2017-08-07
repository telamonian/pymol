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

# pmgui_tk 
# TKinter based gui for PyMol
# NOTE: must have treads support compiled into python to use this module
#
# **This is the only module which should be/need be imported by 
# **PyMol Programs

from .PMGApp import *
import sys, os, threading
import traceback

def run(pymol_instance,poll=0,skin=None,root=None):
    try:
        if not hasattr(sys,"argv"):
            sys.argv=["pymol"]
        PMGApp(pymol_instance,skin,root).run(poll)
    except:
        traceback.print_exc()

def __init__(pymol_instance,poll=0,skin=None,root=None,usethread=True):
    # if not bool(root), set root to None
    if not root: root = None

    if usethread:
        t = threading.Thread(target=run,args=(pymol_instance,poll,skin,root))
        t.setDaemon(1)
        t.start()
    else:
        run(pymol_instance=pymol_instance, poll=poll, skin=skin, root=root)
