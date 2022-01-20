Classic99 v400.000
==================

This depot is not guaranteed to build at any given time - it's my bleeding edge repository.

Open source (but restrictive license) emulator including ROMs licensed by Texas Instruments - see [older documentation](https://github.com/tursilion/classic99/raw/master/dist/Classic99%20Manual.pdf) for license and restrictions.

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

classic99v4.exe 1979 - runs the 99/4
classic99v4.exe 1981 - runs the 99/4A
classic99v4.exe 1983 - runs the 99/4A v2.2

The code is based on Allegro 5.x, so in theory, it should build cross-platform. If you try it, let me know 
what happened.

Download the latest release here:
(Windows) [https://raw.githubusercontent.com/tursilion/Classic99v4/master/dist/Classic99v4.zip] 
(Linux (raw, not a package)) [https://raw.githubusercontent.com/tursilion/Classic99v4/master/dist/classic99v4.tgz] 

Tested Platforms
================

Platform    Version     Status      Date        Notes
---------------------------------------------------------------------------------------------------------------
Windows     10          working     1/6/2022
Linux       Debian 10   working     12/27/2021
Linux       WSL2 Ubuntu working     1/6/2022    VcXsrv, audio not tested
Mac OSX     ?           unstable    12/27/2021  If it boots, it works, but window creation/resize causes crash


Want to donate?
---------------

- Click here for [Ko-Fi](https://ko-fi.com/tursilion)...
- Alternately, there's my [Amazon Wishlist](http://www.amazon.com/gp/registry/2AFCOAM5DD1L6/ref=cm_aya_wl/103-5991996-6483001)

