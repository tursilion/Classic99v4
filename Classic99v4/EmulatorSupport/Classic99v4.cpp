// Classic99 v4xx - Copyright 2021 by Mike Brent (HarmlessLion.com)
// See License.txt, but the answer is "just ask me first". ;)

// This file needs to provide the core functionality for the emulator
// - ability to select a system
// - management of resources (Memory, I/O) for each CPU (just a middle-man)
// - timing management, including debug/single step per CPU
// - sound output management
// - video output management
// - input management (keyboard and joysticks)

// Expects to build with the Allegro5 library located in D:\Work\Allegro5\Build
// Funny to come back to Allegro after so many years...
// Today is 11/7/2020, 2/2/2021

#include <allegro5/allegro.h>
#include <allegro5/allegro_native_dialog.h>
#include <stdio.h>
#include "Classic99v4.h"
#include "tv.h"
#include "debuglog.h"

#include "../Systems/TexasInstruments/TI994.h"
#include "../Systems/TexasInstruments/TI994A.h"
#include "../Systems/TexasInstruments/TI994A_22.h"

#ifdef ALLEGRO_WINDOWS
// for timers
#include <Windows.h>
#endif

// Cleanup - shared function to release resources
void Cleanup() {
    debug_write("Cleaning up...");
    debug_shutdown();
}

// fatal error - write to log and message box, then exit
void fail(const char *str) {
    debug_write("Exitting: %s", str);
    al_show_native_message_box(NULL, "Classic99", "Fatal error", str, nullptr, ALLEGRO_MESSAGEBOX_ERROR);
    Cleanup();
    exit(99);
}

// MAN I wanted this basic signature in the old Classic99... ;)
int main(int argc, char **argv) {
    // everyone relies on debug
    debug_init();

    // set up Allegro
    if (!al_init()) {
        // we have no system and cannot start...
        // hope for stdout?
        fail("Unable to initialize Allegro system\n");
        return 99;
    }

    // Prepare addons...
    if (!al_init_native_dialog_addon()) {
        // we'll try to keep going anyway - file dialogs may not work
        debug_write("Warning: failed to initialize dialog add-on");
    }

    // Platform specific init goes here
#ifdef ALLEGRO_ANDROID
    // TODO: these are from the sample code - do I need them? Want them?
    al_install_touch_input();
    al_android_set_apk_file_interface();
#endif

    // now we can start starting
    debug_write("Starting Classic99 version " VERSION);

    //debug_write("Starting a 99/4...");
    //TI994 *pSys = new TI994();

    debug_write("Starting a 99/4A...");
    TI994 *pSys = new TI994A();

    //debug_write("Starting a v2.2 99/4A...");
    //TI994 *pSys = new TI994A22();

    pSys->initSystem();

    // This tracks elapsed time
    double elapsedUs = 0;

#ifdef ALLEGRO_WINDOWS
    // TODO: how to do this for Linux, Mac, Android?
    LARGE_INTEGER nStart, nEnd, nFreq;
    QueryPerformanceCounter(&nStart);
#endif

    for (;;) {
        // TODO: We could have a new high precision mode that doesn't sleep, just
        // yields, and with the time measurement the quantum then becomes as
        // tight as the system we're on allows for.
        al_rest(0.01);

#ifdef ALLEGRO_WINDOWS
        // TODO: how to do this for Linux, Mac, Android?
        QueryPerformanceCounter(&nEnd);
        QueryPerformanceFrequency(&nFreq);

        if (nEnd.QuadPart > nStart.QuadPart) {
            nEnd.QuadPart -= nStart.QuadPart;
            elapsedUs += ((double)nEnd.QuadPart / nFreq.QuadPart) * 1000000;
        }

        // start timing for the next loop
        QueryPerformanceCounter(&nStart);
#else
        // it wil be more than this thanks to WindowLoop/etc
        elapsedUs += 10000;
#endif

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

        // update audio
        if (nullptr != pSys->getSpeaker()) {
            pSys->getSpeaker()->runSpeakerLoop();
        }

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
