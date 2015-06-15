/* WARNING: KEEP THIS CODE AWAY FROM CHILDREN, PREGNANT WOMEN AND OLD PEOPLE */
#include <stdlib.h>
#include <getopt.h>
#include <string.h>
#include <limits.h>
#include <unistd.h>
#include <stdio.h>
#include <time.h>
#include <math.h>
#include <GLFW/glfw3.h>
#include <GL/glu.h>


#define MAX_NAME_LEN 256
#define VIEW_WIDTH 640
#define VIEW_HEIGHT 480
#define FPS 30.0f
#define ROT_SPEED 2.0f
#define LAND_POINTS 128
#define CUBE_POINTS 64

#define HELP_MESSAGE "Help for FSD (File Spectrum Display):\nCMD     DESCRIPTION\n====================================\n--help     Print this message\n--file     Set file for analysis\n--width    Set viewport width\n--height   Set viewport height"

GLfloat *coords_cube = NULL;
GLfloat *color_cube = NULL;
GLfloat *coords_land = NULL;
GLfloat *color_land = NULL;

/* Allowed options */
const struct option ALLOWED[6] = {{"help", 0, NULL, 0},
                                  {"file", 1, NULL, 1},
                                  {"width", 1, NULL, 2},
                                  {"height", 1, NULL, 3},
                                  {"color", 1, NULL, 4},
                                  {"RPM", 2, NULL, 5}
                                 };

/* List that we are currently using to draw and the one that we are preparing */
GLuint cube_points_list = 0;
GLuint cube_color_list = 0;
GLuint land_points_list = 0;
GLuint land_color_list = 0;

/* Used to calculate rotation */
struct timespec timer = {0, 0};
/* Sleep for this time between frames */
struct timespec sleeper = {0, 1000000000/FPS};

/* Prepare vertex buffer objects */
void prepare_VBO(void){
  glGenBuffersARB(1, &cube_points_list);
  glBindBufferARB(GL_ARRAY_BUFFER_ARB, cube_points_list);
  glBufferDataARB(GL_ARRAY_BUFFER_ARB, CUBE_POINTS*CUBE_POINTS*CUBE_POINTS*3*sizeof(GLfloat), coords_cube, GL_STATIC_DRAW);
  glVertexPointer(3, GL_FLOAT, 0, NULL);
  glEnableClientState(GL_VERTEX_ARRAY);

  glGenBuffersARB(1, &cube_color_list);
  glBindBufferARB(GL_ARRAY_BUFFER_ARB, cube_color_list);
  glBufferDataARB(GL_ARRAY_BUFFER_ARB, CUBE_POINTS*CUBE_POINTS*CUBE_POINTS*4*sizeof(GLfloat), color_cube, GL_DYNAMIC_DRAW);
  glColorPointer(4, GL_FLOAT, 0, NULL);
  glEnableClientState(GL_COLOR_ARRAY);

  glGenBuffersARB(1, &land_points_list);
  glBindBufferARB(GL_ARRAY_BUFFER_ARB, land_points_list);
  glBufferDataARB(GL_ARRAY_BUFFER_ARB, LAND_POINTS*LAND_POINTS*3*sizeof(GLfloat), coords_land, GL_STATIC_DRAW);
  glVertexPointer(3, GL_FLOAT, 0, NULL);
  glEnableClientState(GL_VERTEX_ARRAY);

  glGenBuffersARB(1, &land_color_list);
  glBindBufferARB(GL_ARRAY_BUFFER_ARB, land_color_list);
  glBufferDataARB(GL_ARRAY_BUFFER_ARB, LAND_POINTS*LAND_POINTS*3*sizeof(GLfloat), color_land, GL_STATIC_DRAW);
  glColorPointer(3, GL_FLOAT, 0, NULL);
  glEnableClientState(GL_COLOR_ARRAY);

  glBindBufferARB(GL_ARRAY_BUFFER_ARB, 0);
}

