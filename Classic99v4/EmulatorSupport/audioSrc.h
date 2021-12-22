// Classic99 v4xx - Copyright 2021 by Mike Brent (HarmlessLion.com)
// See License.txt, but the answer is "just ask me first". ;)

#ifndef AUDIOSRC_H
#define AUDIOSRC_H

#include <allegro5/allegro.h>
#include <allegro5/allegro_audio.h>

// Just a simple interface class that defines a method by which to fetch audio from a source
class Classic99AudioSrc {
public:
    Classic99AudioSrc() { }
    virtual ~Classic99AudioSrc() { }

    // it's your job to use the right sample format
    virtual void fillAudioBuffer(void *buf, int bufSize, int samples) = 0;
};

#endif
