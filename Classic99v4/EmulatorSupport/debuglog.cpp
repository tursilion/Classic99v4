// Classic99 v4xx - Copyright 2021 by Mike Brent (HarmlessLion.com)
// See License.txt, but the answer is "just ask me first". ;)

// The ncurses window and pane (or at least pdcurses) is not really up to what I want, so
// I'm just handling it myself. I only need to render the topmost window and I can more easily
// assume the menu bar (ncurses menu isn't available in pdcurses anyway).

// TODO: someday - also render the emulator window in a pane. Then we can have a startup option
// to not initialize graphics and audio, and run entirely in an SSH window. That would be pretty fun.
// TODO: an option to close the console (maybe with a keypress to reopen it?), or at least push it behind the main window
// TODO: future: multiple consoles for multiple debug windows
// TODO: maybe color someday? https://tldp.org/HOWTO/NCURSES-Programming-HOWTO/color.html

#include "os.h"
#include <raylib.h>
#include <cstdio>
#include <cstring>
#include <thread>
#include <vector>
#include <string.h>
#include <malloc.h>
#include "automutex.h"
#include "peripheral.h"
#include "debuglog.h"

// size of debug log in memory - len is characters, lines is lines
#define DEBUGLEN 1024
#define DEBUGLINES 50
static char lines[DEBUGLINES][DEBUGLEN];
static bool debug_dirty = false;
static volatile bool debug_quit = false;
static int currentDebugLine;
static std::recursive_mutex *debugLock;        // our object lock - MUST HOLD FOR ALL NCURSES ACTIVITY, it's not thread safe
static std::thread *debugThread;               // the actual thread object
static std::vector<WindowTrack> debugPanes;
static unsigned int topMost = 0;
static char *debug_buf = nullptr;     // work buffer for fetching screens from the users
static int debug_buf_size = 0;
static bool mouse_btn_down = false;

using namespace std::chrono_literals;

// TODO: someday this library may help us go to UTF8: https://github.com/neacsum/utf8
// TODO: I'll probably create some kind of class to display all the debug windows, including this...

WindowTrack::WindowTrack(Classic99Peripheral *p, int user) 
    : minr(0)
    , minc(0)
    , userval(user)
    , pOwner(p)
{
    autoMutex lock(debugLock);

    szname[0] = '\0';
    if (NULL != p) {
        p->getDebugSize(minc,minr,userval);
        strncpy(szname, p->getName(), sizeof(szname)-1);
        szname[sizeof(szname)-1] = '\0';
    } else {
        // it's our own debug window -- why not make this a peripheral?
        debug_size(minc, minr);
        strcpy(szname, "Debug");
    }
}

WindowTrack::~WindowTrack() {
}

//TODO: mousemask enables mouse input from the console, which in turn disables quickedit, needed
//      for copy/paste. We'll need an option to disable that, or just a keypress to copy the screen
//      might well be enough.
//      I am starting to think maybe I don't need a mouse driven menu - keyboard driven is more than enough
void debug_thread_init() {
    autoMutex mutex(debugLock);

    // curses setup
    initscr();
    raw();                  // no signal keypresses
    noecho();               // no character echo
    keypad(stdscr, TRUE);   // enable extended keys
    nodelay(stdscr, TRUE);  // non-blocking getch
    //mousemask(BUTTON1_PRESSED|BUTTON1_RELEASED, NULL);   // respond to mouse clicks
    curs_set(0);            // invisible cursor

    // set up the panes system
    debugPanes.clear();
    debug_create_view(NULL, 0);
    topMost = 0;

    // our own setup
    memset(lines, 0, sizeof(lines));
    debug_dirty = true;
    debug_quit = false;
}

void debug_handle_resize() {
    autoMutex mutex(debugLock);

    //debug_write("Resize to %d, %d", LINES, COLS);

    // warning: if you do add windows later, don't put any work between the
    // resize_term and the windows being replaced, it can cause the copies to
    // overrun the buffers.
    resize_term(0,0);
}

