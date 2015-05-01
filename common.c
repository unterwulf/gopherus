/*
 * This file is part of the Gopherus project.
 * Copyright (c) 2013 Mateusz Viste
 */

#include <string.h>
#include "common.h"
#include "history.h"
#include "parseurl.h"
#include "ui.h"

void set_statusbar(char *buf, char *msg)
{
    if (buf[0] == 0) { /* accept new status message only if no message set yet */
        int x;
        for (x = 0; (x < 80) && (msg[x] != 0); x++) buf[x] = msg[x];
        buf[x] = 0;
    }
}

void draw_urlbar(struct historytype *history, struct gopherusconfig *cfg)
{
    int url_len, x;
    char urlstr[80];
    ui_putchar('[', cfg->attr_urlbardeco, 0, 0);
    url_len = buildgopherurl(urlstr, 79, history->protocol, history->host, history->port, history->itemtype, history->selector);

    for (x = 0; x < 79; x++) {
        if (x < url_len) {
            ui_putchar(urlstr[x], cfg->attr_urlbar, x+1, 0);
        } else {
            ui_putchar(' ', cfg->attr_urlbar, x+1, 0);
        }
    }

    ui_putchar(']', cfg->attr_urlbardeco, 79, 0);
}

void draw_statusbar(char *origmsg, struct gopherusconfig *cfg)
{
    int x, y, colattr;
    char *msg = origmsg;
    y = ui_getrowcount() - 1;
    if (msg[0] == '!') {
        msg += 1;
        colattr = cfg->attr_statusbarwarn; /* this is an important message */
    } else {
        colattr = cfg->attr_statusbarinfo;
    }
    for (x = 0; x < 80; x++) {
        if (msg[x] == 0) break;
        ui_putchar(msg[x], colattr, x, y); /* Using putchar because otherwise the last line will scroll the screen at its end. */
    }
    for (; x < 80; x++) ui_putchar(' ', colattr, x, y);
    origmsg[0] = 0; /* clear out the status message once it's displayed */
}

/* edits a string on screen. returns 0 if the string hasn't been modified, non-zero otherwise. */
int editstring(char *url, int maxlen, int maxdisplaylen, int xx, int yy, int attr)
{
    int urllen, x, presskey, cursorpos, result = 0, displayoffset;
    urllen = strlen(url);
    cursorpos = urllen;
    ui_cursor_show();

    for (;;) {
        if (urllen > maxdisplaylen - 1) {
            displayoffset = urllen - (maxdisplaylen - 1);
        } else {
            displayoffset = 0;
        }
        if (displayoffset > cursorpos - 8) {
            displayoffset = cursorpos - 8;
            if (displayoffset < 0) displayoffset = 0;
        }
        ui_locate(cursorpos + xx - displayoffset, yy);
        for (x = 0; x < maxdisplaylen; x++) {
            if ((x + displayoffset) < urllen) {
                ui_putchar(url[x + displayoffset], attr, x+xx, yy);
            } else {
                ui_putchar(' ', attr, x+xx, yy);
            }
        }
        presskey = ui_getkey();

        if ((presskey == KEY_ESCAPE) || (presskey == KEY_TAB)) {
            result = 0;
            break;
        } else if (presskey == KEY_HOME) {
            cursorpos = 0;
        } else if (presskey == KEY_END) {
            cursorpos = urllen;
        } else if (presskey == KEY_ENTER) {
            url[urllen] = 0; /* terminate the URL string with a NULL terminator */
            result = -1;
            break;
        } else if (presskey == KEY_LEFT) {
            if (cursorpos > 0) cursorpos -= 1;
        } else if (presskey == KEY_RIGHT) {
            if (cursorpos < urllen) cursorpos += 1;
        } else if (presskey == KEY_BACKSPACE) {
            if (cursorpos > 0) {
                int y;
                urllen -= 1;
                cursorpos -= 1;
                for (y = cursorpos; y < urllen; y++) url[y] = url[y + 1];
            }
        } else if (presskey == KEY_DELETE) {
            if (cursorpos < urllen) {
                int y;
                for (y = cursorpos; y < urllen; y++) url[y] = url[y+1];
                urllen -= 1;
            }
        } else if (presskey == KEY_QUIT) {
            result = 0;
            break;
        } else if ((presskey > 0x1F) && (presskey < 127)) {
            if (urllen < maxlen - 1) {
                int y;
                for (y = urllen; y > cursorpos; y--) url[y] = url[y - 1];
                url[cursorpos] = presskey;
                urllen += 1;
                cursorpos += 1;
            }
        }
    }
    ui_cursor_hide();
    return result;
}

/* returns 0 if a new URL has been entered, non-zero otherwise */
int edit_url(struct historytype **history, struct gopherusconfig *cfg)
{
    char url[256];
    int urllen = buildgopherurl(url, 256, (*history)->protocol, (*history)->host, (*history)->port, (*history)->itemtype, (*history)->selector);

    if (urllen < 0)
        return -1;

    if (editstring(url, 256, 78, 1, 0, cfg->attr_urlbar) != 0) {
        char itemtype;
        char hostaddr[256];
        char selector[256];
        int hostport;
        int protocol;
        if ((protocol = parsegopherurl(url, hostaddr, &hostport, &itemtype, selector)) >= 0) {
            history_add(history, protocol, hostaddr, hostport, itemtype, selector);
            draw_urlbar(*history, cfg);
            return 0;
        }
    }

    draw_urlbar(*history, cfg); /* the url didn't changed - redraw it and forget about the whole thing */
    return -1;
}

/* Asks for a confirmation to quit. Returns 0 if Quit aborted, non-zero otherwise. */
int ask_quit_confirmation(struct gopherusconfig *cfg)
{
    int keypress;
    char msg[] =
        "!YOU ARE ABOUT TO QUIT. PRESS ESC TO CONFIRM, OR ANY OTHER KEY TO ABORT.";

    draw_statusbar(msg, cfg);

    do {
        keypress = ui_getkey(); /* fetch the next recognized keypress */
    } while (keypress == 0x00);

    return ((keypress == KEY_ESCAPE) || (keypress == KEY_QUIT));
}
