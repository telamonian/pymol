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


import math
import selector

import cmd
from cmd import _cmd,lock,unlock,Shortcut,QuietException
from chempy import cpv

def sculpt_clear(object):
   '''
   undocumented
'''
   r = 0
   try:
      lock()
      r = _cmd.sculpt_clear(str(object))
   finally:
      unlock()
   return r
   
def sculpt_imprint(object,state=0):
   '''
undocumented.
'''
   r = 0
   try:
      lock()
      r = _cmd.sculpt_imprint(str(object),int(state)-1)
   finally:
      unlock()
   return r

def sculpt_iterate(object,state=0,cycles=10):
   '''
undocumented.
'''
   r = 0
   try:
      lock()
      r = _cmd.sculpt_iterate(str(object),int(state)-1,int(cycles))
   finally:
      unlock()
   return r

def set_geometry(selection,geometry,valence):
   '''
DESCRIPTION
  
   "set_geometry" changes PyMOL's assumptions about the proper valence
   and geometry of the picked atom.
      
USAGE
 
   set_geometry geometry, valence
 
PYMOL API
  
   cmd.set_geometry(int geometry,int valence )

NOTES

   Immature functionality. See code for details.

SEE ALSO

   remove, attach, fuse, bond, unbond
'''
   r = 0
   # preprocess selection
   selection = selector.process(selection)
   try:
      lock()
      r = _cmd.set_geometry(str(selection),int(geometry),int(valence))
   finally:
      unlock()
   return r

def undo():
   '''
DESCRIPTION

   "undo" restores the previous conformation of the object currently
   being edited.
   
USAGE
 
   undo

SEE ALSO

   redo, push_undo
'''
   r = None
   try:
      lock()
      r = _cmd.undo(-1)
   finally:
      unlock()
   return r

def push_undo(selection,state=0):
   '''
DESCRIPTION

   "push_undo" stores the currently conformations of objects in the
   selection onto their individual kill rings.
   
USAGE
 
   push_undo (all)

SEE ALSO

   undo, redo
'''
   # preprocess selections
   selection = selector.process(selection)
   #
   r = None
   try:
      lock()
      r = _cmd.push_undo("("+str(selection)+")",int(state)-1)
   finally:
      unlock()
   return r

def redo():
   '''
DESCRIPTION

   "redo" reapplies the conformational change of the object currently
   being edited.
   
USAGE
 
   redo

SEE ALSO

   undo, push_undo
'''
   try:
      lock()
      _cmd.undo(1)
   finally:
      unlock()


def bond(atom1="(lb)",atom2="(rb)",order=1,edit=1):
   '''
DESCRIPTION
 
   "bond" creates a new bond between two selections, each of
   which should contain one atom.
 
USAGE

   bond [atom1,atom2 [,order]]
   
PYMOL API

   cmd.bond(string atom1, string atom2)
   
NOTES

   The atoms must both be within the same object.
   
   The default behavior is to create a bond between the (lb) and (rb)
   selections.

SEE ALSO

   unbond, fuse, attach, replace, remove_picked
'''
   # preprocess selections
   atom1 = selector.process(atom1)
   atom2 = selector.process(atom2)
   try:
      lock()
      r = _cmd.bond(atom1,atom2,int(order),1)
      if r and edit:
         _cmd.edit(str(atom1),str(atom2),'','',0)
   finally:
      unlock()
   return r

def invert(selection1="(lb)",selection2="(rb)"):
   '''
DESCRIPTION

   "invert" inverts the stereo-chemistry of the atom currently picked
   for editing (pk1).  Two additional atom selections must be provided
   in order to indicate which atoms remain stationary during the
   inversion process.

USAGE

   invert (selection1),(selection2)

PYMOL API

   cmd.api( string selection1="(lb)", string selection2="(lb)" )
   
NOTE

   The invert function is usually bound to CTRL-E in editing mode.

   The default selections are (lb) and (rb), meaning that you can pick
   the atom to invert with CTRL-middle click and then pick the
   stationary atoms with CTRL-SHIFT/left-click and CTRL-SHIFT/right-
   click, then hit CTRL-E to invert the atom.

'''
   # preprocess selections
   selection1 = selector.process(selection1)
   selection2 = selector.process(selection2)
   #
   try:
      lock()
      r = _cmd.invert(str(selection1),str(selection2),0)
   finally:
      unlock()
   return r

