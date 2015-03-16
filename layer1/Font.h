
/* 
A* -------------------------------------------------------------------
B* This file contains source code for the PyMOL computer program
C* copyright 1998-2003 by Warren Lyford Delano of DeLano Scientific. 
D* -------------------------------------------------------------------
E* It is unlawful to modify or remove this copyright notice.
F* -------------------------------------------------------------------
G* Please see the accompanying LICENSE file for further information. 
H* --------------------------------------------------\-----------------
I* Additional authors of this source file include:
-* 
-* 
-*
Z* -------------------------------------------------------------------
*/

#ifndef _H_Font
#define _H_Font

#include"PyMOLGlobals.h"
#include"Base.h"

#define cFontNULL        0
#define cFontGLUT        1
#define cFontBitmap      2
#define cFontVector      3
#define cFontExtrude     4
#define cFontPixmap      5
#define cFontTexture     6

#define cFontModePixels    0
#define cFontModeSpacial   1

typedef struct _CFont CFont;

#include "os_gl.h"

#define SHADERCGOARG , CGO *shaderCGO
#define SHADERCGOARGVAR ,shaderCGO
#define SHADERCGOARGV shaderCGO

typedef char *FontRenderOpenGLFn(RenderInfo * info, CFont *, const char *text, float size,
                                 float *rpos SHADERCGOARG);
typedef char *FontRenderRayFn(CRay * ray, CFont *, const char *text, float size, float *rpos);

struct _CFont {
  PyMOLGlobals *G;
  int TextID;
  void (*fFree) (CFont *);
  FontRenderOpenGLFn *fRenderOpenGL;
  FontRenderOpenGLFn *fRenderOpenGLFlat;
  FontRenderRayFn *fRenderRay;
};

int FontInit(PyMOLGlobals * G, CFont * I);

void FontPurge(CFont * I);

#endif
