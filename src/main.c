/* WARNING: KEEP THIS CODE AWAY FROM CHILDREN, PREGNANT WOMEN AND OLD PEOPLE */
#include <getopt.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#ifndef WIN32
#include <sys/mman.h>
#include <sys/wait.h>
#include <sys/types.h>
#else
#include "mman.h"
#endif
#include "data_drawer.h"
#include "vinyl_disk.h"
#include <math.h>
#include <memory.h>

#define WIN_SIZE 10e4
#define VIEW_WIDTH                                                             \
  -1 /* Invalid sizes used to trigger autodetection of monitor size */
#define VIEW_HEIGHT -1
#define FPS 60.0f            /* Frames per second we want */
#define ROT_SPEED 2.0f       /* Rotation speed of our cube */
#define CUBE_POINTS 64       /* Number of points in our cube */
#define DEFAULT_TARG "./FSD" /* Read from root by default */
#define DEF_NSAMPS 100        /* How many samples to read per second */

#define HELP_MESSAGE                                                           \
  "Help for FSD (File Spectrum Display):\nCMD     "                            \
  "DESCRIPTION\n====================================\n--help     Print this "  \
  "message\n--file     Set file for analysis\n--width    Set viewport "        \
  "width\n--height   Set viewport height\n--color    Set color of the "        \
  "cube\n--RPM      Set cube rotation speed\n--win_size Set window size for smaples\n--pps      Number of "   \
  "bytes per sample\n--picture  Set visual for rendering\n--axis     Set "     \
  "origin for rotation\n--readtou  Set timeout for points\n--readrate Set "    \
  "sampling rate per second\n--coloring set coloring mode of the cube\n"

GLfloat *color_cube = NULL;

/* Allowed options */
const struct option ALLOWED[15] = {
    {"help", 0, NULL, 0},       {"file", 1, NULL, 1},
    {"width", 1, NULL, 2},      {"height", 1, NULL, 3},
    {"color", 1, NULL, 4},      {"RPM", 2, NULL, 5},
    {"win_size", 1, NULL, 6},   {"pps", 1, NULL, 7},
    {"picture", 1, NULL, 8},    {"axis", 1, NULL, 9},
    {"readtiout", 1, NULL, 10}, {"readrate", 1, NULL, 11},
    {"coloring", 1, NULL, 12},  {"xscr", 0, NULL, 13},
    {NULL, 0, 0, 1499} };

