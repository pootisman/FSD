/* WARNING: KEEP THIS CODE AWAY FROM CHILDREN, PREGNANT WOMEN AND OLD PEOPLE */
#include <stdlib.h>
#include <getopt.h>
#include <string.h>
#include <limits.h>
#include <unistd.h>
#include <stdio.h>
#include <time.h>
#include <sys/mman.h>
#include <math.h>
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <dirent.h>
#include "../../plw/src/stdout_messages.h"
#include "../../plw/src/file_messages.h"
#include "vinyl.h"

#define WIN_SIZE 10e6
#define VIEW_WIDTH 1366
#define VIEW_HEIGHT 768
#define FPS 60.0f /* Frames per second we want */
#define ROT_SPEED 2.0f /* Rotation speed of our cube */
#define CUBE_POINTS 64 /* Number of points in our cube */
#define DEFAULT_TARG "./FSD" /* Read from root by default */
#define DEF_NSAMPS 50 /* How many samples to read per second */

#define HELP_MESSAGE "Help for FSD (File Spectrum Display):\nCMD     DESCRIPTION\n====================================\n--help     Print this message\n--file     Set file for analysis\n--width    Set viewport width\n--height   Set viewport height\n--color    Set color of the cube\n--RPM      Set cube rotation speed\n--dir      Set directory to read from\n--win_size Set window size for smaples\n--pps      Number of bytes per sample\n--picture  Set visual for rendering\n--axis     Set origin for rotation\n--readtou  Set timeout for points\n--readrate Set sampling rate per second\n"

GLfloat* coords_cube = NULL;
GLfloat* color_cube = NULL;
GLenum error = 0;

/* Allowed options */
const struct option ALLOWED[12] = {{"help", 0, NULL, 0},
        {"file", 1, NULL, 1},
        {"width", 1, NULL, 2},
        {"height", 1, NULL, 3},
        {"color", 1, NULL, 4},
        {"RPM", 2, NULL, 5},
        {"win_size", 1, NULL, 6},
        {"pps", 1, NULL, 7},
        {"picture", 1, NULL, 8},
        {"axis", 1, NULL, 9},
        {"readtiout", 1, NULL, 10},
        {"readrate", 1, NULL, 11}
};

/* List that we are currently using to draw and the one that we are preparing */
GLuint cube_points_list = 0;
GLuint cube_color_list = 0;

/* Sleep for this time between frames */
struct timespec sleeper = {0, 1e9 / FPS};

/*
 *  Prepare vertex buffer objects
 */

void prepare_VBO ( void )
{
        glGenBuffers ( 1, &cube_points_list );
        glBindBuffer ( GL_ARRAY_BUFFER, cube_points_list );
        glBufferData ( GL_ARRAY_BUFFER, CUBE_POINTS * CUBE_POINTS * CUBE_POINTS * 3 * sizeof ( GLfloat ), coords_cube, GL_STATIC_DRAW );
        glVertexPointer ( 3, GL_FLOAT, 0, NULL );
        glEnableClientState ( GL_VERTEX_ARRAY );

        glGenBuffers ( 1, &cube_color_list );
        glBindBuffer ( GL_ARRAY_BUFFER, cube_color_list );
        glBufferData ( GL_ARRAY_BUFFER, CUBE_POINTS * CUBE_POINTS * CUBE_POINTS * 4 * sizeof ( GLfloat ), color_cube, GL_DYNAMIC_DRAW );
        glColorPointer ( 4, GL_FLOAT, 0, NULL );
        glEnableClientState ( GL_COLOR_ARRAY );

        glBindBuffer ( GL_ARRAY_BUFFER, 0 );
}

void free_VBO ( void )
{
        glBindBuffer ( GL_ARRAY_BUFFER, cube_points_list );
        glDeleteBuffers ( 1, &cube_points_list );
        glBindBuffer ( GL_ARRAY_BUFFER, cube_color_list );
        glDeleteBuffers ( 1, &cube_color_list );
}

void update_VBO ( GLfloat* color_source )
{
        GLfloat* ptr = NULL;
        glBindBuffer ( GL_ARRAY_BUFFER, cube_color_list );
        ptr = glMapBuffer ( GL_ARRAY_BUFFER, GL_WRITE_ONLY );
        memcpy ( ptr, color_source, sizeof ( GLfloat ) * 4 * CUBE_POINTS * CUBE_POINTS * CUBE_POINTS );
        glUnmapBuffer ( GL_ARRAY_BUFFER );
        glBindBuffer ( GL_ARRAY_BUFFER, 0 );
}

/*
 * This function will generate coordinates where we must draw all of our points
 */