void debug_update() {
    autoMutex mutex(debugLock);

    if (topMost >= debugPanes.size()) {
        topMost = 0;
        if (debugPanes.size() < 1) {
            int sr, sc;
            getmaxyx(stdscr, sr, sc);
            (void)sr;
            char buf[64];
            strcpy(buf, "Classic99 v4xx - No Debug Panes Available");
            if (sc < 64) buf[sc]=0;
            mvprintw(0, 0, buf);
            clrtoeol();
            return;
        }
    }

    // screen size information
    int sr, sc;
    getmaxyx(stdscr, sr, sc);

    // handle the debug screen, get info and menu line
    WindowTrack *top = &debugPanes[topMost];
    int r,c;
    char buf[64];
    if (top->pOwner == nullptr) {
        snprintf(buf, sizeof(buf), "Classic99 v4xx - [tab] - Debug");
        buf[sizeof(buf)-1] = '\0';
        debug_size(c, r);
    } else {
        snprintf(buf, sizeof(buf), "Classic99 v4xx - [tab] - %s", top->pOwner->getName());
        buf[sizeof(buf)-1] = '\0';
        top->pOwner->getDebugSize(c,r, top->userval);
    }
    mvprintw(0, 0, buf);
    clrtoeol();
    mvhline(1, 0, ACS_HLINE, sc);

    int neededSize = r*c;
    if (neededSize > debug_buf_size) {
        if (nullptr != debug_buf) {
            free(debug_buf);
        }
        // might convert these to ints later for attribute support
        debug_buf = (char*)malloc(neededSize*sizeof(unsigned char));
        if (nullptr == debug_buf) {
            debug_buf_size = 0;
        } else {
            debug_buf_size = neededSize;
        }
    }

    sr -= 2;    // 2 lines for menu - get what's left
    if (sr <= 0) {
        // something went wrong! we lost the term! resized too small?
        return;
    }
    int firstOutLine = 0;
    if (top->pOwner == nullptr) {
        fetch_debug(debug_buf);
        // for the debug log, we want to show the latest lines that fit
        // this will be true for things like the disassembly view later too, 
        // so we might make it a flag on get size or get window
        firstOutLine = r-sr;
        if (firstOutLine < 0) firstOutLine = 0;
    } else {
        top->pOwner->getDebugWindow(debug_buf, top->userval);
    }

    // now we need to write this into the actual screen window, whatever size we have
    char *workbuf = (char*)alloca(sc+1);
    for (int line = 0; line < sr; ++line) {
        if (line >= r) {
            // if the screen is taller than the debug
            move(line+2,0);
            clrtoeol();
            continue;
        }
        char *adr = (line+firstOutLine)*c+debug_buf;
#ifdef min
        int w = min(c, sc);
#else
        int w = std::min(c, sc);
#endif
        strncpy(workbuf, adr, w);
        workbuf[w]='\0';
        mvprintw(line+2, 0, "%s", workbuf);     // +2 for menu
        clrtoeol();
    }

    refresh();
}

// the main loop that manages the debug system - debugLock must be created BEFORE calling
void debug_thread() {
    threadname("debug_thread");

    // initialization
    debug_thread_init();

    while (!debug_quit) {
        int ch;
        {
            autoMutex mutex(debugLock);
            ch = getch();

            if (ch == KEY_MOUSE) {
                MEVENT event;
                if (getmouse(&event) == OK) {
                    // we also get x,y movement while the button is down, but with bstate==0
                    if (event.bstate & BUTTON1_PRESSED) {
                        debug_write("Mouse click at %d,%d", event.x, event.y);
                        mouse_btn_down = true;
                    } else if (event.bstate & BUTTON1_RELEASED) {
                        mouse_btn_down = false;
                    }
                    // TODO: eventually a menu system
                }
            } else if (ch == KEY_RESIZE) {
                debug_handle_resize();
            } else if (ch != ERR) {
                //debug_write("Got char: %c", ch);
                if (ch =='Q') {
                    RequestClose();
                    //TODO: obviously I don't want to quit on 'Q'
                } else if (ch == '\t') {
                    // change panel - again, this is all temporary test code
                    topMost++;
                    if (topMost >= debugPanes.size()) {
                        topMost = 0;
                    }
                }
            }

            // update the panel system
            if (!mouse_btn_down) {
                debug_update();
            }
        }

        // sleep 5ms or whatever quantum is, it's only keypresses
        std::this_thread::sleep_for(5ms);
    }

    endwin();
}

