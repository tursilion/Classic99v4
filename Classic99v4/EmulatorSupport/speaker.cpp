// Classic99 v4xx - Copyright 2021 by Mike Brent (HarmlessLion.com)
// See License.txt, but the answer is "just ask me first". ;)

#include <allegro5/allegro.h>
#include <allegro5/allegro_audio.h>
#include "Classic99v4.h"
#include "debuglog.h"
#include "speaker.h"
#include "automutex.h"
#include "autostream.h"

// constructor and destructor
Classic99Speaker::Classic99Speaker()
    : delay(80)
    , bufsize(10)
    , freq(44100)
    , depth(ALLEGRO_AUDIO_DEPTH_INT16)
    , conf(ALLEGRO_CHANNEL_CONF_1)
{
    init();
}
Classic99Speaker::~Classic99Speaker() {
    debug_write("Cleaning up Speaker...");
}

// initialize the audio system - start by tearing down any layers we had
// return false if we fail to start
bool Classic99Speaker::init() {
    layers.clear();

    if (!al_is_audio_installed()) {
        if (!al_install_audio()) {
            debug_write("** Failed to install allegro audio **");
            return false;
        }
    }
    if (!al_restore_default_mixer()) {
        debug_write("Failed to install allegro mixer");
        return false;
    }
    al_set_mixer_quality(al_get_default_mixer(), ALLEGRO_MIXER_QUALITY_CUBIC);
    al_set_mixer_playing(al_get_default_mixer(), true);

    return true;
}

std::shared_ptr<autoStream> Classic99Speaker::requestStream(Classic99AudioSrc *pSrc) {
    std::shared_ptr<autoStream> ptr(new autoStream(pSrc, delay, bufsize, freq, depth, conf));
    layers.push_back(ptr);
    return ptr;
}

// check if any streams need service and service them
bool Classic99Speaker::runSpeakerLoop() {
    // render the layers
    for (unsigned int idx=0; idx<layers.size(); ++idx) {
        std::shared_ptr<autoStream> ptr = layers[idx];
        while (ptr->checkStreamHungry()) {
            void *pBuf = ptr->getStreamPointer();
            if (nullptr != pBuf) {
                // bufferSize is in samples, so we multiply by 2 for ALLEGRO_AUDIO_DEPTH_INT16
                ptr->pSrc->fillAudioBuffer(pBuf, ptr->bufferSize*2, ptr->bufferSize);
                ptr->finishPointer(pBuf);
            }
        }
    }

    return true;
}
