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

#include"os_predef.h"
#include"os_std.h"
#include"os_gl.h"

#include"OOMac.h"
#include"RepDistLabel.h"
#include"Color.h"
#include"Scene.h"
#include"main.h"
#include"Vector.h"
#include"Setting.h"
#include"PyMOLObject.h"
#include"Text.h"

typedef char DistLabel[8];

typedef struct RepDistLabel {
  Rep R;
  float *V;
  int N;
  DistLabel *L;
  CObject *Obj;
  DistSet *ds;
} RepDistLabel;

#include"ObjectDist.h"

void RepDistLabelFree(RepDistLabel *I);

void RepDistLabelFree(RepDistLabel *I)
{
  VLAFreeP(I->V);
  VLAFreeP(I->L);
  RepPurge(&I->R);
  OOFreeP(I);
}

static void RepDistLabelRender(RepDistLabel *I,RenderInfo *info)
{
  CRay *ray = info->ray;
  Picking **pick = info->pick;
  PyMOLGlobals *G=I->R.G;
  float *v=I->V;
  int c=I->N;
  DistLabel *l = I->L;
  int n = 0;
  int color;
  int font_id = SettingGet_i(G,I->ds->Setting,I->Obj->Setting,cSetting_label_font_id);
  float font_size = SettingGet_f(G,I->ds->Setting,I->Obj->Setting,cSetting_label_size);

  if(ray) {

    color = SettingGet_color(G,I->ds->Setting,I->Obj->Setting,cSetting_label_color);
    
    if(color>=0)
      TextSetColor(G,ColorGet(G,color));
    else
      TextSetColor(G,ColorGet(G,I->Obj->Color));

	 while(c--) {
      TextSetPos(G,v);
      TextRenderRay(G,ray,font_id,l[n],font_size,NULL);
      v+=3;
      n++;
	 }
  } else if(G->HaveGUI && G->ValidContext) {
    if(pick) {
    } else {
      int float_text = SettingGet_i(G,I->ds->Setting,
                                      I->Obj->Setting,
                                      cSetting_float_labels);
      if(float_text)
        glDisable(GL_DEPTH_TEST);	 
      glDisable(GL_LIGHTING);
    
      color = SettingGet_color(G,I->ds->Setting,I->Obj->Setting,cSetting_label_color);
    
      if(color>=0)
        TextSetColor(G,ColorGet(G,color));
      else
        TextSetColor(G,ColorGet(G,I->Obj->Color));
      while(c--) {

        TextSetPos(G,v);
        TextRenderOpenGL(G,info,font_id,l[n],font_size,NULL);
        v+=3;
        n++;
      }
      glEnable(GL_LIGHTING);
      if(float_text)
        glEnable(GL_DEPTH_TEST);	 
    }
  }
}

