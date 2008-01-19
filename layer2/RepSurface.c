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

#include"Base.h"
#include"MemoryDebug.h"
#include"OOMac.h"
#include"RepSurface.h"
#include"Map.h"
#include"Scene.h"
#include"Sphere.h"
#include"Setting.h"
#include"Color.h"
#include"ObjectMolecule.h"
#include"Triangle.h"
#include"Vector.h"
#include"Feedback.h"
#include"main.h"
#include"Util.h"
#include"CGO.h"
#include"P.h"
#include"Selector.h"

#ifdef NT
#undef NT
#endif

typedef struct RepSurface {
  Rep R;
  int N;
  int NT;
  int proximity;
  float *V,*VN,*VC,*VA;
  int *RC;
  int *Vis;
  int *T,*S; /* S=strips */
  int solidFlag;
  int oneColorFlag,oneColor;
  int allVisibleFlag;
  int *LastVisib;
  int *LastColor;
  int Type;
  float max_vdw;
  CGO *debug;
} RepSurface;


void RepSurfaceFree(RepSurface *I);
int RepSurfaceSameVis(RepSurface *I,CoordSet *cs);

void RepSurfaceColor(RepSurface *I,CoordSet *cs);

void RepSurfaceFree(RepSurface *I)
{
  FreeP(I->V);
  FreeP(I->VN);
  FreeP(I->VC);
  FreeP(I->VA);
  FreeP(I->RC);
  FreeP(I->Vis);
  FreeP(I->LastColor);
  FreeP(I->LastVisib);
  CGOFree(I->debug);
  VLAFreeP(I->T);
  VLAFreeP(I->S);
  RepPurge(&I->R); /* unnecessary, but a good idea */
  /*  VLAFreeP(I->N);*/
  OOFreeP(I);
}

typedef struct {
  int nDot;
  float *dot;
  float *dotNormal;
  int *dotCode;
} SolventDot;

typedef struct {
  float vdw;
  int flags;
} SurfaceJobAtomInfo;

static SolventDot *SolventDotNew(PyMOLGlobals *G, float *coord,
                                 SurfaceJobAtomInfo *atom_info,
                                 float probe_radius,SphereRec *sp,
                                 int *present,
                                 int circumscribe, int surface_mode, 
                                 int surface_solvent, int cavity_cull,
                                 int all_visible_flag,
                                 float max_vdw);

static void SolventDotFree(SolventDot *I)
{
  if(I) {
    VLAFreeP(I->dot);	 
    VLAFreeP(I->dotNormal);
    VLAFreeP(I->dotCode);
  }
  OOFreeP(I);
}

#if 0
static int ZOrderFn(float *array,int l,int r)
{
  return (array[l]<=array[r]);
}

static int ZRevOrderFn(float *array,int l,int r)
{
  return (array[l]>=array[r]);
}
#endif

static int check_and_add(int *cache, int spacing, int t0,int t1) {
  int *rec;
  int cnt;
  t0++;
  t1++;
  
  rec = cache + spacing*t0;
  cnt=spacing;
  while(cnt>0) {
    if(*rec==t1) 
      return 1;
    if(!*rec) {
      *rec = t1;
      break;
    }
    rec++;
    cnt--;
  }
  rec = cache + spacing*t1;
  cnt=spacing;
  while(cnt>0) {
    if(*rec==t0)
      return 1;
    if(!*rec) {
      *rec = t0;
      break;
    }
    rec++;
    cnt--;
  }
  return 0;
}