def unbond(atom1="(lb)",atom2="(rb)"):
   '''
DESCRIPTION
 
   "unbond" removes all bonds between two selections.

USAGE

   unbond atom1,atom2
   
PYMOL API

   cmd.unbond(selection atom1="(lb)",selection atom2="(rb)")
   
SEE ALSO

   bond, fuse, remove_picked, attach, detach, replace
 
'''
   # preprocess selections
   atom1 = selector.process(atom1)
   atom2 = selector.process(atom2)   
   try:
      lock()
      r = _cmd.bond(str(atom1),str(atom2),0,0)
   finally:
      unlock()
   return r

def remove(selection):
   '''
DESCRIPTION
  
   "remove" eleminates a selection of atoms from models.
      
USAGE
 
   remove (selection)
 
PYMOL API
  
   cmd.remove( string selection )
    
EXAMPLES
 
   remove ( resi 124 )

SEE ALSO

   delete
'''
   # preprocess selection
   selection = selector.process(selection)
   #   
   r = 1
   try:
      lock()   
      r = _cmd.remove("("+selection+")")
   finally:
      unlock()
   return r


def remove_picked(hydrogens=1):
   '''
DESCRIPTION
  
   "remove_picked" removes the atom or bond currently
   picked for editing. 
      
USAGE
 
   remove_picked [hydrogens]
 
PYMOL API
  
   cmd.remove_picked(integer hydrogens=1)

NOTES

   This function is usually connected to the
   DELETE key and "CTRL-D".
   
   By default, attached hydrogens will also be deleted unless
   hydrogen-flag is zero.

SEE ALSO

   attach, replace
'''
   r = 1
   try:
      lock()   
      r = _cmd.remove_picked(int(hydrogens))
   finally:
      unlock()
   return r


def cycle_valence(h_fill=1):
   '''
DESCRIPTION
  
   "cycle_valence" cycles the valence on the currently selected bond.
      
USAGE
 
   cycle_valence [ h_fill ]
 
PYMOL API
  
   cmd.cycle_valence(int h_fill)

EXAMPLES

   cycle_valence
   cycle_valence 0
   
NOTES

   If the h_fill flag is true, hydrogens will be added or removed to
   satisfy valence requirements.
   
   This function is usually connected to the DELETE key and "CTRL-W".

SEE ALSO

   remove_picked, attach, replace, fuse, h_fill
'''
   r = 1
   try:
      lock()   
      r = _cmd.cycle_valence()
   finally:
      unlock()
   if h_fill:
      globals()['h_fill']()
   return r


def attach(element,geometry,valence):
   '''
DESCRIPTION
  
   "attach" adds a single atom onto the picked atom.
      
USAGE
 
   attach element, geometry, valence
 
PYMOL API
  
   cmd.attach( element, geometry, valence )

NOTES

   Immature functionality.  See code for details.

'''
   r = 1
   try:
      lock()   
      r = _cmd.attach(str(element),int(geometry),int(valence))
   finally:
      unlock()
   return r


def fuse(selection1="(lb)",selection2="(rb)",mode=0):
   '''
DESCRIPTION
  
   "fuse" joins two objects into one by forming a bond.  A copy of
   the object containing the first atom is moved so as to form an
   approximately resonable bond with the second, and is then merged
   with the first object.
      
USAGE
 
   fuse (selection1), (selection2)
 
PYMOL API
  
   cmd.fuse( string selection1="(lb)", string selection2="(lb)" )

NOTES

   Each selection must include a single atom in each object.
   The atoms can both be hydrogens, in which case they are
   eliminated, or they can both be non-hydrogens, in which
   case a bond is formed between the two atoms.

SEE ALSO

   bond, unbond, attach, replace, fuse, remove_picked
'''
   # preprocess selections
   selection1 = selector.process(selection1)
   selection2 = selector.process(selection2)
   #   
   try:
      lock()
      r = _cmd.fuse(str(selection1),str(selection2),int(mode))
   finally:
      unlock()
   return r

def unpick(*arg):
   '''
DESCRIPTION

   "unpick" deletes the special "pk" atom selections (pk1, pk2, etc.)
   used in atom picking and molecular editing.

USAGE

   unpick

PYMOL API

   cmd.unpick()

SEE ALSO

   edit
   '''
   try:
      lock()   
      r = _cmd.unpick()
   finally:
      unlock()
   return r

      

