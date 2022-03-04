// Classic99 v4xx - Copyright 2021 by Mike Brent (HarmlessLion.com)
// See License.txt, but the answer is "just ask me first". ;)

#include <allegro5/allegro.h>
#ifdef ALLEGRO_WINDOWS
#include <Windows.h>
#endif
#include <cstdio>
#include "automutex.h"

#define DEBUGLEN 120
#define DEBUGLINES 34
static char lines[DEBUGLINES][DEBUGLEN];
bool bDebugDirty = false;
int currentDebugLine;
ALLEGRO_MUTEX *debugLock;      // our object lock

#ifdef ALLEGRO_WINDOWS
    #define vsnprintf _vsnprintf
#endif

// TODO: someday this library may help us go to UTF8: https://github.com/neacsum/utf8

// TODO: I'll probably create some kind of class to display all the debug windows, including this...
void debug_init() {
    debugLock = al_create_mutex_recursive();
    memset(lines, 0, sizeof(lines));
    bDebugDirty = true;
}
void debug_shutdown() {
    al_destroy_mutex(debugLock);
}

// Write a line to the debug buffer displayed on the debug screen
void debug_write(const char *s, ...)
{
    const int bufSize = 1024;
    char buf[bufSize];

    // full length debug output
    va_list argptr;
    va_start(argptr, s);
    vsnprintf(buf, bufSize-1, s, argptr);
    buf[bufSize-1]='\0';

#ifdef ALLEGRO_WINDOWS
    // output to Windows debug listing...
    OutputDebugString(buf);
    OutputDebugString("\n");
#endif
    fprintf(stderr, "%s\n", buf);

    // truncate to rolling array size
    buf[DEBUGLEN-1]='\0';

    {
        autoMutex lock(debugLock);

        memset(&lines[currentDebugLine][0], ' ', DEBUGLEN);	// clear line
        strncpy(&lines[currentDebugLine][0], buf, DEBUGLEN);    // copy in new line
        lines[currentDebugLine][DEBUGLEN-1]='\0';               // zero terminate
        if (++currentDebugLine >= DEBUGLINES) currentDebugLine = 0;

    	bDebugDirty=true;									    // flag redraw
    }

}

// size of the debug text output
void debug_size(int &x, int &y) {
    x = DEBUGLEN+2; // assumes each line includes \r\n on output
    y = DEBUGLINES;
}

// fill in a text buffer
void fetch_debug(char *buf) {
    // buf MUST be debug_size() x*y bytes long
    memset(buf, ' ', DEBUGLINES*(DEBUGLEN+2));

    {
        autoMutex mutex(debugLock);

        int line = currentDebugLine;
        for (int idx=0; idx<DEBUGLINES; ++idx) {
            sprintf(buf, "%s\r\n", lines[line++]);
            buf += DEBUGLEN+2;
        }
    }
}