static void RepSurfaceRender(RepSurface *I,RenderInfo *info)
{
  CRay *ray = info->ray;
  Picking **pick = info->pick;
  PyMOLGlobals *G=I->R.G;
  float *v=I->V;
  float *vn=I->VN;
  float *vc=I->VC;
  float *va=I->VA;
  int *rc=I->RC;
  int *t=I->T;
  int *s=I->S;
  int c=I->N;
  int *vi=I->Vis;
  float alpha;
  int t_mode;

  if((I->Type!=1) && (!s)) {
    return;
  }

  alpha = SettingGet_f(G,I->R.cs->Setting,I->R.obj->Setting,cSetting_transparency);
  alpha=1.0F-alpha;
  if(fabs(alpha-1.0)<R_SMALL4)
    alpha=1.0F;
  if(ray) {
    ray->fTransparentf(ray,1.0F-alpha);
    if(I->Type==1) {
      /* dot surface */

      float radius;
      
      radius = SettingGet_f(G,I->R.cs->Setting,I->R.obj->Setting,cSetting_dot_radius);
      
      if(radius==0.0F) {
        radius = ray->PixelRadius*SettingGet_f(G,I->R.cs->Setting,
                                               I->R.obj->Setting,
                                               cSetting_dot_width)/1.4142F;
      }
      
      if(I->oneColorFlag) {
        float col[3];
        ColorGetEncoded(G,I->oneColor,col);
        ray->fColor3fv(ray,col);
      }
      
      if(c) 
        while(c--) {
          if(*vi) {
            if(!I->oneColorFlag) {
              ray->fColor3fv(ray,vc);
            }
            ray->fSphere3fv(ray,v,radius);
          }
          vi++;
          vc+=3;
          v+=3;
        }
    } else if((I->Type==0)||(I->Type==3)||(I->Type==4)||(I->Type==5)) { /* solid surface */
      c=I->NT;

      if(I->oneColorFlag) {
        float col[3];
        ColorGetEncoded(G,I->oneColor,col);
        while(c--) {
          if((I->proximity&&((*(vi+(*t)))||(*(vi+(*(t+1))))||(*(vi+(*(t+2))))))||
             ((*(vi+(*t)))&&(*(vi+(*(t+1))))&&(*(vi+(*(t+2)))))) 
            ray->fTriangle3fv(ray,v+(*t)*3,v+(*(t+1))*3,v+(*(t+2))*3,
                              vn+(*t)*3,vn+(*(t+1))*3,vn+(*(t+2))*3,
                              col,col,col);
          t+=3;
        }
      } else {
        float colA[3],colB[3],colC[3];
        while(c--) {
          register int ttA = *t,ttB = *(t+1),ttC = *(t+2);
          if((I->proximity&&((*(vi+ttA))||(*(vi+ttB))||(*(vi+ttC))))||
             ((*(vi+ttA))&&(*(vi+ttB))&&(*(vi+ttC)))) {
            register int ttA3 = ttA*3, ttB3 = ttB*3, ttC3 = ttC*3;
            register float *cA=vc+ttA3, *cB=vc+ttB3, *cC=vc+ttC3;
            if(rc) {
              if(rc[ttA]<-1) ColorGetEncoded(G,rc[ttA], (cA=colA));
              if(rc[ttB]<-1) ColorGetEncoded(G,rc[ttB], (cB=colB));
              if(rc[ttC]<-1) ColorGetEncoded(G,rc[ttC], (cC=colC));
            }
            if((*(vi+ttA))||(*(vi+ttB))||(*(vi+ttC))) {
              if(va) {
                ray->fTriangleTrans3fv(ray,v+ttA3,v+ttB3,v+ttC3,
                                       vn+ttA3,vn+ttB3,vn+ttC3,
                                       cA, cB, cC,
                                       1.0F - va[ttA], 1.0F - va[ttB], 1.0F - va[ttC]);
              } else {
                ray->fTriangle3fv(ray,v+ttA3,v+ttB3,v+ttC3,
                                  vn+ttA3,vn+ttB3,vn+ttC3,
                                  cA, cB, cC);
              }
            }
          }
          t+=3;
        }
      }
    } else if(I->Type==2) { /* triangle mesh surface */

      float radius;
      int t0,t1,t2;
      int spacing = 10;
      int *cache = Calloc(int,spacing*(I->N+1));
      
      radius = SettingGet_f(G,I->R.cs->Setting,I->R.obj->Setting,cSetting_mesh_radius);

      if(radius==0.0F) {
        radius = ray->PixelRadius*SettingGet_f(G,I->R.cs->Setting,
                                               I->R.obj->Setting,
                                               cSetting_mesh_width)/2.0F;
      }

      c=I->NT;      
      if(I->oneColorFlag) {
        float col[3];
        ColorGetEncoded(G,I->oneColor,col);
        while(c--) {
          t0 = (*t);
          t1 = (*(t+1));
          t2 = (*(t+2));
          if((I->proximity&&((*(vi+t0))||(*(vi+t1))||(*(vi+t2))))||
             ((*(vi+t0))&&(*(vi+t1))&&(*(vi+t2)))) {
            if(!check_and_add(cache,spacing,t0,t1))
              ray->fSausage3fv(ray,v+t0*3,v+t1*3,radius,col,col);
            if(!check_and_add(cache,spacing,t1,t2))
              ray->fSausage3fv(ray,v+t1*3,v+t2*3,radius,col,col);
            if(!check_and_add(cache,spacing,t2,t0))
              ray->fSausage3fv(ray,v+t2*3,v+t0*3,radius,col,col);
          }
          t+=3;
        }
      } else {
        while(c--)
          {
            t0 = (*t);
            t1 = (*(t+1));
            t2 = (*(t+2));

            if((I->proximity&&((*(vi+t0))||(*(vi+t1))||(*(vi+t2))))||
               ((*(vi+t0))&&(*(vi+t1))&&(*(vi+t2)))) 
              if((*(vi+t0))||(*(vi+t1))||(*(vi+t2))) {
                if(!check_and_add(cache,spacing,t0,t1))
                  ray->fSausage3fv(ray,v+t0*3,v+t1*3,radius,vc+t0*3,vc+t1*3);
                if(!check_and_add(cache,spacing,t1,t2))
                  ray->fSausage3fv(ray,v+t1*3,v+t2*3,radius,vc+t1*3,vc+t2*3);
                if(!check_and_add(cache,spacing,t2,t0))
                  ray->fSausage3fv(ray,v+t2*3,v+t0*3,radius,vc+t2*3,vc+t0*3);
              }
            t+=3;
          }
      }
      FreeP(cache);
    }
    ray->fTransparentf(ray,0.0);
  } else if(G->HaveGUI && G->ValidContext) {
    if(pick) {
    } else {
      if(I->debug)
        CGORenderGL(I->debug,NULL,NULL,NULL,info);
      if(I->Type==1) {

        /* no triangle information, so we're rendering dots only */

        int normals = SettingGet_i(G,I->R.cs->Setting,I->R.obj->Setting,cSetting_dot_normals);
        int lighting = SettingGet_i(G,I->R.cs->Setting,I->R.obj->Setting,cSetting_dot_lighting);
        int use_dlst;
        if(!normals)
          SceneResetNormal(G,true);
        if(!lighting)
          glDisable(GL_LIGHTING);

        use_dlst = (int)SettingGet(G,cSetting_use_display_lists);
        if(use_dlst&&I->R.displayList) {
          glCallList(I->R.displayList);
        } else { 
        
          if(use_dlst) {
            if(!I->R.displayList) {
              I->R.displayList = glGenLists(1);
              if(I->R.displayList) {
                glNewList(I->R.displayList,GL_COMPILE_AND_EXECUTE);
              }
            }
          }
          glPointSize(SettingGet_f(G,I->R.cs->Setting,I->R.obj->Setting,cSetting_dot_width));
        
          if(c) {
            glColor3f(1.0,0.0,0.0);
            glBegin(GL_POINTS);
            if(I->oneColorFlag) {
              glColor3fv(ColorGet(G,I->oneColor));
            }
          
            while(c--)
              {
                if(*vi) {
                  if(!I->oneColorFlag) {
                    glColor3fv(vc);
                  }
                  if(normals) 
                    glNormal3fv(vn);
                  glVertex3fv(v);
                }
                vi++;
                vc+=3;
                vn+=3;
                v+=3;
              }
            glEnd();
          }
        
          if(use_dlst&&I->R.displayList) {
            glEndList();
          }
        }
        if(!lighting)
          glEnable(GL_LIGHTING);
      } else if(I->Type==2) { /* rendering triangle mesh */
      
        int normals = SettingGet_i(G,I->R.cs->Setting,I->R.obj->Setting,cSetting_mesh_normals); 
        int lighting = SettingGet_i(G,I->R.cs->Setting,I->R.obj->Setting,cSetting_mesh_lighting);
        int use_dlst;
        if(!normals)
          SceneResetNormal(G,true);
        if(!lighting)
          glDisable(GL_LIGHTING);
      
        use_dlst = (int)SettingGet(G,cSetting_use_display_lists);
        if(use_dlst&&I->R.displayList) {
          glCallList(I->R.displayList);
        } else { 
        
        
          glLineWidth(SettingGet_f(G,I->R.cs->Setting,I->R.obj->Setting,cSetting_mesh_width));
        
          if(use_dlst) {
            if(!I->R.displayList) {
              I->R.displayList = glGenLists(1);
              if(I->R.displayList) {
                glNewList(I->R.displayList,GL_COMPILE_AND_EXECUTE);
              }
            }
          }
        
        
          c=I->NT;
          if(c) {
            if(I->oneColorFlag) {
              glColor3fv(ColorGet(G,I->oneColor));
              while(c--) {
                if((I->proximity&&((*(vi+(*t)))||(*(vi+(*(t+1))))||(*(vi+(*(t+2))))))||
                   ((*(vi+(*t)))&&(*(vi+(*(t+1))))&&(*(vi+(*(t+2)))))) {
                  if(normals) {
                  
                    glBegin(GL_LINE_STRIP);
                  
                    glNormal3fv(vn+(*(t+2))*3);
                    glVertex3fv(v+(*(t+2))*3);
                  
                    glNormal3fv(vn+(*t)*3);
                    glVertex3fv(v+(*t)*3);
                    t++;
                    glNormal3fv(vn+(*t)*3);
                    glVertex3fv(v+(*t)*3);
                    t++;
                    glNormal3fv(vn+(*t)*3);
                    glVertex3fv(v+(*t)*3);
                    t++;
                    glEnd();
                  } else {
                    glBegin(GL_LINE_STRIP);
                  
                    glVertex3fv(v+(*(t+2))*3);
                    glVertex3fv(v+(*t)*3);
                    t++;
                    glVertex3fv(v+(*t)*3);
                    t++;
                    glVertex3fv(v+(*t)*3);
                    t++;
                    glEnd();
                  }
                } else
                  t+=3;
              }
            } else {
              while(c--) {
                if((I->proximity&&((*(vi+(*t)))||(*(vi+(*(t+1))))||(*(vi+(*(t+2))))))||
                   ((*(vi+(*t)))&&(*(vi+(*(t+1))))&&(*(vi+(*(t+2)))))) {
                  if(normals) {

                    glBegin(GL_LINE_STRIP);
                  
                    glColor3fv(vc+(*(t+2))*3);
                    glNormal3fv(vn+(*(t+2))*3);
                    glVertex3fv(v+(*(t+2))*3);
                  
                    glColor3fv(vc+(*t)*3);
                    glNormal3fv(vn+(*t)*3);
                    glVertex3fv(v+(*t)*3);
                    t++;
                    glColor3fv(vc+(*t)*3);
                    glNormal3fv(vn+(*t)*3);
                    glVertex3fv(v+(*t)*3);
                    t++;
                    glColor3fv(vc+(*t)*3);
                    glNormal3fv(vn+(*t)*3);
                    glVertex3fv(v+(*t)*3);
                    t++;
                    glEnd();
                  } else {
                    glBegin(GL_LINE_STRIP);
                  
                    glColor3fv(vc+(*(t+2))*3);
                    glVertex3fv(v+(*(t+2))*3);
                  
                    glColor3fv(vc+(*t)*3);
                    glVertex3fv(v+(*t)*3);
                    t++;
                    glColor3fv(vc+(*t)*3);
                    glVertex3fv(v+(*t)*3);
                    t++;
                    glColor3fv(vc+(*t)*3);
                    glVertex3fv(v+(*t)*3);
                    t++;
                    glEnd();

                  }
                } else
                  t+=3;
              }
            }
          }
          if(use_dlst&&I->R.displayList) {
            glEndList();
          }
        }
        if(!lighting)
          glEnable(GL_LIGHTING);
      } else {
        /* we're rendering triangles */
      
        if((alpha!=1.0)||va) {
        
          t_mode  = SettingGet_i(G,I->R.cs->Setting,I->R.obj->Setting,cSetting_transparency_mode);

          if(info && info->alpha_cgo) {
            t_mode = 0; 
          }

          if(t_mode) {
            
            float **t_buf=NULL,**tb;
            float *z_value=NULL,*zv;
            int *ix=NULL;
            int n_tri = 0;
            float sum[3];
            float matrix[16];

            glGetFloatv(GL_MODELVIEW_MATRIX,matrix);

            if(I->oneColorFlag) {
              t_buf = Alloc(float*,I->NT*6);
            } else {
              t_buf = Alloc(float*,I->NT*12);
            }

            z_value = Alloc(float,I->NT);
            ix = Alloc(int,I->NT);

            zv = z_value;
            tb = t_buf;
            c = I->NT;
            if(I->oneColorFlag) {
              while(c--) {       
                if((I->proximity&&((*(vi+(*t)))||(*(vi+(*(t+1))))||(*(vi+(*(t+2))))))||
                   ((*(vi+(*t)))&&(*(vi+(*(t+1))))&&(*(vi+(*(t+2)))))) {

                  *(tb++) = vn+(*t)*3;
                  *(tb++) = v+(*t)*3;
                  *(tb++) = vn+(*(t+1))*3;
                  *(tb++) = v+(*(t+1))*3;
                  *(tb++) = vn+(*(t+2))*3;
                  *(tb++) = v+(*(t+2))*3;
              
                  add3f(tb[-1],tb[-3],sum);
                  add3f(sum,tb[-5],sum);

                  *(zv++) = matrix[2]*sum[0]+matrix[6]*sum[1]+matrix[10]*sum[2];
                  n_tri++;
                }
                t+=3;
              }
            } else {
              while(c--) {
                if((I->proximity&&((*(vi+(*t)))||(*(vi+(*(t+1))))||(*(vi+(*(t+2))))))||
                   ((*(vi+(*t)))&&(*(vi+(*(t+1))))&&(*(vi+(*(t+2))))))
                  if((*(vi+(*t)))||(*(vi+(*(t+1))))||(*(vi+(*(t+2))))) {
                
                    if(va) 
                      *(tb++) = va+(*t);
                    else
                      *(tb++) = &alpha;
                    *(tb++) = vc+(*t)*3;
                    *(tb++) = vn+(*t)*3;
                    *(tb++) = v+(*t)*3;

                    if(va) 
                      *(tb++) = va+(*(t+1));
                    else
                      *(tb++) = &alpha;
                    *(tb++) = vc+(*(t+1))*3;
                    *(tb++) = vn+(*(t+1))*3;
                    *(tb++) = v+(*(t+1))*3;

                    if(va) 
                      *(tb++) = va+(*(t+2));
                    else
                      *(tb++) = &alpha;
                    *(tb++) = vc+(*(t+2))*3;
                    *(tb++) = vn+(*(t+2))*3;
                    *(tb++) = v+(*(t+2))*3;
                
                    add3f(tb[-1],tb[-5],sum);
                    add3f(sum,tb[-9],sum);

                    *(zv++) = matrix[2]*sum[0]+matrix[6]*sum[1]+matrix[10]*sum[2];
                    n_tri++;
                  }
                t+=3;
              }
            }
        
            switch(t_mode) {
            case 1:
              UtilSemiSortFloatIndex(n_tri,z_value,ix,true);
              /* UtilSortIndex(n_tri,z_value,ix,(UtilOrderFn*)ZOrderFn); */
              break;
            default:
              UtilSemiSortFloatIndex(n_tri,z_value,ix,false);
              /* UtilSortIndex(n_tri,z_value,ix,(UtilOrderFn*)ZRevOrderFn); */
              break;
            }

            c=n_tri;
            if(I->oneColorFlag) {
              float col[3];
              ColorGetEncoded(G,I->oneColor,col);
              glColor4f(col[0],col[1],col[2],alpha);
              glBegin(GL_TRIANGLES);
              for(c=0;c<n_tri;c++) {
                
                tb = t_buf+6*ix[c];
            
                glNormal3fv(*(tb++));
                glVertex3fv(*(tb++));
                glNormal3fv(*(tb++));
                glVertex3fv(*(tb++));
                glNormal3fv(*(tb++));
                glVertex3fv(*(tb++));
              }
              glEnd();
            } else {
              glBegin(GL_TRIANGLES);
              for(c=0;c<n_tri;c++) {
                float *vv,*v_alpha;
            
                tb = t_buf+12*ix[c];

                v_alpha = *(tb++);
                vv = *(tb++);
                glColor4f(vv[0],vv[1],vv[2],*v_alpha);
                glNormal3fv(*(tb++));
                glVertex3fv(*(tb++));
            
                v_alpha = *(tb++);
                vv = *(tb++);
                glColor4f(vv[0],vv[1],vv[2],*v_alpha);
                glNormal3fv(*(tb++));
                glVertex3fv(*(tb++));
            
                v_alpha = *(tb++);
                vv = *(tb++);
                glColor4f(vv[0],vv[1],vv[2],*v_alpha);
                glNormal3fv(*(tb++));
                glVertex3fv(*(tb++));
            
              }
              glEnd();
            }
        
            FreeP(ix);
            FreeP(z_value);
            FreeP(t_buf);
          } else { 
            if(info->alpha_cgo) { /* global transparency sort */
              
              if(I->allVisibleFlag) {
                if(I->oneColorFlag) {
                  float col[3];
                  ColorGetEncoded(G,I->oneColor,col);
                  
                  glColor4f(col[0],col[1],col[2],alpha);
                  c=*(s++);
                  while(c) {
                    int parity=0;
                    s+=2;
                    while(c--) {
                      CGOAlphaTriangle(info->alpha_cgo,
                                       v+s[-2]*3, v+s[-1]*3, v+(*s)*3,
                                       vn+s[-2]*3, vn+s[-1]*3, vn+(*s)*3,
                                       col, col, col, 
                                       alpha, alpha, alpha, parity);

                      s++;
                      parity = !parity;
                    }
                    c=*(s++);
                  }
                } else {
                  c=*(s++);
                  while(c) {
                    float *col0,*col1,*col2;
                    int parity=0;
                    float alpha0,alpha1,alpha2;
                    col0 = vc+(*s)*3;
                    if(va) {
                      alpha0=va[(*s)];
                    } else {
                      alpha0 = alpha;
                    }
                    s++;
                    col1 = vc+(*s)*3;
                    if(va) {
                      alpha1 = va[(*s)];
                    } else {
                      alpha1 = alpha;
                    }
                    s++;
                    while(c--) {
                      col2 = vc+(*s)*3;
                      if(va) {
                        alpha2 = va[(*s)];
                      } else {
                        alpha2 = alpha;
                      }
                      CGOAlphaTriangle(info->alpha_cgo,
                                       v+s[-2]*3, v+s[-1]*3, v+(*s)*3,
                                       vn+s[-2]*3, vn+s[-1]*3, vn+(*s)*3,
                                       col0, col1, col2, 
                                       alpha0, alpha1, alpha2, parity);
                      alpha0 = alpha1;
                      alpha1 = alpha2;
                      col0 = col1;
                      col1 = col2;
                      s++;
                      parity = !parity;
                    }
                    c=*(s++);
                  }
                }
              } else { /* subset s */
                c=I->NT;
                if(c) {
                  if(I->oneColorFlag) {
                    float color[3];
                    ColorGetEncoded(G,I->oneColor,color);
                    while(c--) {
                      if((I->proximity&&((*(vi+(*t)))||(*(vi+(*(t+1))))||(*(vi+(*(t+2))))))||
                         ((*(vi+(*t)))&&(*(vi+(*(t+1))))&&(*(vi+(*(t+2)))))) {
                        
                        CGOAlphaTriangle(info->alpha_cgo,
                                         v+t[0]*3, v+t[1]*3, v+t[2]*3,
                                         vn+t[0]*3, vn+t[1]*3, vn+t[2]*3,
                                         color,color,color,
                                         alpha,alpha,alpha,0);
                      }
                      t+=3;
                    }
                  } else {
                    while(c--) {
                      if((I->proximity&&((*(vi+(*t)))||(*(vi+(*(t+1))))||(*(vi+(*(t+2))))))||
                         ((*(vi+(*t)))&&(*(vi+(*(t+1))))&&(*(vi+(*(t+2)))))) {
                        
                        if(va) {
                          CGOAlphaTriangle(info->alpha_cgo,
                                           v+t[0]*3, v+t[1]*3, v+t[2]*3,
                                           vn+t[0]*3, vn+t[1]*3, vn+t[2]*3,
                                           vc+t[0]*3, vc+t[1]*3, vc+t[2]*3,
                                           va[t[0]], va[t[1]], va[t[2]], 0);
                        } else {
                          CGOAlphaTriangle(info->alpha_cgo,
                                           v+t[0]*3, v+t[1]*3, v+t[2]*3,
                                           vn+t[0]*3, vn+t[1]*3, vn+t[2]*3,
                                           vc+t[0]*3, vc+t[1]*3, vc+t[2]*3,
                                           alpha, alpha, alpha, 0);
                        }
                      }
                      t+=3;
                    }
                  }
                  glEnd();
                }
              }
            } else {

              /* fast and ugly */
              /*          glCullFace(GL_BACK);
                          glEnable(GL_CULL_FACE);
                          glDepthMask(GL_FALSE);*/
              if(I->allVisibleFlag) {
                if(I->oneColorFlag) {
                  float col[3];
                  ColorGetEncoded(G,I->oneColor,col);
                  
                  glColor4f(col[0],col[1],col[2],alpha);
                  c=*(s++);
                  while(c) {
                    glBegin(GL_TRIANGLE_STRIP);
                    glNormal3fv(vn+(*s)*3);
                    glVertex3fv(v+(*s)*3);
                    s++;
                    glNormal3fv(vn+(*s)*3);
                    glVertex3fv(v+(*s)*3);
                    s++;
                    while(c--) {
                      glNormal3fv(vn+(*s)*3);
                      glVertex3fv(v+(*s)*3);
                      s++;
                    }
                    glEnd();
                    c=*(s++);
                  }
                } else {
                  c=*(s++);
                  while(c) {
                    float *col;

                    glBegin(GL_TRIANGLE_STRIP);
                    col = vc+(*s)*3;
                    if(va) {
                      glColor4f(col[0],col[1],col[2],va[(*s)]);            
                    } else {
                      glColor4f(col[0],col[1],col[2],alpha);            
                    }
                    glNormal3fv(vn+(*s)*3);
                    glVertex3fv(v+(*s)*3);
                    s++;
                    col = vc+(*s)*3;
                    if(va) {
                      glColor4f(col[0],col[1],col[2],va[(*s)]);            
                    } else {
                      glColor4f(col[0],col[1],col[2],alpha);            
                    }
                    glNormal3fv(vn+(*s)*3);
                    glVertex3fv(v+(*s)*3);
                    s++;
                    while(c--) {
                      col = vc+(*s)*3;
                      if(va) {
                        glColor4f(col[0],col[1],col[2],va[(*s)]);            
                      } else {
                        glColor4f(col[0],col[1],col[2],alpha);            
                      }
                      glNormal3fv(vn+(*s)*3);
                      glVertex3fv(v+(*s)*3);
                      s++;
                    }
                    glEnd();
                    c=*(s++);
                  }
                }
              
              } else { /* subset s*/
                c=I->NT;
                if(c) {
                  glBegin(GL_TRIANGLES);
            
                  if(I->oneColorFlag) {
                    float color[3];
                    float *col;
                    ColorGetEncoded(G,I->oneColor,color);
                  
                    glColor4f(color[0],color[1],color[2],alpha);
                    while(c--) {
                      if((I->proximity&&((*(vi+(*t)))||(*(vi+(*(t+1))))||(*(vi+(*(t+2))))))||
                         ((*(vi+(*t)))&&(*(vi+(*(t+1))))&&(*(vi+(*(t+2)))))) {
                      
                        col = vc+(*t)*3;
                        glNormal3fv(vn+(*t)*3);
                        glVertex3fv(v+(*t)*3);
                        t++;
                        col = vc+(*t)*3;
                        glNormal3fv(vn+(*t)*3);
                        glVertex3fv(v+(*t)*3);
                        t++;
                        col = vc+(*t)*3;
                        glNormal3fv(vn+(*t)*3);
                        glVertex3fv(v+(*t)*3);
                        t++;
                      } else
                        t+=3;
                    }
                  } else {
                    float *col;
                    while(c--) {
                      if((I->proximity&&((*(vi+(*t)))||(*(vi+(*(t+1))))||(*(vi+(*(t+2))))))||
                         ((*(vi+(*t)))&&(*(vi+(*(t+1))))&&(*(vi+(*(t+2)))))) {
                  
                        col = vc+(*t)*3;
                        if(va) {
                          glColor4f(col[0],col[1],col[2],va[(*t)]);            
                        } else {
                          glColor4f(col[0],col[1],col[2],alpha);            
                        }
                        glNormal3fv(vn+(*t)*3);
                        glVertex3fv(v+(*t)*3);
                        t++;
                        col = vc+(*t)*3;
                        if(va) {
                          glColor4f(col[0],col[1],col[2],va[(*t)]);            
                        } else {
                          glColor4f(col[0],col[1],col[2],alpha);            
                        }
                        glNormal3fv(vn+(*t)*3);
                        glVertex3fv(v+(*t)*3);
                        t++;
                        col = vc+(*t)*3;
                        if(va) {
                          glColor4f(col[0],col[1],col[2],va[(*t)]);            
                        } else {
                          glColor4f(col[0],col[1],col[2],alpha);            
                        }
                        glNormal3fv(vn+(*t)*3);
                        glVertex3fv(v+(*t)*3);
                        t++;
                      } else
                        t+=3;
                    }
                  }
                  glEnd();
                }
              }
            }
            /*          glDisable(GL_CULL_FACE);
                        glDepthMask(GL_TRUE);*/
          }
        } else { /* opaque */

          int use_dlst,simplify=0;
          use_dlst = (int)SettingGet(G,cSetting_use_display_lists);
          simplify = (int)SettingGet(G,cSetting_simplify_display_lists);
          if(use_dlst&&I->R.displayList) {
            glCallList(I->R.displayList);
          } else { 
          
            if(use_dlst) {
              if(!I->R.displayList) {
                I->R.displayList = glGenLists(1);
                if(I->R.displayList) {
                  glNewList(I->R.displayList,GL_COMPILE_AND_EXECUTE);
                }
              }
            }

            if(I->allVisibleFlag) {
              if(I->oneColorFlag) {
                if(use_dlst&&simplify) { /* simplify: try to help display list optimizer */
                  glColor3fv(ColorGet(G,I->oneColor));
                  c=*(s++);
                  while(c) {
                    glBegin(GL_TRIANGLES); 
                    s+=2;
                    while(c--)
                      {
                        s-=2;
                        glNormal3fv(vn+(*s)*3);
                        glVertex3fv(v+(*s)*3);
                        s++;
                        glNormal3fv(vn+(*s)*3);
                        glVertex3fv(v+(*s)*3);
                        s++;
                        glNormal3fv(vn+(*s)*3);
                        glVertex3fv(v+(*s)*3);
                        s++;
                      }
                    glEnd();
                    c=*(s++);
                  }
                } else {
                  glColor3fv(ColorGet(G,I->oneColor));
                  c=*(s++);
                  while(c) {
                    glBegin(GL_TRIANGLE_STRIP);
                    glNormal3fv(vn+(*s)*3);
                    glVertex3fv(v+(*s)*3);
                    s++;
                    glNormal3fv(vn+(*s)*3);
                    glVertex3fv(v+(*s)*3);
                    s++;
                    while(c--)
                      {
                        glNormal3fv(vn+(*s)*3);
                        glVertex3fv(v+(*s)*3);
                        s++;
                      }
                    glEnd();
                    c=*(s++);
                  }
                } /* use_dlst&&simplify */
              } else { /* not one color */
                if(use_dlst&&simplify) {  /* simplify: try to help display list optimizer */
                  c=*(s++);
                  while(c) {
                    glBegin(GL_TRIANGLES);
                    s+=2;
                    while(c--)
                      {
                        s-=2;
                        glColor3fv(vc+(*s)*3);
                        glNormal3fv(vn+(*s)*3);
                        glVertex3fv(v+(*s)*3);
                        s++;
                        glColor3fv(vc+(*s)*3);
                        glNormal3fv(vn+(*s)*3);
                        glVertex3fv(v+(*s)*3);
                        s++;
                        glColor3fv(vc+(*s)*3);
                        glNormal3fv(vn+(*s)*3);
                        glVertex3fv(v+(*s)*3);
                        s++;
                      }
                    glEnd();
                    c=*(s++);
                  }
                } else {
                  c=*(s++);
                  while(c) {
                    glBegin(GL_TRIANGLE_STRIP);
                    glColor3fv(vc+(*s)*3);
                    glNormal3fv(vn+(*s)*3);
                    glVertex3fv(v+(*s)*3);
                    s++;
                    glColor3fv(vc+(*s)*3);
                    glNormal3fv(vn+(*s)*3);
                    glVertex3fv(v+(*s)*3);
                    s++;
                    while(c--)
                      {
                        glColor3fv(vc+(*s)*3);
                        glNormal3fv(vn+(*s)*3);
                        glVertex3fv(v+(*s)*3);
                        s++;
                      }
                    glEnd();
                    c=*(s++);
                  }
                }
              } /* one color */
            } else { /* subsets */
              c=I->NT;
              if(c) {
                glBegin(GL_TRIANGLES);
                if(I->oneColorFlag) {
                  glColor3fv(ColorGet(G,I->oneColor));
                  while(c--) {
                    if((I->proximity&&((*(vi+(*t)))||(*(vi+(*(t+1))))||(*(vi+(*(t+2))))))||
                       ((*(vi+(*t)))&&(*(vi+(*(t+1))))&&(*(vi+(*(t+2)))))) {
                    
                      glNormal3fv(vn+(*t)*3);
                      glVertex3fv(v+(*t)*3);
                      t++;
                      glNormal3fv(vn+(*t)*3);
                      glVertex3fv(v+(*t)*3);
                      t++;
                      glNormal3fv(vn+(*t)*3);
                      glVertex3fv(v+(*t)*3);
                      t++;
                    } else
                      t+=3;
                  }
                } else {
                  while(c--) {
                    if((I->proximity&&((*(vi+(*t)))||(*(vi+(*(t+1))))||(*(vi+(*(t+2))))))||
                       ((*(vi+(*t)))&&(*(vi+(*(t+1))))&&(*(vi+(*(t+2)))))) {
                    
                      glColor3fv(vc+(*t)*3);
                      glNormal3fv(vn+(*t)*3);
                      glVertex3fv(v+(*t)*3);
                      t++;
                      glColor3fv(vc+(*t)*3);
                      glNormal3fv(vn+(*t)*3);
                      glVertex3fv(v+(*t)*3);
                      t++;
                      glColor3fv(vc+(*t)*3);
                      glNormal3fv(vn+(*t)*3);
                      glVertex3fv(v+(*t)*3);
                      t++;
                    } else
                      t+=3;
                  }
                }
                glEnd();
              }
            }
            if(use_dlst&&I->R.displayList) {
              glEndList();
            }
          }
        }
      }
      if(SettingGet(G,cSetting_surface_debug)) {
        t=I->T;
        c=I->NT;
        if(c) {
          glBegin(GL_TRIANGLES);
          while(c--) {

            if(I->allVisibleFlag||((I->proximity&&((*(vi+(*t)))||(*(vi+(*(t+1))))||(*(vi+(*(t+2))))))||
                                   ((*(vi+(*t)))&&(*(vi+(*(t+1))))&&(*(vi+(*(t+2))))))) {
            
              glNormal3fv(vn+(*t)*3);
              glVertex3fv(v+(*t)*3);
              t++;
              glNormal3fv(vn+(*t)*3);
              glVertex3fv(v+(*t)*3);
              t++;
              glNormal3fv(vn+(*t)*3);
              glVertex3fv(v+(*t)*3);
              t++;
            } else {
              t+=3;
            }
          }
          glEnd();
        }
      
        t=I->T;
        c=I->NT;
        if(c) {
          glColor3f(0.0,1.0,0.0);
          glLineWidth(1.0F);
          while(c--)
            {
              glBegin(GL_LINE_STRIP);
            
              if(I->allVisibleFlag||((I->proximity&&((*(vi+(*t)))||(*(vi+(*(t+1))))||(*(vi+(*(t+2))))))||
                                     ((*(vi+(*t)))&&(*(vi+(*(t+1))))&&(*(vi+(*(t+2))))))) {
              
              
                glNormal3fv(vn+(*t)*3);
                glVertex3fv(v+(*t)*3);
                t++;
                glNormal3fv(vn+(*t)*3);
                glVertex3fv(v+(*t)*3);
                t++;
                glNormal3fv(vn+(*t)*3);
                glVertex3fv(v+(*t)*3);
                t++;
              } else {
                t+=3;
              }
              glEnd();
            }
        }
        c=I->N;
        if(c) {
          glColor3f(1.0,0.0,0.0);
          glBegin(GL_LINES);
          SceneResetNormal(G,true);
          while(c--)
            {
              glVertex3fv(v);
              glVertex3f(v[0]+vn[0]/2,v[1]+vn[1]/2,v[2]+vn[2]/2);
              v+=3;
              vn+=3;
            }
          glEnd();
        }
      }
    }
  }
}