def edit(selection1='',selection2='',selection3='',selection4='',pkresi=0):
   '''
DESCRIPTION
  
   "edit" picks an atom or bond for editing.
      
USAGE
 
   edit (selection) [ ,(selection) ]
 
PYMOL API
  
   cmd.edit( string selection  [ ,string selection ] )

NOTES

   If only one selection is provided, an atom is picked.
   If two selections are provided, the bond between them
   is picked (if one exists).

SEE ALSO

   unpick, remove_picked, cycle_valence, torsion
'''
   # preprocess selections
   selection1 = selector.process(selection1)
   selection2 = selector.process(selection2)
   selection3 = selector.process(selection3)
   selection4 = selector.process(selection4)
   #
   r = 1
   try:
      lock()   
      r = _cmd.edit(str(selection1),str(selection2),
                    str(selection3),str(selection4),int(pkresi))
   finally:
      unlock()
   return r


def torsion(angle):
   '''
DESCRIPTION
  
   "torsion" rotates the torsion on the bond currently
   picked for editing.  The rotated fragment will correspond
   to the first atom specified when picking the bond (or the
   nearest atom, if picked using the mouse).
      
USAGE
 
   torsion angle
 
PYMOL API
  
   cmd.torsion( float angle )

SEE ALSO

   edit, unpick, remove_picked, cycle_valence
'''
   try:
      lock()   
      r = _cmd.torsion(float(angle))
   finally:
      unlock()
   return r

def h_fill():
   '''
DESCRIPTION
  
   "h_fill" removes and replaces hydrogens on the atom
   or bond picked for editing.  
      
USAGE
 
   h_fill
 
PYMOL API
  
   cmd.h_fill()

NOTES
   
   This is useful for fixing hydrogens after changing
   bond valences.

SEE ALSO

   edit, cycle_valences, h_add
'''
   r = 1
   try:
      lock()   
      r = _cmd.h_fill()
   finally:
      unlock()
   return r

def h_add(selection="(all)"):
   '''
DESCRIPTION
  
   "h_add" uses a primitive algorithm to add hydrogens
   onto a molecule.
      
USAGE
 
   h_add (selection)
 
PYMOL API
  
   cmd.h_add( string selection="(all)" )

SEE ALSO

   h_fill
'''
   # preprocess selection
   selection = selector.process(selection)
   #   
   r = 1
   try:
      lock()   
      r = _cmd.h_add(selection)
   finally:
      unlock()
   return r
   


def sort(object=""):
   '''
DESCRIPTION
 
   "sort" reorders atoms in the structure.  It usually only necessary
   to run this routine after an "alter" command which has modified the
   names of atom properties.  Without an argument, sort will resort
   all atoms in all objects.

USAGE
 
   sort [object]

PYMOL API

   cmd.sort(string object)

SEE ALSO

   alter
'''
   try:
      lock()
      r = _cmd.sort(str(object))
   finally:
      unlock()
   return r


def replace(element,geometry,valence,h_fill=1):
   '''
DESCRIPTION
  
   "replace" replaces the picked atom with a new atom.
      
USAGE
 
   replace element, geometry, valence
 
PYMOL API
  
   cmd.replace(string element, int geometry,int valence )

NOTES

   Immature functionality. See code for details.

SEE ALSO

   remove, attach, fuse, bond, unbond
'''
   r = 1
   if not "pk1" in cmd.get_names("selections"):
      print " Error: you must first pick an atom to replace."
      raise QuietException
   try:
      if h_fill: # strip off existing hydrogens
         remove("((neighbor pk1) and elem h)")
      lock()
      r = _cmd.replace(str(element),int(geometry),int(valence))
   finally:
      unlock()
   return r

def rename(object,force=0):
   '''
DESCRIPTION
  
   "rename" creates new atom names which are unique within residues.
      
USAGE

   CURRENT
      rename object-name [ ,force ]
      
      force = 0 or 1 (default: 0)
      
   PROPOSED
      rename object-or-selection,force   

PYMOL API

   CURRENT
      cmd.rename( string object-name, int force )

   PROPOSED
      cmd.rename( string object-or-selection, int force )

NOTES

   To regerate only some atom names in a molecule, first clear them
   with an "alter (sele),name=''" commmand, then use "rename"

SEE ALSO

   alter
'''   
   try:
      lock()   
      r = _cmd.rename(str(object),int(force))
   finally:
      unlock()
   return r

