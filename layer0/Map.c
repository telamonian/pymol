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
#include"MemoryDebug.h"
#include"Err.h"
#include"OOMac.h"
#include"Map.h"
#include"Setting.h"

#ifndef true
#define true 1
#endif

#ifndef false
#define false 0
#endif

void MapFree(MapType *I)
{
  if(I)
	 {
		FreeP(I->Head);
		FreeP(I->Link);
		FreeP(I->EHead);
		FreeP(I->Cache);
		FreeP(I->CacheLink);
		VLAFreeP(I->EList);
	 }
  OOFreeP(I);
}

void MapCacheInit(MapType *I) 
{
  int a,*p;
  I->Cache = Alloc(int,I->NVert);
  I->CacheLink = Alloc(int,I->NVert);
  I->CacheStart = -1;
  p=I->Cache;
  for(a=0;a<I->NVert;a++)
	 *(p++) = 0;
}

void MapCacheReset(MapType *I) 
{
  int i;
  i=I->CacheStart;
  while(i>=0) {
	 I->Cache[i]=0;
	 i=I->CacheLink[i];
  }
  I->CacheStart=-1;
}

#define MapSafety 0.01

int MapInsideXY(MapType *I,float *v,int *a,int *b,int *c) /* special version for ray-tracing */
{
  *a=((v[0]-I->Min[0])/I->Div)+MapBorder;
  if(*a<I->iMin[0]) { 
	 if((I->iMin[0]-*a)>1) 
		return(false);
	 else 
		*a=I->iMin[0]; 
  } else if(*a>I->iMax[0]) { 
	 if((*a-I->iMax[0])>1) 
		return(false);
	 else 
		*a=I->iMax[0]; 
  }
  *b=((v[1]-I->Min[1])/I->Div)+MapBorder;
  if(*b<I->iMin[1]) { 
	 if((I->iMin[1]-*b)>1) 
		return(false);
	 else 
		*b=I->iMin[1];
  } else if(*b>I->iMax[1]) {
	 if((*b-I->iMax[1])>1) 
		return(false);
	 else 
		*b=I->iMax[1];
  }
  *c=((v[2]-I->Min[2])/I->Div)+MapBorder+1;
  if(*c<I->iMin[2]) *c=I->iMin[2];
  else if(*c>I->iMax[2]) *c=I->iMax[2];

  return(true);  
}

void MapSetupExpressXY(MapType *I) /* setup a list of XY neighbors for each square */
{
  int n=0;
  int a,b,c,d,e,i;
  unsigned int mapSize;
  int st,flag;

  mapSize = I->Dim[0]*I->Dim[1]*I->Dim[2];
  I->EHead=Alloc(int,mapSize);
  ErrChkPtr(I->EHead);
  I->EList=VLAMalloc(10000,sizeof(int),5,0);

  n=1;
  for(a=I->iMin[0];a<=I->iMax[0];a++)
	 for(b=I->iMin[1];b<=I->iMax[1];b++)
		for(c=I->iMin[2];c<=I->iMax[2];c++) /* a better alternative exists... */
		  {
			 st=n;
			 flag=false;
			 for(d=a-1;d<=a+1;d++)
				for(e=b-1;e<=b+1;e++)
				  {
					 i=*MapFirst(I,d,e,c);
					 if(i>=0) {
						flag=true;
						while(i>=0) {
						  VLACheck(I->EList,int,n);
						  I->EList[n]=i;
						  n++;
						  i=MapNext(I,i);
						}
					 }
				  }
			 if(flag) {
				*(MapEStart(I,a,b,c))=st;
				VLACheck(I->EList,int,n);
				I->EList[n]=-1;
				n++;
			 } else {
				*(MapEStart(I,a,b,c))=0;
			 }
		  }
  I->NEElem=n;
}

void MapSetupExpress(MapType *I) /* setup a list of neighbors for each square */
{
  int n=0;
  int a,b,c,d,e,f,i;
  unsigned int mapSize;
  int st,flag;

  mapSize = I->Dim[0]*I->Dim[1]*I->Dim[2];
  I->EHead=Alloc(int,mapSize);
  ErrChkPtr(I->EHead);
  I->EList=VLAMalloc(1000,sizeof(int),5,0);

  n=1;
  for(a=(I->iMin[0]-1);a<=(I->iMax[0]+1);a++)
	 for(b=(I->iMin[1]-1);b<=(I->iMax[1]+1);b++)
		for(c=(I->iMin[2]-1);c<=(I->iMax[2]+1);c++)
		  {
			 st=n;
			 flag=false;
			 for(d=a-1;d<=a+1;d++)
				for(e=b-1;e<=b+1;e++)
				  for(f=c-1;f<=c+1;f++)
					 {
						i=*MapFirst(I,d,e,f);
						if(i>=0) {
						  flag=true;
						  while(i>=0) {
							 VLACheck(I->EList,int,n);
							 I->EList[n]=i;
							 n++;
							 i=MapNext(I,i);
						  }
						}
					 }
			 if(flag) {
				*(MapEStart(I,a,b,c))=st;
				VLACheck(I->EList,int,n);
				I->EList[n]=-1;
				n++;
			 } else {
				*(MapEStart(I,a,b,c))=0;
			 }
		  }
}

void MapLocus(MapType *I,float *v,int *a,int *b,int *c)
{
  *a=((v[0]-I->Min[0])/I->Div)+MapBorder;
  *b=((v[1]-I->Min[1])/I->Div)+MapBorder;
  *c=((v[2]-I->Min[2])/I->Div)+MapBorder;
  /* range checking...*/
  if(*a<I->iMin[0]) *a=I->iMin[0];
  else if(*a>I->iMax[0]) *a=I->iMax[0];
  if(*b<I->iMin[1]) *b=I->iMin[1];
  else if(*b>I->iMax[1]) *b=I->iMax[1];
  if(*c<I->iMin[2]) *c=I->iMin[2];
  else if(*c>I->iMax[2]) *c=I->iMax[2];
}

