#ifndef _DATA_DRAWER_
#define _DATA_DRAWER_

#include <GL/glew.h>
#include <GL/gl.h>
#include <GL/glx.h>
#include <X11/X.h>
#include <X11/Xlib.h>
#include <X11/Xatom.h>

#define CUBE 0x00
#define FLAT 0x0F
#define DISK 0xF0
#define SPHERE 0xFF

#define CUBE_POINTS 64
#define FPS 60.0f /* Frames per second we want */
#define ROT_SPEED 2.0f /* Rotation speed of our cube */

static const int Visual_attribs[] ={
  GLX_X_RENDERABLE    , True,
  GLX_DRAWABLE_TYPE   , GLX_WINDOW_BIT,
  GLX_RENDER_TYPE     , GLX_RGBA_BIT,
  GLX_X_VISUAL_TYPE   , GLX_TRUE_COLOR,
  GLX_RED_SIZE        , 8,
  GLX_GREEN_SIZE      , 8,
  GLX_BLUE_SIZE       , 8,
  GLX_ALPHA_SIZE      , 8,
  GLX_DEPTH_SIZE      , 24,
  GLX_STENCIL_SIZE    , 8,
  GLX_DOUBLEBUFFER    , True,
  GLX_SAMPLE_BUFFERS  , 1,
  GLX_SAMPLES         , 4,
  None
};

Window *initRenderer(int width, int height, int window_mode);

GLfloat *initCube(unsigned short dim, GLfloat rot_speed);
GLfloat *initFlat(unsigned short dim, GLfloat *(color));
GLfloat *initDisk(unsigned short dim);
GLfloat *initSphere(unsigned short dim);

void reloadVBO(void);
int renderSpectrum(void);

void terminateDrawer(void);
#endif
