#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <dirent.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <memory.h>
#include <time.h>
#include <sys/types.h>
#include "vinyl_disk.h"

extern VINYL_PARAMS *params;
extern CUBE_PATTERN *pattern;
extern double *stat_intern;
extern struct timespec win_timer;

int vinyl_read_disk(char *root){
  struct stat source_stat;
  unsigned long int read = 1;
  unsigned int fill = 0;
  char fullpath[PATH_MAX] = {'\0'};
  char *finalpath = NULL;
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
          (void)vinyl_read_disk(finalpath);
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