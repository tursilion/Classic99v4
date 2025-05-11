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

    mkdir build
    cd build
    cmake ..
    make

Under Debian, I noted I needed to install these additional libraries. There may be others:

    apt-get install cmake libx11-dev libxrandr-dev libxinerama-dev libxcursor-dev libxi-dev libgl1-mesa-dev libncurses-dev
    
For Raspberry PI, install these libs, and then run CMAKE with these commands:

    sudo apt-get install --no-install-recommends raspberrypi-ui-mods lxterminal gvfs
    sudo apt-get install libx11-dev libxcursor-dev libxinerama-dev libxrandr-dev libxi-dev libasound2-dev mesa-common-dev libgl1-mesa-dev libncurses-dev

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
- Mac OSX     Sequoia15.2 working     5/7/2025    OpenGL build marked deprecated, audio not tested
- PI 4	      R-PiOS 6.12 working     5/7/2025    Requires OpenGL 2.1 build as noted above

Credits
=======

Raylib is available here: https://www.raylib.com/

Copyright (c) 2013-2024 Ramon Santamaria (@raysan5)

This software is provided "as-is", without any express or implied warranty. In no event 
will the authors be held liable for any damages arising from the use of this software.

Permission is granted to anyone to use this software for any purpose, including commercial 
applications, and to alter it and redistribute it freely, subject to the following restrictions:

  1. The origin of this software must not be misrepresented; you must not claim that you 
  wrote the original software. If you use this software in a product, an acknowledgment 
  in the product documentation would be appreciated but is not required.

  2. Altered source versions must be plainly marked as such, and must not be misrepresented
  as being the original software.

  3. This notice may not be removed or altered from any source distribution.

Classic99 uses modified source. Any modifications are tagged (mb).

---

PDCurses is used in the Windows port and is available here: https://pdcurses.org/

The core package and most ports are in the public domain, but a few files in the demos and X11 directories 
are subject to copyright under licenses described there.

This software is provided AS IS with NO WARRANTY whatsoever.

Classic99 does not include demos or X11 directories.