Rep *RepDistLabelNew(DistSet *ds)
{
  PyMOLGlobals *G=ds->State.G;
  int a;
  int n = 0;
  float *v,*v1,*v2,*v3,d[3],di;
  char buffer[255];

  OOAlloc(G,RepDistLabel);

  if(!(ds->NIndex||ds->NAngleIndex||ds->NDihedralIndex)) {
    OOFreeP(I);
    return(NULL); 
  }

  RepInit(G,&I->R);

  I->R.fRender=(void (*)(struct Rep *, RenderInfo *))RepDistLabelRender;
  I->R.fFree=(void (*)(struct Rep *))RepDistLabelFree;
  I->R.fRecolor=NULL;

  I->N=0;
  I->V=NULL;
  I->R.P=NULL;
  I->Obj = (CObject*)ds->Obj;
  I->ds = ds;

  if(ds->NIndex || ds->NAngleIndex || ds->NDihedralIndex) {
	 I->V=VLAlloc(float,3*(ds->NIndex/2+ds->NAngleIndex/5)+1);
    I->L=VLAlloc(DistLabel,(ds->NIndex/2+ds->NAngleIndex/5)+1);
    
    n=0;
    if(ds->NIndex) {
      for(a=0;a<ds->NIndex;a=a+2) {
        v1 = ds->Coord+3*a;
        v2 = ds->Coord+3*(a+1);
        average3f(v2,v1,d);
        di = (float)diff3f(v1,v2);
        sprintf(buffer,"%1.2f",di);
        buffer[7]=0;
        
        VLACheck(I->V,float,3*n+2);
        VLACheck(I->L,DistLabel, n);
        v = I->V + 3 * n;
        strcpy(I->L[n],buffer);
        copy3f(d,v);
        
        n++;
      }
    }

    if(ds->NAngleIndex) {
      float d1[3],d2[3],n1[3],n2[3];
      float avg[3];

      float l1,l2;
      float radius;

      for(a=0;a<ds->NAngleIndex;a=a+5) {
        v1 = ds->AngleCoord+3*a;
        v2 = ds->AngleCoord+3*(a+1);
        v3 = ds->AngleCoord+3*(a+2);
        subtract3f(v1,v2,d1);
        subtract3f(v3,v2,d2);
        
        normalize23f(d1,n1);
        normalize23f(d2,n2);

        average3f(n1,n2,avg);
        
        l1 = (float)length3f(d1);
        l2 = (float)length3f(d2);
        
        if(l1>l2)
          radius = l2;
        else
          radius = l1;
        radius *= SettingGet_f(G,ds->Setting,ds->Obj->Obj.Setting,cSetting_angle_size) *
          SettingGet_f(G,ds->Setting,ds->Obj->Obj.Setting,cSetting_angle_label_position);
          
        normalize3f(avg);
        if((avg[0]==0.0F) && (avg[1]==0.0F) && (avg[2]==0.0F))
          avg[0]=1.0F;
        
        scale3f(avg,radius,avg);
        add3f(v2,avg,avg);

        di = (float)(180.0F * get_angle3f(d1,d2)/PI);
        sprintf(buffer,"%1.2f",di);
        buffer[7]=0;
        
        VLACheck(I->V,float,3*n+2);
        VLACheck(I->L,DistLabel, n);
        v = I->V + 3 * n;
        strcpy(I->L[n],buffer);
        copy3f(avg,v);
        
        n++;
      }
    }

    if(ds->NDihedralIndex) {

      float d12[3], d32[3], d43[3] , n32[3];
      float p12[3], p43[3], np12[3], np43[3];
      
      float a32[3];
      float l1,l2;
      float radius;
      float dihedral_size = SettingGet_f(G,ds->Setting,ds->Obj->Obj.Setting,cSetting_dihedral_size);
      float dihedral_label_position = SettingGet_f(G,ds->Setting,ds->Obj->Obj.Setting,cSetting_dihedral_label_position);
      
      float *v4, *v5, *v6;
      float avg[3];

      for(a=0;a<ds->NDihedralIndex;a=a+6) {
        v1 = ds->DihedralCoord+3*a;
      v2 = v1 + 3;
      v3 = v1 + 6;
      v4 = v1 + 9;
      v5 = v1 + 12;
      v6 = v1 + 15;
      
      subtract3f(v1,v2,d12);
      subtract3f(v3,v2,d32);
      subtract3f(v4,v3,d43);
      
      normalize23f(d32,n32);

      remove_component3f(d12,n32,p12);
      remove_component3f(d43,n32,p43);

      average3f(v2,v3,a32);

      l1 = (float)length3f(p12);
      l2 = (float)length3f(p43);

      if(l1>l2)
        radius = l2;
      else
        radius = l1;
      radius *= dihedral_size * dihedral_label_position;
      
      normalize23f(p12,np12);
      normalize23f(p43,np43);
      

      average3f(np12,np43,avg);
      
      normalize3f(avg);
      if((avg[0]==0.0F) && (avg[1]==0.0F) && (avg[2]==0.0F))
        copy3f(np12,avg);
      
        scale3f(avg,radius,avg);
        add3f(a32,avg,avg);

        di = (float)(180.0F * get_dihedral3f(v1,v2,v3,v4)/PI);
        sprintf(buffer,"%1.2f",di);
        buffer[7]=0;
        
        VLACheck(I->V,float,3*n+2);
        VLACheck(I->L,DistLabel, n);
        v = I->V + 3 * n;
        strcpy(I->L[n],buffer);
        copy3f(avg,v);
        
        n++;
      }
    }

    
  }
  
  I->N=n;

  return((void*)(struct Rep*)I);
}


