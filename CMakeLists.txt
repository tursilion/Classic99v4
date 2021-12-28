cmake_minimum_required(VERSION 3.13.0)
project(classic99v4 VERSION 400.0.0)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/")

include(CTest)
enable_testing()

find_package(Allegro REQUIRED)

include_directories( ${ALLEGRO_INCLUDE_DIRS} )

# linux
#SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3 -Wall -Wno-stringop-truncation -DALLEGRO_LINUX -DALLEGRO_WITH_XWINDOWS")

# mac... allegro with brew
# use cmake -DCMAKE_CXX_COMPILER=/usr/bin/g++
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -Wall -std=c++14")

add_executable(classic99v4)

add_subdirectory(Classic99v4)
set_target_properties(classic99v4 PROPERTIES OUTPUT_NAME classic99)

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)

#TARGET_LINK_LIBRARIES(classic99v4 allegro allegro_main allegro_dialog allegro_audio)
TARGET_LINK_LIBRARIES(classic99v4 ${ALLEGRO_LIBRARIES} )
