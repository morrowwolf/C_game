cmake_minimum_required(VERSION 3.29.2)
project(C_game VERSION 0.1.0 LANGUAGES C)

include(CTest)
enable_testing()

#debug stuff
#set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /fsanitize=address")

set(INCLUDED_FILES  source/main.c)
set(INCLUDED_FILES ${INCLUDED_FILES} source/data_structures/list.c)
set(INCLUDED_FILES ${INCLUDED_FILES} source/data_structures/list_iterator.c)
set(INCLUDED_FILES ${INCLUDED_FILES} source/data_structures/read_write_lock.c)
set(INCLUDED_FILES ${INCLUDED_FILES} source/entity.c)
set(INCLUDED_FILES ${INCLUDED_FILES} source/entity/asteroid.c)
set(INCLUDED_FILES ${INCLUDED_FILES} source/entity/bullet.c)
set(INCLUDED_FILES ${INCLUDED_FILES} source/entity/fighter.c)
set(INCLUDED_FILES ${INCLUDED_FILES} source/gamestate.c)
set(INCLUDED_FILES ${INCLUDED_FILES} source/window.c)
set(INCLUDED_FILES ${INCLUDED_FILES} source/windowCPUBuffer.c)

add_executable(C_game WIN32 ${INCLUDED_FILES})

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)