int MapExclLocus(MapType *I,float *v,int *a,int *b,int *c)
{
  *a=((v[0]-I->Min[0])/I->Div)+MapBorder;
  if(*a<I->iMin[0]) return(0);
  else if(*a>I->iMax[0]) return(0);
  *b=((v[1]-I->Min[1])/I->Div)+MapBorder;
  if(*b<I->iMin[1]) return(0);
  else if(*b>I->iMax[1]) return(0);
  *c=((v[2]-I->Min[2])/I->Div)+MapBorder;
  if(*c<I->iMin[2]) return(0);
  else if(*c>I->iMax[2]) return(0);
  return(1);
}

MapType *MapNew(float range,float *vert,int nVert,float *extent)
{
  Vector3f diagonal;
  float size,subDiv;
  int a,c;
  int mapSize;
  int h,k,l;
  int *i;
  int *list;
  float *v;
  int maxSize;

  OOAlloc(MapType);

  maxSize = SettingGet(cSetting_hash_max);
  I->Head = NULL;
  I->Link = NULL;
  I->Cache = NULL;
  I->CacheLink = NULL;
  I->CacheStart = -1;
  I->EHead = NULL;
  I->EList = NULL;
  I->NEElem=0;
  
  I->Link=Alloc(int,nVert);
  ErrChkPtr(I->Link);

  for(a=0;a<nVert;a++)
	 I->Link[a] = -1;

  if(extent)
	 {
		I->Min[0]=extent[0];
		I->Max[0]=extent[1];
		I->Min[1]=extent[2];
		I->Max[1]=extent[3];
		I->Min[2]=extent[4];
		I->Max[2]=extent[5];
	 }
  else
	 {
		v=vert;
		for(c=0;c<3;c++)
		  {
			 I->Min[c] = v[c];
			 I->Max[c] = v[c];
		  }
		v+=3;
		for(a=1;a<nVert;a++)
		  {
			 for(c=0;c<3;c++)
				{
				  if(I->Min[c]>v[c])
					 I->Min[c]=v[c];
				  if(I->Max[c]<v[c])
					 I->Max[c]=v[c];
				}
			 v+=3;
		  }
	 }
  for(c=0;c<3;c++)
	 {
		I->Min[c]-=MapSafety;
		I->Max[c]+=MapSafety;
	 }

  if(range<0.0) { /* negative range is a flag to expand edges using "range".*/
	 range=-range;
	 for(c=0;c<3;c++)
		{
		  I->Min[c]-=range;
		  I->Max[c]+=range;
		}
  }
  /* find longest axis */

  subtract3f(I->Max,I->Min,diagonal);
  size=diagonal[0];
  if(diagonal[1]>size) size=diagonal[1];
  if(diagonal[2]>size) size=diagonal[2];

  /* compute maximum number of subdivisions */
  subDiv = (int)(size/(range+MapSafety)); 
  if(subDiv>maxSize ) subDiv = maxSize; /* keep it reasonable - we're talking N^3 here... */
  if(subDiv<1.0) subDiv = 1.0;

  /* compute final box size */
  I->Div = size/subDiv;

  /* add borders to avoid special edge cases */
  I->Dim[0]=(diagonal[0]/I->Div)+1+(2*MapBorder); 
  I->Dim[1]=(diagonal[1]/I->Div)+1+(2*MapBorder);
  I->Dim[2]=(diagonal[2]/I->Div)+1+(2*MapBorder);

  /*  printf(" MapSetup: I->Div: %8.3f\n",I->Div);
  printf(" MapSetup: %8.3f %8.3f %8.3f %8.3f %8.3f %8.3f\n",
			I->Min[0],I->Min[1],I->Min[2],
			I->Max[0],I->Max[1],I->Max[2]);
  printf(" MapSetup: %8d %8d %8d\n",
  I->Dim[0],I->Dim[1],I->Dim[2]);*/

  I->D1D2 = I->Dim[1]*I->Dim[2];

  I->iMin[0]=MapBorder;
  I->iMin[1]=MapBorder;
  I->iMin[2]=MapBorder;

  I->iMax[0]=I->Dim[0]-(1+MapBorder);
  I->iMax[1]=I->Dim[1]-(1+MapBorder);
  I->iMax[2]=I->Dim[2]-(1+MapBorder);

  /* compute size and allocate */
  mapSize = I->Dim[0]*I->Dim[1]*I->Dim[2];
  I->Head=Alloc(int,mapSize);
  /*  printf("%d\n",mapSize);*/
  ErrChkPtr(I->Head);

  /* initialize */
  /*  for(a=0;a<I->Dim[0];a++)
	 for(b=0;b<I->Dim[1];b++)
		for(c=0;c<I->Dim[2];c++)
		*(MapFirst(I,a,b,c))=-1;*/
  a=mapSize;
  i=I->Head;
  while(a--)
	 *(i++)=-1;

  I->NVert = nVert;

  /* create 3-D hash of the vertices*/
  v=vert;
  for(a=0;a<nVert;a++)
	 {
		if(MapExclLocus(I,v,&h,&k,&l)) {
		  list = MapFirst(I,h,k,l);
		  I->Link[a] = *list; 
		  *list = a; /*add to top of list*/
		}
		v+=3;
	 }

  return(I);
}
