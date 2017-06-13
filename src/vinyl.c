#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <dirent.h>
#include <string.h>
#include <GL/gl.h>
#include <unistd.h>
#include <limits.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <memory.h>
#include <time.h>
#include <sys/types.h>
#include <signal.h>
#include "vinyl.h"

VINYL_PARAMS *params = NULL;
CUBE_PATTERN *pattern = NULL;
double *stat_intern = NULL;

double latest_max_var = 0.0;
double latest_max_mean = 0.0;
double latest_max_kurt = 0.0;

double curr_variance = 0.0;
double curr_mean = 0.0;
double curr_kurt = 0.0;

double prev_variance = 0.0;
double prev_mean = 0.0;
double prev_kurt = 0.0;

struct timespec win_timer = {0, 1e9/600};

void abrt_handle(int sig){
#ifdef DEBUG
  (void)printf("Received signal %d", sig);
  fflush(stdout);
#endif
  if(sig == SIGABRT){
    *(params->stop) = 1;
  }
}

void vinyl_prep(unsigned long int window_size, unsigned int pps, unsigned char loop, GLfloat *spectrum,
unsigned char spec_dim, unsigned char *stop, GLfloat R, GLfloat G, GLfloat B, unsigned char win_figure, unsigned int reads_per_sec, double timeout){
  params = calloc(1, sizeof(VINYL_PARAMS));
  params->win_size = (window_size >= 1e1) ? (window_size) : (1e1);
  params->win_pos = 0;
  params->dest = calloc(window_size * 3, sizeof(unsigned char));
  params->pps = (pps == 0) ? (MIN_PPS) : (pps);
  params->loop = loop;
  params->spectrum = spectrum;
  params->spec_dimensions = spec_dim;
  params->stop = stop;
  params->R = (R >= 0.1 && R <= 1.0) ? (R) : (1.0);
  params->G = (G >= 0.1 && G <= 1.0) ? (G) : (1.0);
  params->B = (B >= 0.1 && B <= 1.0) ? (B) : (1.0);
  params->win_figure = win_figure;
  params->dynamic = 0;
  params->timeout = (timeout > 0) ? (timeout) : (MAXTIMEOUT);

  stat_intern = calloc(spec_dim*spec_dim*spec_dim, sizeof(double));
  win_timer.tv_nsec = 1e9/reads_per_sec;
  signal(SIGABRT, &abrt_handle);
}

void init_mean(void){
  unsigned int i = 0;
  for(i = 0; i < params->win_size * 3; i++){
    curr_mean += params->dest[i]/(params->win_size * 3);
  }

  prev_mean = curr_mean;
  latest_max_mean = 0.0;

  params->R = (GLfloat)(fabs(curr_mean - prev_mean));
#ifdef DEBUG
  (void)printf("New R is %f\n", params->R);
#endif
}

void vinyl_dynamic_coloring(unsigned char dynamic){
  params->dynamic = dynamic;
}

void update_dynamic_coloring(unsigned char *vector_add, unsigned char *vector_rm){
  unsigned int i = 0;
  for(i = 0; i < params->pps; i++){
    curr_mean = curr_mean - (double)(vector_rm[i])/(params->win_size * 3) + (double)(vector_add[i])/(params->win_size * 3);
  }

  if(fabs(curr_mean - prev_mean) > latest_max_mean){
    latest_max_mean = fabs(curr_mean - prev_mean);
  }

  params->R = (GLfloat)(1.0 - fabs(curr_mean - prev_mean)/latest_max_mean);
#ifdef DEBUG
  (void)printf("New R is %f\n", params->R);
  (void)fflush(stdout);
#endif
}

