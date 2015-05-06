/*
 * This file is part of the Gopherus project.
 * Copyright (c) 2013 Mateusz Viste
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "common.h"
#include "gopher.h"
#include "history.h"
#include "menuview.h"
#include "parseurl.h"
#include "ui.h"
#include "wordwrap.h"

/* Internal itemtypes: */
#define GOPHERUS_ITEM_CONT      0    /* continuation of the previous menu item */
#define GOPHERUS_ITEM_INVALID   0x7F /* malformed menu item */

/* used by display_menu to tell whether an itemtype is selectable or not */
static int isitemtypeselectable(char itemtype)
{
    switch (itemtype) {
        case GOPHER_ITEM_INLINE_MSG:
        case GOPHER_ITEM_ERROR:
        case GOPHERUS_ITEM_CONT:
        case GOPHERUS_ITEM_INVALID:
            return 0;
        default:   /* everything else is selectable */
            return 1;
    }
}

int display_menu(struct gopherus *g)
{
    char *description, *cursor, *selector, *host, *port, itemtype;
    int endofline;
    long bufferlen;
    char *line_description[1024];
    struct url line_url[1024];
    char curURL[512];
    unsigned char line_description_len[1024];
    int linecount = 0, x, y, column;
    char singlelinebuf[128];
    int *selectedline = &(g->history->displaymemory[0]);
    int *screenlineoffset = &(g->history->displaymemory[1]);
    int firstlinkline = -1, lastlinkline = -1, keypress;
    if (*screenlineoffset < 0) *screenlineoffset = 0;
    /* copy the history content into buffer - we need to do this because we'll perform changes on the data */
    bufferlen = g->history->cachesize;
    memcpy(g->buf, g->history->cache, g->history->cachesize);
    g->buf[bufferlen] = 0;
    linecount = 0;

    for (cursor = g->buf; cursor < (g->buf + bufferlen) ;) {
        itemtype = *cursor;
        cursor += 1;
        column = 0;
        description = cursor;
        selector = NULL;
        host = NULL;
        port = NULL;
        endofline = 0;
        for (; cursor < (g->buf + bufferlen); cursor += 1) { /* read the whole line */
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

        if (isitemtypeselectable(itemtype) && !(selector && host))
            itemtype = GOPHERUS_ITEM_INVALID;

        if (linecount < 1024) {
            char *wrapptr = description;
            int wraplen;
            int firstiteration = 0;
            if (isitemtypeselectable(itemtype) != 0) {
                if (firstlinkline < 0) firstlinkline = linecount;
                lastlinkline = linecount;
            }
            for (;; firstiteration += 1) {
                if ((firstiteration > 0) &&
                    (itemtype != GOPHER_ITEM_INLINE_MSG) &&
                    (itemtype != GOPHER_ITEM_ERROR))
                    itemtype = GOPHERUS_ITEM_CONT;

                if (itemtype == GOPHER_ITEM_INLINE_MSG) {
                    wraplen = 80;
                } else {
                    wraplen = 76;
                }
                line_description[linecount] = wrapptr;
                wrapptr = wordwrap(singlelinebuf, wrapptr, wraplen);
                line_description_len[linecount] = strlen(singlelinebuf);
                line_url[linecount].protocol = PARSEURL_PROTO_GOPHER;
                line_url[linecount].selector = selector;
                line_url[linecount].host = host;
                line_url[linecount].itemtype = itemtype;
                if (port != NULL) {
                    line_url[linecount].port = atoi(port);
                    if (line_url[linecount].port < 1) line_url[linecount].port = 70;
                } else {
                    line_url[linecount].port = 70;
                }
                linecount += 1;
                if (wrapptr == NULL) break;
                if (linecount >= 1024) break;
            }
        }
    }

    /* trim out the last line if its starting with a '.' (gopher's end of menu marker) */
    if (linecount > 0) {
        if (line_url[linecount - 1].itemtype == '.') linecount -= 1;
    }

    /* if there is at least one position, and nothing is selected yet, make it active */
    if ((firstlinkline >= 0) && (*selectedline < 0))
        *selectedline = firstlinkline;

    for (;;) {
        curURL[0] = 0;
        if (*selectedline >= 0) {   /* if any position is selected, print the url in status bar */
            build_url(curURL, sizeof curURL, &line_url[*selectedline]);
            set_statusbar(g->statusbar, curURL);
        }
        /* start drawing lines of the menu */
        for (x = *screenlineoffset; x < *screenlineoffset + (ui_getrowcount() - 2); x++) {
            if (x < linecount) {
                int z, attr;
                char *prefix = NULL;
                attr = g->cfg.attr_menuselectable;
                if (x == *selectedline) { /* change the background if item is selected */
                    attr = g->cfg.attr_menucurrent;
                    /* attr &= 0x0F;
                       attr |= 0x20; */
                } else {
                    attr = g->cfg.attr_menutype;
                }
                switch (line_url[x].itemtype) {
                    case GOPHER_ITEM_INLINE_MSG: /* message */
                        break;
                    case GOPHER_ITEM_HTML: /* html */
                        prefix = "HTM";
                        break;
                    case GOPHER_ITEM_FILE: /* text */
                        prefix = "TXT";
                        break;
                    case GOPHER_ITEM_DIR:
                        prefix = "DIR";
                        break;
                    case GOPHER_ITEM_ERROR:
                        prefix = "ERR";
                        break;
                    case GOPHER_ITEM_DOSBINARC:
                    case GOPHER_ITEM_BINARY:
                        prefix = "BIN";
                        break;
                    case GOPHER_ITEM_INDEX_SEARCH_SERVER:
                        prefix = "ASK";
                        break;
                    case GOPHER_ITEM_IMAGE:
                        prefix = "IMG";
                        break;
                    case 'P':
                    case 'd':
                        prefix = "PDF";
                        break;
                    case GOPHERUS_ITEM_CONT:
                        prefix = "   ";
                        break;
                    case GOPHERUS_ITEM_INVALID:
                        prefix = "INV";
                        break;
                    default: /* unknown type */
                        prefix = "UNK";
                        break;
                }
                z = 0;
                if (prefix != NULL) {
                    ui_cputs(prefix, attr, 0, 1 + (x - *screenlineoffset));
                    ui_putchar(' ', attr, 3, 1 + (x - *screenlineoffset));
                    z = 4;
                }
                /* select foreground color */
                /* attr &= 0xF0; */
                if (x == *selectedline) {
                    /* attr |= 0x00; */
                    attr = g->cfg.attr_menucurrent;
                } else if (line_url[x].itemtype == GOPHER_ITEM_INLINE_MSG) {
                    /* attr |= 0x07; */
                    attr = g->cfg.attr_textnorm;
                } else if (line_url[x].itemtype == GOPHER_ITEM_ERROR) {
                    attr = g->cfg.attr_menuerr;
                    /* attr |= 0x04; */
                } else {
                    if (isitemtypeselectable(line_url[x].itemtype) != 0) {
                        attr = g->cfg.attr_menuselectable;
                        /* attr |= 0x02; */
                    } else {
                        attr = g->cfg.attr_textnorm;
                        /* attr |= 0x08; */
                    }
                }
                /* print the the line's description */
                draw_field(line_description[x],
                    attr,
                    z,
                    1 + (x - *screenlineoffset),
                    80,
                    line_description_len[x]);
            } else { /* x >= linecount */
                for (y = 0; y < 80; y++) ui_putchar(' ', g->cfg.attr_textnorm, y, 1 + (x - *screenlineoffset));
            }
        }

        draw_statusbar(g->statusbar, &(g->cfg));

        /* wait for keypress */
        keypress = ui_getkey();

        switch (keypress) {
            case KEY_BACKSPACE:
                return DISPLAY_ORDER_BACK;
                break;
            case KEY_TAB:
                if (edit_url(&(g->history), &(g->cfg)) == 0) return DISPLAY_ORDER_NONE;
                break;
            case KEY_F9:
            case KEY_ENTER:
                if (*selectedline >= 0) {
                    if ((line_url[*selectedline].itemtype == GOPHER_ITEM_INDEX_SEARCH_SERVER) && (keypress != KEY_F9)) { /* a query needs to be issued */
                        char query[64];
                        char *finalselector;
                        sprintf(query, "Enter a query: ");
                        draw_statusbar(query, &(g->cfg));
                        query[0] = 0;
                        if (editstring(query, 64, 64, 15, ui_getrowcount() - 1, g->cfg.attr_statusbarinfo, NULL) == 0) break;
                        finalselector = malloc(strlen(line_url[*selectedline].selector) + strlen(query) + 2); /* add 1 for the TAB, and 1 for the NULL terminator */
                        if (finalselector == NULL) {
                            set_statusbar(g->statusbar, "Out of memory");
                            break;
                        } else {
                            struct url final_url = line_url[*selectedline];
                            sprintf(finalselector, "%s\t%s", line_url[*selectedline].selector, query);
                            final_url.selector = finalselector;
                            history_add(&(g->history), &final_url);
                            free(finalselector);
                            return DISPLAY_ORDER_NONE;
                        }
                    } else { /* itemtype is anything else than type 7 */
                        struct url next_url;

                        if (parse_url(curURL, &next_url) == 0) {
                            if (keypress == KEY_F9) {
                                /* force the itemtype to 'binary' if 'save as' was requested */
                                next_url.itemtype = GOPHER_ITEM_BINARY;
                            }
                            history_add(&(g->history), &next_url);
                            return DISPLAY_ORDER_NONE;
                        } else {
                            set_statusbar(g->statusbar, "!Unknown protocol");
                            break;
                        }
                    }
                }
                break;
            case KEY_ESCAPE:
                if (ask_quit_confirmation(&(g->cfg)) != 0) return DISPLAY_ORDER_QUIT;
                break;
            case KEY_F1: /* help */
                go_to_help(g);
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
                    while (isitemtypeselectable(line_url[--(*selectedline)].itemtype) == 0); /* select the next item that is selectable */
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
                    while (isitemtypeselectable(line_url[++(*selectedline)].itemtype) == 0); /* select the next item that is selectable */
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
                   set_statusbar(g->statusbar, singlelinebuf); */
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
