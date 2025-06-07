// Classic99 v4xx - Copyright 2025 by Mike Brent (HarmlessLion.com)
// See License.txt, but the answer is "just ask me first". ;)

// TODO: Windows extension to inject real menu bar and file dialog etc for better integration?

#include "os.h"
#include <raylib.h>
#include <cstdio>
#include <cstring>
#include <thread>
#include <vector>
#include <string.h>
#include <stdlib.h>
#include "Classic99v4.h"
#include "automutex.h"
#include "peripheral.h"
#include "debuglog.h"
#include "debugmenu.h"
#include "interestingData.h"

extern std::recursive_mutex *debugLock;        // our object lock - MUST HOLD FOR ALL NCURSES ACTIVITY, it's not thread safe
static std::vector<MenuTrack> menuPanes;
static std::vector<MenuTrack> peripheralMenus;
static std::vector<MenuTrack> openMenus;

// TODO: Support hotkey on first uppercase letter
MenuInit menuFile[] = {
    {   "FILE", 0,                                  },  // first row is title line
    {   "Cold Reset",   DEBUG_CMD_FILE_COLDRESET    },
    {   "Quit",         DEBUG_CMD_FILE_QUIT         },
    { "",-1 }
};
MenuInit menuEdit[] = {
    {   "EDIT", 0,                                      },  // first row is title line
    {   "Split Debug",   DEBUG_CMD_EDIT_SPLIT_DEBUG     },
    {   "Collapse Debug",DEBUG_CMD_EDIT_COLLAPSE_DEBUG  },
    { "",-1 }
};
MenuInit menuSystem[] = {
    {   "SYSTEM", 0,                                    },  // first row is title line
    {   "Split Debug",   DEBUG_CMD_EDIT_SPLIT_DEBUG     },
    {   "Collapse Debug",DEBUG_CMD_EDIT_COLLAPSE_DEBUG  },
    { "",-1 }
};
// TODO: this is probably different for non-TI systems
MenuInit menuCart[] = {
    {   "CARTRIDGE",    0,                      },  // first row is title line
    {   "Apps",         DEBUG_CMD_CART_APPS     },
    {   "Games",        DEBUG_CMD_CART_GAMES    },
    {   "User (Open)",  DEBUG_CMD_CART_OPEN     },
    { "",-1 }
};
MenuInit menuPeripheral[] = {
    {   "PERIPHERAL",   0                       },  // this one is dynamically updated
    { "",-1 }
};
MenuInit menuHelp[] = {
    {   "HELP",         0,                      },  // first row is title line
    {   "About",        DEBUG_CMD_HELP_ABOUT    },
    { "",-1 }
};

using namespace std::chrono_literals;

MenuTrack::MenuTrack(Classic99Peripheral *p, MenuInit *m)
    : pOwner(p)
    , currentSelection(0)
    , pMenu(m)
{
    static MenuInit x = { "unset", 0 };
    menuCnt = 0;
    maxWidth = 0;
    // work out how many elements, and the maximum width
    // we MUST have a MenuInit
    if (nullptr == m) {
        pMenu = &x;
        maxWidth = 5;   // length of "unset"
        menuCnt = 1;
    } else {
        MenuInit *pIdx = m;
        while (pIdx->cmd >= 0) {
            int len = (int)strlen(pIdx->str);
            if (len > maxWidth) {
                maxWidth = len;
            }
            ++menuCnt;
            ++pIdx;
        }
    }
}

MenuTrack::~MenuTrack() {
}

