import os

env = Environment(CC='gcc', CC_FLAGS='-v -Wall -O2')

Object_source_list = Glob('src/!main.c')
Objects = env.Object(Object_source_list)
SideEffect('FSD.d', Objects)
ParseDepends('FSD.d')
FSD_rel = env.Program("bin/FSD", ['src/main.c', 'src/data_drawer.c', 'src/vinyl.c'], LIBS=['GL', 'GLEW', 'glfw', 'm', 'GLU'], LIBPATH=['/usr/lib'])

Depends(FSD_rel, Objects)