int RepSurfaceSameVis(RepSurface *I,CoordSet *cs)
{
  int same = true;
  int *lv,*lc,*cc;
  int a;
  AtomInfoType *ai;

  ai = cs->Obj->AtomInfo;
  lv = I->LastVisib;
  lc = I->LastColor;
  cc = cs->Color;

  for(a=0;a<cs->NIndex;a++)
    {
      if(*(lv++)!=(ai + cs->IdxToAtm[a])->visRep[cRepSurface] ) {
        same=false;
        break;
      }
      if(*(lc++)!=*(cc++)) {
        same=false;
        break;
      }
    }
  return(same);
}

void RepSurfaceColor(RepSurface *I,CoordSet *cs)
{
  PyMOLGlobals *G=cs->State.G;
  MapType *map;
  int a,i0,i,j,c1;
  float *v0,*vc,*c0,*va;
  float *n0;
  int *vi,*lv,*lc,*cc;
  int first_color;
  float *v_pos,v_above[3];
  int ramp_above;
  ObjectMolecule *obj;
  float probe_radius;
  float dist,minDist;
  float cutoff;
  int inclH;
  int cullByFlag = false;
  int surface_mode;
  int surface_color;
  int *present=NULL,*ap;
  int *rc;
  int ramped_flag = false;
  int carve_state = 0;
  int carve_flag = false;
  float carve_cutoff;
  float carve_normal_cutoff;
  int carve_normal_flag;
  char *carve_selection = NULL;
  float *carve_vla = NULL;
  MapType *carve_map = NULL;

  int clear_state = 0;
  int clear_flag = false;
  float clear_cutoff;
  char *clear_selection = NULL;
  float *clear_vla = NULL;
  int state = I->R.context.state;
  float transp;
  int variable_alpha = false;

  MapType *clear_map = NULL;

  AtomInfoType *ai2=NULL,*ai1;

  obj=cs->Obj;
  surface_mode = SettingGet_i(G,cs->Setting,obj->Obj.Setting,cSetting_surface_mode);
  ramp_above  = SettingGet_i(G,cs->Setting,obj->Obj.Setting,cSetting_surface_ramp_above_mode);
  surface_color = SettingGet_color(G,cs->Setting,obj->Obj.Setting,cSetting_surface_color);
  cullByFlag = (surface_mode==cRepSurface_by_flags);
  inclH = !(surface_mode==cRepSurface_heavy_atoms);
  probe_radius = SettingGet_f(G,cs->Setting,obj->Obj.Setting,cSetting_solvent_radius);
  I->proximity = SettingGet_b(G,cs->Setting,obj->Obj.Setting,cSetting_surface_proximity);
  carve_cutoff = SettingGet_f(G,cs->Setting,obj->Obj.Setting,cSetting_surface_carve_cutoff);
  clear_cutoff = SettingGet_f(G,cs->Setting,obj->Obj.Setting,cSetting_surface_clear_cutoff);
  transp = SettingGet_f(G,cs->Setting,obj->Obj.Setting,cSetting_transparency);
  carve_normal_cutoff = SettingGet_f(G,cs->Setting,obj->Obj.Setting,cSetting_surface_carve_normal_cutoff);
  carve_normal_flag = carve_normal_cutoff>(-1.0F);
  
  cutoff = I->max_vdw+2*probe_radius;
  
  if(!I->LastVisib) I->LastVisib = Alloc(int,cs->NIndex);
  if(!I->LastColor) I->LastColor = Alloc(int,cs->NIndex);
  lv = I->LastVisib;
  lc = I->LastColor;
  cc = cs->Color;
  ai2=obj->AtomInfo;
  for(a=0;a<cs->NIndex;a++) {
    *(lv++) = (ai2 + cs->IdxToAtm[a])->visRep[cRepSurface];
    *(lc++) = *(cc++);
  }
  
  if(I->N) {
    if(carve_cutoff>0.0F) {
      carve_state = SettingGet_i(G,cs->Setting,obj->Obj.Setting,cSetting_surface_carve_state) - 1;
      carve_selection = SettingGet_s(G,cs->Setting,obj->Obj.Setting,cSetting_surface_carve_selection);
      if(carve_selection) 
        carve_map = SelectorGetSpacialMapFromSeleCoord(G,
                                                       SelectorIndexByName(G,carve_selection),
                                                       carve_state,
                                                       carve_cutoff,&carve_vla);
      if(carve_map) 
        MapSetupExpress(carve_map);
      carve_flag = true;
    }

    if(clear_cutoff>0.0F) {
      clear_state = SettingGet_i(G,cs->Setting,obj->Obj.Setting,cSetting_surface_clear_state) - 1;
      clear_selection = SettingGet_s(G,cs->Setting,obj->Obj.Setting,cSetting_surface_clear_selection);
      if(clear_selection) 
        clear_map = SelectorGetSpacialMapFromSeleCoord(G,
                                                       SelectorIndexByName(G,clear_selection),
                                                       clear_state,
                                                       clear_cutoff,&clear_vla);
      if(clear_map) 
        MapSetupExpress(clear_map);
      clear_flag = true;
    }

    if(!I->VC) I->VC = Alloc(float,3*I->N);
    vc=I->VC;
    if(!I->VA) I->VA = Alloc(float,I->N);
    va=I->VA;
    if(!I->RC) I->RC = Alloc(int,I->N);
    rc=I->RC;
    if(!I->Vis) I->Vis = Alloc(int,I->N);
    vi=I->Vis;
    if(ColorCheckRamped(G,surface_color)) {
      I->oneColorFlag=false;
    } else {
      I->oneColorFlag=true;
    }
    first_color=-1;

    present = Alloc(int,cs->NIndex); 
    ap = present;
    for(a=0;a<cs->NIndex;a++) {
      ai1 = obj->AtomInfo + cs->IdxToAtm[a];
      if(ai1->visRep[cRepSurface]&&
         (inclH||(!ai1->hydrogen))&&
         ((!cullByFlag)||
          (!(ai1->flags&(cAtomFlag_ignore|cAtomFlag_exfoliate)))))
        *ap = 2; 
      else 
        *ap = 0;
      ap++;
    }
    
    map=MapNewFlagged(G,2*I->max_vdw+probe_radius,cs->Coord,cs->NIndex,NULL,present);
    MapSetupExpress(map);

    {
      float probe_radiusX2 = probe_radius*2;    
      for(a=0;a<cs->NIndex;a++)
        if(!present[a]) {
          ai1 = obj->AtomInfo+cs->IdxToAtm[a];
          if((!cullByFlag)||!(ai1->flags&cAtomFlag_ignore)) {
            v0 = cs->Coord+3*a;
            i=*(MapLocusEStart(map,v0));
            if(i) {
              j=map->EList[i++];
              while(j>=0) {
                if(present[j]>1) {
                  ai2 = obj->AtomInfo+cs->IdxToAtm[j];                  
                  if(within3f(cs->Coord+3*j,v0,ai1->vdw+ai2->vdw+probe_radiusX2)) {
                    present[a]=1;
                    break;
                  }
                }
                j=map->EList[i++];
              }
            }
          }
        }
    }
    MapFree(map);
    map = NULL;

    /* now, assign colors to each point */
    map=MapNewFlagged(G,cutoff,cs->Coord,cs->NIndex,NULL,present);
    if(map) {
      MapSetupExpress(map);
      for(a=0;a<I->N;a++) {
        float at_transp = transp;

        AtomInfoType *ai0 = NULL;
        c1=1;
        minDist=MAXFLOAT;
        i0=-1;
        v0 = I->V+3*a;
        n0 = I->VN+3*a;
        vi = I->Vis+a;
        /* colors */
        i=*(MapLocusEStart(map,v0));
        if(i) {
          j=map->EList[i++];
          while(j>=0) {
            ai2 = obj->AtomInfo + cs->IdxToAtm[j];
            if((inclH||(!ai2->hydrogen))&&
               ((!cullByFlag)||
                (!(ai2->flags&cAtomFlag_ignore)))) {
              dist = (float)diff3f(v0,cs->Coord+j*3)-ai2->vdw;
              if(dist<minDist) {
                i0=j;
                ai0=ai2;
                minDist=dist;
              }
            }
            j=map->EList[i++];
          }
        }
        if(i0>=0) {
          int at_surface_color;
          transp = SettingGet_f(G,cs->Setting,obj->Obj.Setting,cSetting_transparency);

          AtomInfoGetSetting_f(G, ai0, cSetting_transparency, transp, &at_transp);

          AtomInfoGetSetting_color(G, ai0, cSetting_surface_color, 
                                   surface_color, &at_surface_color);
          
          if(at_surface_color!=-1) {
            c1 = at_surface_color;
          } else {
            c1=*(cs->Color+i0);
          }
            
          if(I->oneColorFlag) {
            if(first_color>=0) {
              if(first_color!=c1)
                I->oneColorFlag=false;
            } else first_color=c1;
          }
          if(I->allVisibleFlag)
            *vi = 1;
          else {
            ai2 = obj->AtomInfo+cs->IdxToAtm[i0];                
            if(ai2->visRep[cRepSurface]&&
               (inclH||(!ai2->hydrogen))&&
               ((!cullByFlag)||
                (!(ai2->flags&(cAtomFlag_ignore|cAtomFlag_exfoliate)))))
              *vi = 1;
            else
              *vi = 0;
          }
        } else {
          *vi = 0;
        }
        if(carve_flag && (*vi)) { /* is point visible, and are we carving? */
          *vi = 0;
          
          if(carve_map) {
            
            minDist=MAXFLOAT;                
            i=*(MapLocusEStart(carve_map,v0));
            if(i) {
              j=carve_map->EList[i++];
              while(j>=0) {
                register float *v_targ = carve_vla+3*j;
                if(within3f(v_targ,v0,carve_cutoff)) {
                  if(!carve_normal_flag) {
                    *vi=1;
                    break;
                  } else {
                    float v_to[3];
                    subtract3f(v_targ,v0,v_to);
                    if(dot_product3f(v_to,n0)>=carve_normal_cutoff) {
                      *vi = 1;
                      break;
                    }
                  }
                }
                j=carve_map->EList[i++];
              }
            }
          }
        }
        if(clear_flag && (*vi)) { /* is point visible, and are we clearing? */
          if(clear_map) {
            
            minDist=MAXFLOAT;                
            i=*(MapLocusEStart(clear_map,v0));
            if(i) {
              j=clear_map->EList[i++];
              while(j>=0) {
                if(within3f(clear_vla+3*j,v0,clear_cutoff)) {
                  *vi=0;
                  break;
                }
                j=clear_map->EList[i++];
              }
            }
          }
        }
        /*
          if(ColorCheckRamped(G,surface_color)) {
          c1 = surface_color;
          }
        */

        if(ColorCheckRamped(G,c1)) {
          I->oneColorFlag=false;
          switch(ramp_above) {
          case 1:
            copy3f(n0,v_above);
            scale3f(v_above,probe_radius,v_above);
            add3f(v0,v_above,v_above);
            v_pos = v_above;
            rc[0] = -1;
            break;
          default:
            v_pos = v0;
            rc[0] = c1;
            ramped_flag=true;
            break;
          }
          ColorGetRamped(G,c1,v_pos,vc,state);
          vc+=3;
          rc++;
        } else {
          c0 = ColorGet(G,c1);
          *(rc++) = c1;
          *(vc++) = *(c0++);
          *(vc++) = *(c0++);
          *(vc++) = *(c0++);
        }
        if(at_transp!=transp)
          variable_alpha = true;
        *(va++) = 1.0F - at_transp;

        if(!*vi)
          I->allVisibleFlag=false;
        vi++;
      }
      MapFree(map);
    }

    if(variable_alpha)
      I->oneColorFlag = false;

    if(I->oneColorFlag) {
      I->oneColor=first_color;
    }
  }
  /*
  if(surface_color>=0) {
    I->oneColorFlag=true;
    I->oneColor=surface_color;
  }
  */

  if(G->HaveGUI) {
    if(I->R.displayList) {
      if(PIsGlutThread()) {
        if(G->ValidContext) {
          glDeleteLists(I->R.displayList,1);
          I->R.displayList = 0;
        }
      } else {
        char buffer[255]; /* pass this off to the main thread */
        sprintf(buffer,"_cmd.gl_delete_lists(cmd._COb,%d,%d)\n",I->R.displayList,1);
        PParse(G,buffer);
      }
    }
  }

  if(I->VA&&(!variable_alpha)) {
    FreeP(I->VA);
    I->VA = NULL;
  }
     
  if(carve_map)
    MapFree(carve_map);
  VLAFreeP(carve_vla);
  if(clear_map)
    MapFree(clear_map);
  VLAFreeP(clear_vla);
  if( (!ramped_flag) || (!SettingGet_b(G,cs->Setting,obj->Obj.Setting,cSetting_ray_color_ramps)))
    FreeP(I->RC);
  FreeP(present);
}


