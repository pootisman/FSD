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

typedef struct CUBE_PATTERN{
  unsigned long int R_dims[3];
  unsigned long int G_dims[3];
  unsigned long int B_dims[3];
  double *R,*G,*B;
}CUBE_PATTERN;

void vinyl_prep(unsigned long int window_size, unsigned int pps, unsigned char loop, GLfloat *spectrum,
unsigned char spec_dim, unsigned char *stop, GLfloat R, GLfloat G, GLfloat B, unsigned char win_figure, unsigned int reads_per_sec, double timeout);
int load_pattern(char *fname);
void vinyl_stop(void);
void update_stats(unsigned char *vector1, unsigned char *vector2);
void update_stats_simple(unsigned char *vector1, double maxdist);


#endif
