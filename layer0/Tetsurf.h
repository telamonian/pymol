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
#ifndef _H_Tetsurf
#define _H_Tetsurf

#include"Map.h"
#include"MemoryDebug.h"
#include"Crystal.h"
#include"Field.h"

#define F3(field,P1,P2,P3) Ffloat3(field,P1,P2,P3)
#define F3Ptr(field,P1,P2,P3) Ffloat3p(field,P1,P2,P3)

#define F4(field,P1,P2,P3,P4) Ffloat4(field,P1,P2,P3,P4)
#define F4Ptr(field,P1,P2,P3,P4) Ffloat4p(field,P1,P2,P3,P4)

int	TetsurfVolume(Isofield *field,float level,int **num,
                    float **vert,int *range,int mode,
                    MapType *voxelmap,float *a_vert,
                    float carvebuffer);
void TetsurfGetRange(Isofield *field,CCrystal *cryst,float *mn,float *mx,int *range);

int	TetsurfInit(void);

#endif
