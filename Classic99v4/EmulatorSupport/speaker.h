// Classic99 v4xx - Copyright 2021 by Mike Brent (HarmlessLion.com)
// See License.txt, but the answer is "just ask me first". ;)

// This file provides an audio system for the emulator to use,
// and the handler code needed to keep the buffers filled. This adds
// allegro_audio as a dependency.

#ifndef SPEAKER_H
#define SPEAKER_H

#ifndef CONSOLE_BUILD
#include <raylib.h>
#endif
#include <memory>
#include <vector>
#include "autostream.h"

// forward reference
class Classic99System;

// there is meant to be only one speaker in a system, here at least
// nothing is virtual because there is only one kind of speaker.
class Classic99Speaker {
public:
    Classic99Speaker(Classic99System *core);
    ~Classic99Speaker();

    // initialize the speaker - this creates the default mixer
    // TODO: we might be able to have finer grained volume control
    // if we use multiple mixers. For now, just one.
    bool init();

    // request a stream layer - this is destroyed when the speaker is
    // init'd or destroyed.
    // Caller gets the pointer but does not solely own it. 
    // all streams are mixed together ultimately.
    std::shared_ptr<autoStream> requestStream(Classic99AudioSrc *pSrc);

    // Run speaker processing, service any streams needing data
    // we're using polling instead of events, but that should be fine,
    // since we'd only check the events at fixed points anyway
    bool runSpeakerLoop();

    // TODO: these are all hard-coded for now, eventually we need to export for the sake of config
    unsigned int bufsize;
    unsigned int freq;
    unsigned int depth;
    unsigned int conf;

//private:
    std::vector<std::shared_ptr<autoStream>> layers;
};

#endif
