/*
 * This file is part of the Gopherus project.
 * Copyright (c) 2013 Mateusz Viste
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "common.h"
#include "history.h"
#include "menuview.h"
#include "parseurl.h"
#include "ui.h"
#include "wordwrap.h"

/* used by display_menu to tell whether an itemtype is selectable or not */
static int isitemtypeselectable(char itemtype)
{
    switch (itemtype) {
        case 'i':  /* inline message */
        case '3':  /* error */
        case 0:    /* special internal type for menu lines continuations */
            return 0;
        default:   /* everything else is selectable */
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
            case KEY_BACKSPACE:
                return DISPLAY_ORDER_BACK;
                break;
            case KEY_TAB:
                if (edit_url(history, cfg) == 0) return DISPLAY_ORDER_NONE;
                break;
            case KEY_F9:
            case KEY_ENTER:
                if (*selectedline >= 0) {
                    if ((line_itemtype[*selectedline] == '7') && (keypress != KEY_F9)) { /* a query needs to be issued */
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
                        if (keypress == KEY_F9) tmpitemtype = '9'; /* force the itemtype to 'binary' if 'save as' was requested */
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
            case KEY_ESCAPE:
                if (ask_quit_confirmation(cfg) != 0) return DISPLAY_ORDER_QUIT;
                break;
            case KEY_F1: /* help */
                history_add(history, PARSEURL_PROTO_GOPHER, "#manual", 70, '0', "");
                return DISPLAY_ORDER_NONE;
                break;
            case KEY_F5: /* refresh */
                return DISPLAY_ORDER_REFR;
                break;
            case KEY_HOME:
                if (*selectedline >= 0) *selectedline = firstlinkline;
                *screenlineoffset = 0;
                break;
            case KEY_UP:
                if (*selectedline > firstlinkline) {
                    while (isitemtypeselectable(line_itemtype[--(*selectedline)]) == 0); /* select the next item that is selectable */
                } else {
                    if (*screenlineoffset > 0) *screenlineoffset -= 1;
                    continue; /* do not force the selected line to be on screen */
                }
                break;
            case KEY_PAGEUP:
                if (*selectedline >= 0) {
                    *selectedline -= (ui_getrowcount() - 3);
                    if (*selectedline < firstlinkline) *selectedline = firstlinkline;
                }
                break;
            case KEY_END:
                if (*selectedline >= 0) *selectedline = lastlinkline;
                *screenlineoffset = linecount - (ui_getrowcount() - 3);
                if (*screenlineoffset < 0) *screenlineoffset = 0;
                break;
            case KEY_DOWN:
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
            case KEY_PAGEDOWN:
                if (*selectedline >= 0) {
                    *selectedline += (ui_getrowcount() - 3);
                    if (*selectedline > lastlinkline) *selectedline = lastlinkline;
                }
                break;
            case KEY_QUIT: /* quit immediately */
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
