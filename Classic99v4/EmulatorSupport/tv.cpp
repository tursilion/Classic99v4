// Classic99 v4xx - Copyright 2021 by Mike Brent (HarmlessLion.com)
// See License.txt, but the answer is "just ask me first". ;)

#include <cstdio>
#include <raylib.h>
#include "debuglog.h"
#include "tv.h"
#include "automutex.h"
#include "autobitmap.h"

// TODO: Filters go here now
//#include "../2xSaI\2xSaI.h"
//#include "../FilterDLL\sms_ntsc.h"

// Replacement for Raylib color to int so I can write littleendian longs
int InvColorToInt(Color color)
{
    int result = 0;

    result = (int)(((unsigned int)color.r) |
                   ((unsigned int)color.g << 8) |
                   ((unsigned int)color.b << 16) |
                    (unsigned int)color.a << 24);

    return result;
}

// constructor and destructor
Classic99TV::Classic99TV()
    : windowXSize(0)
    , windowYSize(0)
    , drawReady(false)
{
    windowLock = new std::recursive_mutex();
    bgColor = BLACK;
}
Classic99TV::~Classic99TV() {
    debug_write("Cleaning up TV...");
    
    delete windowLock;

    CloseWindow();
}

// initialize the window system - start by tearing down any layers we had
// return false if we fail to start
bool Classic99TV::init() {
    autoMutex lock(windowLock);

    layers.clear();

    if (!IsWindowReady()) {
        // we need to create a window
        // TODO: read window size and position from the configuration
        debug_write("Creating window...");

        windowXSize = 284*4;    // todo: we might not need these anymore... except for load/save - see the draw loop
        windowYSize = 243*4;

        SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_VSYNC_HINT | FLAG_WINDOW_ALWAYS_RUN | FLAG_WINDOW_HIGHDPI);
        InitWindow(windowXSize, windowYSize, "Classic99v4");            // TODO: actual size
        SetWindowMinSize(284, 243);
        SetExitKey(KEY_NULL);  // don't trap escape

        debug_write("Screen is %d x %d", GetScreenWidth(), GetScreenHeight());


        // SetWindowPosition(x, y);                // TODO: actual position

        // TODO: /can/ we fail?
        //if (nullptr == myWnd) {
        //    printf("Failed to create display\n");
        //    return false;
        //}
    }

    // TODO: configurable FPS
    SetTargetFPS(60);

    drawReady = true;

    return true;
}

std::shared_ptr<autoBitmap> Classic99TV::requestLayer(int w, int h) {
    std::shared_ptr<autoBitmap> ptr(new autoBitmap(w, h));
    layers.push_back(ptr);
    return ptr;
}

// change the background color
void Classic99TV::setBgColor(Color col) {
    bgColor = col;
}

// Run window processing, blit the display, process all events
// if it returns true, we must exit the application
bool Classic99TV::runWindowLoop() {
    static bool dontDraw = false;
    int ret = false;    // don't exit

    // TODO: handle resize. Do we have to handle anything else window/os wise?

    if ((!dontDraw) && (drawReady)) {
        BeginDrawing();

            // clear the backdrop
            ClearBackground(bgColor);

            // need to honor alpha and use with DrawTexture
            //BeginBlendMode(BLEND_ALPHA);

                // render the layers
                for (unsigned int idx=0; idx<layers.size(); ++idx) {
                    Texture2D texture = layers[idx]->getTexture();
                    Rectangle source = { 0.0f, 0.0f, (float)texture.width, (float)texture.height };
                    Rectangle dest = { 0, 0, (float)GetScreenWidth(), (float)GetScreenHeight() };
                    Vector2 origin = { 0.0f, 0.0f };
                    DrawTexturePro(texture, source, dest, origin, 0.0f, WHITE);
                }

            //EndBlendMode();

        EndDrawing();   // by default, timing handled here - that means this will block!

        // wait to be told it's okay to draw again
        drawReady = false;
    }

    return ret;
}
