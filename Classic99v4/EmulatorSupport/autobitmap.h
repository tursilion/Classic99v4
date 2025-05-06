// Classic99 v4xx - Copyright 2025 by Mike Brent (HarmlessLion.com)
// See License.txt, but the answer is "just ask me first". ;)

#ifndef AUTOBITMAP_H
#define AUTOBITMAP_H

#include <raylib.h>

// Just a simple class that wraps the allegro bitmap for that fancy RAI the kids all love these days
class autoBitmap {
public:
    autoBitmap(int width, int height) : w(width), h(height) {
        pixels = (unsigned char*)malloc(w*h*4);
        memset(pixels, 0, w*h*4);
        Image tmpImg = GenImageColor(w, h, BLACK );
        //ImageFormat(&tmpImg, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8);  // not needed, that is the format
        tex = LoadTextureFromImage(tmpImg);
        dirty = true;
    }
    autoBitmap() = delete;

    ~autoBitmap() {
        free(pixels);
        UnloadTexture(tex);
    }

    void setDirty() {
        dirty = true;
    }

    unsigned char* getPixels() {
        return pixels;
    }

    Texture2D getTexture() {
        if (dirty) {
            UpdateTexture(tex, pixels);
            dirty = false;
        }
        return tex;
    }

    unsigned char *pixels;     // RGBA buffer
    Texture2D tex;
    int w,h;
    bool dirty;
};

#endif
