/* 
A* -------------------------------------------------------------------
B* This file contains source code for the PyMOL computer program
C* copyright 1998-2000 by Warren Lyford Delano of DeLano Scientific. 
D* -------------------------------------------------------------------
E* It is unlawful to modify or remove this copyright notice.
F* -------------------------------------------------------------------
G* Please see the accompanying LICENSE file for further information. 
H* -------------------------------------------------------------------
I* Additional authors of this source file include:
-* 
-* 
-*
Z* -------------------------------------------------------------------
*/
#ifndef _H_AtomInfo
#define _H_AtomInfo

#include"Rep.h"
#include"Setting.h"

/* FLAGS 0-3 have the following conventional usage for molecular modeling */

/* FLAG 0 - Atoms of interest - i.e. a ligand in an active site */
#define cAtomFlag_focus         0x00000001
/* FLAG 1 - Free atoms - those which can move subject to a force-field */
#define cAtomFlag_free          0x00000002
/* FLAG 2 - Restrained atoms - atoms subject to a harmonic restraint */
#define cAtomFlag_restrain      0x00000004
/* FLAG 3 - Fixed atoms - no movement allowed */
#define cAtomFlag_fix           0x00000008
/* FLAG 4 - Exclude these atoms when performing simulation, minimization */
#define cAtomFlag_exclude       0x00000010

/* FLAGS 4-7 are reserved for additional molecular modeling tasks */

/* FLAGS 8-15 are free for end users to manipulate */
 
/* FLAGS 16-21 are reserved for external GUIs and linked applications */

/* FLAGS 22-23 are for temporary use only (inside of self-contained loops) */

/* FLAGS 24-31 are reserved for PyMOL's internal use */

/* FLAG 24 - don't surface these atoms (waters, ligands, etc.) */

#define cAtomFlag_exfoliate     0x01000000
/* FLAG 25 - ignore atoms altogether when surfacing */

/* FLAG 26 - ? */

/* FLAG 27 - polymer */
#define cAtomFlag_polymer       0x08000000
/* FLAG 28 - waters */
#define cAtomFlag_solvent       0x10000000
/* FLAG 29 - organics */
#define cAtomFlag_organic       0x20000000
/* FLAG 30 - inorganics */
#define cAtomFlag_inorganic     0x40000000

/* FLAG 31 - guide atom: e.g. CA in proteins */
#define cAtomFlag_guide         0x80000000

#define cAtomFlag_class         0xF8000000
#define cAtomFlag_class_mask    0x07FFFFFF

#define cAtomFlag_ignore        0x02000000
#define cAtomFlag_no_smooth     0x04000000

#define cResnLen 5
#define cResiLen 5
#define cAtomNameLen 4
#define cSegiLen 4
#define cTextTypeLen 20
#define cLabelTypeLen 20

#define cAtomInfoTetrahedral 4
#define cAtomInfoPlaner 3
#define cAtomInfoLinear 2
#define cAtomInfoSingle 1
#define cAtomInfoNone 5


#define cAN_H   1
#define cAN_He  2
#define cAN_B   5
#define cAN_C   6
#define cAN_N   7
#define cAN_O   8
#define cAN_F   9
#define cAN_Na 11
#define cAN_Mg 12
#define cAN_Si 14
#define cAN_P  15
#define cAN_S  16
#define cAN_Cl 17
#define cAN_K  19
#define cAN_Ca 20
#define cAN_Mn 25
#define cAN_Fe 26
#define cAN_Cu 29
#define cAN_Zn 30
#define cAN_Se 34
#define cAN_Br 35
#define cAN_I  53
#define cAN_Hg 80

typedef char Chain[2];
typedef char SSType[2];
typedef char SegIdent[cSegiLen+1];
typedef char ResIdent[cResiLen+1];
typedef char ResName[cResnLen+1];
typedef char AtomName[cAtomNameLen+1];
typedef char TextType[cTextTypeLen+1];
typedef char LabelType[cLabelTypeLen+1];

#define cAtomInfoNoType -9999

typedef struct AtomInfoType {
  int resv;
  Chain chain;
  Chain alt;
  ResIdent resi;
  SegIdent segi;
  ResName resn;
  AtomName name;
  AtomName elem;
  TextType textType;
  LabelType label;
  SSType ssType; /* blank or 'L' = turn/loop, 'H' = helix, 'S' = beta-strand/sheet */
  int hydrogen;
  int customType;
  int priority;
  float b,q,vdw,partialCharge;
  int formalCharge;
  signed char hetatm;
  short int model; /* obsolete?? */
  int atom;       /* obsolete?? */
  int selEntry;
  short int visRep[cRepCnt]; /* why isn't this a char? */
  int color;
  int id; 
  int cartoon; /* 0 = default which is auto (use ssType) */
  unsigned int flags;
  signed char bonded; /* be careful not to write at these as (int*) */
  signed char chemFlag;
  signed char geom;
  signed char valence;
  signed char deleteFlag;
  signed char masked;
  signed char protekted;
  signed char protons;
  signed char hb_donor; 
  signed char hb_acceptor;

  int temp1; /* kludge field - to remove */
  int sculpt_id; /* introduced in version 0.77 */
  int stereo; /* for 2D representation  (NOTE: could probably be converted to signed char */
  int discrete_state; /* state+1 for atoms in discrete objects */
  float bohr_radius;
  int rank;
  int atomic_color; /* what color was this atom originally assigned? */
} AtomInfoType;

