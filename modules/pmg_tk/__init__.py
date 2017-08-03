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
import time

import Tkinter

# def testwindow():
#     w1=Tkinter.Tk()
#     w1.title("Finestra 1")
#     # Width, height in pixels
#     f1=Tkinter.Frame(w1, height=50, width=50)
#     f1.pack()
#     w1.mainloop()

class MyTkApp(threading.Thread):
    def __init__(self, root=None):
        self.root = root
        threading.Thread.__init__(self)

    def run(self):
        if self.root is None: self.root = Tkinter.Tk(mtDebug=9)

        keep_alive = 1
        while keep_alive:
            self.root.update()
            time.sleep(0.05)

def run(pymol_instance,root,poll=0,skin=None):
    try:
        if not hasattr(sys,"argv"):
            sys.argv=["pymol"]

        # import ipdb; ipdb.set_trace()
        # testwindow()
        # # root = Tkinter.Tk(); root.mainloop()
        # import ipdb; ipdb.set_trace()
        # pass
        PMGApp(pymol_instance,root,skin).run(poll)
    except:
        traceback.print_exc()
        
def __init__(pymol_instance,root,poll=0,skin=None):
    # import ipdb; ipdb.set_trace()

    # root = Tkinter.Tk(mtDebug=9)

    # run(root, pymol_instance=pymol_instance, poll=poll, skin=skin)

    # t0 = MyTkApp()
    # t0.start()
    # root = t0.root
    # print root

    t1 = threading.Thread(target=run,args=(pymol_instance,root,poll,skin))
    t1.setDaemon(1)
    t1.start()
    #
    # # time.sleep(5)
    # root.mainloop()