/* Generate a landscape, generate simple mountains, */
void recursive_land(GLfloat *buffer_points, GLfloat *buffer_colors, unsigned char offset, unsigned char top){
  unsigned int i = 0, j = 0;
  if(offset > 0){
  #ifdef DEBUG
    (void)printf("Depth %d\n", offset);
  #endif
    for(i = 0; i < LAND_POINTS*3; i += offset*3){
      for(j = 0; j < LAND_POINTS*3; j += offset*3){
        /* Prepare basis value */
        *(buffer_points + i + j*LAND_POINTS) = (GLfloat)i / (GLfloat)LAND_POINTS / 3.0f;
        *(buffer_points + i + j*LAND_POINTS + 1) = (GLfloat)j / (GLfloat)LAND_POINTS / 3.0f;
        *(buffer_points + i + j*LAND_POINTS + 2) += ((GLfloat)rand()/(GLfloat)RAND_MAX)*((GLfloat)offset/(GLfloat)LAND_POINTS);
        /* Add noise from neighbors */
      }
    }
    recursive_land(buffer_points, buffer_colors, offset/2, 0);
  }

  if(top){
    for(i = 0; i < LAND_POINTS*3; i += offset*3){
      for(j = 0; j < LAND_POINTS*3; j += offset*3){
        *(buffer_colors + i + j*LAND_POINTS) = *(buffer_colors + i + j*LAND_POINTS + 1) = *(buffer_colors + i + j*LAND_POINTS + 2) += 0.1f + 0.9f * ((GLfloat)rand()/(GLfloat)RAND_MAX)*((GLfloat)offset/(GLfloat)LAND_POINTS)/2.0f;
      }
    }
  }
}