void AtomInfoFree(PyMOLGlobals *G);
int AtomInfoInit(PyMOLGlobals *G);


int *AtomInfoGetSortedIndex(PyMOLGlobals *G,AtomInfoType *rec,int n,int **outdex);
void AtomInfoAssignParameters(PyMOLGlobals *G,AtomInfoType *I);
void AtomInfoFreeSortedIndexes(PyMOLGlobals *G,int *index,int *outdex);
void AtomInfoPrimeColors(PyMOLGlobals *G);
void AtomInfoAssignColors(PyMOLGlobals *G,AtomInfoType *at1);
int AtomInfoGetColor(PyMOLGlobals *G,AtomInfoType *at1);
int AtomInfoGetExpectedValence(PyMOLGlobals *G,AtomInfoType *I);
PyObject *AtomInfoAsPyList(PyMOLGlobals *G,AtomInfoType *at);
int AtomInfoFromPyList(PyMOLGlobals *G,AtomInfoType *at,PyObject *list);
int AtomInfoMatch(PyMOLGlobals *G,AtomInfoType *at1,AtomInfoType *at2);
int AtomInfoCompare(PyMOLGlobals *G,AtomInfoType *at1,AtomInfoType *at2);
int AtomInfoCompareIgnoreHet(PyMOLGlobals *G,AtomInfoType *at1,AtomInfoType *at2);
float AtomInfoGetBondLength(PyMOLGlobals *G,AtomInfoType *ai1,AtomInfoType *ai2);
int AtomInfoSameResidue(PyMOLGlobals *G,AtomInfoType *at1,AtomInfoType *at2);
int AtomInfoSameResidueP(PyMOLGlobals *G,AtomInfoType *at1,AtomInfoType *at2);
int AtomInfoSameChainP(PyMOLGlobals *G,AtomInfoType *at1,AtomInfoType *at2);
int AtomInfoSameSegmentP(PyMOLGlobals *G,AtomInfoType *at1,AtomInfoType *at2);
int AtomInfoSequential(PyMOLGlobals *G,AtomInfoType *at1,AtomInfoType *at2);

void AtomInfoBracketResidue(PyMOLGlobals *G,AtomInfoType *ai0,int n0,AtomInfoType *ai,int *st,int *nd);
void AtomInfoBracketResidueFast(PyMOLGlobals *G,AtomInfoType *ai0,int n0,int cur,int *st,int *nd);

void AtomInfoUniquefyNames(PyMOLGlobals *G,AtomInfoType *atInfo0,int n0,AtomInfoType *atInfo1,int n1);
int AtomInfoGetCarbColor(PyMOLGlobals *G);
int AtomResvFromResi(char *resi);

int AtomInfoKnownWaterResName(PyMOLGlobals *G,char *resn);
int AtomInfoKnownPolymerResName(PyMOLGlobals *G,char *resn);
void AtomInfoGetPDB3LetHydroName(PyMOLGlobals *G,char *resn, char *iname, char *oname);

#define cAIC_ct        0x0001
#define cAIC_fc        0x0002
#define cAIC_pc        0x0004
#define cAIC_b         0x0008
#define cAIC_q         0x0010
#define cAIC_id        0x0020
#define cAIC_flags     0x0080
#define cAIC_tt        0x0100
#define cAIC_state     0x0200
#define cAIC_rank      0x0400

#define cAIC_IDMask (cAIC_id|cAIC_rank)
#define cAIC_PDBMask (cAIC_b|cAIC_q|cAIC_id|cAIC_rank)
#define cAIC_MMDMask (cAIC_pc|cAIC_ct|cAIC_id|cAIC_rank)
#define cAIC_MOLMask (cAIC_fc|cAIC_id|cAIC_rank)
#define cAIC_AllMask 0xFFFF

void AtomInfoCombine(PyMOLGlobals *G,AtomInfoType *dst,AtomInfoType *src,int mask);
int AtomInfoNameOrder(PyMOLGlobals *G,AtomInfoType *at1,AtomInfoType *at2);
int AtomInfoUpdateAutoColor(PyMOLGlobals *G);

typedef struct  {
  int resv1,resv2;
  ResIdent resi1,resi2;
  unsigned char chain1,chain2;
  unsigned char type;
  int next;
} SSEntry;

/* legacy binary file support */
#if 0

