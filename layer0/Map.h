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
#ifndef _H_Map
#define _H_Map

/* Map - a 3-D hash object for optimizing neighbor searches */

#include"Vector.h"

typedef struct { 
  float Div;
  Vector3i Dim;
  int D1D2;
  Vector3i iMin,iMax;
  int *Head,*Link;
  int *EHead,*EList;
  int *Cache,*CacheLink,CacheStart;
  int NVert;
  int NEElem;
  Vector3f Max,Min;
} MapType;

#define MapBorder 2

MapType *MapNew(float range,float *vert,int nVert,float *extent);
MapType *MapNewFlagged(float range,float *vert,int nVert,float *extent,int *flag);
void MapSetupExpress(MapType *I);
void MapFree(MapType *I);

#define MapFirst(m,a,b,c) (m->Head + ((a) * m->D1D2) + ((b)*m->Dim[2]) + (c))

#define MapEStart(m,a,b,c) (m->EHead + ((a) * m->D1D2) + ((b)*m->Dim[2]) + (c))

#define MapNext(m,a) (*(m->Link+a))
void MapLocus(MapType *map,float *v,int *a,int *b,int *c);
int *MapLocusEStart(MapType *map,float *v);
int MapExclLocus(MapType *map,float *v,int *a,int *b,int *c);

#define MapCache(m,a) {m->Cache[a]=1;m->CacheLink[a]=m->CacheStart;m->CacheStart=a;}
#define MapCached(m,a) (m->Cache[a])

void MapCacheInit(MapType *I);
void MapCacheReset(MapType *I);

float MapGetSeparation(float range,float *mx,float *mn,float *diagonal);

/* special routines for raytracing */

int MapInsideXY(MapType *I,float *v,int *a,int *b,int *c); 
void MapSetupExpressXY(MapType *I);

#endif









