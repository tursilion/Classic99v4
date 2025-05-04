// Classic99 v4xx - Copyright 2021 by Mike Brent (HarmlessLion.com)
// See License.txt, but the answer is "just ask me first". ;)

#ifndef AUTOMUTEX_H
#define AUTOMUTEX_H

#include <mutex>

// Just a simple class that wraps the C++ mutex for that fancy RAI the kids all love these days
class autoMutex {
public:
    autoMutex(std::mutex *m) : mymutex(m) {
        mymutex->lock();
    }
    autoMutex() = delete;

    ~autoMutex() {
        mymutex->unlock();
    }

private:
    std::mutex *mymutex;
};

#endif