typedef struct {
  /* input */

  float *coord;
  SurfaceJobAtomInfo *atomInfo;

  float maxVdw;
  int allVisibleFlag;

  int nPresent;
  int *presentVla;

  int solventSphereIndex, sphereIndex;

  int surfaceType;
  int circumscribe;
  float probeRadius, probeRadMore, probeRadLess, probeRadLess2;
  float carveCutoff;
  float *carveVla;
  
  int surfaceMode;
  int surfaceSolvent;
  int cavityCull;
  float pointSep;
  float trimCutoff;
  float trimFactor;

  /* results */
  float *V, *VN;
  int N;

} SurfaceJob;

static SurfaceJob *SurfaceJobNew(PyMOLGlobals *G)
{
  OOCalloc(G, SurfaceJob);
  return I;
}

static void SurfaceJobPurgeResult(PyMOLGlobals *G, SurfaceJob *I)
{
  FreeP(I->V); I->V = NULL;
  FreeP(I->VN); I->VN = NULL;
}

static void SurfaceJobFree(PyMOLGlobals *G, SurfaceJob *I)
{
  SurfaceJobPurgeResult(G,I);
  VLAFreeP(I->coord);
  VLAFreeP(I->presentVla);
  VLAFreeP(I->atomInfo);
  VLAFreeP(I->carveVla);
  OOFreeP(I);
}

