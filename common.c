/*
 * This file is part of the Gopherus project.
 * Copyright (c) 2013 Mateusz Viste
 */

#include <string.h>
#include "common.h"
#include "history.h"
#include "parseurl.h"
#include "ui.h"

void draw_field(const char *str, int attr, int x, int y, int width, int len)
{
    int i;

    if (len == -1)
        len = strlen(str);

    if (len < width) {
        ui_cputs(str, attr, x, y);

        for (i = len; i < width; i++)
            ui_putchar(' ', attr, x+i, y);
    } else {
        for (i = 0; i < width; i++)
            ui_putchar(str[i], attr, x+i, y);
    }
}

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
    char urlstr[80];
    int url_len = build_url(urlstr, 79, history->protocol, history->host, history->port, history->itemtype, history->selector);

    ui_putchar('[', cfg->attr_urlbardeco, 0, 0);
    draw_field(urlstr, cfg->attr_urlbar, 1, 0, 78, url_len);
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
int editstring(char *str, int maxlen, int maxdisplaylen, int x, int y, int attr, const char *prefix)
{
    int i, presskey;
    int len = strlen(str);
    int cursorpos = len;
    int result = 0;
    int is_first_keypress = 1;

    ui_cursor_show();

    for (;;) {
        int displayoffset = (len > maxdisplaylen - 1)
            ? len - (maxdisplaylen - 1) : 0;

        if (displayoffset > cursorpos - 8) {
            displayoffset = cursorpos - 8;
            if (displayoffset < 0)
                displayoffset = 0;
        }

        ui_locate(cursorpos + x - displayoffset, y);

        draw_field(str + displayoffset, attr, x, y, maxdisplaylen, len - displayoffset);

        presskey = ui_getkey();

        switch (presskey) {
            case KEY_ESCAPE:
            case KEY_TAB:
            case KEY_QUIT:
                result = 0;
                goto exit;
            case KEY_HOME:
                cursorpos = 0;
                break;
            case KEY_END:
                cursorpos = len;
                break;
            case KEY_ENTER:
                result = -1;
                goto exit;
            case KEY_LEFT:
                if (cursorpos > 0)
                    cursorpos--;
                break;
            case KEY_RIGHT:
                if (cursorpos < len)
                    cursorpos++;
                break;
            case KEY_BACKSPACE:
                if (cursorpos > 0) {
                    len--;
                    cursorpos--;
                    for (i = cursorpos; i <= len; i++)
                        str[i] = str[i+1];
                }
                break;
            case KEY_DELETE:
                if (cursorpos < len) {
                    for (i = cursorpos; i <= len; i++)
                        str[i] = str[i+1];
                    len--;
                }
                break;
            default:
                if ((presskey > 0x1F) && (presskey < 127)) {
                    if (is_first_keypress) {
                        if (prefix) {
                            cursorpos = len = strlen(prefix);
                            memcpy(str, prefix, len + 1);
                        } else {
                            cursorpos = len = 0;
                            str[len] = '\0';
                        }
                    }
                    if (len < maxlen - 1) {
                        for (i = len; i > cursorpos; i--)
                            str[i] = str[i-1];
                        str[cursorpos] = presskey;
                        len++;
                        str[len] = '\0';
                        cursorpos++;
                    }
                }
        }
        is_first_keypress = 0;
    }

exit:
    ui_cursor_hide();
    return result;
}

/* returns 0 if a new URL has been entered, non-zero otherwise */
int edit_url(struct historytype **history, struct gopherusconfig *cfg)
{
    char url[256];
    int urllen = build_url(url, 256, (*history)->protocol, (*history)->host, (*history)->port, (*history)->itemtype, (*history)->selector);

    if (urllen < 0)
        return -1;

    if (editstring(url, 256, 78, 1, 0, cfg->attr_urlbar, "gopher://") != 0) {
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
