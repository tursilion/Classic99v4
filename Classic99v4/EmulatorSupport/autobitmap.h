// Classic99 v4xx - Copyright 2021 by Mike Brent (HarmlessLion.com)
// See License.txt, but the answer is "just ask me first". ;)

#ifndef AUTOBITMAP_H
#define AUTOBITMAP_H

#include <allegro5/allegro.h>

// Just a simple class that wraps the allegro bitmap for that fancy RAI the kids all love these days
class autoBitmap {
public:
    autoBitmap(int width, int height) : w(width), h(height) {
        bmp = al_create_bitmap(w, h);
    }
    autoBitmap() = delete;

    ~autoBitmap() {
        al_destroy_bitmap(bmp);
    }

    ALLEGRO_BITMAP *bmp;
    int w,h;
};

#endif
