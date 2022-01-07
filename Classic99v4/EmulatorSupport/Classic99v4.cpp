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
#ifdef ALLEGRO_LINUX
// for timers
#include <time.h>
#include <errno.h>
#endif

// we expect time steps of 15-20 milliseconds, so we'll set an upper limit of 100ms
// Any more than that, and we discard the lost time. This is in microseconds.
#define MAX_TIME_SKIP 100000

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
    double nStart, nEnd;
    nStart = al_get_time();

    for (;;) {
        // TODO: We could have a new high precision mode that doesn't sleep, just
        // yields, and with the time measurement the quantum then becomes as
        // tight as the system we're on allows for.
        al_rest(0.01);

        // it wil be more than this thanks to WindowLoop/etc
        //elapsedUs += 10000;
        nEnd = al_get_time();
        double diff = (nEnd - nStart) * 1000000;
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
