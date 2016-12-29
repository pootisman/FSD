#ifndef _DATA_DRAWER_
#define _DATA_DRAWER_

#include <GL/glew.h>
#include <GL/gl.h>
#include <GLFW/glfw3.h>

#define CUBE 0x00
#define FLAT 0x0F
#define DISK 0xF0
#define SPHERE 0xFF

#define CUBE_POINTS 64
#define FPS 60.0f /* Frames per second we want */
#define ROT_SPEED 2.0f /* Rotation speed of our cube */

GLFWwindow *initRenderer(int width, int height);

GLfloat *initCube(unsigned short dim);
GLfloat *initFlat(unsigned short dim, GLfloat *(color));
GLfloat *initDisk(unsigned short dim);
GLfloat *initSphere(unsigned short dim);

void reloadVBO(void);
void renderSpectrum(void);

void terminateDrawer(void);
#endif