/* Load pattern that will be displayed in the cube */
int load_pattern(char *fname){
  FILE *I = fopen(fname, "r");
  unsigned int red_total = 0, green_total = 0, blue_total = 0, j = 0;
  unsigned long int x = 0, y = 0, z = 0;
  unsigned char red_steps[3] = {0}, green_steps[3] = {0}, blue_steps[3] = {0};

  if(I == NULL){
    (void)puts("Error! Failed to load pattern!");
    return -1;
  }
  
  pattern = calloc(1, sizeof(CUBE_PATTERN));
  
  if(3 != fscanf(I, "%ld %ld %ld\n", &(pattern->R_dims[0]), &(pattern->R_dims[1]), &(pattern->R_dims[2])) ||
     3 != fscanf(I, "%ld %ld %ld\n", &(pattern->G_dims[0]), &(pattern->G_dims[1]), &(pattern->G_dims[2])) ||
     3 != fscanf(I, "%ld %ld %ld\n\n", &(pattern->B_dims[0]), &(pattern->B_dims[1]), &(pattern->B_dims[2]))){
    (void)puts("Error! Failed to load pattern!");
    return -1;
  }
  
  red_total = pattern->R_dims[0] * pattern->R_dims[1] * pattern->R_dims[2];
  green_total = pattern->G_dims[0] * pattern->G_dims[1] * pattern->G_dims[2];
  blue_total = pattern->B_dims[0] * pattern->B_dims[1] * pattern->B_dims[2];

  pattern->R = calloc(red_total, sizeof(double));
  pattern->G = calloc(green_total, sizeof(double));
  pattern->B = calloc(blue_total, sizeof(double));

  for(j = 0; j < red_total - 1; j++){
    if(j % pattern->R_dims[0] == 0 && j > 0){
      if(fscanf(I,"\n%lf ", pattern->R + j)){
        continue;
      }else{
        (void)puts("ERROR: Failed to fully read red pattern component from file!");
      }
    }else if(j % (pattern->R_dims[0] * pattern->R_dims[1]) == 0 && j > 0){
      if(fscanf(I,"\n\n%lf", pattern->R + j)){
        continue;
      }else{
        (void)puts("ERROR: Failed to fully read red pattern component from file!");
      }
    }else{
      if(fscanf(I,"%lf ", pattern->R + j)){
        continue;
      }else{
        (void)puts("ERROR: Failed to fully read red pattern component from file!");
      }
    }
  }
  if(fscanf(I, "%lf\n\n", pattern->R + j) < 1){
    (void)puts("ERROR: Failed to fully read red pattern component from file!");
  }

  for(j = 0; j < green_total - 1; j++){
    if(j % pattern->G_dims[0] == 0 && j > 0){
      if(fscanf(I,"\n%lf ", pattern->G + j)){
        continue;
      }else{
        (void)puts("ERROR: Failed to fully read red pattern component from file!");
      }
    }else if(j % (pattern->G_dims[0] * pattern->G_dims[1]) == 0 && j > 0){
      if(fscanf(I,"\n\n%lf", pattern->G + j)){
        continue;
      }else{
        (void)puts("ERROR: Failed to fully read red pattern component from file!");
      }
    }else{
      if(fscanf(I,"%lf ", pattern->G + j)){
        continue;
      }else{
        (void)puts("ERROR: Failed to fully read red pattern component from file!");
      }
    }
  }
  if(fscanf(I, "%lf\n\n", pattern->G + j) < 1){
    (void)puts("ERROR: Failed to fully read red pattern component from file!");
  }
  
  for(j = 0; j < blue_total - 1; j++){
    if(j % pattern->B_dims[0] == 0 && j > 0){
      if(fscanf(I,"\n%lf ", pattern->B + j)){
        continue;
      }else{
        (void)puts("ERROR: Failed to fully read red pattern component from file!");
      }
    }else if(j % (pattern->B_dims[0] * pattern->B_dims[1]) == 0 && j > 0){
      if(fscanf(I,"\n\n%lf", pattern->B + j)){
        continue;
      }else{
        (void)puts("ERROR: Failed to fully read red pattern component from file!");
      }
    }else{
      if(fscanf(I,"%lf ", pattern->B + j)){
        continue;
      }else{
        (void)puts("ERROR: Failed to fully read red pattern component from file!");
      }
    }
  }
  if(fscanf(I, "%lf\n\n", pattern->B + j) < 1){
    (void)puts("ERROR: Failed to fully read red pattern component from file!");
  }

  (void)fclose(I);
  
  (void)puts("Pattern loaded");
  
  red_steps[0] = (unsigned long int)floor(params->spec_dimensions/pattern->R_dims[0]);
  red_steps[1] = (unsigned long int)floor(params->spec_dimensions/pattern->R_dims[1]);
  red_steps[2] = (unsigned long int)floor(params->spec_dimensions/pattern->R_dims[2]);

  green_steps[0] = (unsigned long int)floor(params->spec_dimensions/pattern->G_dims[0]);
  green_steps[1] = (unsigned long int)floor(params->spec_dimensions/pattern->G_dims[1]);
  green_steps[2] = (unsigned long int)floor(params->spec_dimensions/pattern->G_dims[2]);

  blue_steps[0] = (unsigned long int)floor(params->spec_dimensions/pattern->B_dims[0]);
  blue_steps[1] = (unsigned long int)floor(params->spec_dimensions/pattern->B_dims[1]);
  blue_steps[2] = (unsigned long int)floor(params->spec_dimensions/pattern->B_dims[2]);
  
  /*
   * Fill the cube layer by layer [R,G,B]
   */
  for(j = 0; j < (unsigned long int)(params->spec_dimensions * params->spec_dimensions * params->spec_dimensions); j++){
    x = (unsigned long int)floor((j % params->spec_dimensions) / red_steps[0]);
    x = (x >= pattern->R_dims[0]) ? (pattern->R_dims[0] - 1) : (x);
    y = (unsigned long int)floor((j % (params->spec_dimensions * params->spec_dimensions)) / params->spec_dimensions / red_steps[1]);
    y = (y >= pattern->R_dims[1]) ? (pattern->R_dims[1] - 1) : (y);
    z = (unsigned long int)floor(j / (params->spec_dimensions * params->spec_dimensions) / red_steps[2]);
    z = (z >= pattern->R_dims[2]) ? (pattern->R_dims[2] - 1) : (z);
    //(void)printf("Coords: %ld %ld %ld\n", x, y, z);
    *(params->spectrum + j * 4) = (GLfloat)*(pattern->R + x + y * pattern->R_dims[0] + z * pattern->R_dims[0] * pattern->R_dims[1]);
  }
  
  for(j = 0; j < (unsigned long int)(params->spec_dimensions * params->spec_dimensions * params->spec_dimensions); j++){
    x = (unsigned long int)floor((j % params->spec_dimensions) / green_steps[0]);
    x = (x >= pattern->G_dims[0]) ? (pattern->G_dims[0] - 1) : (x);
    y = (unsigned long int)floor((j % (params->spec_dimensions * params->spec_dimensions)) / params->spec_dimensions / green_steps[1]);
    y = (y >= pattern->G_dims[1]) ? (pattern->G_dims[1] - 1) : (y);
    z = (unsigned long int)floor(j / (params->spec_dimensions * params->spec_dimensions) / green_steps[2]);
    z = (z >= pattern->G_dims[2]) ? (pattern->G_dims[2] - 1) : (z);
    *(params->spectrum + j * 4 + 1) = (GLfloat)*(pattern->G + x + y * pattern->G_dims[0] + z * pattern->G_dims[0] * pattern->G_dims[1]);
  }
  
  for(j = 0; j < (unsigned long int)(params->spec_dimensions * params->spec_dimensions * params->spec_dimensions); j++){
    x = (unsigned long int)floor((j % params->spec_dimensions) / blue_steps[0]);
    x = (x >= pattern->B_dims[0]) ? (pattern->B_dims[0] - 1) : (x);
    y = (unsigned long int)floor((j % (params->spec_dimensions * params->spec_dimensions)) / params->spec_dimensions / blue_steps[1]);
    y = (y >= pattern->B_dims[1]) ? (pattern->B_dims[1] - 1) : (y);
    z = (unsigned long int)floor(j / (params->spec_dimensions * params->spec_dimensions) / blue_steps[2]);
    z = (z >= pattern->B_dims[2]) ? (pattern->B_dims[2] - 1) : (z);
    *(params->spectrum + j * 4 + 2) = (GLfloat)*(pattern->B + x + y * pattern->B_dims[0] + z * pattern->B_dims[0] * pattern->B_dims[1]);
  }
  
  return EXIT_SUCCESS;
}

