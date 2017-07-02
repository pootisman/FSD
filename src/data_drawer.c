#include <stdlib.h>
#include <stdio.h>
#include <memory.h>
#ifndef WIN32
#include <sys/mman.h>
#else
#include "mman.h"
#endif
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <math.h>
#include <time.h>
#include <signal.h>
#include <unistd.h>
#include "data_drawer.h"

GLuint VBO_buffers[2];
GLfloat *coords = NULL;
GLfloat *colors = NULL;
GLfloat deg_step = 0.5, rot_Angle = 0.0, rot_Axis[3] = {0.0, 1.0, 0.0}, rot_Speed = ROT_SPEED;

unsigned short dimensions = CUBE_POINTS;
unsigned char shape = CUBE;

Window win, root_win;
Display *dpy = NULL;
XVisualInfo *XVInfo = NULL;
int scr, fbcount = 0, xscreensaver_mode = 0;
GLXContext context;
GLXFBConfig fbconfig = 0;
XSetWindowAttributes winattr;
XEvent xev;

/* Sleep for this time between frames */
struct timespec sleeper = {0, 1e9 / FPS};

void fullscreen(Display* dpy, Window win) {
  Atom atoms[2] = { XInternAtom(dpy, "_NET_WM_STATE_FULLSCREEN", False), None };
  XChangeProperty(dpy, win, XInternAtom(dpy, "_NET_WM_STATE", False), XA_ATOM, 32, PropModeReplace, atoms, 1);
}

void term_handle(int sig){
#ifdef DEBUG
  (void)printf("Received signal %d", sig);
  fflush(stdout);
#endif
  sleep(10);
  terminateDrawer();
}

Window *initRenderer(int width, int height, int window_mode)
{
  int attribs[100];
  GLXFBConfig *configs = NULL;
  xscreensaver_mode = window_mode;
  
  signal(SIGTERM, &term_handle);
  
  dpy = XOpenDisplay(NULL);
  
  if(dpy == NULL) {
#ifdef DEBUG
    (void)puts("Connection to X server failed.");
#endif
    return NULL;
  }

  scr = XDefaultScreen(dpy);
  
  memcpy(&(attribs[0]), &(Visual_attribs), sizeof(Visual_attribs));
  
  configs = glXChooseFBConfig(dpy, scr, &(attribs[0]), &fbcount);
  
  if(configs == NULL || fbcount <= 0){
    (void)puts("Error, requested videomode not available");
    XCloseDisplay(dpy);
    return NULL;
  }
  
  fbconfig = *(configs);
  XFree(configs);
  
  XVInfo = glXGetVisualFromFBConfig(dpy, fbconfig);
  
  if(XVInfo == NULL){
    (void)puts("Error, failed to get visual info for FB configuration");
    XCloseDisplay(dpy);
    return NULL;
  }
  

  if(width <= 0 || height <= 0){
    width = XWidthOfScreen(XScreenOfDisplay(dpy, scr));
    height = XHeightOfScreen(XScreenOfDisplay(dpy, scr));

#ifdef DEBUG
    (void)printf("Invalid W,H set to %d,%d\n", width, height);
#endif
  }

#ifdef DEBUG
  (void)printf("W,H set to %d,%d\n", width, height);
#endif

  root_win = DefaultRootWindow(dpy);

  winattr.background_pixel = 0;
  winattr.border_pixel = 0;
  winattr.background_pixmap = None;
  winattr.event_mask = StructureNotifyMask | ExposureMask | KeyPressMask;
  
  winattr.colormap = XCreateColormap(dpy, root_win, XVInfo->visual, AllocNone);

  if(xscreensaver_mode == 0){
    win = XCreateWindow(dpy, root_win, 0, 0, width, height, 0, XVInfo->depth,
        InputOutput, XVInfo->visual, CWColormap | CWEventMask, &winattr);
  }else{
    win = root_win;
  }

  context = glXCreateContext(dpy, XVInfo, NULL, GL_TRUE);

  glXMakeCurrent(dpy, win, context);
  
  if(GLEW_OK != glewInit()) {
#ifdef DEBUG
    (void)puts("GLEW initialization failed!");
#endif
    glXDestroyContext(dpy, context);
    XDestroyWindow(dpy, win);
    XCloseDisplay(dpy);
    return NULL;
  }


  if(xscreensaver_mode == 0){
    XMapWindow(dpy, win);
    fullscreen(dpy, win);
    XStoreName(dpy, win, "FSD");
  }



  glClearColor(0.0, 0.0, 0.0, 0.0);
  glClear(GL_COLOR_BUFFER_BIT);
  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();
  glPointSize(2.0);
  glViewport(0, 0, width, height);
  gluPerspective(75, (float)width / (float)height, 1.0, 10);
  gluLookAt(1.0, 1.0, 1.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0);
  glEnable(GL_POINT_SMOOTH);
  glEnable(GL_BLEND);
  glEnable(GL_ALPHA_TEST);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  glAlphaFunc(GL_NOTEQUAL, 0);
  glHint(GL_POINT_SMOOTH_HINT, GL_NICEST);

  #ifdef DEBUG
    (void)puts("Graphics ready!");
  #endif

  return &win;
}


