# this is /not/ intended for use with Windows - use the sln
# root CMakeLists.txt

cmake_minimum_required(VERSION 3.13.0)
project(classic99v4 VERSION 400.0.0)

option(CONSOLE_BUILD "Build a console-only build - no gfx or sound" OFF)

set(CMAKE_BUILD_TYPE Release)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/")

# don't need openGL for console only build
if (NOT CONSOLE_BUILD)
    find_package(OpenGL REQUIRED)
endif(NOT CONSOLE_BUILD)

find_package(Curses REQUIRED)
find_package(Threads REQUIRED)

# todo someday, perhaps
include(CTest)
enable_testing()
add_executable(classic99v4)

SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -Wall -std=c++14")

if (CONSOLE_BUILD)
    # no raylib here
    target_compile_definitions(classic99v4 PUBLIC CONSOLE_BUILD)
    set_target_properties(classic99v4 PROPERTIES OUTPUT_NAME classic99_console)
    TARGET_LINK_LIBRARIES(classic99v4 ${CURSES_LIBRARIES} Threads::Threads)
else(CONSOLE_BUILD)
    # yes raylib here
    set_target_properties(classic99v4 PROPERTIES OUTPUT_NAME classic99)
    TARGET_LINK_LIBRARIES(classic99v4 raylib ${CURSES_LIBRARIES} Threads::Threads)
endif(CONSOLE_BUILD)

add_subdirectory(Classic99v4)

include_directories( ${CURSES_INCLUDE_DIRS} )

# todo someday, perhaps
set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)

