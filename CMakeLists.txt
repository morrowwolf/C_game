cmake_minimum_required(VERSION 3.29.2)
project(C_game VERSION 0.1.0 LANGUAGES C)

include(CTest)
enable_testing()

#debug stuff
#set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /fsanitize=address /Zi")
#set ASAN_SAVE_DUMPS=”MyFileName.dmpx” https://devblogs.microsoft.com/cppblog/asan-for-windows-x64-and-debug-build-support/

# AppVerifier

#tracy stuff - readd CXX to project languages
#add_definitions(-DTRACY_ENABLE)
#set(INCLUDED_FILES ${INCLUDED_FILES} tracy-0.10/public/TracyClient.cpp)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /std:clatest")

set(INCLUDED_FILES ${INCLUDED_FILES} source/main.c)

set(INCLUDED_FILES ${INCLUDED_FILES} source/assets/resources.c)
set(INCLUDED_FILES ${INCLUDED_FILES} source/data_structures/list.c)
set(INCLUDED_FILES ${INCLUDED_FILES} source/data_structures/list_iterator.c)
set(INCLUDED_FILES ${INCLUDED_FILES} source/data_structures/list_iterator_thread.c)
set(INCLUDED_FILES ${INCLUDED_FILES} source/data_structures/memory_pool.c)
set(INCLUDED_FILES ${INCLUDED_FILES} source/data_structures/read_write_lock.c)
set(INCLUDED_FILES ${INCLUDED_FILES} source/data_structures/read_write_lock_priority.c)
set(INCLUDED_FILES ${INCLUDED_FILES} source/data_structures/signal.c)
set(INCLUDED_FILES ${INCLUDED_FILES} source/data_structures/stack.c)
set(INCLUDED_FILES ${INCLUDED_FILES} source/entity.c)
set(INCLUDED_FILES ${INCLUDED_FILES} source/entity/asteroid.c)
set(INCLUDED_FILES ${INCLUDED_FILES} source/entity/bullet.c)
set(INCLUDED_FILES ${INCLUDED_FILES} source/entity/fighter.c)
set(INCLUDED_FILES ${INCLUDED_FILES} source/gamestate.c)
set(INCLUDED_FILES ${INCLUDED_FILES} source/globals.c)
set(INCLUDED_FILES ${INCLUDED_FILES} source/graphics/GPU_handler.c)
set(INCLUDED_FILES ${INCLUDED_FILES} source/memory_manager.c)
set(INCLUDED_FILES ${INCLUDED_FILES} source/tasks.c)
set(INCLUDED_FILES ${INCLUDED_FILES} source/window.c)

add_executable(C_game WIN32 ${INCLUDED_FILES})

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)