void update_stats(unsigned char *vector1, unsigned char
*vector2){
  unsigned long int i = 0, max = 0;

  /* Modify local stats */
  for(i = 0; i < params->pps; i++){
    *(stat_intern + vector1[i*3]/4 + vector1[i*3 + 1]/4 *
    (params->spec_dimensions - 1) + vector1[i*3 + 2]/4 * params->spec_dimensions
    * (params->spec_dimensions - 1)) -= 1;

    if(*(stat_intern + vector1[i*3]/4 + vector1[i*3 + 1]/4 *
    (params->spec_dimensions - 1) + vector1[i*3 + 2]/4 * params->spec_dimensions
    * (params->spec_dimensions - 1)) < 0){
      *(stat_intern + vector1[i*3]/4 + vector1[i*3 + 1]/4 *
      (params->spec_dimensions - 1) + vector1[i*3 + 2]/4 * params->spec_dimensions
      * (params->spec_dimensions - 1)) = 0;
    }
  }

  for(i = 0; i < params->pps; i++){
    *(stat_intern + vector2[i*3]/4 + vector2[i*3 + 1]/4 *
    (params->spec_dimensions - 1) + vector2[i*3 + 2]/4 * params->spec_dimensions
    * (params->spec_dimensions - 1)) += 1;

    if(*(stat_intern + vector2[i*3]/4 + vector2[i*3 + 1]/4 *
    (params->spec_dimensions - 1) + vector2[i*3 + 2]/4 * params->spec_dimensions
    * (params->spec_dimensions - 1)) > LONG_MAX){
      *(params->spectrum + vector2[i*3]/4 + vector2[i*3 + 1]/4 *
      (params->spec_dimensions - 1) + vector2[i*3 + 2]/4 * params->spec_dimensions
      * (params->spec_dimensions - 1)) = LONG_MAX;
    }
  }

  /* Find new maximum */
  for(i = 0; i < (unsigned long int)(params->spec_dimensions*params->spec_dimensions*params->spec_dimensions); i++){
    if(max < *(stat_intern + i)){
      max = *(stat_intern + i);
    }
  }

  /* Update the actual stats */
  for(i = 0; i < (unsigned long int)(params->spec_dimensions*params->spec_dimensions*params->spec_dimensions); i++){
    *(params->spectrum + i*4 + 3) = (GLfloat)(logf(*(stat_intern + i))/logf(max));
    if(pattern == NULL){
      *(params->spectrum + i*4) = (GLfloat)params->R;
      *(params->spectrum + i*4 + 1) = (GLfloat)params->G;
      *(params->spectrum + i*4 + 2) = (GLfloat)params->B;
    }
  }
}


