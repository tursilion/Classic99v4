// Classic99 v4xx - Copyright 2021 by Mike Brent (HarmlessLion.com)
// See License.txt, but the answer is "just ask me first". ;)

// This file provides a display system for the emulator to use, based
// on bitplanes.

#ifndef TV_H
#define TV_H

#include <allegro5/allegro.h>
#include <memory>
#include <vector>
#include "autobitmap.h"

// ... but I think I will build it so that each layer can have its own resolution and be scaled
// to whatever the window is, rather than compositing a single bitmap. Then I simply don't care
// and Allegro can manage the buffers.

// TODO: want to use setPixelFormat with ALLEGRO_PIXEL_FORMAT_ANY_32_WITH_ALPHA 

#define VERSION "400.000"

// there is meant to be only one television in a system, here at least
// nothing is virtual because there is only one kind of display. This works
// solely with 32-bit pixels with alpha information, so it's up to the VDP
// driver to create that. That keeps this layer lean.
class Classic99TV {
public:
    Classic99TV();
    ~Classic99TV();

    // initialize the TV - this creates a single backplane with
    // a single color, which is probably going to be black because
    // the palette isn't set yet.
    bool init();

    // request a bitmap layer - this is destroyed when the TV is
    // init'd or destroyed (assuming the VDP also is). 
    // Caller gets the pointer but does not solely own it. 
    // A VDP must request layers in the order that
    // they will be drawn - no adding or removing layers later.
    std::shared_ptr<autoBitmap> requestLayer(int w, int h);

    // change the background color
    void setBgColor(ALLEGRO_COLOR col);

    // tell the window loop it's okay to draw
    void setDrawReady(bool isReady) { drawReady = isReady; }

    // Run window processing, blit the display, process all events
    // to be called 60 times a second for NTSC
    // if it returns true, we must exit the application
    bool runWindowLoop();

    // allow systems to find these...
    ALLEGRO_EVENT_QUEUE *evtQ;
    ALLEGRO_DISPLAY *myWnd;

private:
    std::vector< std::shared_ptr<autoBitmap> > layers;
    ALLEGRO_COLOR bgColor;
    ALLEGRO_MUTEX *windowLock;
    int windowXSize, windowYSize;
    bool drawReady;
};

#endif
