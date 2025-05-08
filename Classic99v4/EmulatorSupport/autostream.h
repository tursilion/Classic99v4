// Classic99 v4xx - Copyright 2021 by Mike Brent (HarmlessLion.com)
// See License.txt, but the answer is "just ask me first". ;)

#ifndef AUTOSTREAM_H
#define AUTOSTREAM_H

#include <raylib.h>
#include <cstring>
#include "../EmulatorSupport/audioSrc.h"

// TODO: hard coded settings for now...

// Just a simple class that wraps the allegro stream for that fancy RAI the kids all love these days
class autoStream {
public:
    // bufsize in ms, bufsize MUST be less than 1000 we crash, freq in hz, depth in bits, conf in # channels (1)
    autoStream(Classic99AudioSrc *src, unsigned int bufsize, unsigned int freq, unsigned int depth, unsigned int conf) {
        pSrc = src;
        sampleCnt = (freq/(1000/bufsize));
        bufferSize = sampleCnt*(depth/8);
        buffer = (unsigned char*)malloc(bufferSize);
        if (NULL == buffer) throw;
        memset(buffer, 0, bufferSize);
        SetAudioStreamBufferSizeDefault(sampleCnt);
        stream = LoadAudioStream(freq, depth, conf);
    }
    autoStream() = delete;

    ~autoStream() {
        StopAudioStream(stream);
        UnloadAudioStream(stream);
        free(buffer);
    }

    // return true if the stream needs data
    bool checkStreamHungry() {
        return IsAudioStreamProcessed(stream);
    }

    // finish filling in a stream buffer - assumes that buffer is filled with bufferSize new bytes!
    void updateBuffer() {
        UpdateAudioStream(stream, buffer, sampleCnt);
    }

    AudioStream stream;
    unsigned int sampleCnt;      // buffer size in samples
    unsigned int bufferSize;     // in bytes
    unsigned char *buffer;
    Classic99AudioSrc *pSrc;
};

#endif