void update_stats_simple(unsigned char *vector1, double maxdist){
  unsigned long int i = 0;
  float temp = 0.0;
  struct timespec times;

  clock_gettime(CLOCK_MONOTONIC_RAW, &times);

  /* Update the points of interest */
  for(i = 0; i < params->pps; i++){
    *(stat_intern + vector1[i*3]/4 + vector1[i*3 + 1]/4 *
    (params->spec_dimensions - 1) + vector1[i*3 + 2]/4 * params->spec_dimensions
    * (params->spec_dimensions - 1)) = times.tv_sec + (1e-9)*times.tv_nsec;
  }

  /* Update the actual stats and attenuate old points */
  for(i = 0; i < (unsigned long int)(params->spec_dimensions*params->spec_dimensions*params->spec_dimensions); i++){
    temp = (float)(times.tv_sec + (1e-9)*times.tv_nsec - *(stat_intern + i))/(float)maxdist;
    *(params->spectrum + i*4 + 3) = (temp >= 1.0) ? (GLfloat)(0.0) : (GLfloat)(1.0 - temp);
    if(pattern == NULL){
      *(params->spectrum + i*4) = (GLfloat)params->R;
      *(params->spectrum + i*4 + 1) = (GLfloat)params->G;
      *(params->spectrum + i*4 + 2) = (GLfloat)params->B;
    }
  }
}

void vinyl_stop(void){
  (void)free(params->dest);
  (void)free(params);
  if(pattern){
    (void)free(pattern->R);
    (void)free(pattern->G);
    (void)free(pattern->B);
    (void)free(pattern);
  }
  (void)free(stat_intern);
#ifdef DEBUG
  (void)puts("Vinyl stop");
#endif
}
