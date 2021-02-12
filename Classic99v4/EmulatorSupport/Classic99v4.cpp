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

// singletons that we need
Classic99TV *television = nullptr;

// Cleanup - shared function to release resources
void Cleanup() {
    debug_write("Cleaning up...");
    delete television;
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
    debug_init();                       // everyone relies on debug

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

    // we need this early for the event queue
    television = new Classic99TV();

    debug_write("Doing nonsense...");
    for (;;) {
        television->setBgColor(al_map_rgba(255,0,0,255));
        if (television->runWindowLoop()) break;
        al_rest(0.100);
        television->setBgColor(al_map_rgba(0,0,255,255));
        if (television->runWindowLoop()) break;
        al_rest(0.100);
    }

    // ... shutdown
    Cleanup();

    // all done!
    return 0;
}
