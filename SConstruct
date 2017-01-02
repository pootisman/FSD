import os

debug = ARGUMENTS.get('debug',False)
env = Environment()

if not debug:
	env.Append(CCFLAGS='-s -v -Wall -Wextra -O2')
else:
	env.Append(CCFLAGS='-DDEBUG -g -v -Wall -Wextra -O0')


Object_source_list = Glob('src/!main.c')
Objects = env.Object(Object_source_list)
SideEffect('FSD.d', Objects)
ParseDepends('FSD.d')
FSD = env.Program("bin/FSD", ['src/main.c', 'src/data_drawer.c', 'src/vinyl.c', 'src/vinyl_disk.c'], LIBS=['GL', 'GLEW', 'X11', 'm', 'GLU'], LIBPATH=['/usr/lib'])

Depends(FSD, Objects)
