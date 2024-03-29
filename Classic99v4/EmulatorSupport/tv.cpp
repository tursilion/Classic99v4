// Classic99 v4xx - Copyright 2021 by Mike Brent (HarmlessLion.com)
// See License.txt, but the answer is "just ask me first". ;)

#include "Classic99v4.h"
#include <cstdio>
#include <allegro5/allegro_native_dialog.h>
#include "debuglog.h"
#include "tv.h"
#include "automutex.h"
#include "autobitmap.h"

// TODO: Filters go here now
//#include "../2xSaI\2xSaI.h"
//#include "../FilterDLL\sms_ntsc.h"

static const int wndFlags =  ALLEGRO_WINDOWED        // or ALLEGRO_FULLSCREEN_WINDOW | ALLEGRO_FRAMELESS
                          |  ALLEGRO_RESIZABLE
#ifdef ALLEGRO_LINUX
                          |  ALLEGRO_GTK_TOPLEVEL    // needed for menus under X
#endif
                          |  0                       // or ALLEGRO_DIRECT3D or ALLEGRO_OPENGL
                          |  ALLEGRO_GENERATE_EXPOSE_EVENTS;    // maybe?

// constructor and destructor
Classic99TV::Classic99TV()
	: evtQ(nullptr)
        , myWnd(nullptr)
        , windowXSize(0)
        , windowYSize(0)
        , drawReady(false)
{
    windowLock = al_create_mutex_recursive();
    bgColor = al_map_rgba(0,0,0,255);
}
Classic99TV::~Classic99TV() {
    debug_write("Cleaning up TV...");
    al_destroy_mutex(windowLock);
    al_destroy_display(myWnd);
}

// initialize the window system - start by tearing down any layers we had
// return false if we fail to start
bool Classic99TV::init() {
    autoMutex lock(windowLock);

    layers.clear();

    if (nullptr == evtQ) {
        // we'll maintain the event queue here
        evtQ = al_create_event_queue();
    }

    if (nullptr == myWnd) {
        // we need to create a window
        // TODO: read window size and position from the configuration
        debug_write("Creating window...");

        al_set_new_display_flags(wndFlags);                 // some flags are user-configurable
        al_set_new_display_option(ALLEGRO_COLOR_SIZE, 32, ALLEGRO_REQUIRE);  // require a 32-bit display buffer
        al_set_new_display_option(ALLEGRO_SAMPLE_BUFFERS, 0, ALLEGRO_SUGGEST);  // suggest no multisampling
        // al_set_new_window_position(x, y);                // TODO: actual position
        myWnd = al_create_display(284*4, 243*4);            // TODO: actual size

        windowXSize = 284*4;
        windowYSize = 243*4;

        if (nullptr == myWnd) {
            printf("Failed to create display\n");
            return false;
        }

        al_register_event_source(evtQ, al_get_display_event_source(myWnd));
        al_set_new_bitmap_format(ALLEGRO_PIXEL_FORMAT_ARGB_8888);   // let's see how well forcing it works...

        // Windows needs to be a video bitmap, or the background color is not properly alpha'd (border goes black)
        // Mac needs a memory bitmap, or the image is lost immediately after it's displayed (blank screen except during updates, lots of stretching and corruption)
        // Linux appears to be similar
        // Raspberry PI 4 requires memory bitmap, but has black borders suggesting the alpha isn't working. Framerate is also very dependent on window size suggesting no acceleration
#ifdef ALLEGRO_WINDOWS
        al_set_new_bitmap_flags(ALLEGRO_CONVERT_BITMAP|ALLEGRO_NO_PRESERVE_TEXTURE|ALLEGRO_ALPHA_TEST|ALLEGRO_MIN_LINEAR);    // old win
#else
        al_set_new_bitmap_flags(ALLEGRO_MEMORY_BITMAP|ALLEGRO_FORCE_LOCKING|ALLEGRO_ALPHA_TEST|ALLEGRO_MIN_LINEAR);           // old everyone else
#endif

        debug_write("Bitmap format is %d", al_get_new_bitmap_format());
    }

    drawReady = true;

    return true;
}

std::shared_ptr<autoBitmap> Classic99TV::requestLayer(int w, int h) {
    std::shared_ptr<autoBitmap> ptr(new autoBitmap(w, h));
    layers.push_back(ptr);
    return ptr;
}

// change the background color
void Classic99TV::setBgColor(ALLEGRO_COLOR col) {
    bgColor = col;
}

// Run window processing, blit the display, process all events
// if it returns true, we must exit the application
bool Classic99TV::runWindowLoop() {
    static bool dontDraw = false;
    int ret = false;    // don't exit

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
            windowXSize = evt.display.width;
            windowYSize = evt.display.height;
            al_acknowledge_resize(myWnd);
            break;
        case ALLEGRO_EVENT_DISPLAY_CLOSE:
            ret = true;
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
            dontDraw = true;
            // If we take too long to send this, we will be considered unresponsive
            al_acknowledge_drawing_halt(myWnd);
            break;
        case ALLEGRO_EVENT_DISPLAY_RESUME_DRAWING:
            // this is the resume for DISPLAY_HALT_DRAWING, now
            // we can restart our timers, audio, and drawing
            debug_write("Resume drawing request");
            dontDraw = false;
            // If we take too long to send this, we will be considered unresponsive
            al_acknowledge_drawing_resume(myWnd);
            break;
        case ALLEGRO_EVENT_JOYSTICK_CONFIGURATION:
            al_reconfigure_joysticks();
            break;
        default:
            debug_write("Got unexpected event %d", evt.type);
        }
    }

    if ((!dontDraw) && (drawReady)) {
        // confirmed okay on Linux and Windows
        // TODO: can we do these outside the loop?
        al_set_blender(ALLEGRO_ADD, ALLEGRO_ALPHA, ALLEGRO_ZERO);
        al_set_render_state(ALLEGRO_ALPHA_TEST, 1);
        al_set_render_state(ALLEGRO_ALPHA_FUNCTION, ALLEGRO_RENDER_EQUAL);
        al_set_render_state(ALLEGRO_ALPHA_TEST_VALUE, 255);

        // clear the backdrop
        al_clear_to_color(bgColor);

        // render the layers
        for (unsigned int idx=0; idx<layers.size(); ++idx) {
            if (nullptr != layers[idx]->bmp) {
                al_draw_scaled_bitmap(layers[idx]->bmp, 0, 0, layers[idx]->w, layers[idx]->h, 
                                                        0, 0, windowXSize, windowYSize, 0);
            }
        }

        al_flip_display();

        // wait to be told it's okay to draw again
        drawReady = false;
    }

    return ret;
}