void prepare_coordinates ( GLfloat* cube_grid_array )
{
        unsigned int i = 0;
        if ( cube_grid_array == NULL ) {
                ( void ) puts ( "ERROR: cube_grid_array is NULL for some reason!" );
                return;
        }

        for ( i = 0; i < CUBE_POINTS * CUBE_POINTS * CUBE_POINTS; i++ ) {
                * ( cube_grid_array + i * 3 ) = ( GLfloat ) ( i % CUBE_POINTS ) / ( GLfloat ) CUBE_POINTS;
                * ( cube_grid_array + i * 3 + 1 ) = ( GLfloat ) ( ( unsigned int ) ( floor ( ( GLfloat ) i / ( GLfloat ) CUBE_POINTS ) ) % CUBE_POINTS ) / ( GLfloat ) ( CUBE_POINTS );
                * ( cube_grid_array + i * 3 + 2 ) = floor ( ( GLfloat ) ( i ) / ( GLfloat ) ( CUBE_POINTS * CUBE_POINTS ) ) / ( GLfloat ) CUBE_POINTS;
        }

        return;
}

int main ( int argc, char* argv[] )
{
        GLFWwindow* render_target = NULL;
        GLfloat rot_Angle = 0.0f, red_set = 1.0f, green_set = 1.0f, blue_set = 1.0f, rot_speed = ROT_SPEED, rot_axis[3] = {0.0, 1.0, 0.0};
        double timeoutval = MAXTIMEOUT;
        unsigned char color_string[9] = "0xFFFFFF", run_loop = 0, figure_mode = 1;
        unsigned char* cube_ready = NULL, *window_buffer = NULL, *analyzed_name = realpath ( DEFAULT_TARG, NULL );

        DIR* directory = NULL;

        pid_t forked;

        directory = opendir ( "." );

        closedir ( directory );
        /* Various information relating to memory and stuff */
        unsigned int color_int = 0, sample_rate = DEF_NSAMPS;
        unsigned long int window_size = WIN_SIZE, points_per_step = 20;
        /* Used in option parsing, indexes */
        int opt_i = 0, opt_j = 1;
        /* Size of our screen */
        unsigned short viewport_w = VIEW_WIDTH, viewport_h = VIEW_HEIGHT;

        while ( ( opt_i = getopt_long_only ( argc, argv, "", & ( ALLOWED[0] ), NULL ) ) != -1 ) {
                switch ( opt_i ) {
                case ( 0 ) : {
                        ( void ) puts ( HELP_MESSAGE );
                        return EXIT_FAILURE;
                }
                case ( 1 ) : {
                        ( void ) free ( analyzed_name );
                        analyzed_name = realpath ( argv[opt_j + 1], NULL );
                        opt_j += 2;
                        break;
                }
                case ( 2 ) : {
                        viewport_w = atoi ( argv[opt_j + 1] );
                        opt_j += 2;
                        break;
                }
                case ( 3 ) : {
                        viewport_h = atoi ( argv[opt_j + 1] );
                        opt_j += 2;
                        break;
                }
                case ( 4 ) : {
                        strncpy ( & ( color_string[0] ), argv[opt_j + 1], 8 );
                        /* Parse color set by user */
                        sscanf ( & ( color_string[0] ), "%x", &color_int );
                        blue_set = ( GLfloat ) ( color_int & 255 ) / 255.0f;
                        green_set = ( GLfloat ) ( ( color_int & 255 * 256 ) >> 8 ) / 255.0f;
                        red_set = ( GLfloat ) ( ( color_int & 255 * 256 * 256 ) >> 16 ) / 255.0f;
                        opt_j += 2;
                        break;
                }
                case ( 5 ) : {
                        rot_speed = atof ( argv[opt_j + 1] );
                        opt_j += 2;
                        break;
                }
                case ( 6 ) : {
                        if ( argv[opt_j + 1] ) {
                                window_size = atoi( argv[opt_j + 1] );
                                opt_j += 2;
                        }
                        break;
                }
                case ( 7 ) : {
                        if ( argv[opt_j + 1] ) {
                                points_per_step = atoi( argv[opt_j + 1] );
                                opt_j += 2;
                        }
                        break;
                }
                case ( 8 ) : {
                        if ( argv[opt_j + 1] ) {
                                figure_mode = atoi( argv[opt_j + 1] );
                                opt_j += 2;
                        }
                        break;
                }
                case ( 9 ) : {
                        if ( argv[opt_j + 1] ) {
                                ( void ) sscanf ( argv[opt_j + 1], "[%f,%f,%f]", & ( rot_axis[0] ), & ( rot_axis[1] ), & ( rot_axis[2] ) );
                                opt_j += 2;
                        }
                        break;
                }
                case ( 10 ) : {
                        if ( argv[opt_j + 1] ) {
                                timeoutval =  atof( argv[opt_j + 1] );
                                opt_j += 2;
                        }
                        break;
                }
                case ( 11 ) : {
                        if ( argv[opt_j + 1] ) {
                                sample_rate = atoi( argv[opt_j + 1] );
                                opt_j += 2;
                        }
                        break;
                }
                default: {
                        ( void ) printf ( "Unknown option detected: %s\n", argv[opt_j] );
                        opt_j++;
                        break;
                }
                }
        }

        cube_ready = mmap ( NULL, sizeof ( unsigned char ), PROT_WRITE | PROT_READ, MAP_ANONYMOUS | MAP_SHARED, -1, 0 );
        window_buffer = mmap ( NULL, window_size * 3 * sizeof ( unsigned char ), PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_SHARED, -1, 0 );
        color_cube = mmap ( NULL, CUBE_POINTS * CUBE_POINTS * CUBE_POINTS * 4 * sizeof ( GLfloat ), PROT_WRITE | PROT_READ, MAP_ANONYMOUS | MAP_SHARED, -1, 0 );
        coords_cube = calloc ( CUBE_POINTS * CUBE_POINTS * CUBE_POINTS * 3, sizeof ( GLfloat ) );
        vinyl_prep ( window_size * 3, window_buffer, points_per_step, run_loop, color_cube, CUBE_POINTS, cube_ready, red_set, green_set, blue_set, figure_mode, sample_rate, timeoutval);

        prepare_coordinates ( coords_cube );

#ifdef DEBUG
        ( void ) puts ( "Statistics are ready, init graphics." );
        ( void ) fflush ( stdout );
#endif

        if ( !glfwInit() ) {
#ifdef DEBUG
                ( void ) puts ( "Severe failure, glfwInit() failed." );
#endif
                return EXIT_FAILURE;
        }

        if ( ! ( render_target = glfwCreateWindow ( viewport_w, viewport_h, "FSD",
                                 glfwGetPrimaryMonitor(), NULL ) ) ) {
#ifdef DEBUG
                printf ( "Error while initializing graphics" );
#endif
                glfwTerminate();
                return EXIT_FAILURE;
        }

        glfwMakeContextCurrent ( render_target );

        if ( GLEW_OK != glewInit() ) {
                ( void ) puts ( "GLEW initialization failed!" );
                glfwDestroyWindow ( render_target );
                glfwTerminate();
                return EXIT_FAILURE;
        }

        glClearColor ( 0.0, 0.0, 0.0, 0.0 );
        glClear ( GL_COLOR_BUFFER_BIT );
        glMatrixMode ( GL_MODELVIEW );
        glLoadIdentity();
        glPointSize ( 2.0 );
        glViewport ( 0, 0, viewport_w, viewport_h );
        gluPerspective ( 75, ( float ) viewport_w / ( float ) viewport_h, 1.0, 10 );
        gluLookAt ( 1.0, 1.0, 1.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0 );
        //glPointSize(1.0);
        glEnable ( GL_POINT_SMOOTH );
        glEnable ( GL_BLEND );
        glEnable ( GL_ALPHA_TEST );
        glfwWindowHint ( GLFW_SAMPLES, 8 );
        glBlendFunc ( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
        glAlphaFunc ( GL_NOTEQUAL, 0 );
        glHint ( GL_POINT_SMOOTH_HINT, GL_NICEST );

        prepare_VBO();

        forked = fork();

        while ( !glfwWindowShouldClose ( render_target ) ) {
                if ( forked == 0 ) {
                        vinyl_read ( analyzed_name );
                        vinyl_stop();
                        break;
                } else {
                        ( void ) update_VBO ( color_cube );
                        ( void ) glClear ( GL_COLOR_BUFFER_BIT );
                        ( void ) glPushMatrix();
                        /* Move the cube to the position */
                        ( void ) glTranslatef ( -0.2, -0.2, -0.2 );
                        /* Rotate around Z axis */
                        ( void ) glRotatef ( rot_Angle, rot_axis[0], rot_axis[1], rot_axis[2] );
                        /* Translate to origin */
                        ( void ) glTranslatef ( -0.5, -0.5, -0.5 );
                        glBindBuffer ( GL_ARRAY_BUFFER, 0 );
                        glBindBuffer ( GL_ARRAY_BUFFER, cube_points_list );
                        glVertexPointer ( 3, GL_FLOAT, 0, NULL );
                        glBindBuffer ( GL_ARRAY_BUFFER, cube_color_list );
                        glColorPointer ( 4, GL_FLOAT, 0, NULL );
                        ( void ) glDrawArrays ( GL_POINTS, 0, CUBE_POINTS * CUBE_POINTS * CUBE_POINTS );
                        ( void ) glPopMatrix();
                        ( void ) glfwSwapBuffers ( render_target );
                        ( void ) nanosleep ( &sleeper, NULL );
                        ( void ) glfwPollEvents();
                        rot_Angle += 360.0f * rot_speed / FPS / 60.0f;
                        ( rot_Angle > 360.0f ) ? ( rot_Angle = rot_Angle - 360.0f ) : ( rot_Angle = rot_Angle );
                }
        }

        *cube_ready = 1;
        ( void ) sleep ( 1 );
        ( void ) munmap ( color_cube, CUBE_POINTS * CUBE_POINTS * CUBE_POINTS * 4 * sizeof ( GLfloat ) );
        ( void ) free_VBO();
        ( void ) glfwDestroyWindow ( render_target );
        ( void ) glfwTerminate();
        ( void ) free ( analyzed_name );
        ( void ) free ( coords_cube );
        ( void ) munmap ( cube_ready, sizeof ( char ) );
        ( void ) munmap ( window_buffer, window_size * 3 * sizeof ( unsigned char ) );
        ( void ) munmap ( color_cube, CUBE_POINTS * CUBE_POINTS * CUBE_POINTS * 4 * sizeof ( GLfloat ) );
        return EXIT_SUCCESS;
}