int main(int argc, char *argv[])
{
  GLFWwindow *render_target = NULL;
  GLfloat rot_Angle = 0.0f, red_set = 1.0f, green_set = 1.0f, blue_set = 1.0f, rot_speed = ROT_SPEED;
  FILE *file_to_analyze = NULL;
  char analyzed_name[MAX_NAME_LEN] = "FSD", color_string[9] = "0xFFFFFF";
  unsigned char *data_buffer = NULL;
  /* Various information relating to memory and stuff */
  unsigned int file_size = 0, page_size = 0, i = 0, j = 0, max_val = 0, color_int = 0, file_sum = 0;
  /* Used in option parsing, indexes */
  int opt_i = 0, opt_j = 1;
  /* Size of our screen */
  unsigned short viewport_w = VIEW_WIDTH, viewport_h = VIEW_HEIGHT;

  while((opt_i = getopt_long_only(argc, argv, "", &(ALLOWED[0]), NULL)) != -1){
    switch(opt_i){
      case(0):{
        (void)puts(HELP_MESSAGE);
        return EXIT_FAILURE;
      }
      case(1):{
        strncpy(&(analyzed_name[0]), argv[opt_j + 1], MAX_NAME_LEN);
        opt_j+=2;
        break;
      }
      case(2):{
        viewport_w = atoi(argv[opt_j + 1]);
        opt_j+=2;
        break;
      }
      case(3):{
        viewport_h = atoi(argv[opt_j + 1]);
        opt_j+=2;
        break;
      }
      case(4):{
        strncpy(&(color_string[0]), argv[opt_j + 1], 8);
        /* Parse color set by user */
        sscanf(&(color_string[0]), "%x", &color_int);
        blue_set = (GLfloat)(color_int & 255)/255.0f;
        green_set = (GLfloat)((color_int & 255*256)>>8)/255.0f;
        red_set = (GLfloat)((color_int & 255*256*256)>>16)/255.0f;
        opt_j+=2;
        break;
      }
      case(5):{
        rot_speed = atof(argv[opt_j + 1]);
        opt_j += 2;
        break;
      }
      default:{
        (void)printf("Unknown option detected: %s\n", argv[opt_j]);
        opt_j++;
        break;
      }
    }
  }

  file_to_analyze = fopen(&(analyzed_name[0]), "rb");

  if(file_to_analyze == NULL){
    (void)printf("Error: %s not found!\n", analyzed_name);
    return EXIT_FAILURE;
  }

  /* What is the size of our page? Optimise memory throughput */
  page_size = sysconf(_SC_PAGESIZE);

  data_buffer = calloc(page_size * 3, sizeof(char));

  /* Allocate data cube, checking for success */
  if(data_buffer == NULL){
    (void)fclose(file_to_analyze);
    (void)printf("Error: Failed to allocate %d bytes!\n", page_size * 3);
    return EXIT_FAILURE;
  }

  /* Allocate a cube of points */
  coords_cube = calloc(CUBE_POINTS*CUBE_POINTS*CUBE_POINTS*3, sizeof(GLfloat));
  color_cube = calloc(CUBE_POINTS*CUBE_POINTS*CUBE_POINTS*4, sizeof(GLfloat));

  /* Fill up the cube with magic */
  while((i = fread(data_buffer, sizeof(char), page_size * 3, file_to_analyze)) > 0){
    j = i;
    file_size += i;
    for(i = 0; i < (j / 3) * 3; i+=3){
      file_sum += data_buffer[i] + data_buffer[i + 1] + data_buffer[i + 2];
      *(color_cube + 4*(data_buffer[i]/4 + CUBE_POINTS*(data_buffer[i+1]/4) + CUBE_POINTS*CUBE_POINTS*(data_buffer[i+2]/4)) ) = red_set;
      *(color_cube + 4*(data_buffer[i]/4 + CUBE_POINTS*(data_buffer[i+1]/4) + CUBE_POINTS*CUBE_POINTS*(data_buffer[i+2]/4)) + 1) = green_set;
      *(color_cube + 4*(data_buffer[i]/4 + CUBE_POINTS*(data_buffer[i+1]/4) + CUBE_POINTS*CUBE_POINTS*(data_buffer[i+2]/4)) + 2) = blue_set;
      *(color_cube + 4*(data_buffer[i]/4 + CUBE_POINTS*(data_buffer[i+1]/4) + CUBE_POINTS*CUBE_POINTS*(data_buffer[i+2]/4)) + 3) += 1.0;
      if(*(color_cube + 4*(data_buffer[i]/4 + CUBE_POINTS*(data_buffer[i+1]/4) + CUBE_POINTS*CUBE_POINTS*(data_buffer[i+2]/4)) + 3) > max_val){
        max_val = *(color_cube + 4*(data_buffer[i]/4 + CUBE_POINTS*(data_buffer[i+1]/4) + CUBE_POINTS*CUBE_POINTS*(data_buffer[i+2]/4)) + 3);
      }
    }
  }

  /* Check if our file is zero length (No data gained from it) */
  if(file_size == 0){
    (void)fclose(file_to_analyze);
    (void)free(data_buffer);
    (void)printf("Error: File %s is empty!\n", analyzed_name);
    return EXIT_FAILURE;
  }else{
    (void)printf("Read %d bytes\n", file_size);
  }

  (void)fclose(file_to_analyze);
  (void)free(data_buffer);

  coords_land = calloc(LAND_POINTS * LAND_POINTS * 3, sizeof(GLfloat));
  color_land = calloc(LAND_POINTS * LAND_POINTS * 3, sizeof(GLfloat));

  /* Prepare to generate random landscape */
  (void)srand(file_sum%file_size);

  recursive_land(coords_land, color_land, LAND_POINTS - 1, 1);

  /* Normalize all values! */
  for(i = 0, j = 0; i < CUBE_POINTS*CUBE_POINTS*CUBE_POINTS*3; i+=3, j+=4){
    if(color_cube[j + 3] > 1){
      color_cube[j + 3] = logf(color_cube[j + 3])/logf(max_val*100);
    }else{
      color_cube[j + 3] /= (GLfloat)max_val;
    }
    coords_cube[i] = ((GLfloat)(i & CUBE_POINTS - 1)/(GLfloat)CUBE_POINTS);
    coords_cube[i + 1] = ((GLfloat)((i & ((CUBE_POINTS - 1) * CUBE_POINTS))>>6)/(GLfloat)CUBE_POINTS);
    coords_cube[i + 2] = ((GLfloat)((i & ((CUBE_POINTS - 1) * CUBE_POINTS * CUBE_POINTS))>>12)/(GLfloat)CUBE_POINTS);
  }

#ifdef DEBUG
  (void)puts("Statistics are ready, init graphics.");
#endif


  if(!glfwInit()){
#ifdef DEBUG
    (void)puts("Severe failure, glfwInit() failed.");
#endif
    return EXIT_FAILURE;
  }

  cube_points_list = glGenLists(1);
  cube_color_list = glGenLists(1);

  if(!(render_target = glfwCreateWindow(viewport_w, viewport_h, "FSD", NULL, NULL))){
#ifdef DEBUG
    printf("Error while initializing graphics");
#endif
    glfwTerminate();
    return EXIT_FAILURE;
  }

  glfwMakeContextCurrent(render_target);
  glClearColor(0.0, 0.0, 0.0, 0.0);
  glClear(GL_COLOR_BUFFER_BIT);
  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  glEnable(GL_BLEND);
  glEnable(GL_POINT_SMOOTH);
  glEnable(GL_SMOOTH);
  glHint(GL_POINT_SMOOTH_HINT, GL_NICEST);
  glPointSize(2.0);
  glViewport(0, 0, viewport_w, viewport_h);

  gluPerspective(75, (float)viewport_w/(float)viewport_h, 1.0, 10);
  gluLookAt(1.0, 1.0, 1.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0);

  prepare_VBO();

  while(!glfwWindowShouldClose(render_target)){
    (void)glClear(GL_COLOR_BUFFER_BIT);

    (void)glPushMatrix();
    /* Move the cube to the position */
    (void)glTranslatef(-0.2, -0.2, -0.2);
    /* Rotate around Z axis */
    (void)glRotatef(rot_Angle, 0.0, 1.0, 0.0);
    /* Translate to origin */
    (void)glTranslatef(-0.5, -0.5, -0.5);
    glBindBufferARB(GL_ARRAY_BUFFER_ARB, 0);
    glBindBufferARB(GL_ARRAY_BUFFER_ARB, cube_points_list);
    glVertexPointer(3, GL_FLOAT, 0, NULL);
    glBindBufferARB(GL_ARRAY_BUFFER_ARB, cube_color_list);
    glColorPointer(4, GL_FLOAT, 0, NULL);
    (void)glDrawArrays(GL_POINTS, 0, CUBE_POINTS*CUBE_POINTS*CUBE_POINTS);
    (void)glPopMatrix();

    glPushMatrix();
    glTranslatef(-0.2f, -0.2f, -0.2f);
    glRotatef(270.0f, 1.0f, 0.0f, 0.0f);
    glTranslatef(-0.5f, -0.5f, -0.5f);

    glRotatef(45.0f, 0.0f, 0.0f, 1.0f);

    glScalef( 4.0f, 4.0f, 1.0f);
    glTranslatef(0.0f, 0.0f, -2.0f);

    glBindBufferARB(GL_ARRAY_BUFFER_ARB, 0);
    glBindBufferARB(GL_ARRAY_BUFFER_ARB, land_points_list);
    glVertexPointer(3, GL_FLOAT, 0, NULL);
    glBindBufferARB(GL_ARRAY_BUFFER_ARB, land_color_list);
    glColorPointer(3, GL_FLOAT, 0, NULL);
    (void)glDrawArrays(GL_POINTS, 0, LAND_POINTS*LAND_POINTS);

    glPopMatrix();

    (void)glfwSwapBuffers(render_target);
    (void)nanosleep(&sleeper, NULL);
    (void)glfwPollEvents();
    rot_Angle += 360.0f*rot_speed/FPS/60.0f;
    (rot_Angle > 360.0f) ? (rot_Angle = rot_Angle - 360.0f) : (rot_Angle = rot_Angle);
  }

  glDeleteBuffers(1, &cube_points_list);
  glDeleteBuffers(1, &cube_color_list);
  glDeleteBuffers(1, &land_points_list);
  glDeleteBuffers(1, &land_color_list);
  glfwTerminate();

  (void)free(color_cube);
  (void)free(coords_cube);
  (void)free(coords_land);
  (void)free(color_land);
  return EXIT_SUCCESS;
}

