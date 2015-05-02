/**************************************************************************
 * Gopherus - a console-mode gopher client                                *
 * Copyright (C) Mateusz Viste 2013                                       *
 *                                                                        *
 * This program is free software: you can redistribute it and/or modify   *
 * it under the terms of the GNU General Public License as published by   *
 * the Free Software Foundation, either version 3 of the License, or      *
 * (at your option) any later version.                                    *
 *                                                                        *
 * This program is distributed in the hope that it will be useful,        *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of         *
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 * GNU General Public License for more details.                           *
 *                                                                        *
 * You should have received a copy of the GNU General Public License      *
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.  *
 *                                                                        *
 **************************************************************************/


#include <string.h>  /* strlen() */
#include <stdlib.h>  /* malloc(), getenv() */
#include <stdio.h>   /* sprintf(), fwrite()... */
#include <time.h>    /* time_t */
#include <unistd.h>  /* usleep() */
#include "dnscache.h"
#include "history.h"
#include "net.h"
#include "parseurl.h"
#include "ui.h"
#include "wordwrap.h"
#include "startpg.h"

#define pVer "1.0a"
#define pDate "2013"

#define DISPLAY_ORDER_NONE 0
#define DISPLAY_ORDER_QUIT 1
#define DISPLAY_ORDER_BACK 2
#define DISPLAY_ORDER_REFR 3

#define TXT_FORMAT_RAW 0
#define TXT_FORMAT_HTM 1

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

int hex2int(char c)
{
    switch (c) {
        case '0':
            return 0;
        case '1':
            return 1;
        case '2':
            return 2;
        case '3':
            return 3;
        case '4':
            return 4;
        case '5':
            return 5;
        case '6':
            return 6;
        case '7':
            return 7;
        case '8':
            return 8;
        case '9':
            return 9;
        case 'a':
        case 'A':
            return 10;
        case 'b':
        case 'B':
            return 11;
        case 'c':
        case 'C':
            return 12;
        case 'd':
        case 'D':
            return 13;
        case 'e':
        case 'E':
            return 14;
        case 'f':
        case 'F':
            return 15;
        default:
            return -1;
    }
}

