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
#define FPS 30.0

#define HELP_MESSAGE "Help for FSD (File Spectrum Display):\nCMD     DESCRIPTION\n====================================\n--help     Print this message\n--file     Set file for analysis\n--width    Set viewport width\n--height   Set viewport height"

GLfloat *coords = NULL;
GLfloat *color = NULL;

/* Allowed options */
const struct option ALLOWED[4] = {{.name = "help", .has_arg = 0, .flag = NULL, .val = 0},
                                  {.name = "file", .has_arg = 1, .flag = NULL, .val = 1},
                                  {.name = "width", .has_arg = 1, .flag = NULL, .val = 2},
                                  {.name = "height", .has_arg = 1, .flag = NULL, .val = 3}
                                 };

/* List that we are currently using to draw and the one that we are preparing */
GLuint primary_list = 0;
GLuint back_list = 0;

/* Used to calculate rotation */
struct timespec timer = {.tv_sec = 0, .tv_nsec = 0};
/* Sleep for this time between frames */
struct timespec sleeper = {.tv_sec = 0, .tv_nsec = 1000000000/FPS};

/* Prepare vertex buffer objects */
void prepare_VBO(void){
  glGenBuffersARB(1, &primary_list);
  glBindBufferARB(GL_ARRAY_BUFFER_ARB, primary_list);
  glBufferDataARB(GL_ARRAY_BUFFER_ARB, 64*64*64*3*sizeof(GLfloat), coords, GL_STATIC_DRAW);
  glVertexPointer(3, GL_FLOAT, 0, NULL);
  glEnableClientState(GL_VERTEX_ARRAY);

  glGenBuffersARB(1, &back_list);
  glBindBufferARB(GL_ARRAY_BUFFER_ARB, back_list);
  glBufferDataARB(GL_ARRAY_BUFFER_ARB, 64*64*64*4*sizeof(GLfloat), color, GL_DYNAMIC_DRAW);
  glColorPointer(4, GL_FLOAT, 0, NULL);
  glEnableClientState(GL_COLOR_ARRAY);
}

int main(int argc, char *argv[])
{
  GLFWwindow *render_target = NULL;
  GLfloat rot_Angle = 0.0f;
  FILE *file_to_analyze = NULL;
  char analyzed_name[MAX_NAME_LEN] = "FSD";
  unsigned char *data_buffer = NULL;
  /* Various information relating to memory and stuff */
  unsigned int file_size = 0, page_size = 0, i = 0, j = 0, max_val = 0;
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
  coords = calloc(64*64*64*3, sizeof(GLfloat));
  color = calloc(64*64*64*4, sizeof(GLfloat));

  /* Fill up the cube with magic */
  while((i = fread(data_buffer, sizeof(char), page_size * 3, file_to_analyze)) > 0){
    file_size += i;
    for(i = 0; i < page_size * 3; i+=3){
      *(color + 4*(data_buffer[i]/4 + 64*(data_buffer[i+1]/4) + 64*64*(data_buffer[i+2]/4)) ) = 1.0;
      *(color + 4*(data_buffer[i]/4 + 64*(data_buffer[i+1]/4) + 64*64*(data_buffer[i+2]/4)) + 1) = 1.0;
      *(color + 4*(data_buffer[i]/4 + 64*(data_buffer[i+1]/4) + 64*64*(data_buffer[i+2]/4)) + 2) = 1.0;
      *(color + 4*(data_buffer[i]/4 + 64*(data_buffer[i+1]/4) + 64*64*(data_buffer[i+2]/4)) + 3) += 1.0;
      if(*(color + 4*(data_buffer[i]/4 + 64*(data_buffer[i+1]/4) + 64*64*(data_buffer[i+2]/4)) + 3) > max_val){
        max_val = *(color + 4*(data_buffer[i]/4 + 64*(data_buffer[i+1]/4) + 64*64*(data_buffer[i+2]/4)) + 3);
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

  /* Normalize all values, we may not have to! */
  for(i = 0, j = 0; i < 64*64*64*3; i+=3, j+=4){
    if(color[j + 3] > 1){
      color[j + 3] = logf(color[j + 3])/logf(max_val*10);
    }else{
      color[j + 3] /= (GLfloat)max_val;
    }
#ifdef DEBUG
    if(color[j + 3] == 1.0){
      (void)puts("Maximum found");
    }
#endif
    coords[i] = ((GLfloat)(i & 63)/64.0f);
    coords[i + 1] = ((GLfloat)((i & (63 * 64))>>6)/64.0f);
    coords[i + 2] = ((GLfloat)((i & (63 * 64 * 64))>>12)/64.0f);
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

  primary_list = glGenLists(1);
  back_list = glGenLists(1);

  if(!(render_target = glfwCreateWindow(VIEW_WIDTH, VIEW_HEIGHT, "FSD", NULL, NULL))){
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
  glViewport(0, 0, VIEW_WIDTH, VIEW_HEIGHT);

  gluPerspective(75, (float)viewport_w/(float)viewport_h, 1.3, 10);
  gluLookAt(1.0, 1.0, 1.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0);

  prepare_VBO();

  while(!glfwWindowShouldClose(render_target)){
    (void)glClear(GL_COLOR_BUFFER_BIT);
    (void)glPushMatrix();
    /* Move the cube to the centre */
    (void)glTranslatef(-0.2, -0.2, -0.2);
    /* Rotate around Z axis */
    (void)glRotatef(rot_Angle, 0.0, 1.0, 0.0);
    /* Translate to origin */
    (void)glTranslatef(-0.5, -0.5, -0.5);
    glBindBufferARB(GL_ARRAY_BUFFER_ARB, primary_list);
    (void)glDrawArrays(GL_POINTS, 0, 64*64*64);

    (void)glPopMatrix();
    (void)glfwSwapBuffers(render_target);
    (void)nanosleep(sleeper, NULL);
    (void)glfwPollEvents();
    rot_Angle += 0.1f;
    (rot_Angle > 360.0) ? (rot_Angle = 0.0) : (rot_Angle = rot_Angle);
  }

  glfwTerminate();
  return EXIT_SUCCESS;
}