//--- functions above this point are intended to be used by the debug_thread() only
//--- functions below this point are intended to be called from anywhere in the application
extern "C" {
    void rl_set_debug_write(void (*ptr)(const char*, va_list));
};

void debug_init() {
    debugLock = new std::recursive_mutex();

    // Start the debug thread
    debugThread = new std::thread(debug_thread);
    // give the thread ample time to start
    std::this_thread::sleep_for(100ms);

    // try to acquire the lock - that will tell us it's finished
    {
        autoMutex mutex(debugLock);
    }

    // tell raylib it can redirect to us
    rl_set_debug_write(debug_write_var);

    debug_write("Debug thread initialized");
}

// stop debug, but don't clean up yet
void debug_stop() {
    // stop running the debug thread, so other objects can safely shut down
    debug_quit = true;
    if (nullptr != debugThread) {
        debugThread->join();
    }
}

void debug_shutdown() {
    debug_quit = true;
    if (nullptr != debugThread) {
        if (debugThread->joinable()) {
            debugThread->join();
        }
    }
    endwin();
    delete debugLock;
}

// Write a line to the debug buffer displayed on the debug screen
// TODO: add a disk log which can be configured on/off
void debug_write(const char *s, ...)
{
    va_list argptr;
    va_start(argptr, s);
    debug_write_var(s, argptr);
}

void debug_write_var(const char *s, va_list argptr) {
    char buf[DEBUGLEN];

    vsnprintf(buf, DEBUGLEN-1, s, argptr);
    buf[DEBUGLEN-1]='\0';

    size_t p = strlen(buf);
    while ((p>0) && (buf[p-1] < ' ')) {
        buf[p-1]='\0'; 
        --p;
    }

#ifdef _WINDOWS
    // output to Windows debug listing...
    OutputDebugString(buf);
    OutputDebugString("\n");
#endif

    {
        autoMutex lock(debugLock);

        memset(&lines[currentDebugLine][0], 0, DEBUGLEN);	    // clear line
        strncpy(&lines[currentDebugLine][0], buf, DEBUGLEN-1);  // copy in new line
        if (++currentDebugLine >= DEBUGLINES) currentDebugLine = 0;

    	debug_dirty=true;									    // flag redraw
    }
}

// request a new debug view of the specified size
// TODO: Should this run through peripheral? I made a standard interface...
WindowTrack *debug_create_view(Classic99Peripheral *pOwner, int userval) {
    autoMutex lock(debugLock);
    debugPanes.emplace_back(pOwner, userval);
    return &debugPanes.back();
}

// Unregister our view(s)
void debug_unregister_view(Classic99Peripheral *pOwner) {
    autoMutex lock(debugLock);

    for (std::vector<WindowTrack>::iterator it = debugPanes.begin(); it != debugPanes.end(); ) {
        if (it->pOwner == pOwner) {
            it = debugPanes.erase(it);
        } else {
            ++it;
        }
    }

    topMost = 0;
}

// size of the debug text output
void debug_size(int &x, int &y) {
    x = DEBUGLEN;
    y = DEBUGLINES;
}

// fill in a text buffer
void fetch_debug(char *buf) {
    // buf MUST be debug_size() x*y bytes long
    memset(buf, '\0', DEBUGLINES*DEBUGLEN);

    {
        autoMutex mutex(debugLock);

        int line = currentDebugLine;
        for (int idx=0; idx<DEBUGLINES; ++idx) {
            snprintf(buf, DEBUGLEN-1, "%s", lines[line++]);
            if (line >= DEBUGLINES) line=0;
            buf += DEBUGLEN;
        }
    }
}

