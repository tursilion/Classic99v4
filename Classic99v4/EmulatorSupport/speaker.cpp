// Classic99 v4xx - Copyright 2021 by Mike Brent (HarmlessLion.com)
// See License.txt, but the answer is "just ask me first". ;)

#include <raylib.h>
#include "Classic99v4.h"
#include "debuglog.h"
#include "speaker.h"
#include "automutex.h"
#include "autostream.h"

// constructor and destructor
Classic99Speaker::Classic99Speaker()
    : bufsize(10)
    , freq(44100)
    , depth(16)
    , conf(1)
{
}
Classic99Speaker::~Classic99Speaker() {
    debug_write("Cleaning up Speaker...");
}

// initialize the audio system - start by tearing down any layers we had
// return false if we fail to start
bool Classic99Speaker::init() {
    layers.clear();

    if (!IsAudioDeviceReady()) {
        InitAudioDevice();
    }

    return true;
}

std::shared_ptr<autoStream> Classic99Speaker::requestStream(Classic99AudioSrc *pSrc) {
    std::shared_ptr<autoStream> ptr(new autoStream(pSrc, bufsize, freq, depth, conf));
    layers.push_back(ptr);
    return ptr;
}

// check if any streams need service and service them
bool Classic99Speaker::runSpeakerLoop() {
    // render the layers
    for (unsigned int idx=0; idx<layers.size(); ++idx) {
        std::shared_ptr<autoStream> ptr = layers[idx];
        while (ptr->checkStreamHungry()) {
            void *pBuf = ptr->buffer;
            if (nullptr != pBuf) {
                ptr->pSrc->fillAudioBuffer(pBuf, ptr->bufferSize, ptr->sampleCnt);
                ptr->updateBuffer();
            }
        }
    }

    return true;
}