def alter(selection,expression):
   '''
DESCRIPTION
 
   "alter" changes one or more atomic properties over a selection
   using the python evaluator with a separate name space for each
   atom.  The symbols defined in the name space are:
 
      name, resn, resi, chain, alt, elem, q, b, segi,
      type (ATOM,HETATM), partial_charge, formal_charge,
      text_type, numeric_type, ID
   
   All strings must be explicitly quoted.  This operation typically
   takes several seconds per thousand atoms altered.

   WARNING: You should always issue a "sort" command on an object
   after modifying any property which might affect canonical atom
   ordering (names, chains, etc.).  Failure to do so will confound
   subsequent "create" and "byres" operations.
   
USAGE
 
   alter (selection),expression
 
EXAMPLES
 
   alter (chain A),chain='B'
   alter (all),resi=str(int(resi)+100)
   sort
   
SEE ALSO

   alter_state, iterate, iterate_state, sort
   '''
   # preprocess selections
   selection = selector.process(selection)
   #
   try:
      lock()
      r = _cmd.alter("("+str(selection)+")",str(expression),0)
   finally:
      unlock()   
   return r


def iterate(selection,expression):
   '''
DESCRIPTION
 
   "iterate" iterates over an expression with a separate name space
   for each atom.  However, unlike the "alter" command, atomic
   properties can not be altered.  Thus, "iterate" is more efficient
   than "alter".

   It can be used to perform operations and aggregations using atomic
   selections, and store the results in any global object, such as the
   predefined "stored" object.

   The local namespace for "iterate" contains the following names

      name, resn, resi, chain, alt, elem,
      q, b, segi, and type (ATOM,HETATM),
      partial_charge, formal_charge,
      text_type, numeric_type, ID
 
   All strings in the expression must be explicitly quoted.  This
   operation typically takes a second per thousand atoms.
 
USAGE
 
   iterate (selection),expression
 
EXAMPLES

   stored.net_charge = 0
   iterate (all),stored.net_charge = stored.net_charge + partial_charge

   stored.names = []
   iterate (all),stored.names.append(name)

SEE ALSO

   iterate_state, atler, alter_state
   '''
   # preprocess selection
   selection = selector.process(selection)
   #
   try:
      lock()
      r = _cmd.alter("("+str(selection)+")",str(expression),1)
   finally:
      unlock()   
   return r

def alter_state(state,selection,expression):
   '''
DESCRIPTION
 
   "alter_state" changes the atomic coordinates of a particular state
   using the python evaluator with a separate name space for each
   atom.  The symbols defined in the name space are:
 
      x,y,z
 
USAGE
 
   alter_state state,(selection),expression
 
EXAMPLES
 
   alter 1,(all),x=x+5

SEE ALSO

   iterate_state, alter, iterate
   '''
   # preprocess selection
   selection = selector.process(selection)
   #
   state = int(state)
   if state<0: # hack -- need to replace with "alter_state_atom" command
      state = -state
      atomic_props = 1
   else:
      atomic_props = 0
   try:
      lock()
      r = _cmd.alter_state(int(state)-1,"("+str(selection)+")",str(expression),
                           0,int(atomic_props))
   finally:
      unlock()   
   return r

def iterate_state(state,selection,expression):
   '''
DESCRIPTION
 
   "iterate_state" is to "alter_state" as "iterate" is to "alter"
 
USAGE
 
   iterate_state state,(selection),expression
 
EXAMPLES

   stored.sum_x = 0.0
   iterate 1,(all),stored.sum_x = stored.sum_x + x

SEE ALSO

   iterate, alter, alter_state
   '''
   # preprocess selection
   selection = selector.process(selection)
   state = int(state)
   #
   if state<0: # hack -- need to replace with "alter_state_atom" command
      state = -state
      atomic_props = 1
   else:
      atomic_props = 0
   try:
      lock()
      r = _cmd.alter_state(int(state)-1,"("+str(selection)+")",
                           str(expression),1,int(atomic_props))
   finally:
      unlock()   
   return r

