// Classic99 v4xx - Copyright 2021 by Mike Brent (HarmlessLion.com)
// See License.txt, but the answer is "just ask me first". ;)

#ifndef AUTOSTREAM_H
#define AUTOSTREAM_H

#include <allegro5/allegro.h>
#include <allegro5/allegro_audio.h>
#include "../EmulatorSupport/audioSrc.h"

// TODO: hard coded settings for now...

// Just a simple class that wraps the allegro stream for that fancy RAI the kids all love these days
class autoStream {
public:
    // delay and bufsize in ms, bufsize MUST be less than 1000 we crash, freq in hz, other two as per allegro docs
    autoStream(Classic99AudioSrc *src, int delay, int bufsize, int freq, ALLEGRO_AUDIO_DEPTH depth, ALLEGRO_CHANNEL_CONF conf) {
        pSrc = src;
        bufferSize = freq/(1000/bufsize);
        stream = al_create_audio_stream(delay/bufsize, bufferSize, freq, depth, conf);
    }
    autoStream() = delete;

    ~autoStream() {
        al_destroy_audio_stream(stream);
    }

    // return true if the stream needs data
    bool checkStreamHungry() {
        if (nullptr == stream) return false;
        return (al_get_available_audio_stream_fragments(stream) > 0);
    }

    // return a pointer to the next free buffer, or nullptr. You MUST call finishPointer
    // if you didn't get null. The buffer is bufferSize samples long.
    void *getStreamPointer() {
        return al_get_audio_stream_fragment(stream);
    }

    // finish filling in a stream buffer
    void finishPointer(void *val) {
        al_set_audio_stream_fragment(stream, val);
    }

    ALLEGRO_AUDIO_STREAM *stream;
    int bufferSize;     // in samples
    Classic99AudioSrc *pSrc;
};

#endif
