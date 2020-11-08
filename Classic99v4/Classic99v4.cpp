// Classic99v4.cpp : Defines the entry point for the application.
// Expects to build with the Allegro5 library located in D:\Work\Allegro5\Build
// Funny to come back to Allegro after so many years...
// Today is 11/7/2020

#include <allegro5/allegro.h>
#include <allegro5/allegro_native_dialog.h>
#include <stdio.h>
#include "Classic99v4.h"

#ifdef WIN32
#include <Windows.h>        // for outputdebugstring
#endif

// display
ALLEGRO_DISPLAY *myWnd = NULL;
int width = 256;
int height = 192;
int wndFlags =  ALLEGRO_WINDOWED        // or ALLEGRO_FULLSCREEN_WINDOW | ALLEGRO_FRAMELESS
             |  ALLEGRO_RESIZABLE
             |  0                       // or ALLEGRO_DIRECT3D or ALLEGRO_OPENGL
             |  ALLEGRO_GENERATE_EXPOSE_EVENTS;    // maybe?

// debug
#define DEBUGLEN 120
char lines[34][DEBUGLEN];               // debug lines
ALLEGRO_MUTEX *DebugCS = NULL;          // we should assume that mutexes are NOT re-entrant!

/////////////////////////////////////////////////////////////////////////
// Write a line to the debug buffer displayed on the debug screen
/////////////////////////////////////////////////////////////////////////
void debug_write(const char *s, ...) {
	char buf[1024];

	_vsnprintf(buf, 1023, s, (char*)((&s)+1));
	buf[1023]='\0';     // allow a longer string out to OutputDebugString

    // TODO: disk log here if configured

#ifdef WIN32
	OutputDebugStringA(buf);
	OutputDebugStringA("\n");
#endif

    // now trim to the final saved length
	buf[DEBUGLEN-1]='\0';

	al_lock_mutex(DebugCS);

    // TODO: this always should have been a ring buffer... ;)
	memcpy(&lines[0][0], &lines[1][0], 33*DEBUGLEN);				// scroll data
	strncpy(&lines[33][0], buf, DEBUGLEN);							// copy in new line
	memset(&lines[33][strlen(buf)], 0x20, DEBUGLEN-strlen(buf));	// clear rest of line
	lines[33][DEBUGLEN-1]='\0';										// zero terminate (paranoia)

	al_unlock_mutex(DebugCS);

    // TODO: notify the debug window
	//bDebugDirty=true;												// flag redraw
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
    al_show_native_message_box(myWnd, "Classic99", "Fatal error", str, NULL, ALLEGRO_MESSAGEBOX_ERROR);
    Cleanup();
    exit(99);
}

// MAN I wanted this basic signature in the old Classic99... ;)
int main(int argc, char **argv) {
    // set up mutexes before anything else
    DebugCS = al_create_mutex();

    // now we can start starting
    debug_write("Starting Classic99 version " VERSION);

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

    // create an event queue, register as we go
    ALLEGRO_EVENT_QUEUE *evtQ = al_create_event_queue();

    // Create the window
    debug_write("Creating window...");

    al_set_new_display_flags(wndFlags);                 // some flags are user-configurable
    al_set_new_display_option(ALLEGRO_COLOR_SIZE, 32, ALLEGRO_REQUIRE);  // require a 32-bit display buffer, assume RGBA?
    al_set_new_display_option(ALLEGRO_SAMPLE_BUFFERS, 0, ALLEGRO_SUGGEST);  // suggest no multisampling
    // al_set_new_window_position(x, y);                // TODO
    myWnd = al_create_display(width, height);
    if (NULL == myWnd) {
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
