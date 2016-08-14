#ifndef _VINYL_H_
#define _VINYL_H_

#include <GL/gl.h>

#define MIN_PPS 10
#define MAXTIMEOUT 5

typedef struct VINYL_PARAMS{
  /* Window size is in points, not bytes! */
  unsigned long int win_size;
  unsigned long int win_pos;
  unsigned long int pps;
  unsigned char *dest;
  unsigned char *stop;
  unsigned char loop;
  unsigned char spec_dimensions;
  unsigned char win_figure;
  double timeout;
  GLfloat *spectrum;
  GLfloat R,G,B;
}VINYL_PARAMS;

void vinyl_prep(unsigned long int window_size, unsigned int pps, unsigned char loop, GLfloat *spectrum,
unsigned char spec_dim, unsigned char *stop, GLfloat R, GLfloat G, GLfloat B, unsigned char win_figure, unsigned int reads_per_sec, double timeout);
int vinyl_read(char *root);
void vinyl_stop(void);

#endif