void
prepCubeVBO(void)
{
  glGenBuffers(1, &(VBO_buffers[0]));
  glBindBuffer(GL_ARRAY_BUFFER, VBO_buffers[0]);
  glBufferData(GL_ARRAY_BUFFER, dimensions * dimensions * dimensions * 3 * sizeof(GLfloat), coords, GL_STATIC_DRAW);
  glVertexPointer(3, GL_FLOAT, 0, NULL);
  glEnableClientState(GL_VERTEX_ARRAY);

  glGenBuffers(1, &(VBO_buffers[1]));
  glBindBuffer(GL_ARRAY_BUFFER, VBO_buffers[1]);
  glBufferData(GL_ARRAY_BUFFER, dimensions * dimensions * dimensions * 4 * sizeof(GLfloat), colors, GL_DYNAMIC_DRAW);
  glColorPointer(4, GL_FLOAT, 0, NULL);
  glEnableClientState(GL_COLOR_ARRAY);

  glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void
prepFlatVBO(void)
{
  glGenBuffers(1, &(VBO_buffers[0]));
  glBindBuffer(GL_ARRAY_BUFFER, VBO_buffers[0]);
  glBufferData(GL_ARRAY_BUFFER, dimensions * dimensions * 3 * sizeof(GLfloat),
              coords, GL_STATIC_DRAW);
  glVertexPointer(3, GL_FLOAT, 0, NULL);
  glEnableClientState(GL_VERTEX_ARRAY);

  glGenBuffers(1, &(VBO_buffers[1]));
  glBindBuffer(GL_ARRAY_BUFFER, VBO_buffers[1]);
  glBufferData(GL_ARRAY_BUFFER, dimensions * dimensions * 4 * sizeof(GLfloat),
              colors, GL_DYNAMIC_DRAW);
  glColorPointer(4, GL_FLOAT, 0, NULL);
  glEnableClientState(GL_COLOR_ARRAY);

  glBindBuffer(GL_ARRAY_BUFFER, 0);
}

#ifndef DEBUG
inline
#endif
void
prepDiskVBO(void)
{
  glGenBuffers(1, &(VBO_buffers[0]));
  glBindBuffer(GL_ARRAY_BUFFER, VBO_buffers[0]);
  glBufferData(GL_ARRAY_BUFFER, dimensions * (360.0 / deg_step) * 3 * sizeof(GLfloat), coords, GL_STATIC_DRAW);
  glVertexPointer(3, GL_FLOAT, 0, NULL);
  glEnableClientState(GL_VERTEX_ARRAY);

  glGenBuffers(1, &(VBO_buffers[1]));
  glBindBuffer(GL_ARRAY_BUFFER, VBO_buffers[1]);
  glBufferData(GL_ARRAY_BUFFER, dimensions * dimensions * dimensions * 4 * sizeof(GLfloat), colors, GL_DYNAMIC_DRAW);
  glColorPointer(4, GL_FLOAT, 0, NULL);
  glEnableClientState(GL_COLOR_ARRAY);

  glBindBuffer(GL_ARRAY_BUFFER, 0);
}

#ifndef DEBUG
inline
#endif
void
prepSphereVBO(void)
{
  glGenBuffers(1, &(VBO_buffers[0]));
  glBindBuffer(GL_ARRAY_BUFFER, VBO_buffers[0]);
  glBufferData(GL_ARRAY_BUFFER, dimensions * dimensions * 3 * sizeof(GLfloat), coords, GL_STATIC_DRAW);
  glVertexPointer(3, GL_FLOAT, 0, NULL);
  glEnableClientState(GL_VERTEX_ARRAY);

  glGenBuffers(1, &(VBO_buffers[1]));
  glBindBuffer(GL_ARRAY_BUFFER, VBO_buffers[1]);
  glBufferData(GL_ARRAY_BUFFER, dimensions * dimensions * dimensions * 4 * sizeof(GLfloat), colors, GL_DYNAMIC_DRAW);
  glColorPointer(4, GL_FLOAT, 0, NULL);
  glEnableClientState(GL_COLOR_ARRAY);

  glBindBuffer(GL_ARRAY_BUFFER, 0);
}

/*
 * Cube made out of points
 */
GLfloat *initCube(unsigned short dim, GLfloat rot_speed)
{
  if(dim == 0) {
    dim = CUBE_POINTS;
  }

  rot_Speed = rot_speed;

  unsigned int i = 0;
  dimensions = dim;
  colors = mmap(NULL, sizeof(GLfloat) * dimensions * dimensions * dimensions * 4, PROT_READ | PROT_WRITE,
                MAP_ANONYMOUS | MAP_SHARED, -1, 0);
  coords = calloc(dimensions * dimensions * dimensions * 3, sizeof(GLfloat));

  for(i = 0; i < (unsigned int)(dimensions * dimensions * dimensions); i++) {
    *(coords + i * 3) = (GLfloat)(i % dimensions) / (GLfloat)dimensions;
    *(coords + i * 3 + 1) =
        (GLfloat)((unsigned int)(floor((GLfloat)i / (GLfloat)dimensions)) % dimensions) / (GLfloat)(dimensions);
    *(coords + i * 3 + 2) =
        floor((GLfloat)(i) / (GLfloat)(dimensions * dimensions)) / (GLfloat)dimensions;
  }

  prepCubeVBO();

  (void)free(coords);

  shape = CUBE;

  return colors;
}

/*
 * Bar-like histogram, drawn with cubes,
 * Returns pointer to coordinates of the cube vertexes
 */
GLfloat *initFlat(unsigned short dim, GLfloat *(color))
{
  if(dim == 0) {
    dim = CUBE_POINTS;
  }

  unsigned int i = 0;
  GLfloat X, Y, Z;
  dimensions = dim;
  colors = mmap(NULL, sizeof(GLfloat) * dimensions * dimensions * 8 * 4, PROT_READ | PROT_WRITE,
                MAP_ANONYMOUS | MAP_SHARED, -1, 0);
  coords = calloc(dimensions * dimensions * 3 * 8, sizeof(GLfloat));

  for(i = 0; i < (unsigned int)(dimensions * dimensions); i++) {
    X = (GLfloat)(i % dimensions) / (GLfloat)dimensions;
    Y = (GLfloat)((unsigned int)(floor((GLfloat)i / (GLfloat)dimensions)) % dimensions) / (GLfloat)(dimensions);
    Z = (GLfloat)floor((GLfloat)(i) / (GLfloat)(dimensions * dimensions)) / (GLfloat)dimensions;

    /*
     * Bottom edge
     */
    *(coords + i) = X + 0.75 * 1/(GLfloat)dim;
    *(coords + i + 1) = Y + 0.75 * 1.0/(GLfloat)dim;
    *(coords + i + 2) = Z + 0.0;

    *(coords + i) = X - 0.75 * 1/(GLfloat)dim;
    *(coords + i + 1) = Y + 0.75 * 1.0/(GLfloat)dim;
    *(coords + i + 2) = Z + 0.0;

    *(coords + i) = X + 0.75 * 1/(GLfloat)dim;
    *(coords + i + 1) = Y - 0.75 * 1.0/(GLfloat)dim;
    *(coords + i + 2) = Z + 0.0;

    *(coords + i) = X - 0.75 * 1/(GLfloat)dim;
    *(coords + i + 1) = Y - 0.75 * 1.0/(GLfloat)dim;
    *(coords + i + 2) = Z + 0.0;

    /*
     * Top edge
     */
    *(coords + i) = X + 0.75 * 1/(GLfloat)dim;
    *(coords + i + 1) = Y + 0.75 * 1.0/(GLfloat)dim;
    *(coords + i + 2) = Z + 0.0;

    *(coords + i) = X - 0.75 * 1/(GLfloat)dim;
    *(coords + i + 1) = Y + 0.75 * 1.0/(GLfloat)dim;
    *(coords + i + 2) = Z + 0.0;

    *(coords + i) = X + 0.75 * 1/(GLfloat)dim;
    *(coords + i + 1) = Y - 0.75 * 1.0/(GLfloat)dim;
    *(coords + i + 2) = Z + 0.0;

    *(coords + i) = X - 0.75 * 1/(GLfloat)dim;
    *(coords + i + 1) = Y - 0.75 * 1.0/(GLfloat)dim;
    *(coords + i + 2) = Z + 0.0;
  }

  for(i = 0; i < (unsigned int)(dimensions * dimensions * dimensions * 8); i++){
    *(colors + i) = *(color);
    *(colors + i + 1) = *(color + 1);
    *(colors + i + 2) = *(color + 2);
    *(colors + i + 3) = (GLfloat)0.0;
  }

  prepFlatVBO();

  (void)free(colors);

  shape = FLAT;

  return coords;
}

void reloadVBO(void)
{
  GLfloat *ptr = NULL;
  switch(shape) {
    case(CUBE): {
      glBindBuffer(GL_ARRAY_BUFFER, VBO_buffers[1]);
      ptr = glMapBuffer(GL_ARRAY_BUFFER, GL_WRITE_ONLY);
      memcpy(ptr, colors, sizeof(GLfloat) * 4 * dimensions * dimensions * dimensions);
      glUnmapBuffer(GL_ARRAY_BUFFER);
      glBindBuffer(GL_ARRAY_BUFFER, 0);
      break;
    }
    case(FLAT):{
      glBindBuffer(GL_ARRAY_BUFFER, VBO_buffers[0]);
      ptr = glMapBuffer(GL_ARRAY_BUFFER, GL_WRITE_ONLY);
      memcpy(ptr, coords, sizeof(GLfloat) * 3 * 8 * dimensions * dimensions);
      glUnmapBuffer(GL_ARRAY_BUFFER);
      glBindBuffer(GL_ARRAY_BUFFER, 0);
      break;
    }
    default: {
      (void)puts("Something is wrong, invalid VBO shape!");
      break;
    }
  }
}

int renderSpectrum(void){
  (void)glClear(GL_COLOR_BUFFER_BIT);
  (void)glPushMatrix();
  /* Move the cube to the position */
  (void)glTranslatef(-0.2, -0.2, -0.2);
  /* Rotate around Z axis */
  (void)glRotatef(rot_Angle, rot_Axis[0], rot_Axis[1], rot_Axis[2]);
  /* Translate to origin */
  (void)glTranslatef(-0.5, -0.5, -0.5);
  glBindBuffer(GL_ARRAY_BUFFER, 0);
  glBindBuffer(GL_ARRAY_BUFFER, VBO_buffers[0]);
  glVertexPointer(3, GL_FLOAT, 0, NULL);
  glBindBuffer(GL_ARRAY_BUFFER, VBO_buffers[1]);
  glColorPointer(4, GL_FLOAT, 0, NULL);
  switch(shape){
    case(CUBE):{
      (void)glDrawArrays(GL_POINTS, 0, dimensions * dimensions * dimensions);
      break;
    }
  }
  (void)glPopMatrix();
  
  (void)glXSwapBuffers(dpy, win);
#ifndef XSCREENSAVER
  (void)nanosleep(&sleeper, NULL);
#endif
  rot_Angle += 360.0f * rot_Speed / FPS / 60.0f;
  (rot_Angle > 360.0f) ? (rot_Angle = rot_Angle - 360.0f) : (rot_Angle = rot_Angle);

  if(XCheckMaskEvent(dpy, KeyPressMask, &xev) == True){
    if(xscreensaver_mode == 0){
      terminateDrawer();
    }
    return 1;
  }else{
    return 0;
  }
}

void terminateDrawer()
{
#ifdef DEBUG
  (void)puts("Terminating data drawer");
#endif
  switch(shape) {
  case(CUBE): {
    glBindBuffer(GL_ARRAY_BUFFER, VBO_buffers[0]);
    glDeleteBuffers(1, &(VBO_buffers[0]));
    glBindBuffer(GL_ARRAY_BUFFER, VBO_buffers[1]);
    glDeleteBuffers(1, &(VBO_buffers[1]));
    (void)munmap(colors, dimensions * dimensions * dimensions * 4 * sizeof(GLfloat));
    break;
  }
  default: {
    (void)puts("Something is wrong, invalid VBO shape, memory leak is likely!");
    break;
  }
  }
  glXMakeCurrent(dpy, None, NULL);
  glXDestroyContext(dpy, context);
  XUnmapWindow(dpy, win);
  if(xscreensaver_mode == 0){
    XDestroyWindow(dpy, win);
  }
  XCloseDisplay(dpy);
  XFree(XVInfo);
#ifdef DEBUG
  (void)puts("Data drawer terminated");
#endif
}
