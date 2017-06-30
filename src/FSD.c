#include <getopt.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <memory.h>
#ifndef WIN32
#include <sys/mman.h>
#include <sys/wait.h>
#include <sys/types.h>
#else
#include "mman.h"
#endif
#include "data_drawer.h"
#include "vinyl_disk.h"
#include "screenhack.h"

XSCREENSAVER_MODULE="FSD"
