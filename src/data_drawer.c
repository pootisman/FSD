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
#include "data_drawer.h"

GLuint VBO_buffers[2];
GLfloat *coords = NULL;
GLfloat *colors = NULL;
GLfloat deg_step = 0.5, rot_Angle = 0.0, rot_Axis[3] = {0.0, 1.0, 0.0}, rot_Speed = ROT_SPEED;

unsigned short dimensions = CUBE_POINTS;
unsigned char shape = CUBE;

GLFWwindow *render_target = NULL;

/* Sleep for this time between frames */
struct timespec sleeper = {0, 1e9 / FPS};

GLFWwindow *initRenderer(unsigned short width, unsigned short height)
{
  if(!glfwInit()) {
#ifdef DEBUG
    (void)puts("Severe failure, glfwInit() failed.");
#endif
    return NULL;
  }

  if(!(render_target = glfwCreateWindow(width, height, "FSD", glfwGetPrimaryMonitor(), NULL))) {
#ifdef DEBUG
    printf("Error while initializing graphics");
#endif
    glfwTerminate();
    return NULL;
  }

  glfwMakeContextCurrent(render_target);

  if(GLEW_OK != glewInit()) {
#ifdef DEBUG
    (void)puts("GLEW initialization failed!");
#endif
    glfwDestroyWindow(render_target);
    glfwTerminate();
    return NULL;
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
  glfwWindowHint(GLFW_SAMPLES, 8);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  glAlphaFunc(GL_NOTEQUAL, 0);
  glHint(GL_POINT_SMOOTH_HINT, GL_NICEST);

  #ifdef DEBUG
    (void)puts("Graphics ready!");
  #endif

  return render_target;
}

#ifndef DEBUG
inline
#endif
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

#ifndef DEBUG
inline
#endif
    void
    prepFlatVBO(void)
{
  glGenBuffers(1, &(VBO_buffers[0]));
  glBindBuffer(GL_ARRAY_BUFFER, VBO_buffers[0]);
  glBufferData(GL_ARRAY_BUFFER, dimensions * dimensions * 3 * sizeof(GLfloat), coords, GL_STATIC_DRAW);
  glVertexPointer(3, GL_FLOAT, 0, NULL);
  glEnableClientState(GL_VERTEX_ARRAY);

  glGenBuffers(1, &(VBO_buffers[1]));
  glBindBuffer(GL_ARRAY_BUFFER, VBO_buffers[1]);
  glBufferData(GL_ARRAY_BUFFER, dimensions * dimensions * 4 * sizeof(GLfloat), colors, GL_DYNAMIC_DRAW);
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
GLfloat *initCube(unsigned short dim)
{
  if(dim == 0) {
    dim = CUBE_POINTS;
  }

  unsigned int i = 0;
  dimensions = dim;
  colors = mmap(NULL, sizeof(GLfloat) * dimensions * dimensions * dimensions * 4, PROT_READ | PROT_WRITE,
                MAP_ANONYMOUS | MAP_SHARED, -1, 0);
  coords = calloc(dimensions * dimensions * dimensions * 3, sizeof(GLfloat));

  for(i = 0; i < dimensions * dimensions * dimensions; i++) {
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
 * Bar-like histogram
 *
GLfloat *initFlat(unsigned short dim)
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

  for(i = 0; i < dimensions * dimensions; i++) {
    X = (GLfloat)(i % dimensions) / (GLfloat)dimensions;
    Y = (GLfloat)((unsigned int)(floor((GLfloat)i / (GLfloat)dimensions)) % dimensions) / (GLfloat)(dimensions);
    Z = (GLfloat)floor((GLfloat)(i) / (GLfloat)(dimensions * dimensions)) / (GLfloat)dimensions;
    *(coords + i) = X + ;
    *(coords + i + 1) = Y + ;
    *(coords + i + 2) = Z + 0.0;
  }

  prepFlatVBO();

  (void)free(coords);

  shape = FLAT;

  return colors;
}
*/

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
  default: {
    (void)puts("Something is wrong, invalid VBO shape!");
    break;
  }
  }
}

void renderSpectrum(void)
{
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
  (void)glDrawArrays(GL_POINTS, 0, dimensions * dimensions * dimensions);
  (void)glPopMatrix();
  (void)glfwSwapBuffers(render_target);
  (void)nanosleep(&sleeper, NULL);
  (void)glfwPollEvents();
  rot_Angle += 360.0f * rot_Speed / FPS / 60.0f;
  (rot_Angle > 360.0f) ? (rot_Angle = rot_Angle - 360.0f) : (rot_Angle = rot_Angle);
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
  }
  default: {
    (void)puts("Something is wrong, invalid VBO shape, memory leak is likely!");
    break;
  }
  }

  glfwDestroyWindow(render_target);
  glfwTerminate();

#ifdef DEBUG
  (void)puts("Data drawer terminated");
#endif
}
