// Classic99 v4xx - Copyright 2020 by Mike Brent (HarmlessLion.com)
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
// Today is 11/7/2020

#include <allegro5/allegro.h>
#include <allegro5/allegro_native_dialog.h>
#include <stdio.h>
#include "Classic99v4.h"

#ifdef ALLEGRO_WINDOWS
#include <Windows.h>        // for outputdebugstring
#endif

// display
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

// Write a line to the debug buffer displayed on the debug screen
void debug_write(const char *s, ...) {
	char buf[1024];

	_vsnprintf(buf, 1023, s, (char*)((&s)+1));
	buf[1023]='\0';     // allow a longer string out to OutputDebugString

    // TODO: disk log here if configured

#ifdef ALLEGRO_WINDOWS
	OutputDebugStringA(buf);
	OutputDebugStringA("\n");
#endif

#if 0
    // TODO: trying the Allegro log
    // This works, but it just appends the window forever... would be bad
    // for long runs.
    static ALLEGRO_TEXTLOG *txtLog;
    if (nullptr == txtLog) {
        // TODO: note we need to add the log's events to the queue
        // so we can detect when it's closed
        txtLog = al_open_native_text_log("Classic99 Log", 0);
    }
    if (nullptr != txtLog) {
        al_append_native_text_log(txtLog, buf);
        al_append_native_text_log(txtLog, "\n");
    }
#endif

    // now trim to the final saved length
	buf[DEBUGLEN-1]='\0';

	al_lock_mutex(DebugCS);

        // copy the line in - we already enforced the size above
	    strcpy(&debugLines[currentDebugLine][0], buf);

        // next line
        ++currentDebugLine;
        if (currentDebugLine >= DEBUGLINES) currentDebugLine = 0;

        // flag redraw
        bDebugDirty=true;

    al_unlock_mutex(DebugCS);

}

// copy out the debug log - using a function now with a copy because the ring
// buffer could otherwise cause odd alignment, and we want to control how long
// we hold the mutex without worrying about how long the OS needs to draw
// buf must be a pointer to an array of size [DEBUGLINES][DEBUGLEN]
void get_debug(char *buf[DEBUGLEN]) {
    al_lock_mutex(DebugCS);

        int n = currentDebugLine;
        for (int i = 0; i<DEBUGLINES; ++i) {
            strcpy(buf[i], debugLines[n++]);
            if (n >= DEBUGLINES) n = 0;
        }

        // assume this is being used to redraw the debug
        bDebugDirty = false;

    al_unlock_mutex(DebugCS);
}

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
    // set up mutexes before anything else
    DebugCS = al_create_mutex();

    // clear out the debug buffer
    for (int i=0; i<DEBUGLINES; ++i) {
        memset(debugLines[i], ' ', DEBUGLEN);
        debugLines[i][DEBUGLEN-1] = '\0';
    }

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
