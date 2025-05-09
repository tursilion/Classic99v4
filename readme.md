v400.000

This depot is not guaranteed to build at any given time - it's my bleeding edge repository.

Open source (but restrictive license) emulator including ROMs licensed by Texas Instruments - see [older documentation](https://github.com/tursilion/classic99/raw/main/dist/Classic99%20Manual.pdf) for license and restrictions.

This repository is not yet useful - however, it does boot and incldues the Demonstration cart.

All features of the following hardware are complete:
- CPU TMS9900
- VDP TMS9918A
- PSG SN76xxx/TMS9919
- basic keyboard (99/4 and 99/4A)
- GROM
- System ROM
- Scratchpad RAM

There are a lot of changes compared to Classic99 3xx, with a goal to easier expandability. However,
there is a LONG way to go before it approaches parity.

Note there is NO WAY TO LOAD OR SAVE SOFTWARE. So it's not useful yet. But it runs.

You can run any of the three console variants by passing a year at the command line. This is NOT
a permanent interface and will go away when menus appear someday...

- classic99v4.exe 1979 - runs the 99/4
- classic99v4.exe 1981 - runs the 99/4A
- classic99v4.exe 1983 - runs the 99/4A v2.2

The code is based on Raylib, so in theory, it should build cross-platform. If you try it, let me know 
what happened. All Raylib code is included, though some external dependencies may be needed. The cmake
should tell you.

Building
========

In Windows, open Classic99v4.sln in Visual Studio 2022 (Community is fine), and build as normal.

In Linux and Mac, open a terminal. cd to the Classic99 folder, then:

    md build
    cd build
    cmake ..
    make

Under Debian, I noted I needed to install these additional libraries. There may be others:

    apt-get install cmake libx11-dev libxrandr-dev libxinerama-dev libxcursor-dev libxi-dev libgl1-mesa-dev
    
For Raspberry PI, install these libs, and then run CMAKE with these commands:

    sudo apt-get install --no-install-recommends raspberrypi-ui-mods lxterminal gvfs
    sudo apt-get install libx11-dev libxcursor-dev libxinerama-dev libxrandr-dev libxi-dev libasound2-dev mesa-common-dev libgl1-mesa-dev

    cd build
    cmake -DPLATFORM=Desktop -DGRAPHICS=GRAPHICS_API_OPENGL_21 ..

(See https://github.com/raysan5/raylib/wiki/Working-on-Raspberry-Pi)

Tested Platforms
================

- Platform    Version     Status      Date        Notes
- -------------------------------------------------------------------------------------------------------------
- Windows     11          working     5/7/2025
- Linux       Debian 12   working     5/7/2025
- Linux       WSL2 Debian working     5/7/2025
- Mac OSX     Sequoia15.2 working     5/7/2025    OpenGL build deprecated, audio not tested
- PI 4	      R-PiOS 6.12 working     5/7/2025    Requires OpenGL 2.1 as noted above


