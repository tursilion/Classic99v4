// Classic99 v4xx - Copyright 2021 by Mike Brent (HarmlessLion.com)
// See License.txt, but the answer is "just ask me first". ;)

// The ncurses window and pane (or at least pdcurses) is not really up to what I want, so
// I'm just handling it myself. I only need to render the topmost window and I can more easily
// assume the menu bar (ncurses menu isn't available in pdcurses anyway).

// TODO: future: multiple consoles for multiple debug windows
// TODO: maybe color someday? https://tldp.org/HOWTO/NCURSES-Programming-HOWTO/color.html
// TODO: implement optional split panel mode (maybe an F-key to toggle)
// TODO: At startup/shutdown, record whether in split panel mode or not, and what the two panels are
// TODO: each debug panel should implement an optional help screen - displayed if they support it, else help key is ignored
// TODO: input keys can be directed to the individual debug handlers (for instance, the display debug can 'paste' keypresses
//       to allow operation of the emulator purely in text mode)
// TODO: menu bar acts as an input line for all the commands you used to be able to type

// TODO: command line modes:
// - nogui = don't open the graphical window (can use the debug window to operate)
// - notui = don't open the console text window (if possible)
// - winui = add the Win32 menu system to the UI (on the assumption this is more helpful than the console?)

#include "os.h"
#include <raylib.h>
#include <cstdio>
#include <cstring>
#include <thread>
#include <vector>
#include <string.h>
#include <malloc.h>
#include "Classic99v4.h"
#include "automutex.h"
#include "peripheral.h"
#include "debuglog.h"
#include "..\EmulatorSupport\interestingData.h"

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
static unsigned int topMost[2] = { 0, 0 };
static unsigned int numWin = 1;         // only 1 or 2 is legal right now, some hard coding of this exists
static unsigned int curWin = 0;         // current window is 0 or 1
static char *debug_buf = nullptr;       // work buffer for fetching screens from the users
static int debug_buf_size = 0;

using namespace std::chrono_literals;

