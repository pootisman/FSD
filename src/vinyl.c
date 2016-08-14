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
#include "vinyl.h"

VINYL_PARAMS *params = NULL;
double *stat_intern = NULL;
struct timespec win_timer = {0, 1e9/600};

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
  params->R = R;
  params->G = G;
  params->B = B;
  params->win_figure = win_figure;
  params->timeout = timeout;
  stat_intern = calloc(spec_dim*spec_dim*spec_dim, sizeof(double));
  win_timer.tv_nsec = 1e9/reads_per_sec;
}

#ifndef DEBUG
inline
#endif
void update_stats(unsigned char *vector1, unsigned char 
*vector2){
  unsigned long int i = 0, max = 0;
  float mean = 0.0f, variance = 0.0f, entropy = 0.0f;
  
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
  for(i = 0; i < params->spec_dimensions*params->spec_dimensions*params->spec_dimensions; i++){
    if(max < *(stat_intern + i)){
      max = *(stat_intern + i);
    }
  }
  
  /* Calculate variance, mean, entropy and decide on color that we must choose 
  for(i = 0; i < params->spec_dimensions * params.spec_dimensions * params->spec_dimensions; i++){
    variance += (float)*(stat_intern)
  }
  */
  
  /* Update the actual stats */
  for(i = 0; i < params->spec_dimensions*params->spec_dimensions*params->spec_dimensions; i++){
    *(params->spectrum + i*4 + 3) = (GLfloat)(logf(*(stat_intern + i))/logf(max));
    *(params->spectrum + i*4) = (GLfloat)params->R;
    *(params->spectrum + i*4 + 1) = (GLfloat)params->G;
    *(params->spectrum + i*4 + 2) = (GLfloat)params->B;
  }
}

#ifndef DEBUG
inline
#endif
void update_stats_simple(unsigned char *vector1, double maxdist){
  unsigned long int i = 0;
  float temp = 0.0;
  struct timespec times;
  
  clock_gettime(CLOCK_MONOTONIC_RAW, &times);
  
  for(i = 0; i < params->pps; i++){
    *(stat_intern + vector1[i*3]/4 + vector1[i*3 + 1]/4 * 
    (params->spec_dimensions - 1) + vector1[i*3 + 2]/4 * params->spec_dimensions 
    * (params->spec_dimensions - 1)) = times.tv_sec + (1e-9)*times.tv_nsec;
  }
  
  /* Update the actual stats */
  for(i = 0; i < params->spec_dimensions*params->spec_dimensions*params->spec_dimensions; i++){
    temp = (float)(times.tv_sec + (1e-9)*times.tv_nsec - *(stat_intern + i))/(float)maxdist;
    *(params->spectrum + i*4 + 3) = (temp >= 1.0) ? (GLfloat)(0.0) : (GLfloat)(1.0 - temp);
    *(params->spectrum + i*4) = (GLfloat)params->R;
    *(params->spectrum + i*4 + 1) = (GLfloat)params->G;
    *(params->spectrum + i*4 + 2) = (GLfloat)params->B;
  }
}