// draw this menu r and c is the top left corner
// update will draw the top line, we only need to worry about our lines
void MenuTrack::drawMenu(int r, int c) {
    const int MAXIMUM=32;
    // get screen size
    int sr, sc;
    getmaxyx(stdscr, sr, sc);

    int width = maxWidth + 4;   // space on each side plus room for the border
    if (width+c > sc) width=sc-c;
    if (width < 5) return;      // too small to draw
    if (width > MAXIMUM) width=MAXIMUM;   // be reasonable

    int height = (menuCnt-1) + 2;   // top and bottom border
    if (height+r > sr) height=sr-r;
    if (height < 3) return;     // too small to draw

    // draw the box
    mvhline(r, c, ACS_HLINE, width);
    mvhline(r+height-1, c, ACS_HLINE, width);
    mvvline(r, c, ACS_VLINE, height);
    mvvline(r, c+width-1, ACS_VLINE, height);
    mvaddch(r, c, ACS_ULCORNER);
    mvaddch(r, c+width-1, ACS_URCORNER);
    mvaddch(r+height-1, c, ACS_LLCORNER);
    mvaddch(r+height-1, c+width-1, ACS_LRCORNER);

    // i starts at 1, so we don't need to increment r
    for (int i=1; i<menuCnt; ++i) {
        char buf[MAXIMUM];
        memset(buf, 0, sizeof(buf));
        if (width-2 < sizeof(buf)) {
            mvhline(r+i, c+1, ' ', width-2);
            strncpy(buf, pMenu[i].str, width-2);
            if (i == currentSelection) {
                // only happens once, so only do the extra attribute work once
                attron(A_REVERSE);
                mvprintw(r+i, c+2, buf);
                attroff(A_REVERSE);
            } else {
                mvprintw(r+i, c+2, buf);
            }
        }
    }
}

int MenuTrack::getCurrentKey() {
    if (currentSelection < 1) {
        return 0;
    }
    if (currentSelection >= menuCnt) {
        return 0;
    }
    if (pMenu == nullptr) {
        return 0;
    }
    return pMenu[currentSelection].cmd;
}

void menu_open_first() {
    autoMutex mutex(debugLock);
    debug_open_menu(&menuPanes[0]);
}

// basic menuing system - called every frame overtop of the debug views
// ch - last received keypress (KEY_ERR if none)
// If this function is called, then the menu system is active
// if we return true, the last menu is closed
bool menu_update(int ch) {
    // We need a function to draw a pop up menu list anywhere,
    // then we just track which one we're on and where it is,
    // to navigate up/down. On enter, we just send the
    // resulting command ID to the owner, it deals with whether
    // to close the menu or open a new one. (We'll need a close menu function).
    autoMutex mutex(debugLock);

    if (openMenus.size() == 0) {
        // no menus are open! all done
        return true;
    }

    // the current live menu
    MenuTrack &currentMenu = openMenus.back();
    bool isRootMenu = (openMenus.size() == 1);
    
    // inefficient, but it's a small search
    int currentIdx = -1;
    for (int i=0; i<menuPanes.size(); ++i) {
        if (0 == strcmp(menuPanes[i].getMenuName(), openMenus.back().getMenuName())) {
            currentIdx = i;
            break;
        }
    }

    // Make sure it's got a valid selection
    if (currentMenu.currentSelection == 0) {
        currentMenu.currentSelection = 1;
    }

    // First, use ch to update the menu state
    // up and down select within a menu
    // left and right change which menu you're on, IF on the root menu
    // escape closes the current menu
    // enter sends the command to the menu owner, which may choose to open a new menu or close the current one
    switch (ch) {
        case -1:
            // no key - this is not an error
            break;

        case 0x1b:
            // ESCAPE
            debug_close_menu();
            if (openMenus.size() == 0) {
                // no more menus are open
                return true;
            }
            break;

        case KEY_UP:
            if (currentMenu.currentSelection > 1) --currentMenu.currentSelection;
            break;

        case KEY_DOWN:
            if (currentMenu.currentSelection < currentMenu.getMenuCnt()-1) ++currentMenu.currentSelection;
            break;

        case KEY_LEFT:
            if (isRootMenu) {
                if (currentIdx > 0) {
                    debug_close_menu();
                    --currentIdx;
                    debug_open_menu(&menuPanes[currentIdx]);
                    currentMenu = openMenus.back();
                }
            } else {
                // same as pressing esc
                debug_close_menu();
            }
            break;

        case KEY_RIGHT:
            if (isRootMenu) {
                if (currentIdx < menuPanes.size()-1) {
                    debug_close_menu();
                    ++currentIdx;   // currentMenu becomes invalid, but that's okay
                    debug_open_menu(&menuPanes[currentIdx]);
                    currentMenu = openMenus.back();
                }
            }
            // no else, you have to press enter to select
            break;

        case '\r':
            // ENTER
            if (currentMenu.pOwner != nullptr) {
                currentMenu.pOwner->debugKey(currentMenu.getCurrentKey(), 0);
            } else {
                debug_control(currentMenu.getCurrentKey());
            }
            break;

        default:
            debug_write("Menu got key 0x%02X", ch);
            break;
    }

    // now draw the top line, highlight the active one and remember where it is
    int sr, sc;
    getmaxyx(stdscr, sr, sc);
    move(0,0);
    clrtoeol();

    int dr=0;
    int dc=0;
    int activeDC = 0;
    const char *szMatch = "none";
    if (openMenus.size() > 0) {
        szMatch = openMenus.back().getMenuName();
    }

    for (int i=0; i<menuPanes.size(); ++i) {
        const char *str = menuPanes[i].getMenuName();
        int len = (int)strlen(str);
        if (len + dc >= sc) break;

        if (0 == strcmp(str, szMatch)) {
            attron(A_REVERSE);
            mvprintw(dr, dc+2, str);
            attroff(A_REVERSE);
            activeDC = dc;
        } else {
            mvprintw(dr, dc+2, str);
        }

        dc += len+2;
    }

    // we'll try just stacking them, whatever...
    dr=1;
    dc=activeDC;
    for (int i=0; i<openMenus.size(); ++i) {
        // TODO: Peripheral menu needs to be special
        openMenus[i].drawMenu(dr, dc);
        dc += 4;
    }
    
    // menu not finished
    return false;
}