void loadcfg(struct gopherusconfig *cfg)
{
    char *defaultcolorscheme = "177047707818141220";
    char *colorstring;
    int x;
    colorstring = getenv("GOPHERUSCOLOR");
    if (colorstring != NULL) {
        if (strlen(colorstring) == 18) {
            for (x = 0; x < 18; x++) {
                if (hex2int(colorstring[x]) < 0) {
                    colorstring = NULL;
                    break;
                }
            }
        } else {
            colorstring = NULL;
        }
    }
    if (colorstring == NULL) colorstring = defaultcolorscheme;
    /* interpret values from the color scheme variable */
    cfg->attr_textnorm = (hex2int(colorstring[0]) << 4) | hex2int(colorstring[1]);
    cfg->attr_statusbarinfo = (hex2int(colorstring[2]) << 4) | hex2int(colorstring[3]);
    cfg->attr_statusbarwarn = (hex2int(colorstring[4]) << 4) | hex2int(colorstring[5]);
    cfg->attr_urlbar = (hex2int(colorstring[6]) << 4) | hex2int(colorstring[7]);
    cfg->attr_urlbardeco = (hex2int(colorstring[8]) << 4) | hex2int(colorstring[9]);
    cfg->attr_menutype = (hex2int(colorstring[10]) << 4) | hex2int(colorstring[11]);
    cfg->attr_menuerr = (hex2int(colorstring[12]) << 4) | hex2int(colorstring[13]);
    cfg->attr_menuselectable = (hex2int(colorstring[14]) << 4) | hex2int(colorstring[15]);
    cfg->attr_menucurrent = (hex2int(colorstring[16]) << 4) | hex2int(colorstring[17]);
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
        ui_locate(yy, cursorpos + xx - displayoffset);
        for (x = 0; x < maxdisplaylen; x++) {
            if ((x + displayoffset) < urllen) {
                ui_putchar(url[x + displayoffset], attr, x+xx, yy);
            } else {
                ui_putchar(' ', attr, x+xx, yy);
            }
        }
        presskey = ui_getkey();
        if ((presskey == 0x1B) || (presskey == 0x09)) { /* ESC or TAB */
            result = 0;
            break;
        } else if (presskey == 0x147) { /* HOME */
            cursorpos = 0;
        } else if (presskey == 0x14F) { /* END */
            cursorpos = urllen;
        } else if (presskey == 0x0D) { /* ENTER */
            url[urllen] = 0; /* terminate the URL string with a NULL terminator */
            result = -1;
            break;
        } else if (presskey == 0x14B) { /* LEFT */
            if (cursorpos > 0) cursorpos -= 1;
        } else if (presskey == 0x14D) { /* RIGHT */
            if (cursorpos < urllen) cursorpos += 1;
        } else if (presskey == 0x08) { /* BACKSPACE */
            if (cursorpos > 0) {
                int y;
                urllen -= 1;
                cursorpos -= 1;
                for (y = cursorpos; y < urllen; y++) url[y] = url[y + 1];
            }
        } else if (presskey == 0x153) { /* DEL */
            if (cursorpos < urllen) {
                int y;
                for (y = cursorpos; y < urllen; y++) url[y] = url[y+1];
                urllen -= 1;
            }
        } else if (presskey == 0xFF) { /* QUIT */
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
int askQuitConfirmation(struct gopherusconfig *cfg)
{
    char confirm[80];
    int keypress;
    strcpy(confirm, "!YOU ARE ABOUT TO QUIT. PRESS ESC TO CONFIRM, OR ANY OTHER KEY TO ABORT.");
    draw_statusbar(confirm, cfg);

    while ((keypress = ui_getkey()) == 0x00)
        ; /* fetch the next recognized keypress */

    if ((keypress == 0x1B) || (keypress == 0xFF)) {
        return 1;
    } else {
        return 0;
    }
}

/* used by display_menu to tell whether an itemtype is selectable or not */
static int isitemtypeselectable(char itemtype)
{
    switch (itemtype) {
        case 'i':  /* inline message */
        case '3':  /* error */
        case 0:    /* special internal type for menu lines continuations */
            return 0;
        default: /* everything else is selectable */
            return 1;
    }
}

int display_menu(struct historytype **history, struct gopherusconfig *cfg, char *buffer, char *statusbar)
{
    char *description, *cursor, *selector, *host, *port, itemtype;
    int endofline;
    long bufferlen;
    char *line_description[1024];
    char *line_selector[1024];
    char *line_host[1024];
    char curURL[512];
    int line_port[1024];
    char line_itemtype[1024];
    unsigned char line_description_len[1024];
    int linecount = 0, x, y, column;
    char singlelinebuf[128];
    int *selectedline = &(*history)->displaymemory[0];
    int *screenlineoffset = &(*history)->displaymemory[1];
    int firstlinkline = -1, lastlinkline = -1, keypress;
    if (*screenlineoffset < 0) *screenlineoffset = 0;
    /* copy the history content into buffer - we need to do this because we'll perform changes on the data */
    bufferlen = (*history)->cachesize;
    memcpy(buffer, (*history)->cache, (*history)->cachesize);
    buffer[bufferlen] = 0;
    linecount = 0;

    for (cursor = buffer; cursor < (buffer + bufferlen) ;) {
        itemtype = *cursor;
        cursor += 1;
        column = 0;
        description = cursor;
        selector = NULL;
        host = NULL;
        port = NULL;
        endofline = 0;
        for (; cursor < (buffer + bufferlen); cursor += 1) { /* read the whole line */
            if (*cursor == '\r') continue; /* silently ignore CR chars */
            if ((*cursor == '\t') || (*cursor == '\n')) { /* delimiter */
                if (*cursor == '\n') endofline = 1;
                *cursor = 0; /* put a NULL instead to terminate previous string */
                if (column == 0) {
                    selector = cursor + 1;
                } else if (column == 1) {
                    host = cursor + 1;
                } else if (column == 2) {
                    port = cursor + 1;
                }
                if (endofline != 0) {
                    cursor += 1;
                    break;
                }
                if (column < 16) column += 1;
            }
        }
        if (itemtype == '.') continue; /* ignore lines starting by '.' - it's most probably the end of menu terminator */
        if (linecount < 1024) {
            char *wrapptr = description;
            int wraplen;
            int firstiteration = 0;
            if (isitemtypeselectable(itemtype) != 0) {
                if (firstlinkline < 0) firstlinkline = linecount;
                lastlinkline = linecount;
            }
            for (;; firstiteration += 1) {
                if ((firstiteration > 0) && (itemtype != 'i') && (itemtype != '3')) itemtype = 0;
                if (itemtype == 'i') {
                    wraplen = 80;
                } else {
                    wraplen = 76;
                }
                line_description[linecount] = wrapptr;
                wrapptr = wordwrap(wrapptr, singlelinebuf, wraplen);
                line_description_len[linecount] = strlen(singlelinebuf);
                line_selector[linecount] = selector;
                line_host[linecount] = host;
                line_itemtype[linecount] = itemtype;
                if (port != NULL) {
                    line_port[linecount] = atoi(port);
                    if (line_port[linecount] < 1) line_port[linecount] = 70;
                } else {
                    line_port[linecount] = 70;
                }
                linecount += 1;
                if (wrapptr == NULL) break;
                if (linecount >= 1024) break;
            }
        }
    }

    /* trim out the last line if its starting with a '.' (gopher's end of menu marker) */
    if (linecount > 0) {
        if (line_itemtype[linecount - 1] == '.') linecount -= 1;
    }

    /* if there is at least one position, and nothing is selected yet, make it active */
    if ((firstlinkline >= 0) && (*selectedline < 0))
        *selectedline = firstlinkline;

    for (;;) {
        curURL[0] = 0;
        if (*selectedline >= 0) {   /* if any position is selected, print the url in status bar */
            buildgopherurl(curURL, 512, PARSEURL_PROTO_GOPHER, line_host[*selectedline], line_port[*selectedline], line_itemtype[*selectedline], line_selector[*selectedline]);
            set_statusbar(statusbar, curURL);
        }
        /* start drawing lines of the menu */
        for (x = *screenlineoffset; x < *screenlineoffset + (ui_getrowcount() - 2); x++) {
            if (x < linecount) {
                int z, attr;
                char *prefix = NULL;
                attr = cfg->attr_menuselectable;
                if (x == *selectedline) { /* change the background if item is selected */
                    attr = cfg->attr_menucurrent;
                    /* attr &= 0x0F;
                       attr |= 0x20; */
                } else {
                    attr = cfg->attr_menutype;
                }
                switch (line_itemtype[x]) {
                    case 'i': /* message */
                        break;
                    case 'h': /* html */
                        prefix = "HTM";
                        break;
                    case '0': /* text */
                        prefix = "TXT";
                        break;
                    case '1':
                        prefix = "DIR";
                        break;
                    case '3':
                        prefix = "ERR";
                        break;
                    case '5':
                    case '9':
                        prefix = "BIN";
                        break;
                    case '7':
                        prefix = "ASK";
                        break;
                    case 'I':
                        prefix = "IMG";
                        break;
                    case 'P':
                    case 'd':
                        prefix = "PDF";
                        break;
                    case 0: /* this is an internal itemtype that means 'it's a continuation of the previous (wrapped) line */
                        prefix = "   ";
                        break;
                    default: /* unknown type */
                        prefix = "UNK";
                        break;
                }
                z = 0;
                if (prefix != NULL) {
                    for (y = 0; y < 3; y++) ui_putchar(prefix[y], attr, y, 1 + (x - *screenlineoffset));
                    ui_putchar(' ', attr, y, 1 + (x - *screenlineoffset));
                    z = 4;
                }
                /* select foreground color */
                /* attr &= 0xF0; */
                if (x == *selectedline) {
                    /* attr |= 0x00; */
                    attr = cfg->attr_menucurrent;
                } else if (line_itemtype[x] == 'i') {
                    /* attr |= 0x07; */
                    attr = cfg->attr_textnorm;
                } else if (line_itemtype[x] == '3') {
                    attr = cfg->attr_menuerr;
                    /* attr |= 0x04; */
                } else {
                    if (isitemtypeselectable(line_itemtype[x]) != 0) {
                        attr = cfg->attr_menuselectable;
                        /* attr |= 0x02; */
                    } else {
                        attr = cfg->attr_textnorm;
                        /* attr |= 0x08; */
                    }
                }
                /* print the the line's description */
                for (y = 0; y < (80 - z); y++) {
                    if (y < line_description_len[x]) {
                        ui_putchar(line_description[x][y], attr, y + z, 1 + (x - *screenlineoffset));
                    } else {
                        ui_putchar(' ', attr, y + z, 1 + (x - *screenlineoffset));
                    }
                }
            } else { /* x >= linecount */
                for (y = 0; y < 80; y++) ui_putchar(' ', cfg->attr_textnorm, y, 1 + (x - *screenlineoffset));
            }
        }

        draw_statusbar(statusbar, cfg);

        /* wait for keypress */
        keypress = ui_getkey();

        switch (keypress) {
            case 0x08: /* BACKSPACE */
                return DISPLAY_ORDER_BACK;
                break;
            case 0x09: /* TAB */
                if (edit_url(history, cfg) == 0) return DISPLAY_ORDER_NONE;
                break;
            case 0x143: /* F9 */
            case 0x0D: /* ENTER */
                if (*selectedline >= 0) {
                    if ((line_itemtype[*selectedline] == '7') && (keypress != 0x143)) { /* a query needs to be issued */
                        char query[64];
                        char *finalselector;
                        sprintf(query, "Enter a query: ");
                        draw_statusbar(query, cfg);
                        query[0] = 0;
                        if (editstring(query, 64, 64, 15, ui_getrowcount() - 1, cfg->attr_statusbarinfo) == 0) break;
                        finalselector = malloc(strlen(line_selector[*selectedline]) + strlen(query) + 2); /* add 1 for the TAB, and 1 for the NULL terminator */
                        if (finalselector == NULL) {
                            set_statusbar(statusbar, "Out of memory");
                            break;
                        } else {
                            sprintf(finalselector, "%s\t%s", line_selector[*selectedline], query);
                            history_add(history, PARSEURL_PROTO_GOPHER, line_host[*selectedline], line_port[*selectedline], line_itemtype[*selectedline], finalselector);
                            free(finalselector);
                            return DISPLAY_ORDER_NONE;
                        }
                    } else { /* itemtype is anything else than type 7 */
                        int tmpproto, tmpport;
                        char tmphost[512], tmpitemtype, tmpselector[512];
                        tmpproto = parsegopherurl(curURL, tmphost, &tmpport, &tmpitemtype, tmpselector);
                        if (keypress == 0x143) tmpitemtype = '9'; /* force the itemtype to 'binary' if 'save as' was requested */
                        if (tmpproto < 0) {
                            set_statusbar(statusbar, "!Unknown protocol");
                            break;
                        } else {
                            history_add(history, tmpproto, tmphost, tmpport, tmpitemtype, tmpselector);
                            return DISPLAY_ORDER_NONE;
                        }
                    }
                }
                break;
            case 0x1B: /* Esc */
                if (askQuitConfirmation(cfg) != 0) return DISPLAY_ORDER_QUIT;
                break;
            case 0x13B: /* F1 - help */
                history_add(history, PARSEURL_PROTO_GOPHER, "#manual", 70, '0', "");
                return DISPLAY_ORDER_NONE;
                break;
            case 0x13F: /* F5 - refresh */
                return DISPLAY_ORDER_REFR;
                break;
            case 0x147: /* HOME */
                if (*selectedline >= 0) *selectedline = firstlinkline;
                *screenlineoffset = 0;
                break;
            case 0x148: /* UP */
                if (*selectedline > firstlinkline) {
                    while (isitemtypeselectable(line_itemtype[--(*selectedline)]) == 0); /* select the next item that is selectable */
                } else {
                    if (*screenlineoffset > 0) *screenlineoffset -= 1;
                    continue; /* do not force the selected line to be on screen */
                }
                break;
            case 0x149: /* PGUP */
                if (*selectedline >= 0) {
                    *selectedline -= (ui_getrowcount() - 3);
                    if (*selectedline < firstlinkline) *selectedline = firstlinkline;
                }
                break;
            case 0x14F: /* END */
                if (*selectedline >= 0) *selectedline = lastlinkline;
                *screenlineoffset = linecount - (ui_getrowcount() - 3);
                if (*screenlineoffset < 0) *screenlineoffset = 0;
                break;
            case 0x150: /* DOWN */
                if (*selectedline > *screenlineoffset + ui_getrowcount() - 3) { /* if selected line is below the screen, don't change the selection */
                    *screenlineoffset += 1;
                    continue;
                }
                if (*selectedline < lastlinkline) {
                    while (isitemtypeselectable(line_itemtype[++(*selectedline)]) == 0); /* select the next item that is selectable */
                } else {
                    if (*screenlineoffset < linecount - (ui_getrowcount() - 3)) *screenlineoffset += 1;
                    continue; /* do not force the selected line to be on screen */
                }
                break;
            case 0x151: /* PGDOWN */
                if (*selectedline >= 0) {
                    *selectedline += (ui_getrowcount() - 3);
                    if (*selectedline > lastlinkline) *selectedline = lastlinkline;
                }
                break;
            case 0xFF:  /* 0xFF -> quit immediately */
                return 1;
                break;
            default:
                /* sprintf(singlelinebuf, "Got unknown key press: 0x%02X", keypress);
                   set_statusbar(statusbar, singlelinebuf); */
                continue;
                break;
        }

        /* if the selected line is going out of the screen, adjust the screen (but only if there is a selectedline at all) */
        if ((*selectedline < *screenlineoffset) && (*selectedline >= 0)) {
            *screenlineoffset = *selectedline;
        } else if (*selectedline > *screenlineoffset + (ui_getrowcount() - 3)) {
            *screenlineoffset = *selectedline - (ui_getrowcount() - 3);
            if (*screenlineoffset < 0) *screenlineoffset = 0;
        }
    }
}

int display_text(struct historytype **history, struct gopherusconfig *cfg, char *buffer, char *statusbar, int txtformat)
{
    char *txtptr;
    char linebuff[80];
    long x, y, firstline, lastline, bufferlen;
    int eof_flag;
    sprintf(linebuff, "file loaded (%ld bytes)", (*history)->cachesize);
    set_statusbar(statusbar, linebuff);
    /* copy the content of the file into buffer, and take care to modify dangerous chars and apply formating (if any) */
    bufferlen = 0;

    if (txtformat == TXT_FORMAT_HTM) { /* HTML format */
        int lastcharwasaspace = 0;
        int insidetoken = -1;
        int insidescript = 0;
        char token[8];
        char specialchar[8];
        int insidespecialchar = -1;
        for (x = 0; x < (*history)->cachesize; x++) {
            if ((insidescript != 0) && (insidetoken < 0) && ((*history)->cache[x] != '<')) continue;
            switch ((*history)->cache[x]) {
                case '\t':  /* replace whitespaces by single spaces */
                case '\n':
                case '\r':
                case ' ':
                    if (insidetoken >= 0) {
                        if (insidetoken < 7) token[insidetoken++] = 0;
                        continue;
                    }
                    if (lastcharwasaspace == 0) {
                        buffer[bufferlen++] = ' ';
                        lastcharwasaspace = 1;
                    }
                    break;
                case '<':
                    lastcharwasaspace = 0;
                    insidetoken = 0;
                    break;
                case '>':
                    lastcharwasaspace = 0;
                    if (insidetoken < 0) continue;
                    token[insidetoken] = 0;
                    insidetoken = -1;
                    if ((strcasecmp(token, "/p") == 0) || (strcasecmp(token, "br") == 0) || (strcasecmp(token, "/tr") == 0) || (strcasecmp(token, "/title") == 0)) {
                        buffer[bufferlen++] = '\n';
                    } else if (strcasecmp(token, "script") == 0) {
                        insidescript = 1;
                    } else if (strcasecmp(token, "/script") == 0) {
                        insidescript = 0;
                    }
                    break;
                default:
                    lastcharwasaspace = 0;
                    if (insidetoken >= 0) {
                        if (insidetoken < 7) token[insidetoken++] = (*history)->cache[x];
                        continue;
                    }
                    if ((insidespecialchar < 0) && ((*history)->cache[x] == '&')) {
                        insidespecialchar = 0;
                        continue;
                    }
                    if ((insidespecialchar >= 0) && (insidespecialchar < 7)) {
                        if ((*history)->cache[x] != ';') {
                            specialchar[insidespecialchar++] = (*history)->cache[x];
                            continue;
                        }
                        specialchar[insidespecialchar] = 0;
                        if (strcasecmp(specialchar, "nbsp") == 0) {
                            buffer[bufferlen++] = ' ';
                        } else {
                            buffer[bufferlen++] = '_';
                        }
                        insidespecialchar = -1;
                        continue;
                    }
                    if ((*history)->cache[x] < 32) break; /* ignore ascii control chars */
                    buffer[bufferlen++] = (*history)->cache[x]; /* copy everything else */
                    break;
            }
        }
    } else { /* process content as raw text */
        for (x = 0; x < (*history)->cachesize; x++) {
            switch ((*history)->cache[x]) {
                case 8:     /* replace tabs by 8 spaces */
                    for (y = 0; y < 8; y++) buffer[bufferlen++] = ' ';
                    break;
                case '\n':  /* preserve line feeds */
                    buffer[bufferlen++] = '\n';
                    break;
                case '\r':  /* ignore CR chars */
                case 127:   /* as well as DEL chars */
                    break;
                default:
                    if ((*history)->cache[x] < 32) break; /* ignore ascii control chars */
                    buffer[bufferlen++] = (*history)->cache[x]; /* copy everything else */
                    break;
            }
        }
        /* check if there is a single . on the last line */
        if ((buffer[bufferlen - 1] == '\n') && (buffer[bufferlen - 2] == '.')) bufferlen -= 2;
    }

    /* terminate the buffer with a NULL terminator */
    buffer[bufferlen] = 0;
    /* display the file on screen */
    firstline = 0;
    lastline = ui_getrowcount() - 3;

    for (;;) { /* display-control loop */
        y = 0;
        for (txtptr = buffer; txtptr != NULL; ) {
            txtptr = wordwrap(txtptr, linebuff, 80);
            if (y >= firstline) {
                for (x = 0; linebuff[x] != 0; x++) ui_putchar(linebuff[x], cfg->attr_textnorm, x, y + 1 - firstline);
                for (; x < 80; x++) ui_putchar(' ', cfg->attr_textnorm, x, y + 1 - firstline);
            }
            y++;
            if (y > lastline) break;
        }
        if (y <= lastline) {
            eof_flag = 1;
            for (; y <= lastline ; y++) { /* fill the rest of the screen (if any left) with blanks */
                for (x = 0; x < 80; x++) ui_putchar(' ', cfg->attr_textnorm, x, y + 1 - firstline);
            }
        } else {
            eof_flag = 0;
        }
        draw_statusbar(statusbar, cfg);
        x = ui_getkey();
        switch (x) {
            case 0x08:   /* Backspace */
                return DISPLAY_ORDER_BACK;
                break;
            case 0x09: /* TAB */
                if (edit_url(history, cfg) == 0) return DISPLAY_ORDER_NONE;
                break;
            case 0x1B:   /* ESC */
                if (askQuitConfirmation(cfg) != 0) return DISPLAY_ORDER_QUIT;
                break;
            case 0x13B: /* F1 - help */
                history_add(history, PARSEURL_PROTO_GOPHER, "#manual", 70, '0', "");
                return DISPLAY_ORDER_NONE;
                break;
            case 0x13F: /* F5 - refresh */
                return DISPLAY_ORDER_REFR;
                break;
            case 0x143: /* F9 - download */
                history_add(history, (*history)->protocol, (*history)->host, (*history)->port, '9', (*history)->selector);
                return DISPLAY_ORDER_NONE;
                break;
            case 0x148: /* UP */
                if (firstline > 0) {
                    firstline -= 1;
                    lastline -= 1;
                } else {
                    set_statusbar(statusbar, "Reached the top of the file");
                }
                break;
            case 0x150: /* DOWN */
                if (eof_flag == 0) {
                    firstline += 1;
                    lastline += 1;
                } else {
                    set_statusbar(statusbar, "Reached end of file");
                }
                break;
            case 0x147: /* HOME */
                lastline -= firstline;
                firstline = 0;
                break;
            case 0x149: /* PGUP */
                if (firstline > 0) {
                    firstline -= ui_getrowcount() - 3;
                    if (firstline < 0) firstline = 0;
                    lastline = firstline + ui_getrowcount() - 3;
                } else {
                    set_statusbar(statusbar, "Reached the top of the file");
                }
                break;
            case 0x14F: /* END */
                break;
            case 0x151: /* PGDOWN */
                if (eof_flag == 0) {
                    firstline += ui_getrowcount() - 3;
                    lastline += ui_getrowcount() - 3;
                } else {
                    set_statusbar(statusbar, "Reached end of file");
                }
                break;
            case 0xFF: /* QUIT IMMEDIATELY */
                return 1;
                break;
            default:  /* unhandled key */
                /* sprintf(linebuff, "Got invalid key: 0x%02lX", x);
                   set_statusbar(statusbar, linebuff); */
                break;
        }
    }
}

/* downloads a gopher or http resource and write it to a file or a memory buffer. if *filename is not NULL, the resource will
   be written in the file (but a valid *buffer is still required) */
long loadfile_buff(int protocol, char *hostaddr, unsigned int hostport, char *selector, char *buffer, long buffer_max, char *statusbar, char *filename, struct gopherusconfig *cfg)
{
    unsigned long int ipaddr;
    long reslength, byteread, fdlen = 0;
    char statusmsg[128];
    FILE *fd = NULL;
    int headersdone = 0; /* used notably for HTTP, to localize the end of headers */
    struct net_tcpsocket *sock;
    time_t lastactivity, curtime;
    if (hostaddr[0] == '#') { /* embedded start page */
        reslength = loadembeddedstartpage(buffer, hostaddr + 1, pVer, pDate);
        /* open file, if downloading to a file */
        if (filename != NULL) {
            fd = fopen(filename, "rb"); /* try to open for read - this should fail */
            if (fd != NULL) {
                set_statusbar(statusbar, "!File already exists! Operation aborted.");
                fclose(fd);
                return -1;
            }
            fd = fopen(filename, "wb"); /* now open for write - this will create the file */
            if (fd == NULL) { /* this should not fail */
                set_statusbar(statusbar, "!Error: could not create the file on disk!");
                fclose(fd);
                return -1;
            }
            fwrite(buffer, 1, reslength, fd);
            fclose(fd);
        }
        return reslength;
    }
    ipaddr = dnscache_ask(hostaddr);
    if (ipaddr == 0) {
        sprintf(statusmsg, "Resolving '%s'...", hostaddr);
        draw_statusbar(statusmsg, cfg);
        ipaddr = net_dnsresolve(hostaddr);
        if (ipaddr == 0) {
            set_statusbar(statusbar, "!DNS resolution failed!");
            return -1;
        }
        dnscache_add(hostaddr, ipaddr);
    }
    sprintf(statusmsg, "Connecting to %d.%d.%d.%d...", (int)(ipaddr >> 24) & 0xFF, (int)(ipaddr >> 16) & 0xFF, (int)(ipaddr >> 8) & 0xFF, (int)(ipaddr & 0xFF));
    draw_statusbar(statusmsg, cfg);

    sock = net_connect(ipaddr, hostport);
    if (sock == NULL) {
        set_statusbar(statusbar, "!Connection error!");
        return -1;
    }
    if (protocol == PARSEURL_PROTO_HTTP) { /* http */
        sprintf(buffer, "GET /%s HTTP/1.0\r\nHOST: %s\r\nUSER-AGENT: Gopherus v%s\r\n\r\n", selector, hostaddr, pVer);
    } else { /* gopher */
        sprintf(buffer, "%s\r\n", selector);
    }
    if (net_send(sock, buffer, strlen(buffer)) != (int)strlen(buffer)) {
        set_statusbar(statusbar, "!send() error!");
        net_close(sock);
        return -1;
    }
    /* prepare timers */
    lastactivity = time(NULL);
    curtime = lastactivity;
    /* open file, if downloading to a file */
    if (filename != NULL) {
        fd = fopen(filename, "rb"); /* try to open for read - this should fail */
        if (fd != NULL) {
            set_statusbar(statusbar, "!File already exists! Operation aborted.");
            fclose(fd);
            net_abort(sock);
            return -1;
        }
        fd = fopen(filename, "wb"); /* now open for write - this will create the file */
        if (fd == NULL) { /* this should not fail */
            set_statusbar(statusbar, "!Error: could not create the file on disk!");
            fclose(fd);
            net_abort(sock);
            return -1;
        }
    }
    /* receive answer */
    reslength = 0;
    for (;;) {
        if (buffer_max + fdlen - reslength < 1) { /* too much data! */
            set_statusbar(statusbar, "!Error: Server's answer is too long!");
            reslength = -1;
            break;
        }
        byteread = net_recv(sock, buffer + (reslength - fdlen), buffer_max + fdlen - reslength);
        curtime = time(NULL);
        if (byteread < 0) break; /* end of connection */
        if (ui_kbhit() != 0) { /* a key has been pressed - read it */
            int presskey = ui_getkey();
            if ((presskey == 0x1B) || (presskey == 0x08)) { /* if it's escape or backspace, abort the connection */
                set_statusbar(statusbar, "Connection aborted by the user.");
                reslength = -1;
                break;
            }
        }
        if (byteread > 0) {
            lastactivity = curtime;
            reslength += byteread;
            /* if protocol is http, ignore headers */
            if ((protocol == PARSEURL_PROTO_HTTP) && (headersdone == 0)) {
                int i;
                for (i = 0; i < reslength - 2; i++) {
                    if (buffer[i] == '\n') {
                        if (buffer[i + 1] == '\r') i++; /* skip CR if following */
                        if (buffer[i + 1] == '\n') {
                            i += 2;
                            headersdone = reslength;
                            for (reslength = 0; i < headersdone; i++) buffer[reslength++] = buffer[i];
                            break;
                        }
                    }
                }
            } else {
                sprintf(statusmsg, "Downloading... [%ld bytes]", reslength);
                set_statusbar(statusbar, statusmsg);
                draw_statusbar(statusbar, cfg);
                if ((fd != NULL) && (reslength - fdlen > 4096)) { /* if downloading to file, write stuff to disk */
                    int writeres = fwrite(buffer, 1, reslength - fdlen, fd);
                    if (writeres < 0) writeres = 0;
                    fdlen += writeres;
                }
            }
        } else {
            if (curtime - lastactivity > 2) {
                if (curtime - lastactivity > 20) { /* TIMEOUT! */
                    set_statusbar(statusbar, "!Timeout while waiting for data!");
                    reslength = -1;
                    break;
                } else {
                    usleep(250000);  /* give the cpu some time up (250ms), the transfer is really slow */
                }
            }
        }
    }

    if (reslength >= 0) {
        statusmsg[0] = 0;
        draw_statusbar(statusmsg, cfg);
        net_close(sock);
    } else {
        net_abort(sock);
    }

    if (fd != NULL) { /* finish the buffer */
        char tmpmsg[80];
        if (reslength - fdlen > 0) { /* if anything left in the buffer, write it now */
            fdlen += fwrite(buffer, 1, reslength - fdlen, fd);
        }
        fclose(fd);
        sprintf(tmpmsg, "Saved %ld bytes on disk", fdlen);
        set_statusbar(statusbar, tmpmsg);
    }

    return reslength;
}


#define buffersize 1024*1024

int main(int argc, char **argv)
{
    int exitflag;
    char statusbar[128] = {0};
    char *buffer;
    int bufferlen;
    struct historytype *history = NULL;
    struct gopherusconfig cfg;

    /* Load configuration (or defaults) */
    loadcfg(&cfg);

    ui_init();

    if (history_add(&history, PARSEURL_PROTO_GOPHER, "#welcome", 70, '1', "") != 0) {
        ui_puts("Out of memory.");
        return 2;
    }

    if (argc > 1) { /* if some params have been received, parse them */
        char itemtype;
        char hostaddr[1024];
        char selector[1024];
        int hostport, i;
        int protocol;
        int goturl = 0;

        for (i = 1; i < argc; i++) {
            if ((argv[i][0] == '/') || (argv[i][0] == '-')) { /* unknown parameter */
                ui_puts("Gopherus v" pVer " Copyright (C) Mateusz Viste " pDate);
                ui_puts("");
                ui_puts("Usage: gopherus [url]");
                ui_puts("");
                return 1;
            }
            if (goturl != 0) {
                ui_puts("Invalid parameters list.");
                return 1;
            }
            if ((protocol = parsegopherurl(argv[1], hostaddr, &hostport, &itemtype, selector)) < 0) {
                ui_puts("Invalid URL!");
                return 1;
            }
            goturl = 1;
            history_add(&history, protocol, hostaddr, hostport, itemtype, selector);
            if (history == NULL) {
                ui_puts("Out of memory.");
                return 2;
            }
        }
    }

    buffer = malloc(buffersize);

    if (buffer == NULL) {
        char message[128];
        sprintf(message, "Out of memory. Could not allocate buffer of %d bytes.", buffersize);
        ui_puts(message);
        return 2;
    }

    if (net_init() != 0) {
        ui_puts("Network subsystem initialization failed!");
        free(buffer);
        return 3;
    }

    ui_cursor_hide();
    ui_cls();

    for (;;) {
        if ((history->itemtype == '0') || (history->itemtype == '1') || (history->itemtype == '7') || (history->itemtype == 'h')) { /* if it's a displayable item type... */
            draw_urlbar(history, &cfg);
            if (history->cache == NULL) { /* reload the resource if not in cache already */
                bufferlen = loadfile_buff(history->protocol, history->host, history->port, history->selector, buffer, buffersize, statusbar, NULL, &cfg);
                if (bufferlen < 0) {
                    history_back(&history);
                    continue;
                } else {
                    history_cleanupcache(history);
                    history->cache = malloc(bufferlen);
                    if (history->cache == NULL) {
                        sprintf(statusbar, "Out of memory!");
                        exitflag = 1;
                        break;
                    }
                    if (bufferlen > 0) memcpy(history->cache, buffer, bufferlen);
                    history->cachesize = bufferlen;
                }
            }
            switch (history->itemtype) {
                case '0': /* text file */
                    exitflag = display_text(&history, &cfg, buffer, statusbar, TXT_FORMAT_RAW);
                    break;
                case 'h': /* html file */
                    exitflag = display_text(&history, &cfg, buffer, statusbar, TXT_FORMAT_HTM);
                    break;
                case '1': /* menu */
                case '7': /* query result (also a menu) */
                    exitflag = display_menu(&history, &cfg, buffer, statusbar);
                    break;
                default:
                    set_statusbar(statusbar, "Fatal error: got an unhandled itemtype!");
                    exitflag = DISPLAY_ORDER_QUIT;
                    break;
            }
            if (exitflag == DISPLAY_ORDER_BACK) {
                history_back(&history);
            } else if (exitflag == DISPLAY_ORDER_REFR) {
                free(history->cache);
                history->cache = NULL;
                history->cachesize = 0;
                history->displaymemory[0] = -1;
                history->displaymemory[1] = -1;
            } else if (exitflag == DISPLAY_ORDER_QUIT) {
                break;
            }
        } else { /* the itemtype is not one of the internally displayable types -> ask to download it */
            char filename[64] = {0};
            const char *prompt = "Download as: ";
            int i;
            set_statusbar(filename, ""); /* make sure to clear out the status bar */
            draw_statusbar(filename, &cfg);
            for (i = 0; prompt[i] != 0; i++) ui_putchar(prompt[i], 0x70, i, ui_getrowcount() - 1);
            if (editstring(filename, 63, 63, i, ui_getrowcount() - 1, 0x70) != 0) {
                loadfile_buff(history->protocol, history->host, history->port, history->selector, buffer, buffersize, statusbar, filename, &cfg);
            }
            history_back(&history);
        }
    }

    ui_cursor_show();
    ui_cls();

    if (statusbar[0] != 0)
        ui_puts(statusbar); /* we might have here an error message to show */

    /* Free the main buffer */
    free(buffer);
    /* unallocate all the history */
    history_flush(history);
    return 0;
}