static int SurfaceJobRun(PyMOLGlobals *G, SurfaceJob *I) 
{
  int ok = true;
  int MaxN;
  int n_index = VLAGetSize(I->atomInfo);
  int n_present = I->nPresent;
  SphereRec *sp = G->Sphere->Sphere[I->sphereIndex];
  SphereRec *ssp = G->Sphere->Sphere[I->solventSphereIndex];

  SurfaceJobPurgeResult(G,I);

  {
    /* compute limiting storage requirements */
    int tmp = n_present;
    if(tmp<1) tmp=1;
    if(sp->nDot<ssp->nDot)
      MaxN = tmp*ssp->nDot;
    else
      MaxN = tmp*sp->nDot;
  }

  I->V=Alloc(float,(MaxN+1)*3);
  I->VN=Alloc(float,(MaxN+1)*3);

  if(!(I->V&&I->VN)) { /* bail out point -- try to reduce crashes */
    PRINTFB(G,FB_RepSurface,FB_Errors)
      "Error-RepSurface: insufficient memory to calculate surface at this quality.\n"
      ENDFB(G);
    FreeP(I->V); I->V = NULL;
    FreeP(I->VN); I->VN = NULL;
    ok = false;
  } else {
    SolventDot *sol_dot = NULL;
    float *v=I->V;
    float *vn=I->VN; 
    MapType *carve_map = NULL;
    float probe_radius = I->probeRadius;
    int circumscribe = I->circumscribe;
    int surface_type = I->surfaceType;
    float point_sep = I->pointSep;
    float *I_coord = I->coord;
    int *present_vla = I->presentVla;
    SurfaceJobAtomInfo *I_atom_info = I->atomInfo;
      
    I->N=0;
    
    sol_dot = SolventDotNew(G,I->coord, I->atomInfo, probe_radius,
                            ssp, present_vla,
                            circumscribe, I->surfaceMode, I->surfaceSolvent,
                            I->cavityCull, I->allVisibleFlag, I->maxVdw);
    
    if(!I->surfaceSolvent) {
      
      float probe_rad_more = I->probeRadMore;
      float probe_rad_less = I->probeRadLess;
      float probe_rad_less2 = I->probeRadLess2;
      
      if(surface_type>=5) { /* effectively double-weights atom points */
        if(sol_dot->nDot) { 
          int a;
          float *v0 = sol_dot->dot;
          float *n0 = sol_dot->dotNormal;
          for(a=0;a<sol_dot->nDot;a++) {
            scale3f(n0,-probe_radius,v);
            add3f(v0,v,v);
            copy3f(n0,vn);
            v+=3;
            vn+=3;
            n0+=3;
            v0+=3;
            I->N++;
          }
        }
      }

      {
        MapType *map,*solv_map;
        map=MapNewFlagged(G, I->maxVdw+I->probeRadMore,
                          I->coord, VLAGetSize(I->coord)/3, NULL, NULL);
      
        solv_map=MapNew(G, I->probeRadLess, sol_dot->dot, 
                        sol_dot->nDot, NULL);
        if(map&&solv_map) {

          MapSetupExpress(solv_map);
          MapSetupExpress(map);
          
          if(sol_dot->nDot) {
            int *dc = sol_dot->dotCode;
            Vector3f *dot = Alloc(Vector3f,sp->nDot);
            int a,b;
            float *v0,*n0;

            for(b=0;b<sp->nDot;b++) {
              scale3f(sp->dot[b],probe_radius,dot[b]);
            }
            v0 = sol_dot->dot;
            n0 = sol_dot->dotNormal;
            for(a=0;a<sol_dot->nDot;a++) {
              if(dc[a]||(surface_type<6)) { /* surface type 6 is completely scribed */
                OrthoBusyFast(G,a+sol_dot->nDot*2,sol_dot->nDot*5); /* 2/5 to 3/5 */
                for(b=0;b<sp->nDot;b++) {
                  register int ii;
                  int flag = true;
                  v[0]=v0[0]+dot[b][0];
                  v[1]=v0[1]+dot[b][1];
                  v[2]=v0[2]+dot[b][2];
                  ii=*(MapLocusEStart(solv_map,v));
                  if(ii) {
                    register int jj;
                    register int *elist = solv_map->EList;
                    register float *i_dot = sol_dot->dot;
                    register float v_0=v[0], v_1=v[1], v_2=v[2];
                    register float dist=probe_rad_less;
                    register float dist2=probe_rad_less2;
                    register float *v1,dx,dy,dz;
                    jj=elist[ii++];
                    v1 = i_dot + 3*jj;                          
                    while(jj>=0) {
                      if(jj!=a)  { /*&&!(dc[jj]))*/
                        /* huge bottleneck -- optimized for superscaler processors */
                        dx = v1[0]-v_0;
                        dy = v1[1]-v_1;
                        dz = v1[2]-v_2;
                        dx = (float)fabs(dx);
                        dy = (float)fabs(dy);
                        if(!(dx>dist)) {
                          dx = dx * dx;
                          if(!(dy>dist)) {
                            dz = (float)fabs(dz);
                            dy = dy * dy;
                            if(!(dz>dist)) {
                              dx = dx + dy;
                              dz = dz * dz;
                              if(!(dx>dist2)) 
                                if((dx + dz)<=dist2) {
                                  flag = false; 
                                  break; 
                                }
                            }
                          }
                        }
                      }
                      jj=elist[ii++];
                      v1 = i_dot + 3*jj;                          
                    }
                  }
                  /* at this point, we have points on the interior of the solvent surface,
                     so now we need to further trim that surface to cover atoms that are present */
                  
                  if(flag) {
                    register int i=*(MapLocusEStart(map,v));
                    if(i) {
                      register int j=map->EList[i++];
                      while(j>=0) {
                        SurfaceJobAtomInfo *atom_info = I_atom_info + j;
                        if((!present_vla)||present_vla[j]) {
                          if(within3f(I_coord+3*j,v,atom_info->vdw+probe_rad_more)) {
                            flag=false;
                            break;
                          }
                        }
                        j=map->EList[i++];
                      }
                    }
                    if(!flag) { /* compute the normals */
                      vn[0]=-sp->dot[b][0];
                      vn[1]=-sp->dot[b][1];
                      vn[2]=-sp->dot[b][2];
                      if(I->N<MaxN) {
                        I->N++;
                        v+=3;
                        vn+=3;
                      } else {
                        int v_offset = v-I->V;
                        int vn_offset = vn-I->VN;
                        MaxN = MaxN*2;
                        I->V=Realloc(I->V,float,(MaxN+1)*3);
                        I->VN=Realloc(I->VN,float,(MaxN+1)*3);
                        v = I->V + v_offset;
                        vn = I->VN + vn_offset;
                      }
                    }
                  }
                }
              }
              v0 +=3;
              n0 +=3;
            }
            FreeP(dot);
          }
        }
        MapFree(solv_map);
        MapFree(map);
      }
    } else {
      float *v0 = sol_dot->dot;
      float *n0 = sol_dot->dotNormal;
      int a;
      if(sol_dot->nDot) {
        for(a=0;a<sol_dot->nDot;a++) {
          *(v++)=*(v0++);
          *(vn++)=*(n0++);
          *(v++)=*(v0++);
          *(vn++)=*(n0++);
          *(v++)=*(v0++);
          *(vn++)=*(n0++);
          I->N++;
        }
      }
    }
    
    SolventDotFree(sol_dot); sol_dot = NULL;
    
    {
      int refine,ref_count = 1;
      
      if((surface_type==0) && (circumscribe)) {
        ref_count = 2; /* these constants need more tuning...*/
      }
      
      for(refine=0;refine<ref_count;refine++) {
        
        /* add new vertices in regions where curvature is very high
           or where there are gaps with no points */
          
        if(I->N && (surface_type==0) && (circumscribe)) {
          int n_new = 0;
            
          float neighborhood = 2.6*point_sep; /* these constants need more tuning...*/
          float dot_cutoff = 0.666;
          float insert_cutoff = 1.1*point_sep;
            
          float map_cutoff = neighborhood;
          float *new_dot = VLAlloc(float,1000);

          if(map_cutoff<(2.9*point_sep)) { /* these constants need more tuning...*/
            map_cutoff = 2.9*point_sep;
          }

          {
            MapType *map = NULL;
            int a;
            map=MapNew(G,map_cutoff,I->V,I->N,NULL);
            MapSetupExpress(map);		  
            v=I->V;
            vn=I->VN;
            for(a=0;a<I->N;a++) {
              register int i=*(MapLocusEStart(map,v));
              if(i) {
                register int j=map->EList[i++];
                while(j>=0) {
                  if(j>a) {
                    float *v0 = I->V+3*j;
                    if(within3f(v0,v,map_cutoff)) {
                      int add_new = false;
                      float *n0 = I->VN + 3*j;
                      VLACheck(new_dot,float,n_new*6+5);
                      {
                        float *v1 = new_dot+n_new*6;
                        average3f(v,v0,v1);
                        if((dot_product3f(n0,vn)<dot_cutoff)&&(within3f(v0,v,neighborhood)))
                          add_new = true;
                        else {
                          /* if points are too far apart, insert a new one */
                          register int ii=*(MapLocusEStart(map,v1));
                          if(ii) {
                            int found=false;
                            register int jj=map->EList[ii++];
                            while(jj>=0) {
                              if(jj!=j) {
                                float *vv0 = I->V+3*jj;
                                if(within3f(vv0,v1,insert_cutoff)) {
                                  found = true;
                                  break;
                                }
                              }
                              jj=map->EList[ii++];
                            }
                            if(!found) add_new =true;
                          }
                        }
                        if(add_new) {
                          /* highly divergent */
                          float *n1 = v1+3;
                          n_new ++;
                          average3f(vn,n0,n1);
                          normalize3f(n1);
                        }
                      }
                    }
                  }
                  j=map->EList[i++];
                }
              }
              v+=3;
              vn+=3;
            }
            MapFree(map);
          }
          if(n_new) {
            float *n1 = new_dot+3;
            float *v1 = new_dot;
            I->V = Realloc(I->V,float,3*(I->N+n_new));
            I->VN = Realloc(I->VN,float,3*(I->N+n_new));
            v = I->V + 3*I->N;
            vn = I->VN + 3*I->N;
            I->N += n_new;
            while(n_new--) {
              copy3f(v1,v);
              copy3f(n1,vn);
              v+=3;
              vn+=3;
              v1+=6;
              n1+=6;
            }
          }
          VLAFreeP(new_dot);
        }

        if(I->N && (surface_type==0) && (circumscribe)) {
            
          float cutoff = 0.5*probe_radius;
            
          /* combine scribing with an atom proximity cleanup pass */
            
          int *dot_flag=Calloc(int,I->N);
          MapType *map = MapNewFlagged(G,I->maxVdw+probe_radius,I_coord,n_index,NULL,present_vla);
          int a;
          MapSetupExpress(map); 
          v=I->V;
          for(a=0;a<I->N;a++) {
            register int i=*(MapLocusEStart(map,v));
            if(i) {
              register int j=map->EList[i++];
              while(j>=0) {
                SurfaceJobAtomInfo *atom_info = I_atom_info + j;
                 if((!present_vla)||present_vla[j]) {
                   if(within3f(I_coord+3*j,v,atom_info->vdw+cutoff)) { 
                     dot_flag[a] = true;
                   }
                 }
                j=map->EList[i++];
              }
            }
            v+=3;
          }
            
          MapFree(map);
          map = NULL;

          {
            /* purge unused dots */
              
            float *v0 = I->V;
            float *vn0 = I->VN;
            int *p = dot_flag;
            int c=I->N;
            int a;
            v=I->V;
            vn=I->VN;
            I->N=0;
            for(a=0;a<c;a++) {
              if(*(p++)) {
                *(v0++)=*(v++);
                *(v0++)=*(v++);
                *(v0++)=*(v++);
                *(vn0++)=*(vn++);
                *(vn0++)=*(vn++);
                *(vn0++)=*(vn++);
                I->N++;
              } else {
                v+=3;
                vn+=3;
              }
            }
          }
          FreeP(dot_flag);
        }

        /* now, eliminate dots that are too close to each other*/
          
        /*    CGOColor(I->debug,0.0,1.0,0.0);
              CGOBegin(I->debug,GL_POINTS);
              for(a=0;a<I->N;a++)
              CGOVertexv(I->debug,I->V+3*a);
              CGOEnd(I->debug);
        */
                    
        if(I->N) {
          int repeat_flag=true;
          float min_dot = 0.1F;
          int *dot_flag = Alloc(int,I->N);
          while(repeat_flag) {
            repeat_flag = false;
              
            if(surface_type>=3) { 
              register int jj;
              float dist;
              register float nearest;
              register float min_sep2 = point_sep*point_sep;
              float diff[3];
              {
                int a;
                for(a=0;a<I->N;a++) dot_flag[a]=1;
              }
              {
                MapType *map = MapNew(G,point_sep+0.05F,I->V,I->N,NULL);
                int a;
                MapSetupExpress(map);		  
                v=I->V;
                vn=I->VN;
                for(a=0;a<I->N;a++) {
                  if(dot_flag[a]) {
                    register int i=*(MapLocusEStart(map,v));
                    if(i) {
                      register int j=map->EList[i++];
                      jj=I->N;
                      nearest = point_sep+1.0F;
                      while(j>=0) {
                        if(j>a) {
                          if(dot_flag[j]) {
                            if(dot_product3f(I->VN+(3*j),vn)>min_dot) {
                              if(within3fret(I->V+(3*j),v,point_sep,min_sep2,diff,&dist)) {
                                repeat_flag = true;
                                if(dist<nearest) { 
                                  /* try to be as determinstic as possible
                                     in terms of how we collapse points */
                                  jj = j;
                                  nearest = dist;
                                } else if((j<jj)&&(fabs(dist-nearest)<R_SMALL4)) { 
                                  jj = j;
                                  nearest = dist;
                                }
                              }
                            }
                          }
                        }
                        j=map->EList[i++];
                      }
                  
                      if(jj<I->N) {
                        dot_flag[jj]=0;
                        add3f(vn,I->VN+(3*jj),vn);
                        average3f(I->V+(3*jj),v,v);
                        repeat_flag=true;
                      }
                    }
                  }
                  v+=3;
                  vn+=3;
                }
                MapFree(map);
              }
            } else { /* surface types < 3 */
              int a;
              MapType *map = MapNew(G,-point_sep,I->V,I->N,NULL);
              for(a=0;a<I->N;a++) dot_flag[a]=1;
              MapSetupExpress(map);		  
              v=I->V;
              vn=I->VN;
              for(a=0;a<I->N;a++) {
                if(dot_flag[a]) {
                  register int i=*(MapLocusEStart(map,v));
                  if(i) {
                    register int j=map->EList[i++];
                    while(j>=0) {
                      if(j!=a) 
                        {
                          if(dot_flag[j]) {
                            if(within3f(I->V+(3*j),v,point_sep)) {
                              dot_flag[j]=0;
                              add3f(vn,I->VN+(3*j),vn);
                              average3f(I->V+(3*j),v,v);
                              repeat_flag=true;
                            }
                          }
                        }
                      j=map->EList[i++];
                    }
                  }
                }
                v+=3;
                vn+=3;
              }
              MapFree(map);
            }
            {
              float *v0=I->V;
              float *vn0=I->VN;
              int *p=dot_flag;
              int c=I->N;
              int a;
              v=I->V;
              vn=I->VN;
              I->N=0;
              for(a=0;a<c;a++) {
                if(*(p++)) {
                  *(v0++)=*(v++);
                  *(v0++)=*(v++);
                  *(v0++)=*(v++);
                  normalize3f(vn);
                  *(vn0++)=*(vn++);
                  *(vn0++)=*(vn++);
                  *(vn0++)=*(vn++);
                  I->N++;
                } else {
                  v+=3;
                  vn+=3;
                }
              }
            }
          }
          FreeP(dot_flag);
        }

        /* now eliminate troublesome vertices in regions of extremely high curvature */
        
        if((surface_type!=3)&&
           I->N && (I->trimCutoff>0.0F)&&
           (I->trimFactor>0.0F)) {
          float trim_cutoff = I->trimCutoff;
          float trim_factor = I->trimFactor;
          int repeat_flag=true;
          float neighborhood = trim_factor*point_sep;
          float dot_sum;
          int n_nbr;
          int *dot_flag=Alloc(int,I->N);
          if(surface_type==6) { /* emprical tweaks */
            trim_factor*=2.5;
            trim_cutoff*=1.5;
          }
          while(repeat_flag) {
            int a;
            MapType *map = MapNew(G,neighborhood,I->V,I->N,NULL);
            repeat_flag=false;
          
            for(a=0;a<I->N;a++) dot_flag[a]=1;
            MapSetupExpress(map);		  
            v=I->V;
            vn=I->VN;
            for(a=0;a<I->N;a++) {
              if(dot_flag[a]) {
                register int i=*(MapLocusEStart(map,v));
                if(i) {
                  register int j=map->EList[i++];
                  n_nbr = 0;
                  dot_sum = 0.0F;
                  while(j>=0) {
                    if(j!=a)  {
                      if(dot_flag[j]) {
                        float *v0 = I->V+3*j;
                        if(within3f(v0,v,neighborhood)) {
                          float *n0 = I->VN + 3*j;
                          dot_sum += dot_product3f(n0,vn);
                          n_nbr++;
                        } 
                      }
                    }
                    j=map->EList[i++];
                  }

                  if(n_nbr) {
                    dot_sum/=n_nbr;
                    if(dot_sum<trim_cutoff) {
                      dot_flag[a]=false;
                      repeat_flag = true;
                    }
                  }
                }
              }
              v+=3;
              vn+=3;
            }

            {
              float *v0=I->V;
              float *vn0=I->VN;
              int *p=dot_flag;
              int c=I->N;
              v=I->V;
              vn=I->VN;
              I->N=0;
              for(a=0;a<c;a++) {
                if(*(p++)) {
                  *(v0++)=*(v++);
                  *(v0++)=*(v++);
                  *(v0++)=*(v++);
                  normalize3f(vn);
                  *(vn0++)=*(vn++);
                  *(vn0++)=*(vn++);
                  *(vn0++)=*(vn++);
                  I->N++;
                } else {
                  v+=3;
                  vn+=3;
                }
              }
            }
            MapFree(map);
          }
          FreeP(dot_flag);
        }
      }
    }

    if(I->N && I->V && I->VN) {	
      I->V = ReallocForSure(I->V,float,3*I->N);
      I->VN = ReallocForSure(I->VN,float,3*I->N);
    }
    
    PRINTFD(G,FB_RepSurface)
      " RepSurfaceNew-DEBUG: %i surface points after trimming.\n",I->N
      ENDFD;
    if(carve_map)
      MapFree(carve_map);
  }
  return ok;
}