// request a new menu be added to the main menu list - this is a top level menu
MenuTrack *debug_add_menu(MenuInit *menuList) {
    autoMutex lock(debugLock);
    menuPanes.emplace_back(nullptr, menuList);     // owned by debug
    return &menuPanes.back();
}
// request a new menu be added to the peripheral menu list - this is for peripherals
MenuTrack *debug_add_peripheral_menu(Classic99Peripheral *pOwner, MenuInit *menuList) {
    autoMutex lock(debugLock);
    peripheralMenus.emplace_back(pOwner, menuList);
    return &peripheralMenus.back();
}

// Unregister our view(s)
void debug_unregister_menu(Classic99Peripheral *pOwner) {
    autoMutex lock(debugLock);

    for (std::vector<MenuTrack>::iterator it = openMenus.begin(); it != openMenus.end(); ) {
        if (it->pOwner == pOwner) {
            it = openMenus.erase(it);
        } else {
            ++it;
        }
    }

    for (std::vector<MenuTrack>::iterator it = menuPanes.begin(); it != menuPanes.end(); ) {
        if (it->pOwner == pOwner) {
            it = menuPanes.erase(it);
        } else {
            ++it;
        }
    }

    for (std::vector<MenuTrack>::iterator it = peripheralMenus.begin(); it != peripheralMenus.end(); ) {
        if (it->pOwner == pOwner) {
            it = peripheralMenus.erase(it);
        } else {
            ++it;
        }
    }
}

// open the current menu as the new main menu
void debug_open_menu(MenuTrack *pMenu) {
    autoMutex lock(debugLock);
    openMenus.push_back(*pMenu);
}

// close the current main menu
void debug_close_menu() {
    autoMutex lock(debugLock);
    if (openMenus.size() > 0) {
        openMenus.pop_back();
    }
}

// close all menus
void debug_close_all_menu() {
    autoMutex lock(debugLock);
    while (openMenus.size() > 0) {
        openMenus.pop_back();
    }
}    

// init the menu system
void debug_init_menu() {
    autoMutex lock(debugLock);

    openMenus.clear();          // currently open menus
    menuPanes.clear();          // base menus for the top line
    peripheralMenus.clear();    // menus for peripheral entry

    debug_add_menu(menuFile);
    debug_add_menu(menuEdit);
    debug_add_menu(menuSystem);
    debug_add_menu(menuCart);
    debug_add_menu(menuPeripheral);
    debug_add_menu(menuHelp);

}

// deinit the menu system
void debug_deinit_menu() {
    debug_unregister_menu(nullptr);
}