def translate(vector=[0.0,0.0,0.0],selection="all",state=0,camera=1,object=None):
   # INCOMPLETE:
   # needs to be modified so that selection can be something other
   # than an object name -- some work needs to be done on the
   # underlying C code
   r = 1
   if cmd.is_string(vector):
      vector = eval(vector)
   if not cmd.is_list(vector):
      print "Error: bad vector."
      raise QuietException
   else:
      vector = [float(vector[0]),float(vector[1]),float(vector[2])]
      selection = selector.process(selection)
      view = cmd.get_view(0)
      if camera:
         mat = [ view[0:3],view[3:6],view[6:9] ]
         shift = cpv.transform(mat,vector)
      else:
         shift = vector
      if object==None:
         ttt = [1.0,0.0,0.0,0.0,
                0.0,1.0,0.0,0.0,
                0.0,0.0,1.0,0.0,
                shift[0],shift[1],shift[2],1.0]
         r=cmd.transform_object(selection,ttt)
      else:
         lock()
         ttt = [1.0, 0.0, 0.0, shift[0],
                0.0, 1.0, 0.0, shift[1],
                0.0, 0.0, 1.0, shift[2],
                0.0, 0.0, 0.0, 1.0]
         r=_cmd.combine_object_ttt(str(object),ttt)
         unlock()
         
   return r

def rotate(axis='x',angle=0.0,selection="all",state=0,camera=1,object=None):
   # INCOMPLETE:
   # needs to be modified so that selection can be something other
   # than an object name -- some work needs to be done on the
   # underlying C code
   r = 1
   if axis in ['x','X']:
      axis = [1.0,0.0,0.0]
   elif axis in ['y','Y']:
      axis = [0.0,1.0,0.0]
   elif axis in ['z','Z']:
      axis = [0.0,0.0,1.0]
   else:
      axis = eval(axis)
   if not cmd.is_list(axis):
      print "Error: bad axis."
      raise QuietException
   else:
      axis = [float(axis[0]),float(axis[1]),float(axis[2])]
      angle = math.pi*float(angle)/180.0
      view = cmd.get_view(0)
      if camera:
         vmat = [ view[0:3],view[3:6],view[6:9] ]
         axis = cpv.transform(vmat,axis)
      mat = cpv.rotation_matrix(angle,axis)
      if object==None:
         ttt = [mat[0][0],mat[1][0],mat[2][0],-view[12],
                mat[0][1],mat[1][1],mat[2][1],-view[13],
                mat[0][2],mat[1][2],mat[2][2],-view[14],
                view[12],view[13],view[14],view[15]]
         r=cmd.transform_object(selection,ttt)
      else:
         lock()
         ttt = [mat[0][0],mat[1][0],mat[2][0], 0.0,
                mat[0][1],mat[1][1],mat[2][1], 0.0,
                mat[0][2],mat[1][2],mat[2][2], 0.0,
                0.0      ,0.0      ,0.0      , 1.0]
         r=_cmd.combine_object_ttt(str(object),ttt)
         unlock()
   return r

def set_title(object,state,text):
   '''
DESCRIPTION

   "set_title" attaches a text string to the state of a particular
   object which can be displayed when the state is active.  This is
   useful for display the energies of a set of conformers.

USAGE
   
   set_title object,state,text

PYMOL API

   cmd.set_title(string object,int state,string text)

'''
   r = 1
   try:
      lock()
      r = _cmd.set_title(str(object),int(state)-1,str(text))
   finally:
      unlock()


def transform_object(name,matrix,state=0,log=0,sele=''):
   r = None
   try:
      lock()
      r = _cmd.transform_object(str(name),int(state)-1,list(matrix),int(log),str(sele))
   finally:
      unlock()
   return r

def translate_atom(sele1,v0,v1,v2,state=0,mode=0,log=0):
   r = None
   sele1 = selector.process(sele1)
   try:
      lock()
      r = _cmd.translate_atom(str(sele1),float(v0),float(v1),float(v2),int(state)-1,int(mode),int(log))
   finally:
      unlock()
   return r

def update(target,source):
   '''
DESCRIPTION
  
   "update" transfers coordinates from one selection to another.
USAGE
 
   update (target-selection),(source-selection)
 
EXAMPLES
 
   update target,(variant)

NOTES

   Currently, this applies across all pairs of states.  Fine
   control will be added later.

SEE ALSO

   load
'''
   
   a=target
   b=source
   # preprocess selections
   a = selector.process(a)
   b = selector.process(b)
   #
   if a[0]!='(': a="("+str(a)+")"
   if b[0]!='(': b="("+str(b)+")"   
   try:
      lock()   
      r = _cmd.update(str(a),str(b),-1,-1)
   finally:
      unlock()
   return r