Rep *RepSurfaceNew(CoordSet *cs,int state)
{
  PyMOLGlobals *G=cs->State.G;
  ObjectMolecule *obj;
  int a,i,j;
  MapType *map;
  float *v0=NULL;

  int SurfaceFlag = false;
  float probe_radius,probe_radius2;
  float probe_rad_more;
  float probe_rad_less,probe_rad_less2;
  float trim_cutoff,trim_factor;
  int inclH = true;
  int cullByFlag = false;
  float point_sep;
  int visFlag;
  int surface_quality;
  int surface_mode;
  int *present_vla = NULL,*ap;
  int surface_type;
  int surface_solvent;
  int optimize;
  int sphere_idx=0, solv_sph_idx=0;
  AtomInfoType *ai1,*ai2;
  int n_present = 0;
  float solv_tole;
  int carve_state = 0;
  int carve_flag = false;
  float carve_cutoff;
  char *carve_selection = NULL;
  float *carve_vla = NULL;
  MapType *carve_map = NULL;
  int circumscribe = 0;

  #if 0
  int c1;
  float v1[3];
  float vdw;
#endif
  OOCalloc(G,RepSurface);

  obj = cs->Obj;
  I->R.context.object = (void*)obj;
  I->R.context.state = state;

  surface_mode = SettingGet_i(G,cs->Setting,obj->Obj.Setting,cSetting_surface_mode);
  surface_type = SettingGet_i(G,cs->Setting,obj->Obj.Setting,cSetting_surface_type);
  optimize = SettingGet_i(G,cs->Setting,obj->Obj.Setting,cSetting_surface_optimize_subsets);
  surface_solvent = SettingGet_b(G,cs->Setting,obj->Obj.Setting,cSetting_surface_solvent);
  I->Type = surface_type;

  trim_cutoff = SettingGet_f(G,cs->Setting,obj->Obj.Setting,cSetting_surface_trim_cutoff);
  trim_factor = SettingGet_f(G,cs->Setting,obj->Obj.Setting,cSetting_surface_trim_factor);

  cullByFlag = (surface_mode==cRepSurface_by_flags);
  inclH = !(surface_mode==cRepSurface_heavy_atoms);

  visFlag=false;
  if(obj->RepVisCache[cRepSurface])
    for(a=0;a<cs->NIndex;a++) {
      ai1=obj->AtomInfo+cs->IdxToAtm[a];
      if(ai1->visRep[cRepSurface]&&
         (inclH||(!ai1->hydrogen))&&
         ((!cullByFlag)||
          (!(ai1->flags&(cAtomFlag_exfoliate|cAtomFlag_ignore))))) {
        visFlag=true;
        break;
      }
    }
  if(!visFlag) {
    OOFreeP(I);
    return(NULL); /* skip if no thing visible */
  }

  I->max_vdw = ObjectMoleculeGetMaxVDW(obj);

  RepInit(G,&I->R);
  circumscribe = SettingGet_i(G,cs->Setting,obj->Obj.Setting,cSetting_surface_circumscribe);
  surface_quality = SettingGet_i(G,cs->Setting,obj->Obj.Setting,cSetting_surface_quality);
  if(surface_quality>=4) { /* totally impractical */
    point_sep = SettingGet_f(G,cs->Setting,obj->Obj.Setting,cSetting_surface_best)/4;
    sphere_idx = 4;
    solv_sph_idx = 4;
    if(circumscribe<0)
      circumscribe = 91;
  } else {
    switch(surface_quality) {
    case 3:  /* nearly impractical */
      point_sep = SettingGet_f(G,cs->Setting,obj->Obj.Setting,cSetting_surface_best)/3;
      sphere_idx = 4;
      solv_sph_idx = 3;
      if(circumscribe<0)
        circumscribe = 71;
      break;
    case 2:
      /* nearly perfect */
      point_sep = SettingGet_f(G,cs->Setting,obj->Obj.Setting,cSetting_surface_best)/2;
      sphere_idx = 3;
      solv_sph_idx = 3;
      if(circumscribe<0)
        circumscribe = 41;
      break;
    case 1: 
      /* good */
      point_sep = SettingGet_f(G,cs->Setting,obj->Obj.Setting,cSetting_surface_best);
      sphere_idx = 2;
      solv_sph_idx = 3;
      if((circumscribe<0)&&(surface_type==6))
        circumscribe = 40;
      break;
    case 0: 
      /* 0 - normal */
      point_sep = SettingGet_f(G,cs->Setting,obj->Obj.Setting,cSetting_surface_normal);
      sphere_idx = 1;
      solv_sph_idx = 2;
      if((circumscribe<0)&&(surface_type==6))
        circumscribe = 30;
      break;
    case -1:
      /* -1 */
      point_sep = SettingGet_f(G,cs->Setting,obj->Obj.Setting,cSetting_surface_poor);
      sphere_idx = 1;
      solv_sph_idx = 2;
      if((circumscribe<0)&&(surface_type==6))
        circumscribe = 10;
      break;
    case -2:
      /* -2 god awful*/
      point_sep = SettingGet_f(G,cs->Setting,obj->Obj.Setting,cSetting_surface_poor)*1.5F;
      sphere_idx = 1;
      solv_sph_idx = 1;
      break;
    case -3:
      /* -3 miserable */
      point_sep = SettingGet_f(G,cs->Setting,obj->Obj.Setting,cSetting_surface_miserable);
      sphere_idx = 1;
      solv_sph_idx = 1;
      break;
    default:
      point_sep = SettingGet_f(G,cs->Setting,obj->Obj.Setting,cSetting_surface_miserable)*1.18F;
      sphere_idx = 0;
      solv_sph_idx = 1;
    }
  }

  solv_tole = point_sep * 0.04F;

  if(circumscribe<0) circumscribe=0;

  probe_radius = SettingGet_f(G,cs->Setting,obj->Obj.Setting,cSetting_solvent_radius);

  if(!surface_solvent) {
    if(probe_radius<(2.5F*point_sep)) { /* minimum probe radius allowed */
      probe_radius = 2.5F*point_sep;
    }
  } else {
    circumscribe = 0;
  }

  probe_radius2 = probe_radius*probe_radius;
  probe_rad_more = probe_radius*(1.0F+solv_tole);

  switch(surface_type) {
  case 0: /* solid */
  case 3:
  case 4:
  case 5:
  case 6:
    probe_rad_less = probe_radius;
    break;
  default:
    probe_rad_less = probe_radius*(1.0F-solv_tole);
    break;
  }
  
  probe_rad_less2 = probe_rad_less * probe_rad_less;

  /* OOCalloc takes care of all this 

  I->N=0;
  I->NT=0;
  I->S=NULL;
  I->V=NULL;
  I->VC=NULL;
  I->Vis=NULL;
  I->VN=NULL;
  I->T=NULL;
  I->LastVisib=NULL;
  I->LastColor=NULL;
  I->debug = NULL;
  */

  I->R.fRender=(void (*)(struct Rep *, RenderInfo *info))RepSurfaceRender;
  I->R.fFree=(void (*)(struct Rep *))RepSurfaceFree;
  I->R.fRecolor=(void (*)(struct Rep*, struct CoordSet*))RepSurfaceColor;
  I->R.fSameVis=(int (*)(struct Rep*, struct CoordSet*))RepSurfaceSameVis;
  I->R.obj = (CObject*)(cs->Obj);
  I->R.cs = cs;
  I->allVisibleFlag=true;
  obj = cs->Obj;

  /* don't waist time computing a Surface unless we need it!! */
  for(a=0;a<cs->NIndex;a++) {
    ai1=obj->AtomInfo+cs->IdxToAtm[a];
    if(ai1->visRep[cRepSurface]&&
       ((!cullByFlag)||
        (!(ai1->flags&(cAtomFlag_exfoliate|cAtomFlag_ignore)))))
      SurfaceFlag=true;
    else
      I->allVisibleFlag=false;
  }
  if(SurfaceFlag) {
    SurfaceJobAtomInfo *atom_info = VLACalloc(SurfaceJobAtomInfo, cs->NIndex);
    if(atom_info) {
      int i;
      AtomInfoType *i_ai,*obj_atom_info = obj->AtomInfo;
      int *idx_to_atm = cs->IdxToAtm;
      int n_index = cs->NIndex;
      SurfaceJobAtomInfo *i_atom_info = atom_info;
      for(i=0;i<n_index;i++) {
        i_ai = obj_atom_info + idx_to_atm[i];
        /* just surfacing flags */
        i_atom_info->flags = i_ai->flags & ( cAtomFlag_ignore | cAtomFlag_exfoliate ); 
        i_atom_info->vdw = i_ai->vdw;
        i_atom_info++;
      }
    }

    OrthoBusyFast(G,0,1);

    n_present = cs->NIndex;

    carve_selection = SettingGet_s(G,cs->Setting,obj->Obj.Setting,cSetting_surface_carve_selection);
    carve_cutoff = SettingGet_f(G,cs->Setting,obj->Obj.Setting,cSetting_surface_carve_cutoff);
    if((!carve_selection)||(!carve_selection[0]))
      carve_cutoff=0.0F;
    if(carve_cutoff>0.0F) {
      carve_state = SettingGet_i(G,cs->Setting,obj->Obj.Setting,cSetting_surface_carve_state) - 1;
      carve_cutoff += 2*I->max_vdw+probe_radius;

      if(carve_selection) 
        carve_map = SelectorGetSpacialMapFromSeleCoord(G,
                                                       SelectorIndexByName(G,carve_selection),
                                                       carve_state,
                                                       carve_cutoff,
                                                       &carve_vla);
      if(carve_map) 
        MapSetupExpress(carve_map);
      carve_flag = true;
      I->allVisibleFlag=false;
    }
   if(!I->allVisibleFlag) {
      /* optimize the space over which we calculate a surface */
      
      /* first find out which atoms are actually to be surfaced */

      present_vla = VLAlloc(int,cs->NIndex); 
      ap = present_vla;
      for(a=0;a<cs->NIndex;a++) {
        ai1 = obj->AtomInfo + cs->IdxToAtm[a];
        if(ai1->visRep[cRepSurface]&&
           (inclH||(!ai1->hydrogen))&&
           ((!cullByFlag)||
            (!(ai1->flags&(cAtomFlag_ignore|cAtomFlag_exfoliate)))))
          *ap = 2; 
        else 
          *ap = 0;
        ap++;
      }

      map=MapNewFlagged(G,2*I->max_vdw+probe_radius,cs->Coord,cs->NIndex,NULL,present_vla);
      MapSetupExpress(map);
      
      {
        float probe_radiusX2 = probe_radius*2;
        for(a=0;a<cs->NIndex;a++)
          /* then add in the nearby atoms which are not surfaced and not ignored */

          if(!present_vla[a]) {
            ai1 = obj->AtomInfo+cs->IdxToAtm[a];
            if((!cullByFlag)||!(ai1->flags&cAtomFlag_ignore)) {
              v0 = cs->Coord+3*a;
              i=*(MapLocusEStart(map,v0));
              if(optimize) {
                if(i) {
                  j=map->EList[i++];
                  while(j>=0) {
                    if(present_vla[j]>1) {
                      ai2 = obj->AtomInfo+cs->IdxToAtm[j];                  
                      if(within3f(cs->Coord+3*j,v0,ai1->vdw+ai2->vdw+probe_radiusX2)) {
                        present_vla[a]=1;
                        break;
                      }
                    }
                    j=map->EList[i++];
                  }
                }
              } else 
                present_vla[a]=1;
            }
          }
      }

      if(carve_flag&&(!optimize)) {
        /* and optimize for carved region */
        
        for(a=0;a<cs->NIndex;a++) {
          int include_flag = false;
          if(carve_map) {
            v0 = cs->Coord+3*a;
            i=*(MapLocusEStart(carve_map,v0));
            if(i) {
              j=carve_map->EList[i++];
              while(j>=0) {
                if(within3f(carve_vla+3*j,v0,carve_cutoff)) {
                  include_flag = true;
                  break;
                }
                j=carve_map->EList[i++];
              }
            }
          }
          if(!include_flag)
            present_vla[a]=0;
        }
      }
      MapFree(map);
      map = NULL;

      /* now count how many atoms we actually need to think about */

      n_present = 0;
      for(a=0;a<cs->NIndex;a++) {
        if(present_vla[a]) {
          n_present++;
        }
      }
   }
#if 1
   {
     SurfaceJob *surf_job = SurfaceJobNew(G);

     if(surf_job) {

       surf_job->maxVdw = I->max_vdw;
       surf_job->allVisibleFlag = I->allVisibleFlag;
       
       surf_job->atomInfo = atom_info; atom_info = NULL;
       
       surf_job->nPresent = n_present;
       if(present_vla && optimize) {
         /* implies that n_present < cs->NIndex, so eliminate
            irrelevant atoms & coordinates if we are optimizing subsets */
         surf_job->coord = VLAlloc(float, n_present * 3);
         {
           int a;
           int *p = present_vla;
           SurfaceJobAtomInfo *ai_src = surf_job->atomInfo;
           SurfaceJobAtomInfo *ai_dst = surf_job->atomInfo;
           float *v_src = cs->Coord;
           float *v_dst = surf_job->coord;
           for(a=0;a<cs->NIndex;a++) {
             if(*(p++)) {
               copy3f(v_src,v_dst);
               v_dst+=3;
               if(ai_dst!=ai_src)
                 *ai_dst = *ai_src;
               ai_dst++;
             }
             v_src+=3;
             ai_src++;
           }
         }
         VLASetSize(surf_job->atomInfo,n_present);
         
       } else {
         surf_job->presentVla = present_vla; present_vla = NULL;
         surf_job->coord = VLAlloc(float, cs->NIndex*3);
         if(surf_job->coord)
           UtilCopyMem(surf_job->coord, cs->Coord, sizeof(float)*3*cs->NIndex);
       }

       surf_job->sphereIndex = sphere_idx;
       surf_job->solventSphereIndex = solv_sph_idx;
    
       surf_job->surfaceType = surface_type;
       surf_job->circumscribe = circumscribe;
       surf_job->probeRadius = probe_radius;
       surf_job->probeRadMore = probe_rad_more;
       surf_job->probeRadLess = probe_rad_less;
       surf_job->probeRadLess2 = probe_rad_less2;
       surf_job->pointSep = point_sep;

       surf_job->trimCutoff = trim_cutoff;
       surf_job->trimFactor = trim_factor;

       if(carve_vla) 
         surf_job->carveVla = VLACopy(carve_vla,float);
       surf_job->carveCutoff = carve_cutoff;
        
       surf_job->surfaceMode = SettingGet_i(G,cs->Setting,obj->Obj.Setting,cSetting_surface_mode);
       surf_job->surfaceSolvent = SettingGet_b(G,cs->Setting,obj->Obj.Setting,cSetting_surface_solvent);
       surf_job->cavityCull = SettingGet_i(G,cs->Setting,obj->Obj.Setting,cSetting_cavity_cull);
     }
      
     SurfaceJobRun(G,surf_job);
     I->N = surf_job->N; surf_job->N = 0;
     I->V = surf_job->V; surf_job->V = 0;
     I->VN = surf_job->VN; surf_job->VN = 0;

     SurfaceJobPurgeResult(G,surf_job);
     SurfaceJobFree(G,surf_job);
   }
#else
   {
     SphereRec *sp = G->Sphere->Sphere[sphere_idx];
     SphereRec *ssp = G->Sphere->Sphere[solv_sph_idx];
     SolventDot *sol_dot = NULL;
     MapType *solv_map;
     int b,c;
     float *v,*vn=NULL,*vn0=NULL,*n0=NULL;
     int MaxN;
     int *p;
     int flag,*dot_flag;

     if(n_present<1) n_present=1; /* safety */
  
     if(sp->nDot<ssp->nDot)
       MaxN = n_present*ssp->nDot;
     else
       MaxN = n_present*sp->nDot;

     I->V=Alloc(float,(MaxN+1)*3);
     I->VN=Alloc(float,(MaxN+1)*3);

     if(!(I->V&&I->VN)) { /* bail out point -- try to reduce crashes */
       PRINTFB(G,FB_RepSurface,FB_Errors)
         "Error-RepSurface: insufficient memory to calculate surface at this quality.\n"
         ENDFB(G);
       FreeP(I->V);
       FreeP(I->VN);
       VLAFreeP(present_vla);
       if(carve_map)
         MapFree(carve_map);
       VLAFreeP(carve_vla);
       OOFreeP(I);
       return NULL;
     }
     I->N=0;
     v=I->V;
     vn=I->VN; 
     
     {
       int surface_solvent = SettingGet_b(G,cs->Setting,obj->Obj.Setting,cSetting_surface_solvent);
       int cavity_cull = SettingGet_i(G,cs->Setting,obj->Obj.Setting,cSetting_cavity_cull);
      
       sol_dot = SolventDotNew(G, cs->Coord, atom_info, probe_radius,
                               ssp, present_vla, circumscribe,
                               surface_mode, surface_solvent, cavity_cull,
                               I->allVisibleFlag, I->max_vdw);
     }
  
     if(!surface_solvent) {
      
       if(surface_type>=5) { /* effectively double-weights atom points */
         if(sol_dot->nDot) { 
           v0 = sol_dot->dot;
           n0 = sol_dot->dotNormal;
           for(a=0;a<sol_dot->nDot;a++) {
             scale3f(n0,-probe_radius,v);
             add3f(v0,v,v);
             copy3f(n0,vn);
             v+=3;
             vn+=3;
             n0+=3;
             v0+=3;
             I->N++;
           }
         }
       }

       map=MapNewFlagged(G,I->max_vdw+probe_rad_more,cs->Coord,cs->NIndex,NULL,present_vla);
      
       solv_map=MapNew(G,probe_rad_less,sol_dot->dot,sol_dot->nDot,NULL);
      
       /*

         I->debug=CGONew(G);
      
         CGOBegin(I->debug,GL_POINTS);
         for(a=0;a<sol_dot->nDot;a++)
         CGOVertexv(I->debug,sol_dot->dot+3*a);
         CGOEnd(I->debug);
       */
      
       if(map&&solv_map) {
         MapSetupExpress(solv_map);
         MapSetupExpress(map);
        
         if(sol_dot->nDot) {
           int *dc = sol_dot->dotCode;
           Vector3f *dot = NULL;
          
           dot=Alloc(Vector3f,sp->nDot);
           for(b=0;b<sp->nDot;b++) {
             scale3f(sp->dot[b],probe_radius,dot[b]);
           }
           v0 = sol_dot->dot;
           n0 = sol_dot->dotNormal;
           for(a=0;a<sol_dot->nDot;a++) {
             if(dc[a]||(surface_type<6)) { /* surface type 6 is completely scribed */
               OrthoBusyFast(G,a+sol_dot->nDot*2,sol_dot->nDot*5); /* 2/5 to 3/5 */
               for(b=0;b<sp->nDot;b++) {
                 register int ii;
                 v[0]=v0[0]+dot[b][0];
                 v[1]=v0[1]+dot[b][1];
                 v[2]=v0[2]+dot[b][2];
                 flag=true;
                 ii=*(MapLocusEStart(solv_map,v));
                 if(ii) {
                   register int jj;
                   register int *elist = solv_map->EList;
                   register float *i_dot = sol_dot->dot;
                   register float v_0=v[0], v_1=v[1], v_2=v[2];
                   register float dist=probe_rad_less;
                   register float dist2=probe_rad_less2;
                   register float *v1,dx,dy,dz;
                   jj=elist[ii++];
                   v1 = i_dot + 3*jj;                          
                   while(jj>=0) {
                     if(jj!=a)  { /*&&!(dc[jj]))*/
                       /* huge bottleneck -- optimized for superscaler processors */
                       dx = v1[0]-v_0;
                       dy = v1[1]-v_1;
                       dz = v1[2]-v_2;
                       dx = (float)fabs(dx);
                       dy = (float)fabs(dy);
                       if(!(dx>dist)) {
                         dx = dx * dx;
                         if(!(dy>dist)) {
                           dz = (float)fabs(dz);
                           dy = dy * dy;
                           if(!(dz>dist)) {
                             dx = dx + dy;
                             dz = dz * dz;
                             if(!(dx>dist2)) 
                               if((dx + dz)<=dist2) {
                                 flag = false; 
                                 break; 
                               }
                           }
                         }
                       }
                     }
                     jj=elist[ii++];
                     v1 = i_dot + 3*jj;                          
                   }
                 }
                 /* at this point, we have points on the interior of the solvent surface,
                    so now we need to further trim that surface to cover atoms that are present */
                
                 if(flag) {
                   i=*(MapLocusEStart(map,v));
                   if(i) {
                     j=map->EList[i++];
                     while(j>=0) {
                       ai2 = obj->AtomInfo + cs->IdxToAtm[j];
                       if((!present_vla)||present_vla[j]) {
                         if(within3f(cs->Coord+3*j,v,ai2->vdw+probe_rad_more)) {
                           flag=false;
                           break;
                         }
                       }
                       j=map->EList[i++];
                     }
                   }
                   if(!flag) { /* compute the normals */
                     vn[0]=-sp->dot[b][0];
                     vn[1]=-sp->dot[b][1];
                     vn[2]=-sp->dot[b][2];
                     if(I->N<MaxN) {
                       I->N++;
                       v+=3;
                       vn+=3;
                     } else {
                       int v_offset = v-I->V;
                       int vn_offset = vn-I->VN;
                       MaxN = MaxN*2;
                       I->V=Realloc(I->V,float,(MaxN+1)*3);
                       I->VN=Realloc(I->VN,float,(MaxN+1)*3);
                       v = I->V + v_offset;
                       vn = I->VN + vn_offset;
                     }
                   }
                 }
               }
             }
             v0 +=3;
             n0 +=3;
           }
           FreeP(dot);
         }
       }
       MapFree(solv_map);
       MapFree(map);
     } else {
       v0 = sol_dot->dot;
       n0 = sol_dot->dotNormal;
       if(sol_dot->nDot) {
         for(a=0;a<sol_dot->nDot;a++) {
           *(v++)=*(v0++);
           *(vn++)=*(n0++);
           *(v++)=*(v0++);
           *(vn++)=*(n0++);
           *(v++)=*(v0++);
           *(vn++)=*(n0++);
           I->N++;
         }
       }
     }

     SolventDotFree(sol_dot); sol_dot = NULL;

     {
       int refine,ref_count = 1;
      
       if((surface_type==0) && (circumscribe)) {
         ref_count = 2; /* these constants need more tuning...*/
       }
      
       for(refine=0;refine<ref_count;refine++) {

         /* add new vertices in regions where curvature is very high
            or where there are gaps with no points */
    
         if(I->N && (surface_type==0) && (circumscribe)) {
           int n_new = 0;

           float neighborhood = 2.6*point_sep; /* these constants need more tuning...*/
           float dot_cutoff = 0.666;
           float insert_cutoff = 1.1*point_sep;

           float map_cutoff = neighborhood;
           float *v0,*vv0;
           int ii,jj;
           float *new_dot = VLAlloc(float,1000),*v1,*n1;
           if(map_cutoff<(2.9*point_sep)) { /* these constants need more tuning...*/
             map_cutoff = 2.9*point_sep;
           }
           map=MapNew(G,map_cutoff,I->V,I->N,NULL);
           MapSetupExpress(map);		  
           v=I->V;
           vn=I->VN;
           for(a=0;a<I->N;a++) {
             i=*(MapLocusEStart(map,v));
             if(i) {
               j=map->EList[i++];
               while(j>=0) {
                 if(j>a) {
                   v0 = I->V+3*j;
                   if(within3f(v0,v,map_cutoff)) {
                     int add_new = false;
                     n0 = I->VN + 3*j;
                     VLACheck(new_dot,float,n_new*6+5);
                     v1 = new_dot+n_new*6;
                     average3f(v,v0,v1);
                     if((dot_product3f(n0,vn)<dot_cutoff)&&(within3f(v0,v,neighborhood)))
                       add_new = true;
                     else {
                       /* if points are too far apart, insert a new one */
                       ii=*(MapLocusEStart(map,v1));
                       if(ii) {
                         int found=false;
                         jj=map->EList[ii++];
                         while(jj>=0) {
                           if(jj!=j) {
                             vv0 = I->V+3*jj;
                             if(within3f(vv0,v1,insert_cutoff)) {
                               found = true;
                               break;
                             }
                           }
                           jj=map->EList[ii++];
                         }
                         if(!found) add_new =true;
                       }
                     }
                     if(add_new) {
                       /* highly divergent */
                       n1 = v1+3;
                       n_new ++;
                       average3f(vn,n0,n1);
                       normalize3f(n1);
                     }
                   }
                 }
                 j=map->EList[i++];
               }
             }
             v+=3;
             vn+=3;
           }
           MapFree(map);
           if(n_new) {
             I->V = Realloc(I->V,float,3*(I->N+n_new));
             I->VN = Realloc(I->VN,float,3*(I->N+n_new));
             v = I->V + 3*I->N;
             vn = I->VN + 3*I->N;
             n1 = new_dot+3;
             v1 = new_dot;
             I->N+=n_new;
             while(n_new--) {
               copy3f(v1,v);
               copy3f(n1,vn);
               v+=3;
               vn+=3;
               v1+=6;
               n1+=6;
             }
           }
           VLAFreeP(new_dot);
         }

         if(I->N && (surface_type==0) && (circumscribe)) {
      
           float cutoff = 0.5*probe_radius;
      
           /* combine scribing with an atom proximity cleanup pass */
      
           dot_flag=Calloc(int,I->N);
           map=MapNewFlagged(G,I->max_vdw+probe_radius,cs->Coord,cs->NIndex,NULL,present_vla);
           MapSetupExpress(map); 
           v=I->V;
           for(a=0;a<I->N;a++) {
             i=*(MapLocusEStart(map,v));
             if(i) {
               j=map->EList[i++];
               while(j>=0) {
                 ai2 = obj->AtomInfo+cs->IdxToAtm[j];                  
                 if((!present_vla)||present_vla[j]) {
                   if(within3f(cs->Coord+3*j,v,ai2->vdw+cutoff)) { 
                     dot_flag[a] = true;
                   }
                 }
                 j=map->EList[i++];
               }
             }
             v+=3;
           }
      
           MapFree(map);
           map = NULL;
           {
             /* purge unused dots */

             float *v0;
        
             v0=I->V;
             v=I->V;
             vn0=I->VN;
             vn=I->VN;
             p=dot_flag;
             c=I->N;
             I->N=0;
             for(a=0;a<c;a++) {
               if(*(p++)) {
                 *(v0++)=*(v++);
                 *(v0++)=*(v++);
                 *(v0++)=*(v++);
                 *(vn0++)=*(vn++);
                 *(vn0++)=*(vn++);
                 *(vn0++)=*(vn++);
                 I->N++;
               } else {
                 v+=3;
                 vn+=3;
               }
             }
             FreeP(dot_flag);
           }
         }

         /* now, eliminate dots that are too close to each other*/

         /*    CGOColor(I->debug,0.0,1.0,0.0);
               CGOBegin(I->debug,GL_POINTS);
               for(a=0;a<I->N;a++)
               CGOVertexv(I->debug,I->V+3*a);
               CGOEnd(I->debug);
         */

         PRINTFD(G,FB_RepSurface)
           " RepSurface: %i surface points in refinement pass %d.\n",I->N,refine
           ENDFD(G);

         if(I->N) {
           int repeat_flag=true;
           float min_dot = 0.1F;
           dot_flag=Alloc(int,I->N);
           while(repeat_flag) {
             repeat_flag = false;

             if(surface_type>=3) { 
               register int jj;
               float dist;
               register float nearest;
               register float min_sep2 = point_sep*point_sep;
               float diff[3];
               for(a=0;a<I->N;a++) dot_flag[a]=1;
               map=MapNew(G,point_sep+0.05F,I->V,I->N,NULL);
               MapSetupExpress(map);		  
               v=I->V;
               vn=I->VN;
               for(a=0;a<I->N;a++) {
                 if(dot_flag[a]) {
                   i=*(MapLocusEStart(map,v));
                   if(i) {
                     j=map->EList[i++];
                     jj=I->N;
                     nearest = point_sep+1.0F;
                     while(j>=0) {
                       if(j>a) {
                         if(dot_flag[j]) {
                           if(dot_product3f(I->VN+(3*j),vn)>min_dot) {
                             if(within3fret(I->V+(3*j),v,point_sep,min_sep2,diff,&dist)) {
                               repeat_flag = true;
                               if(dist<nearest) { 
                                 /* try to be as determinstic as possible
                                    in terms of how we collapse points */
                                 jj = j;
                                 nearest = dist;
                               } else if((j<jj)&&(fabs(dist-nearest)<R_SMALL4)) { 
                                 jj = j;
                                 nearest = dist;
                               }
                             }
                           }
                         }
                       }
                       j=map->EList[i++];
                     }
                  
                     if(jj<I->N) {
                       dot_flag[jj]=0;
                       add3f(vn,I->VN+(3*jj),vn);
                       average3f(I->V+(3*jj),v,v);
                       repeat_flag=true;
                     }
                   }
                 }
                 v+=3;
                 vn+=3;
               }
               MapFree(map);
             } else { /* surface types < 3 */

               for(a=0;a<I->N;a++) dot_flag[a]=1;
               map=MapNew(G,-point_sep,I->V,I->N,NULL);
               MapSetupExpress(map);		  
               v=I->V;
               vn=I->VN;
               for(a=0;a<I->N;a++) {
                 if(dot_flag[a]) {
                   i=*(MapLocusEStart(map,v));
                   if(i) {
                     j=map->EList[i++];
                     while(j>=0) {
                       if(j!=a) 
                         {
                           if(dot_flag[j]) {
                             if(within3f(I->V+(3*j),v,point_sep)) {
                               dot_flag[j]=0;
                               add3f(vn,I->VN+(3*j),vn);
                               average3f(I->V+(3*j),v,v);
                               repeat_flag=true;
                             }
                           }
                         }
                       j=map->EList[i++];
                     }
                   }
                 }
                 v+=3;
                 vn+=3;
               }
               MapFree(map);
             }
             v0=I->V;
             v=I->V;
             vn0=I->VN;
             vn=I->VN;
             p=dot_flag;
             c=I->N;
             I->N=0;
             for(a=0;a<c;a++)
               {
                 if(*(p++)) {
                   *(v0++)=*(v++);
                   *(v0++)=*(v++);
                   *(v0++)=*(v++);
                   normalize3f(vn);
                   *(vn0++)=*(vn++);
                   *(vn0++)=*(vn++);
                   *(vn0++)=*(vn++);
                   I->N++;
                 } else {
                   v+=3;
                   vn+=3;
                 }
               }
           }

           FreeP(dot_flag);
        
           if(I->N) {	
             I->V = ReallocForSure(I->V,float,(v0-I->V));
             I->VN = ReallocForSure(I->VN,float,(vn0-I->VN));
           }
         }
    
         /* now eliminate troublesome vertices in regions of extremely high curvature */

         if((surface_type!=3)&&
            I->N && (trim_cutoff>0.0F)&&
            (trim_factor>0.0F))
           {
             int repeat_flag=true;
             float neighborhood = trim_factor*point_sep;
             float *v0,dot_sum;
             int n_nbr;
             dot_flag=Alloc(int,I->N);
             if(surface_type==6) { /* emprical tweaks */
               trim_factor*=2.5;
               trim_cutoff*=1.5;
             }
             while(repeat_flag) {
               repeat_flag=false;
          
               for(a=0;a<I->N;a++) dot_flag[a]=1;
               map=MapNew(G,neighborhood,I->V,I->N,NULL);
               MapSetupExpress(map);		  
               v=I->V;
               vn=I->VN;
               for(a=0;a<I->N;a++) {
                 if(dot_flag[a]) {
                   i=*(MapLocusEStart(map,v));
                   if(i) {
                     n_nbr = 0;
                     dot_sum = 0.0F;
                     j=map->EList[i++];
                     while(j>=0) {
                       if(j!=a) 
                         {
                           if(dot_flag[j]) {
                             v0 = I->V+3*j;
                             if(within3f(v0,v,neighborhood)) {

                               n0 = I->VN + 3*j;
                               dot_sum += dot_product3f(n0,vn);
                               n_nbr++;
                          
                             } 
                           }
                         }
                       j=map->EList[i++];
                     }

                     if(n_nbr) {
                       dot_sum/=n_nbr;
                       if(dot_sum<trim_cutoff) {
                         dot_flag[a]=false;
                         repeat_flag = true;
                       }
                     }
                   }
                 }
                 v+=3;
                 vn+=3;
               }
               MapFree(map);
          
               v0=I->V;
               v=I->V;
               vn0=I->VN;
               vn=I->VN;
               p=dot_flag;
               c=I->N;
               I->N=0;
               for(a=0;a<c;a++)
                 {
                   if(*(p++)) {
                     *(v0++)=*(v++);
                     *(v0++)=*(v++);
                     *(v0++)=*(v++);
                     normalize3f(vn);
                     *(vn0++)=*(vn++);
                     *(vn0++)=*(vn++);
                     *(vn0++)=*(vn++);
                     I->N++;
                   } else {
                     v+=3;
                     vn+=3;
                   }
                 }
             }
             FreeP(dot_flag);
            
             if(I->N) {	
               I->V = ReallocForSure(I->V,float,(v0-I->V));
               I->VN = ReallocForSure(I->VN,float,(vn0-I->VN));
             }
           }
       }
     }
    
     PRINTFD(G,FB_RepSurface)
       " RepSurfaceNew-DEBUG: %i surface points after trimming.\n",I->N
       ENDFD;
   }
#endif

    RepSurfaceColor(I,cs);
    
    PRINTFB(G,FB_RepSurface,FB_Blather)
      " RepSurface: %i surface points.\n",I->N
      ENDFB(G);
    
    OrthoBusyFast(G,3,5);
    if(I->N) {
      if(surface_type!=1) { /* not a dot surface... */
        float cutoff = point_sep*5.0F;
        if((cutoff>probe_radius)&&(!surface_solvent))
          cutoff = probe_radius;
        I->T=TrianglePointsToSurface(G,I->V,I->VN,I->N,cutoff,&I->NT,&I->S,NULL);
        PRINTFB(G,FB_RepSurface,FB_Blather)
          " RepSurface: %i triangles.\n",I->NT
          ENDFB(G);
      }
    } else {
      I->V = ReallocForSure(I->V,float,1);
      I->VN = ReallocForSure(I->VN,float,1);
    }
    VLAFreeP(atom_info);
  }
  if(carve_map)
    MapFree(carve_map);
  VLAFreeP(carve_vla);
  if(I->debug)
    CGOStop(I->debug);
  OrthoBusyFast(G,4,4);
  VLAFreeP(present_vla);
  return((void*)(struct Rep*)I);
}

