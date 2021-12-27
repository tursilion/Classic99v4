cmake_minimum_required(VERSION 3.13.0)
project(classic99v4 VERSION 400.0.0)

include(CTest)
enable_testing()

SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3 -Wall -Wno-stringop-truncation -DALLEGRO_LINUX -DALLEGRO_WITH_XWINDOWS")

add_executable(classic99v4)

add_subdirectory(Classic99v4)

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)

TARGET_LINK_LIBRARIES(classic99v4 allegro allegro_main allegro_dialog allegro_audio)