def set_dihedral(atom1,atom2,atom3,atom4,angle,state=1):
   # preprocess selections
   atom1 = selector.process(atom1)
   atom2 = selector.process(atom2)
   atom3 = selector.process(atom3)
   atom4 = selector.process(atom4)
   #   
   try:
      lock()
      r = _cmd.set_dihe(str(atom1),str(atom2),str(atom3),str(atom4),float(angle),int(state)-1)
   finally:
      unlock()
   return r

def map_set_border(name,level=0.0):
   '''
DESCRIPTION
 
   "map_set_border" is a function (reqd by PDA) which allows you to set the
   level on the edge points of a map

USAGE

   map_set_border <name>,<level>

NOTES

   unsupported.
SEE ALSO

   load
   '''
   try:
      lock()
      r = _cmd.map_set_border(str(name),float(level))
   finally:
      unlock()
   return r


def protect(selection="(all)"):
   '''
DESCRIPTION

   "protect" protects a set of atoms from tranformations performed
   using the editing features.  This is most useful when you are
   modifying an internal portion of a chain or cycle and do not wish
   to affect the rest of the molecule.
   
USAGE

   protect (selection)

PYMOL API

   cmd.protect(string selection)
   
SEE ALSO

   deprotect, mask, unmask, mouse, editing
'''
   # preprocess selection
   selection = selector.process(selection)
   #   
   try:
      lock()   
      r = _cmd.protect("("+str(selection)+")",1)
   finally:
      unlock()
   return r

def deprotect(selection="(all)"):
   '''
DESCRIPTION

   "deprotect" reveres the effect of the "protect" command.

USAGE

   deprotect (selection)
   
PYMOL API

   cmd.deprotect(string selection="(all)")

SEE ALSO

   protect, mask, unmask, mouse, editing
'''
   # preprocess selection
   selection = selector.process(selection)
   #   
   try:
      lock()   
      r = _cmd.protect("("+str(selection)+")",0)
   finally:
      unlock()
   return r

flag_dict = {
# simulation 
   'focus'         : 0,
   'free'          : 1,
   'restrain'      : 2,
   'fix'           : 3,
   'exclude'       : 4,
# rendering
   'exfoliate'     : 24,
   'ignore'        : 25,
}

flag_sc = Shortcut(flag_dict.keys())

flag_action_dict = {
   'reset' : 0,
   'set'   : 1,
   'clear' : 2,
   }

flag_action_sc = Shortcut(flag_action_dict.keys())

   
def flag(flag,selection,action="reset"):
   '''
DESCRIPTION
  
   "flag" sets the indicated flag for atoms in the selection and
    clears the indicated flag for atoms not in the selection.  This
    is primarily useful for passing selection information into
    Chempy models, which have a 32 bit attribute "flag" which holds
    this state information.
   
USAGE

   flag flag, selection [ ,action ]
    
   flag flag = selection [ ,action ]      # (DEPRECATED)

   action can be:
     "reset" (default) sets flag on selection, clear it on other atoms 
     "set" sets the flag for selected atoms, leaves other atoms unchanged
     "clear" clear the flag for selected atoms, leaves other atoms unchanged
   
PYMOL API
  
   cmd.flag( int number, string selection, string action="reset" )
   cmd.flag( string flag_name, string selection, string action="reset" )
 
EXAMPLES  
 
   flag free, (resi 45 x; 6)

RESERVATIONS

   Flags 0-7 are reserved for molecular modeling */
      focus      0 = Atoms of Interest (i.e. a ligand in an active site)
      free       1 = Free Atoms (free to move subject to a force-field)
      restrain   2 = Restrained Atoms (typically harmonically contrained)
      fix        3 = Fixed Atoms (no movement allowed)
      ignore     4 = Atoms which should not be part of any simulation
   Flags 8-15 are free for end users to manipulate
   Flags 16-23 are reserved for external GUIs and linked applications
   Flags 24-31 are reserved for PyMOL internal usage
      exfoliate 24 = Remove surface from atoms when surfacing
      ignore    25 = Ignore atoms altogether when surfacing
   '''
   # preprocess selection
   new_flag = flag_sc.interpret(str(flag))
   if new_flag:
      if cmd.is_string(new_flag):
         flag = flag_dict[new_flag]
      else:
         flag_sc.auto_err(flag,'flag')
   # preprocess selection
   selection = selector.process(selection)
   action = flag_action_dict[flag_action_sc.auto_err(action,'action')]
   #      
   try:
      lock()
      r = _cmd.flag(int(flag),"("+str(selection)+")",int(action))
   finally:
      unlock()
   return r