static SolventDot *SolventDotNew(PyMOLGlobals *G, 
                                 float *coord,
                                 SurfaceJobAtomInfo *atom_info,
                                 float probe_radius,SphereRec *sp,
                                 int *present,
                                 int circumscribe, int surface_mode, 
                                 int surface_solvent, int cavity_cull,
                                 int all_visible_flag,float max_vdw)
{
  int b;
  float vdw;
  int skip_flag;
  float probe_radius_plus;
  int dotCnt=0,maxCnt,maxDot=0;
  int cnt;
  int stopDot;
  int n_coord = VLAGetSize(atom_info);
  OOCalloc(G,SolventDot);
  
  /*  printf("%p %p %p %f %p %p %p %d %d %d %d %d %f\n",
         G,
         coord,
         atom_info,
         probe_radius,sp,
         extent,present,
         circumscribe,  surface_mode, 
         surface_solvent,  cavity_cull,
         all_visible_flag, max_vdw);
  */

  stopDot = n_coord*sp->nDot + 2*circumscribe;
  I->dot=VLAlloc(float, (stopDot+1)*3);
  I->dotNormal=VLAlloc(float, (stopDot+1)*3);
  I->dotCode=VLACalloc(int,stopDot+1);

  probe_radius_plus = probe_radius * 1.5F;

  I->nDot=0;
  {
    MapType *map=MapNewFlagged(G,max_vdw+probe_radius,coord,n_coord,NULL,present);
    if(map) {
      float *v = I->dot;
      float *n = I->dotNormal;
      int *dc = I->dotCode;

      MapSetupExpress(map);
      maxCnt=0;
      {
        int a;
        SurfaceJobAtomInfo *a_atom_info = atom_info;
        for(a=0;a<n_coord;a++) {
          OrthoBusyFast(G,a,n_coord*5);
          if((!present)||(present[a])) {
            register int i;
            float *v0 = coord+3*a;
            dotCnt=0;
            vdw = a_atom_info->vdw+probe_radius;
            
            skip_flag=false;
            
            i=*(MapLocusEStart(map,v0));
            if(i) {
              register int j=map->EList[i++];
              while(j>=0) {
                SurfaceJobAtomInfo *j_atom_info = atom_info + j;
                if(j>a) /* only check if this is atom trails */
                  if((!present)||present[j]) {
                    if((j_atom_info->vdw == a_atom_info->vdw)) { /* handle singularities */
                      float *v1 = coord+3*j;
                      if((v0[0]==v1[0]) &&
                         (v0[1]==v1[1]) &&
                         (v0[2]==v1[2]))
                        skip_flag=true;
                    }
                  }
                j=map->EList[i++];
              }
            }
            if(!skip_flag) {
              for(b=0;b<sp->nDot;b++) {
                register int i;
                int flag=true;
                v[0]=v0[0]+vdw*(n[0] = sp->dot[b][0]);
                v[1]=v0[1]+vdw*(n[1] = sp->dot[b][1]);
                v[2]=v0[2]+vdw*(n[2] = sp->dot[b][2]);
                i=*(MapLocusEStart(map,v));
                if(i) {
                  register int j=map->EList[i++];
                  while(j>=0) {
                    SurfaceJobAtomInfo *j_atom_info = atom_info + j;
                    if((!present)||present[j]) {
                      if(j!=a) 
                        { 
                          skip_flag=false;
                          if(j_atom_info->vdw == a_atom_info->vdw) { /* handle singularities */
                            float *v1 = coord+3*j;                              
                            if((v0[0]==v1[0]) &&
                               (v0[1]==v1[1]) &&
                               (v0[2]==v1[2]))
                              skip_flag=true;
                          }
                          if(!skip_flag)
                            if(within3f(coord+3*j,v,j_atom_info->vdw+probe_radius)) {
                              flag=false;
                              break;
                            }
                        }
                    }
                    j=map->EList[i++];
                  }
                }
                if(flag&&(dotCnt<stopDot)) {
                  dotCnt++;
                  v+=3;
                  n+=3;
                  dc++;
                  I->nDot++;
                }
              }
            }
            if(dotCnt>maxCnt) {
              maxCnt=dotCnt;
              maxDot=I->nDot-1;
            }
          }
          a_atom_info++;
        }
      }

      /* for each pair of proximal atoms, circumscribe a circle for their intersection */

      /*    CGOReset(G->DebugCGO);*/
      
      {
        MapType *map2=NULL;
        if(circumscribe&&(!surface_solvent))
          map2=MapNewFlagged(G,2*(max_vdw+probe_radius),coord,n_coord,NULL,present);
        if(map2) {
          /*        CGOBegin(G->DebugCGO,GL_LINES);*/
          int a;
          SurfaceJobAtomInfo *a_atom_info = atom_info;
          MapSetupExpress(map2);
          for(a=0;a<n_coord;a++) {
            if((!present)||present[a]) {
              register int i;
              float vdw2;
          
              float *v0 = coord+3*a;
              vdw = a_atom_info->vdw+probe_radius;
              vdw2 = vdw*vdw;
          
              skip_flag=false;
          
              i=*(MapLocusEStart(map2,v0));
              if(i) {
                register int j=map2->EList[i++];
                while(j>=0) {
                  SurfaceJobAtomInfo *j_atom_info = atom_info + j;
                  if(j>a) /* only check if this is atom trails */
                    if((!present)||present[j]) {
                      if((j_atom_info->vdw == a_atom_info->vdw)) { /* handle singularities */
                        float *v2 = coord+3*j;
                        if((v0[0]==v2[0]) &&
                           (v0[1]==v2[1]) &&
                           (v0[2]==v2[2]))
                          skip_flag=true;
                      }
                    }
                  j=map2->EList[i++];
                }
              }
                
              if(!skip_flag) {
                register int ii=*(MapLocusEStart(map2,v0));
                if(ii) {
                  register int jj=map2->EList[ii++];
                  while(jj>=0) {
                    SurfaceJobAtomInfo *jj_atom_info = atom_info + jj;
                    float dist;
                    if(jj>a) /* only check if this is atom trails */
                      if((!present)||present[jj]) {
                        float vdw3 = jj_atom_info->vdw+probe_radius;
                        
                        float *v2 = coord+3*jj;
                        dist = (float)diff3f(v0,v2);
                        if((dist>R_SMALL4)&&(dist<(vdw+vdw3))) {
                          float vz[3],vx[3],vy[3], vp[3];
                          float tri_a=vdw, tri_b=vdw3, tri_c = dist;
                          float tri_s = (tri_a+tri_b+tri_c)*0.5F;
                          float area = (float)sqrt1f(tri_s*(tri_s-tri_a)*
                                                     (tri_s-tri_b)*(tri_s-tri_c));
                          float radius = (2*area)/dist;
                          float adj = (float)sqrt1f(vdw2 - radius*radius);
                          
                          subtract3f(v2,v0,vz);
                          get_system1f3f(vz,vx,vy);
                          
                          copy3f(vz,vp);
                          scale3f(vp,adj,vp);
                          add3f(v0,vp,vp);
                          
                          for(b=0;b<=circumscribe;b++) {
                            float xcos = (float)cos((b*2*cPI)/circumscribe);
                            float ysin = (float)sin((b*2*cPI)/circumscribe);
                            float xcosr = xcos * radius;
                            float ysinr = ysin * radius;
                            int flag=true;
                            v[0] = vp[0] + vx[0] * xcosr + vy[0] * ysinr;
                            v[1] = vp[1] + vx[1] * xcosr + vy[1] * ysinr;
                            v[2] = vp[2] + vx[2] * xcosr + vy[2] * ysinr;
                            
                            i=*(MapLocusEStart(map,v));
                            if(i) {
                              register int j=map->EList[i++];
                              while(j>=0) {
                                  SurfaceJobAtomInfo *j_atom_info = atom_info + j;
                                  if((!present)||present[j]) 
                                    if((j!=a)&&(j!=jj)) {
                                      skip_flag=false;
                                      if(a_atom_info->vdw==j_atom_info->vdw) { /* handle singularities */
                                        float *v1 = coord+3*j;                              
                                        if((v0[0]==v1[0]) &&
                                           (v0[1]==v1[1]) &&
                                           (v0[2]==v1[2]))
                                          skip_flag=true;
                                      }
                                      if(jj_atom_info->vdw==j_atom_info->vdw) { /* handle singularities */
                                        float *v1 = coord+3*j;                              
                                        if((v2[0]==v1[0]) &&
                                           (v2[1]==v1[1]) &&
                                           (v2[2]==v1[2]))
                                          skip_flag=true;
                                      }
                                      if(!skip_flag)
                                        if(within3f(coord+3*j,v,j_atom_info->vdw+probe_radius)) {
                                          flag=false;
                                          break;
                                        }
                                    }
                                  j=map->EList[i++];
                                }
                              }
                              if(flag&&(dotCnt<stopDot))
                                {
                                  float vt0[3],vt2[3];
                                  subtract3f(v0,v,vt0);
                                  subtract3f(v2,v,vt2);
                                  normalize3f(vt0);
                                  normalize3f(vt2);
                                  add3f(vt0,vt2,n);
                                  invert3f(n);
                                  normalize3f(n);
                                  /*
                                    n[0] = vx[0] * xcos + vy[0] * ysin;
                                    n[1] = vx[1] * xcos + vy[1] * ysin;
                                    n[2] = vx[2] * xcos + vy[2] * ysin;
                                  */
                                  
#if 0  
                                  {
                                    float sum[3];
                                    scale3f(n,0.2,sum);
                                    add3f(v,sum,sum);
                                    CGOVertexv(G->DebugCGO,v);
                                    CGOVertexv(G->DebugCGO,sum);
                                  }
#endif

                                  *dc=1; /* mark as exempt */

                                  dotCnt++;
                                  v+=3;
                                  n+=3;
                                  dc++;
                                  I->nDot++;
                                }
                          }
                        }
                      }
                    jj=map2->EList[ii++];
                  }
                }
              }
            }
            a_atom_info++;
          }
        }
        MapFree(map2);
      }
    }
    MapFree(map);
    /*    CGOEnd(G->DebugCGO);*/
  }

  if((cavity_cull>0)&&(probe_radius>0.75F)&&(!surface_solvent)) {
    int *dot_flag=Calloc(int,I->nDot);
    ErrChkPtr(G,dot_flag);

    dot_flag[maxDot]=1; /* this guarantees that we have a valid dot */

    {
      MapType *map=MapNew(G,probe_radius_plus,I->dot,I->nDot,NULL);
      if(map) {
        int flag=true;
        MapSetupExpress(map);		  
        while(flag) {
          int *p=dot_flag;
          float *v=I->dot;
		  int a;
          flag=false;
          for(a=0;a<I->nDot;a++)
            {
              if(!dot_flag[a]) {
                register int i=*(MapLocusEStart(map,v));
                cnt=0;
                if(i) {
                  register int j=map->EList[i++];
                  while(j>=0) {
                    if(j!=a) 
                      {
                        if(within3f(I->dot+(3*j),v,probe_radius_plus)) {
                          if(dot_flag[j]) {
                            *p=true;
                            flag=true;
                            break;
                          }
                          cnt++;
                          if(cnt>cavity_cull) {
                            *p=true;
                            flag=true;
                            break;
                          }
                        }
                      }
                    j=map->EList[i++];
                  }
                }
              }
              v+=3;
              p++;
            }
        }
      }
      MapFree(map);
    }
    
    {
      float *v0 = I->dot;
      float *n0 = I->dotNormal;
      int *dc0 = I->dotCode;
      int *p = dot_flag;
      int c=I->nDot;
      float *n = n0;
      float *v = v0;
      int *dc = dc0;
      int a;
      I->nDot=0;
      for(a=0;a<c;a++) {
        if(*(p++)) {
          *(v0++)=*(v++);
          *(n0++)=*(n++);
          *(v0++)=*(v++);
          *(n0++)=*(n++);
          *(v0++)=*(v++);
          *(n0++)=*(n++);
          *(dc0++)=*(dc++);
          I->nDot++;
        } else {
          v+=3;
          n+=3;
        }
      }
      FreeP(dot_flag);
      PRINTFD(G,FB_RepSurface)
        " SolventDotNew-DEBUG: %d->%d\n",c,I->nDot
        ENDFD;
    }
  }

#if 0
  {
    CGOReset(G->DebugCGO);
    CGOBegin(G->DebugCGO,GL_LINES);
    c=I->nDot;
    v = (v0=I->dot);
    n = (n0=I->dotNormal);
    for(a=0;a<c;a++) {
      float sum[3];
      add3f(v,n,sum);
      CGOVertexv(G->DebugCGO,v);
      CGOVertexv(G->DebugCGO,sum);
      v+=3;
      n+=3;
    }
    CGOEnd(G->DebugCGO);
  }
#endif

  return I; 
}


