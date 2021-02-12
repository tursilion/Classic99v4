// Classic99 v4xx - Copyright 2021 by Mike Brent (HarmlessLion.com)
// See License.txt, but the answer is "just ask me first". ;)

#ifndef AUTOMUTEX_H
#define AUTOMUTEX_H

#include <allegro5/allegro.h>

// Just a simple class that wraps the allegro mutex for that fancy RAI the kids all love these days
class autoMutex {
public:
    autoMutex(ALLEGRO_MUTEX *m) : mutex(m) {
        al_lock_mutex(mutex);
    }
    autoMutex() = delete;

    ~autoMutex() {
        al_unlock_mutex(mutex);
    }

private:
    ALLEGRO_MUTEX *mutex;
};

#endif
