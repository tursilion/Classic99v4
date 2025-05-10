# this is /not/ intended for use with Windows - use the sln
# root CMakeLists.txt

cmake_minimum_required(VERSION 3.13.0)
project(classic99v4 VERSION 400.0.0)

set(CMAKE_BUILD_TYPE Release)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/")

find_package(OpenGL)
find_package(Curses)

# todo someday, perhaps
include(CTest)
enable_testing()

SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -Wall -std=c++14")

add_executable(classic99v4)

add_subdirectory(Classic99v4)
set_target_properties(classic99v4 PROPERTIES OUTPUT_NAME classic99)
include_directories( ${CURSES_INCLUDE_DIRS} )

# todo someday, perhaps
set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)

TARGET_LINK_LIBRARIES(classic99v4 raylib ${CURSES_LIBRARIES} )
