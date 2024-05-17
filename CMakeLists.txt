cmake_minimum_required(VERSION 3.29.2)
project(C_game VERSION 0.1.0 LANGUAGES C)

include(CTest)
enable_testing()

#debug
#set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /fsanitize=address")

#release?
#set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /MD /MT")

set(INCLUDED_FILES  source/main.c)
set(INCLUDED_FILES ${INCLUDED_FILES} source/entity.c)
set(INCLUDED_FILES ${INCLUDED_FILES} source/entity/asteroid.c)
set(INCLUDED_FILES ${INCLUDED_FILES} source/entity/fighter.c)
set(INCLUDED_FILES ${INCLUDED_FILES} source/gamestate.c)
set(INCLUDED_FILES ${INCLUDED_FILES} source/window.c)
set(INCLUDED_FILES ${INCLUDED_FILES} source/windowCPUBuffer.c)
set(INCLUDED_FILES ${INCLUDED_FILES} source/list.c)

add_executable(C_game WIN32 ${INCLUDED_FILES})

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)