typedef struct AtomInfoType068 {
  int resv;
  Chain chain;
  Chain alt;
  ResIdent resi;
  SegIdent segi;
  ResName resn;
  AtomName name;
  AtomName elem;
  TextType textType;
  LabelType label;
  SSType ssType; /* blank or 'L' = turn/loop, 'H' = helix, 'S' = beta-strand/sheet */
  int hydrogen;
  int customType;
  int priority;
  float b,q,vdw,partialCharge;
  int formalCharge;
  signed char hetatm;
  short int model; 
  int atom;
  int selEntry;
  short int visRep[cRepCnt];
  int color;
  int id; 
  int cartoon; /* 0 = default which is auto (use ssType) */
  unsigned int flags;
  signed char bonded; /* be careful not to write at these as (int*) */
  signed char chemFlag;
  signed char geom;
  signed char valence;
  signed char deleteFlag;
  signed char masked;
  signed char protekted;
  signed char protons;
} AtomInfoType068;

typedef struct AtomInfoType076 {
  int resv;
  Chain chain;
  Chain alt;
  ResIdent resi;
  SegIdent segi;
  ResName resn;
  AtomName name;
  AtomName elem;
  TextType textType;
  LabelType label;
  SSType ssType; /* blank or 'L' = turn/loop, 'H' = helix, 'S' = beta-strand/sheet */
  int hydrogen;
  int customType;
  int priority;
  float b,q,vdw,partialCharge;
  int formalCharge;
  signed char hetatm;
  short int model; 
  int atom;
  int selEntry;
  short int visRep[cRepCnt];
  int color;
  int id; 
  int cartoon; /* 0 = default which is auto (use ssType) */
  unsigned int flags;
  signed char bonded; /* be careful not to write at these as (int*) */
  signed char chemFlag;
  signed char geom;
  signed char valence;
  signed char deleteFlag;
  signed char masked;
  signed char protekted;
  signed char protons;
  int temp1; /* kludge field - to remove */
} AtomInfoType076;

typedef struct AtomInfoType083 {
  int resv;
  Chain chain;
  Chain alt;
  ResIdent resi;
  SegIdent segi;
  ResName resn;
  AtomName name;
  AtomName elem;
  TextType textType;
  LabelType label;
  SSType ssType; /* blank or 'L' = turn/loop, 'H' = helix, 'S' = beta-strand/sheet */
  int hydrogen;
  int customType;
  int priority;
  float b,q,vdw,partialCharge;
  int formalCharge;
  signed char hetatm;
  short int model; 
  int atom;
  int selEntry;
  short int visRep[cRepCnt];
  int color;
  int id; 
  int cartoon; /* 0 = default which is auto (use ssType) */
  unsigned int flags;
  signed char bonded; /* be careful not to write at these as (int*) */
  signed char chemFlag;
  signed char geom;
  signed char valence;
  signed char deleteFlag;
  signed char masked;
  signed char protekted;
  signed char protons;
  int temp1; /* kludge field - to remove */
  int sculpt_id; /* introduced in version 0.77 */
  int stereo; /* for 2D representation */
} AtomInfoType083;

typedef struct AtomInfoType090 {
  int resv;
  Chain chain;
  Chain alt;
  ResIdent resi;
  SegIdent segi;
  ResName resn;
  AtomName name;
  AtomName elem;
  TextType textType;
  LabelType label;
  SSType ssType; /* blank or 'L' = turn/loop, 'H' = helix, 'S' = beta-strand/sheet */
  int hydrogen;
  int customType;
  int priority;
  float b,q,vdw,partialCharge;
  int formalCharge;
  signed char hetatm;
  short int model; /* obsolete?? */
  int atom;       /* obsolete?? */
  int selEntry;
  short int visRep[cRepCnt];
  int color;
  int id; 
  int cartoon; /* 0 = default which is auto (use ssType) */
  unsigned int flags;
  signed char bonded; /* be careful not to write at these as (int*) */
  signed char chemFlag;
  signed char geom;
  signed char valence;
  signed char deleteFlag;
  signed char masked;
  signed char protekted;
  signed char protons;
  int temp1; /* kludge field - to remove */
  int sculpt_id; /* introduced in version 0.77 */
  int stereo; /* for 2D representation */
} AtomInfoType090;

typedef struct AtomInfoType095 {
  int resv;
  Chain chain;
  Chain alt;
  ResIdent resi;
  SegIdent segi;
  ResName resn;
  AtomName name;
  AtomName elem;
  TextType textType;
  LabelType label;
  SSType ssType; /* blank or 'L' = turn/loop, 'H' = helix, 'S' = beta-strand/sheet */
  int hydrogen;
  int customType;
  int priority;
  float b,q,vdw,partialCharge;
  int formalCharge;
  signed char hetatm;
  short int model; /* obsolete?? */
  int atom;       /* obsolete?? */
  int selEntry;
  short int visRep[cRepCnt];
  int color;
  int id; 
  int cartoon; /* 0 = default which is auto (use ssType) */
  unsigned int flags;
  signed char bonded; /* be careful not to write at these as (int*) */
  signed char chemFlag;
  signed char geom;
  signed char valence;
  signed char deleteFlag;
  signed char masked;
  signed char protekted;
  signed char protons;
  int temp1; /* kludge field - to remove */
  int sculpt_id; /* introduced in version 0.77 */
  int stereo; /* for 2D representation */
  int hb_donor;
  int hb_acceptor;
} AtomInfoType095;

#endif

#endif
