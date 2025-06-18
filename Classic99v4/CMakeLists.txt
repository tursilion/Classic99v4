# CMakeLists.txt for Classic99
# Note that we do not need to build PDCurses on non-Windows, we use native ncurses libs

# disable raylib for console only build
if (NOT CONSOLE_BUILD)
    add_subdirectory(Raylib)
endif(NOT CONSOLE_BUILD)

add_subdirectory(CPUs)
add_subdirectory(EmulatorSupport)
add_subdirectory(Keyboard)
add_subdirectory(Memories)
add_subdirectory(PSGs)
add_subdirectory(Systems)
add_subdirectory(VDPs)
