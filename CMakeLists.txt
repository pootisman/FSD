project(FSD)
cmake_minimum_required(VERSION 2.6)
aux_source_directory(./src SRC_LIST)
add_executable(${PROJECT_NAME} ${SRC_LIST})
target_compile_options(${PROJECT_NAME} PRIVATE "-O2" "-std=gnu99")
target_link_libraries(${PROJECT_NAME} GL glfw GLU m GLEW pthread)

add_executable("${PROJECT_NAME}_dbg" ${SRC_LIST})
target_compile_options("${PROJECT_NAME}_dbg" PRIVATE "-g" "-DDEBUG" 
"-std=gnu99")
target_link_libraries("${PROJECT_NAME}_dbg" GL glfw GLU m GLEW pthread)