// TODO: someday this library may help us go to UTF8: https://github.com/neacsum/utf8
// TODO: I'll probably create some kind of class to display all the debug windows, including this...
// TODO: add the command prompt - if you type other than a command key, you get a command line
//       implement as per windowproc.cpp line 3766 in the old Classic99

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
        debug_size(minc, minr, userval);
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
    debug_create_view(NULL, 0);     // debug log
    debug_create_view(NULL, 1);     // help panel
    topMost[0] = 0;
    topMost[1] = 0;
    numWin = 1;
    curWin = 0;

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
    const int GUARDSIZE = 32;

    autoMutex mutex(debugLock);

    // screen size information
    int sr, sc;
    int end1;
    int r,c;
    char buf[64];
    getmaxyx(stdscr, sr, sc);
    if (numWin == 1) {
        end1 = sc;
    } else if (numWin == 2) {
        end1 = sc/2;
    } else {
        numWin = 1;
        end1 = sc;
    }

    if (debugPanes.size() < 1) {
        // this is an error - we should always at least have debug
        strcpy(buf, CLASSIC99VERSION " - No Debug Panes Available");
        if (sc < 64) buf[sc]=0;
        mvprintw(0, 0, buf);
        clrtoeol();
        refresh();
        return;
    }

    mvprintw(0, 0, CLASSIC99VERSION " - [tab], [f6]");
    clrtoeol();
    mvhline(1, 0, ACS_HLINE, sc);

    sr -= 2;    // 2 lines for menu - get what's left
    if (sr <= 0) {
        // something went wrong! we lost the term! resized too small?
        // there's not really any point printing here...
        return;
    }

    for (unsigned int idx=0; idx<numWin; ++idx) {
        int start = 0;
        int width = sc;
        if (idx > 0) {
            start = end1+1;
        }
        if (numWin > 1) {
            width = sc / 2 - idx;   // right pane is one character smaller
        }
        if (topMost[idx] >= debugPanes.size()) {
            topMost[idx] = 0;
        }

        // handle the debug screen, get info and menu line
        bool bottomUp = false;
        WindowTrack *top = &debugPanes[topMost[idx]];
        if (top->pOwner == nullptr) {
            snprintf(buf, sizeof(buf), "[ Debug ]");
            buf[sizeof(buf)-1] = '\0';
            debug_size(c, r, top->userval);
            bottomUp = true;
        } else {
            snprintf(buf, sizeof(buf), "[ %s ]", top->pOwner->getName());
            buf[sizeof(buf)-1] = '\0';
            top->pOwner->getDebugSize(c,r, top->userval);
            if (r < 0) {
                bottomUp = true;
                r = -r;
            }
        }
        if (idx == curWin) {
            // we assume this will fit
            mvprintw(1, start+2, buf);
        }

        int neededSize = r*c;
        if (neededSize > debug_buf_size) {
            if (nullptr != debug_buf) {
                free(debug_buf);
            }
            // might convert these to ints later for attribute support - 32 bytes of guard data
            debug_buf = (char*)malloc(neededSize*sizeof(unsigned char)+GUARDSIZE);
            if (nullptr == debug_buf) {
                debug_buf_size = 0;
            } else {
                debug_buf_size = neededSize;
            }
        }
        if (debug_buf_size == 0) {
            // we couldn't get a debug buffer, return
            mvprintw(0, 0, CLASSIC99VERSION " - Can't allocate debug memory");
            clrtoeol();
            refresh();
            return;
        }
        memset(debug_buf, 0, neededSize);
        memset(debug_buf+neededSize, 0xfd, GUARDSIZE);

        int firstOutLine = 0;
        if (bottomUp) {
            // for the debug log and disasm, we want to show the latest lines that fit
            firstOutLine = r-sr;
            if (firstOutLine < 0) firstOutLine = 0;
        }
        if (top->pOwner == nullptr)  {
            fetch_debug(debug_buf, top->userval);
        } else {
            top->pOwner->getDebugWindow(debug_buf, top->userval);
        }

        // just check a few guard bytes for damage - cancel this client if dead
        if (*((unsigned int*)(debug_buf+neededSize)) != 0xfdfdfdfd) {
            if (top->pOwner == nullptr) {
                fprintf(stderr, "Buffer overflow damage from debug log!\n");
                return;
            }
            debug_write("*** Buffer overflow damage from %s (%d) - disabling", top->pOwner->getName(), top->userval);
            top->pOwner = nullptr;
            // this will turn this pane into debug, but, it's not supposed to ship broken, so okay
            refresh();
            return;
        }

        // now we need to write this into the actual screen window, whatever size we have
        char *workbuf = (char*)alloca(sc+1);
        for (int line = 0; line < sr; ++line) {
            if (line >= r) {
                // if the screen is taller than the debug
                move(line+2,start);
                clrtoeol();
                continue;
            }
            char *adr = (line+firstOutLine)*c+debug_buf;
    #ifdef min
            int w = min(c, width);
    #else
            int w = std::min(c, width);
    #endif
            strncpy(workbuf, adr, w);
            workbuf[w]='\0';
            mvprintw(line+2, start, "%s", workbuf);     // +2 for menu
            if (strlen(workbuf) < width) {
                clrtoeol();
            }
        }
    }

    // draw some more lines before we go
    if (numWin > 1) {
        mvvline(2, end1, ACS_VLINE, sr);
        mvaddch(1, end1, ACS_TTEE);
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
            if (curWin > 1) curWin = 0;
            ch = getch();

            switch (ch) {
                case ERR:
                    // no key was ready
                    break;

                case KEY_RESIZE:
                    // window resize event
                    debug_handle_resize();
                    break;

//                case 'Q':
//                    RequestClose();
//                    //TODO: obviously I don't want to quit on 'Q'
//                    break;

                case '\t':
                    // Tab - change to next window (use control-tab to skip to next device)
                    topMost[curWin]++;
                    if (topMost[curWin] >= debugPanes.size()) {
                        topMost[curWin] = 0;
                    }
                    break;

                case KEY_BTAB:
                    // change panel backwards
                    topMost[curWin]--;
                    if ((topMost[curWin] < 0) || (topMost[curWin] >= debugPanes.size())) {
                        topMost[curWin] = (unsigned int)(debugPanes.size()-1);
                    }
                    break;

                case CTL_TAB:
                    // change panel to next device (skip multiple panes on same device)
                    {
                        Classic99Peripheral *p = debugPanes[topMost[curWin]].pOwner;
                        for (int idx=0; idx<debugPanes.size(); ++idx) {
                            topMost[curWin]++;
                            if (topMost[curWin] >= debugPanes.size()) {
                                topMost[curWin] = 0;
                            }
                            if (debugPanes[topMost[curWin]].pOwner != p) {
                                break;
                            }
                        }
                    }
                    break;

                case KEY_F(6):
                    // turn on dual-panes, or switch between them
                    if (numWin == 1) {
                        numWin = 2;
                        curWin = 1;
                    } else if (curWin == 0) {
                        curWin = 1;
                    } else {
                        curWin = 0;
                    }
                    break;

                case KEY_F(18):     // Shift F1 is actually F13, okay
                    // turn off dual pane
                    numWin = 1;
                    curWin = 0;
                    break;

                default:
                    // should be an actual key not handled above
                    debug_write("Got char code %X", ch);
                    if (NULL != debugPanes[topMost[curWin]].pOwner) {
                        debugPanes[topMost[curWin]].pOwner->debugKey(ch, debugPanes[topMost[curWin]].userval);
                    }
                    break;
            }

            // update the panel system
            debug_update();
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

    topMost[0] = 0;
    topMost[1] = 0;
}

// size of the debug text output
void debug_size(int &x, int &y, int nUser) {
    if (nUser == 0) {
        x = DEBUGLEN;
        y = DEBUGLINES;
    } else if (nUser == 1) {
        x = 42;
        y = 9;
    } else {
        x = 0;
        y = 0;
    }
}

// fill in a text buffer
void fetch_debug(char *buf, int nUser) {
    int r, c;
    debug_size(c, r, nUser);

    if (nUser == 0) {
        autoMutex mutex(debugLock);

        int line = currentDebugLine;
        for (int idx=0; idx<r; ++idx) {
            snprintf(buf, c-1, "%s", lines[line++]);
            if (line >= r) line=0;
            buf += c;
        }
    } else if (nUser == 1) {
        // just navigation help
        snprintf(buf, c-1, "Navigation keys");
        buf += c;
        buf += c;
        snprintf(buf, c-1, "Tab       - next view");
        buf += c;
        snprintf(buf, c-1, "Sh-Tab    - previous view");
        buf += c;
        snprintf(buf, c-1, "Ctrl-Tab  - next device");
        buf += c;
        snprintf(buf, c-1, "F6        - Split panes or Change Active");
        buf += c;
        snprintf(buf, c-1, "Sh-F6     - Un-Split panes");
        buf += c;
        snprintf(buf, c-1, "Type      - Enter command");
        buf += c;
        snprintf(buf, c-1, "Type HELP - Command help to debug pane");
    }
}