int main(int argc, char *argv[]) {
  /*
   * OpenGL related stuff, scene parameters and buffers
   */
  GLfloat red_set = 1.0f, green_set = 1.0f, blue_set = 1.0f,
          rot_speed = ROT_SPEED, rot_axis[3] = {0.0, 1.0, 0.0};
  
  double timeoutval = 5;
  
  unsigned char *cube_ready = NULL, run_loop = 0, figure_mode = 1, coloring_mode = 1, xscr = 0;
  
  char *pattern_name = NULL,
       *analyzed_name = realpath(DEFAULT_TARG, NULL),
       color_string[9] = "0xFFFFFF";

  pid_t forked;

  /* Various information relating to memory and stuff */
  unsigned int color_int = 0, sample_rate = DEF_NSAMPS;
  unsigned long int window_size = WIN_SIZE, points_per_step = 20;
  /* Used in option parsing, indexes */
  int opt_i = 0, opt_j = 1, *child_pid = calloc(1, sizeof(int));
  /* Size of our screen */
  int viewport_w = VIEW_WIDTH, viewport_h = VIEW_HEIGHT;

  while((opt_i = getopt_long_only(argc, argv, "", &(ALLOWED[0]), NULL)) !=-1){
    switch(opt_i){
    case(0):{
      (void)puts(HELP_MESSAGE);
      return EXIT_FAILURE;
      break;
    }
    case(1):{
      (void)free(analyzed_name);
      analyzed_name = realpath(argv[opt_j + 1], NULL);
      opt_j += 2;
      break;
    }
    case(2):{
      viewport_w = atoi(argv[opt_j + 1]);
      opt_j += 2;
      break;
    }
    case(3):{
      viewport_h = atoi(argv[opt_j + 1]);
      opt_j += 2;
      break;
    }
    case(4):{
      strncpy(&(color_string[0]), argv[opt_j + 1], 8);
      /* Parse color set by user */
      sscanf(&(color_string[0]), "%u", &color_int);
      blue_set = (GLfloat)(color_int & 255) / 255.0f;
      green_set = (GLfloat)((color_int & 255 * 256) >> 8) / 255.0f;
      red_set = (GLfloat)((color_int & 255 * 256 * 256) >> 16) / 255.0f;
      opt_j += 2;
      break;
    }
    case(5):{
      rot_speed = atof(argv[opt_j + 1]);
      opt_j += 2;
      break;
    }
    case(6):{
      if(argv[opt_j + 1]) {
        window_size = atoi(argv[opt_j + 1]);
        opt_j += 2;
      }
      break;
    }
    case(7):{
      if(argv[opt_j + 1]) {
        points_per_step = atoi(argv[opt_j + 1]);
        opt_j += 2;
      }
      break;
    }
    case(8):{
      if(argv[opt_j + 1]) {
        figure_mode = atoi(argv[opt_j + 1]);
        opt_j += 2;
      }
      break;
    }
    case(9):{
      if(argv[opt_j + 1]) {
        (void)sscanf(argv[opt_j + 1], "[%f,%f,%f]", &(rot_axis[0]),
                     &(rot_axis[1]), &(rot_axis[2]));
        opt_j += 2;
      }
      break;
    }
    case(10): {
      if (argv[opt_j + 1]) {
        timeoutval = atof(argv[opt_j + 1]);
        opt_j += 2;
      }
      break;
    }
    case(11):{
      if (argv[opt_j + 1]) {
        sample_rate = atoi(argv[opt_j + 1]);
        opt_j += 2;
      }
      break;
    }
    case(12):{
      if (argv[opt_j + 1]) {
        if (strncmp(argv[opt_j + 1], "static", 6) == 0) {
          coloring_mode = 0;
#ifdef DEBUG
          (void)puts("Enabled static coloring mode");
#endif
        } else if (strncmp(argv[opt_j + 1], "probab", 6) == 0) {
          coloring_mode = 1;
#ifdef DEBUG
          (void)puts("Enabled probability coloring mode");
#endif
        } else {
          if (argv[opt_j + 1]) {
            coloring_mode = 2;
            pattern_name = argv[opt_j + 1];
#ifdef DEBUG
            (void)puts("Enabled pattern coloring mode");
#endif
          }
        }
        opt_j += 2;
      }
      break;
    }
    case(13):{
      xscr = 1;
      opt_j++;
      break;
    }
    default:{
      opt_j++;
      break;
    }
    }
  }

  initRenderer(viewport_w, viewport_h, xscr);

  cube_ready = mmap(NULL, sizeof(unsigned char), PROT_WRITE | PROT_READ,
                    MAP_ANONYMOUS | MAP_SHARED, -1, 0);
  color_cube = initCube(64, rot_speed);

  vinyl_prep(window_size * 3, points_per_step, run_loop, color_cube,
             CUBE_POINTS, cube_ready, red_set, green_set, blue_set, figure_mode,
             sample_rate, timeoutval);
             
  if(pattern_name != NULL && coloring_mode == 2){
    load_pattern(pattern_name);
  }

  if(coloring_mode == 1){
    vinyl_dynamic_coloring(coloring_mode);
  }
  
#ifdef DEBUG
  (void)puts("Statistics are ready.");
  (void)fflush(stdout);
#endif

  *cube_ready = 0;

  forked = fork();

  while(*cube_ready == 0) {
    if(forked == 0) {
      vinyl_read_disk(analyzed_name);
      vinyl_stop();
      return EXIT_SUCCESS;
    }else{
      (void)reloadVBO();
      *cube_ready = renderSpectrum();
    }
  }

  (void)kill(forked, SIGALRM);

  (void)waitpid(forked, child_pid, 0);
  (void)free(analyzed_name);
  (void)munmap(cube_ready, sizeof(unsigned char));
  return EXIT_SUCCESS;
}
