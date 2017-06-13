import os

debug = ARGUMENTS.get('debug',False)
env = Environment()

if not debug:
	env.Append(CCFLAGS='-std=gnu99 -s -Wall -Wextra -O3')
else:
	env.Append(CCFLAGS='-std=gnu99 -DDEBUG -g -Wall -Wextra -O0')


Object_source_list = Glob('src/!main.c')
Objects = env.Object(Object_source_list)
SideEffect('FSD.d', Objects)
ParseDepends('FSD.d')
FSD = env.Program("bin/FSD", ['src/main.c', 'src/data_drawer.c', 'src/vinyl.c', 'src/vinyl_disk.c'], LIBS=['GL', 'GLEW', 'X11', 'm', 'GLU'], LIBPATH=['/usr/lib'])

Depends(FSD, Objects)
