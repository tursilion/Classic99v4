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
#include "debuglog.h"

#ifdef ALLEGRO_WINDOWS
#include <Windows.h>        // for outputdebugstring
#endif

// display -- TODO: move this to a global "TV" object at window res and/or power of two?? and let the caller register bitplanes
// I'm thinking the TV only bothers with a background color, everything else is rendered on top (preferably in hardware).
// we just allow subwindows that have a position and resolution, and then we only need to fill that in for each layer.
// Sprites are 32x32 pixel layers with an offset (though still need a collision buffer), F18A bitmap overlay fits this
// nicely, text mode would be a subwindow too, and F18A can register /two/ bitplanes since it has two screens.
// Need a create layer and a destroy layer method, since things like overlay can vary in size, and then we don't need
// to keep unused resolutions around. Would need to choose a minimum resolution to work in.
ALLEGRO_DISPLAY *myWnd = nullptr;
int width = 256;
int height = 192;
int wndFlags =  ALLEGRO_WINDOWED        // or ALLEGRO_FULLSCREEN_WINDOW | ALLEGRO_FRAMELESS
             |  ALLEGRO_RESIZABLE
#ifdef ALLEGRO_LINUX
             |  ALLEGRO_GTK_TOPLEVEL    // needed for menus under X
#endif
             |  0                       // or ALLEGRO_DIRECT3D or ALLEGRO_OPENGL
             |  ALLEGRO_GENERATE_EXPOSE_EVENTS;    // maybe?

// debug
#define DEBUGLEN 240
#define DEBUGLINES 34
char debugLines[DEBUGLINES][DEBUGLEN];  // debug lines
int currentDebugLine = 0;               // next line to write to
bool bDebugDirty = true;                // never been drawn, must be dirty!
ALLEGRO_MUTEX *DebugCS = nullptr;       // we should assume that this mutex is NOT re-entrant!

// Cleanup - shared function to release resources
void Cleanup() {
    debug_write("Cleaning up...");
    al_destroy_mutex(DebugCS);
    al_destroy_display(myWnd);
}

// fatal error - write to log and message box, then exit
void fail(const char *str) {
    debug_write("Exitting: %s", str);
    al_show_native_message_box(myWnd, "Classic99", "Fatal error", str, nullptr, ALLEGRO_MESSAGEBOX_ERROR);
    Cleanup();
    exit(99);
}

// MAN I wanted this basic signature in the old Classic99... ;)
int main(int argc, char **argv) {
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

    // create an event queue, register as we go
    ALLEGRO_EVENT_QUEUE *evtQ = al_create_event_queue();

    // Create the window
    debug_write("Creating window...");

    al_set_new_display_flags(wndFlags);                 // some flags are user-configurable
    al_set_new_display_option(ALLEGRO_COLOR_SIZE, 32, ALLEGRO_REQUIRE);  // require a 32-bit display buffer, assume RGBA?
    al_set_new_display_option(ALLEGRO_SAMPLE_BUFFERS, 0, ALLEGRO_SUGGEST);  // suggest no multisampling
    // al_set_new_window_position(x, y);                // TODO
    myWnd = al_create_display(width, height);
    if (nullptr == myWnd) {
        printf("Failed to create display\n");
        return 98;
    }
    al_register_event_source(evtQ, al_get_display_event_source(myWnd));

    debug_write("Doing nonsense...");
    bool run = true;
    while (run) {
        al_clear_to_color(al_map_rgba(255,0,0,255));
        al_flip_display();
        al_rest(0.100);
        al_clear_to_color(al_map_rgba(0,0,255,255));
        al_flip_display();
        al_rest(0.100);

        ALLEGRO_EVENT evt;
        // process ALL pending events
        while (al_get_next_event(evtQ, &evt)) {
            switch (evt.type) {
            // display events
            case ALLEGRO_EVENT_DISPLAY_EXPOSE:
                debug_write("Window uncovered");
                break;
            case ALLEGRO_EVENT_DISPLAY_RESIZE:
                debug_write("Resize to %d x %d", evt.display.width, evt.display.height);
                al_acknowledge_resize(myWnd);
                break;
            case ALLEGRO_EVENT_DISPLAY_CLOSE:
                run = false;
                break;
            case ALLEGRO_EVENT_DISPLAY_SWITCH_OUT:
                debug_write("Lost focus");
                break;
            case ALLEGRO_EVENT_DISPLAY_SWITCH_IN:
                debug_write("Got focus");
                break;
            case ALLEGRO_EVENT_DISPLAY_ORIENTATION:
                debug_write("Rotated to %d", evt.display.orientation);
                break;
            case ALLEGRO_EVENT_DISPLAY_HALT_DRAWING:
                // this one is a big deal - for IOS and Android it means we
                // are moved to the background. We should stop ALL timers and audio and
                // we MUST NOT draw anything - we must ensure that any draw functions
                // are finished before we acknowledge. We can't start up again till
                // we get the resume event
                debug_write("Halt drawing request");
                // If we take too long to send this, we will be considered unresponsive
                al_acknowledge_drawing_halt(myWnd);
                break;
            case ALLEGRO_EVENT_DISPLAY_RESUME_DRAWING:
                // this is the resume for DISPLAY_HALT_DRAWING, now
                // we can restart our timers, audio, and drawing
                debug_write("Resume drawing request");
                // If we take too long to send this, we will be considered unresponsive
                al_acknowledge_drawing_resume(myWnd);
                break;
            default:
                debug_write("Got unexpected event %d", evt.type);
            }
        }

    }




    // ... shutdown
    Cleanup();

    // all done!
    return 0;
}
