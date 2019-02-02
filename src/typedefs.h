#ifndef _PIX32RGBA_
#define _PIX32RGBA_

#include <GL/glew.h>

typedef struct p32rgba{
    GLfloat R;
    GLfloat G;
    GLfloat B;
    GLfloat A;
}p32rgba;

#endif

#ifndef _GLOPARAM_
#define _GLOPARAM_

#include <linux/limits.h>

typedef struct globalParams{
    char filePath[PATH_MAX] = {'\0'};
    
}globalParams;

#endif
