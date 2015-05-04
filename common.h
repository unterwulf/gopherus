/*
 * This file is part of the Gopherus project.
 * Copyright (c) 2013 Mateusz Viste
 */

#ifndef COMMON_H
#define COMMON_H

#include "history.h"

#define DISPLAY_ORDER_NONE 0
#define DISPLAY_ORDER_QUIT 1
#define DISPLAY_ORDER_BACK 2
#define DISPLAY_ORDER_REFR 3

#define TXT_FORMAT_RAW 0
#define TXT_FORMAT_HTM 1

#define KEY_BACKSPACE  0x08
#define KEY_TAB        0x09
#define KEY_ENTER      0x0D
#define KEY_ESCAPE     0x1B
#define KEY_F1         0x13B
#define KEY_F5         0x13F
#define KEY_F9         0x143
#define KEY_HOME       0x147
#define KEY_UP         0x148
#define KEY_PAGEUP     0x149
#define KEY_LEFT       0x14B
#define KEY_RIGHT      0x14D
#define KEY_END        0x14F
#define KEY_DOWN       0x150
#define KEY_PAGEDOWN   0x151
#define KEY_DELETE     0x153
#define KEY_QUIT       0xFF

struct gopherusconfig {
    int attr_textnorm;
    int attr_menucurrent;
    int attr_menutype;
    int attr_menuerr;
    int attr_menuselectable;
    int attr_statusbarinfo;
    int attr_statusbarwarn;
    int attr_urlbar;
    int attr_urlbardeco;
};

struct gopherus {
    char statusbar[128];
    char *buf;
    struct historytype *history;
    struct gopherusconfig cfg;
};

void set_statusbar(char *buf, char *msg);

void draw_field(const char *str, int attr, int x, int y, int width, int len);

void draw_urlbar(struct historytype *history, struct gopherusconfig *cfg);

void draw_statusbar(char *origmsg, struct gopherusconfig *cfg);

/* edits a string on screen. returns 0 if the string hasn't been modified, non-zero otherwise. */
int editstring(char *str, int maxlen, int maxdisplaylen, int x, int y, int attr, const char *prefix);

/* returns 0 if a new URL has been entered, non-zero otherwise */
int edit_url(struct historytype **history, struct gopherusconfig *cfg);

/* Asks for a confirmation to quit. Returns 0 if Quit aborted, non-zero otherwise. */
int ask_quit_confirmation(struct gopherusconfig *cfg);

#endif