int vinyl_read(char *root){
  struct stat source_stat;
  unsigned long int read = 1;
  unsigned int fill = 0;
  unsigned char fullpath[PATH_MAX] = {'\0'};
  unsigned char *finalpath = NULL;
#ifdef DEBUG
  (void)printf("Playing source %s\n", root);
  (void)fflush(stdout);
#endif
  stat(root, &source_stat);
  
  if(S_ISDIR(source_stat.st_mode)){
    DIR *listing = opendir(root);
    if(listing != NULL){
      struct dirent *dir_enrtry = readdir(listing);
      while(dir_enrtry != NULL){
        if(strcmp(dir_enrtry->d_name, ".") != 0 && strcmp("..", dir_enrtry->d_name) != 0){
          (void)memset(&(fullpath[0]), 0, sizeof(fullpath));
          (void)strncat(&(fullpath[0]), root, PATH_MAX/2);
          (void)strncat(&(fullpath[0]), "/", PATH_MAX/2);
          (void)strncat(&(fullpath[0]), dir_enrtry->d_name, PATH_MAX/2);
          finalpath = realpath(&(fullpath[0]), NULL);
#ifdef DEBUG
          (void)printf("d_name %s\n", dir_enrtry->d_name);
          (void)printf("fullpath %s\n", &(fullpath[0]));
          (void)printf("Analysing %s\n", finalpath);
          (void)fflush(stdout);
#endif    
          (void)fflush(stdout);
          (void)vinyl_read(finalpath);
          (void)free(finalpath);
        }
        dir_enrtry = readdir(listing);
      }
    }
  }else if(S_ISREG(source_stat.st_mode)){
    unsigned char input[3 * params->pps], deletion[3 * params->pps];
    FILE *handle = NULL;
    if(root != NULL){
      handle = fopen(root, "rb");
#ifdef DEBUG
      (void)printf("Reading %s\n", root);
      (void)fflush(stdout);
#endif
      do{
        while(handle != NULL && !feof(handle) && *(params->stop) == 0 && read != 0){
          read = fread(&(input[0]), sizeof(char), params->pps*3, handle);
          fill += read;
          if(fill > params->win_size){
            fill = params->win_size;
          }
          for(unsigned int i = 0; i < read; i++){
            deletion[i] = *(params->dest + params->win_pos + i);
          }
          for(unsigned int i = 0; i < read; i++){
            *(params->dest + (params->win_pos + i)%params->win_size) = input[i];
          }
          switch(params->win_figure){
            case(0):{
              update_stats(&(deletion[0]), &(input[0]));
              break;
            }
            case(1):{
              update_stats_simple(&(input[0]), params->timeout);
              break;
            }
            default:{
              break;
            }
          }
          params->win_pos = (params->win_pos + read)%params->win_size;
          (void)nanosleep(&win_timer, NULL);
        }
        (void)fseek(handle, 0, SEEK_SET);
      }while(params->loop == 1 && *(params->stop) == 0);
      (void)fclose(handle);
    }
  }else if(S_ISCHR(source_stat.st_mode) || S_ISBLK(source_stat.st_mode)){
    unsigned long int num_reads = rand();
    unsigned char input[3 * params->pps], deletion[3 * params->pps];
    FILE *handle = NULL;
    if(root != NULL){
      handle = fopen(root, "rb");
#ifdef DEBUG
      (void)printf("Reading %s\n", root);
      (void)fflush(stdout);
#endif
      do{
        while(handle != NULL && !feof(handle) && *(params->stop) == 0 && read != 0){
          read = fread(&(input[0]), sizeof(char), params->pps*3, handle);
          fill += read;
          if(fill > params->win_size){
            fill = params->win_size;
          }
          for(unsigned int i = 0; i < read; i++){
            deletion[i] = *(params->dest + params->win_pos + i);
          }
          for(unsigned int i = 0; i < read; i++){
            *(params->dest + (params->win_pos + i)%params->win_size) = input[i];
          }
          switch(params->win_figure){
            case(0):{
              update_stats(&(deletion[0]), &(input[0]));
              break;
            }
            case(1):{
              update_stats_simple(&(input[0]), MAXTIMEOUT);
              break;
            }
            default:{
              break;
            }
          }
          params->win_pos = (params->win_pos + read)%params->win_size;
          (void)nanosleep(&win_timer, NULL);
        }
        (void)fseek(handle, 0, SEEK_SET);
        num_reads--;
      }while(num_reads > 0);
      (void)fclose(handle);
    }
  }else{
    return EXIT_FAILURE;
  }
#ifdef DEBUG
  (void)printf("Vinyl exit\n");
  (void)fflush(stdout);
#endif
  (void)fflush(stdout);
  return EXIT_SUCCESS;
}

void vinyl_stop(void){
  (void)free(params->dest);
  (void)free(params);
  (void)free(stat_intern);
#ifdef DEBUG
  (void)puts("Vinyl stop");
#endif
}
