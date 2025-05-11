// Classic99 v4xx - Copyright 2021 by Mike Brent (HarmlessLion.com)
// See License.txt, but the answer is "just ask me first". ;)

// This file needs to provide the core functionality for the emulator
// - ability to select a system
// - management of resources (Memory, I/O) for each CPU (just a middle-man)
// - timing management, including debug/single step per CPU
// - sound output management
// - video output management
// - input management (keyboard and joysticks)

// Expects to build with Raylib
// Today is 11/7/2020, 2/2/2021, 5/5/2025

#include <raylib.h>
#include <cstdio>
#include <chrono>
#include <thread>
#include "Classic99v4.h"
#include "tv.h"
#include "debuglog.h"

#include "../Systems/TexasInstruments/TI994.h"
#include "../Systems/TexasInstruments/TI994A.h"
#include "../Systems/TexasInstruments/TI994A_22.h"

// TODO: all the cross platform stuff is nice, but we could do
// better for the blind users - the TUI stuff probably won't play
// nicely with text to speech systems. We should be able to have a
// build option for the Windows version to integrate a menu. 
// We have all the Win32 code in the old Classic99,
// so that should not be too big a challenge. It's okay if it
// has both the GUI menu and the TUI menu, I think. I'll just
// have to make the Windows menu call into the TUI ops.
// 
// It would only be the menu, I think, I don't think I'd re-integrate
// all the debug and configuration dialogs.
// 
// Alternately, maybe that's a question to ask. If the console can
// be read aloud, and the interface to the menu is not too difficult
// (keyboard interface? F10 and arrows?), then I wonder if it might
// be possible to do it with just the TUI anyway?
// 
// I am not currently concerned about Linux/Mac... that starts
// to get into the frameworks that I was so discouraged about
// avoiding in the first place, and I've no idea if they even work.
//
// Documentation on set up and operation will be important to help.

// we expect time steps of 15-20 milliseconds, so we'll set an upper limit of 100ms
// Any more than that, and we discard the lost time. This is in microseconds.
#define MAX_TIME_SKIP 100000

// Cleanup - shared function to release resources
void Cleanup() {
    debug_write("Cleaning up...");
    debug_shutdown();
}

// MAN I wanted this basic signature in the old Classic99... ;)
int main(int argc, char **argv) {
    int sys = 1981;

    if (argc > 1) {
        sys = atoi(argv[1]);
        switch (sys) {
            case 1979:
            case 1981:
            case 1983:
                break;

            default:
                printf("Run classic99v4 with a number to select a specific system:\n");
                printf("  classic99v4.exe 1979 - selects 99/4\n");
                printf("  classic99v4.exe 1981 - selects 99/4A\n");
                printf("  classic99v4.exe 1983 - selects 99/4A v2.2\n");
                return 0;
        }
    }

    // everyone relies on debug
    debug_init();

    // Platform specific init goes here

    // now we can start starting
    debug_write("Starting Classic99 version " VERSION);

    TI994 *pSys;

    if (sys == 1979) {
        debug_write("Starting a 99/4...");
        pSys = new TI994();
    } else if (sys == 1983) {
        debug_write("Starting a v2.2 99/4A...");
        pSys = new TI994A22();
    } else {
        debug_write("Starting a 99/4A...");
        pSys = new TI994A();
    }

    if (!pSys->initSystem()) {
        debug_write("** System failed to initialize **");
        return 99;
    }

    // This tracks elapsed time
    double elapsedUs = 0;
    std::chrono::high_resolution_clock::time_point nStart, nEnd;
    nStart = std::chrono::high_resolution_clock::now();
    
    while (!WindowShouldClose()) {
        // TODO: We could have a new high precision mode that doesn't sleep, just
        // yields, and with the time measurement the quantum then becomes as
        // tight as the system we're on allows for.
        // But right now, sleeps are happening in the TV class, so we need to sort that out
        // However, to avoid spinning we'll put some kind of sleep here for now...
        std::this_thread::sleep_for(std::chrono::milliseconds(1));

        nEnd = std::chrono::high_resolution_clock::now();
        long long diff = (std::chrono::duration_cast<std::chrono::microseconds>(nEnd - nStart)).count();
        if (diff > MAX_TIME_SKIP) diff = MAX_TIME_SKIP;
        if (diff > 0) elapsedUs += diff;
        nStart = nEnd;

        // to try for rough scanline accuracy, we'll run 64uS slices up to the desired time
        // TODO: this is working, but it's running fast on the CPU (? just fast)
        // If I mess with the timing, the VDP runs fast too - in theory neither SHOULD...
        while (elapsedUs > 64.0) {
            pSys->runSystem(64);
            elapsedUs -= 64.0;
        }
        // and for precision mode, run whatever's left
        if (elapsedUs > 1.0) {
            pSys->runSystem((int)elapsedUs);
            elapsedUs -= (int)elapsedUs;
        }

        // update audio (via callback now)
        //if (nullptr != pSys->getSpeaker()) {
        //    pSys->getSpeaker()->runSpeakerLoop();
        //}

        // update the display
        if (nullptr != pSys->getTV()) {
            if (pSys->getTV()->runWindowLoop()) break;  // TODO: maybe this comes out of runSystem instead.
        }
    }

    // ... shutdown
    pSys->deInitSystem();
    delete pSys;
    Cleanup();

    // all done!
    return 0;
}
