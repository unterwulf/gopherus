/*
 * This file is part of the gopherus project.
 */

#include <string.h>   /* strstr() */
#include <stdlib.h>   /* atoi() */
#include "gopher.h"
#include "int2str.h"  /* int2str() is used to convert the port into a string */
#include "parseurl.h"

int parsegopherurl(char *url, char *host, int *port, char *itemtype, char *selector)
{
    int parserstate = 0;
    int protocol = PARSEURL_PROTO_GOPHER, x;
    char *curtoken;
    /* set default values */
    *port = 70;
    *itemtype = GOPHER_ITEM_DIR;
    *selector = 0;
    /* skip the protocol part, if present */
    for (x = 0; url[x] != 0; x++) {
        if (url[x] == '/') { /* no protocol */
            protocol = PARSEURL_PROTO_GOPHER;
            break;
        }
        if (url[x] == ':') { /* found a colon. check if it's for proto declaration */
            if (url[x + 1] == '/') {
                if (url[x + 2] == '/') {
                    char *protostr = url;
                    url[x] = 0;
                    url += x + 3;
                    if (strcasecmp(protostr, "gopher") == 0) {
                        protocol = PARSEURL_PROTO_GOPHER;
                    } else if (strcasecmp(protostr, "http") == 0) {
                        protocol = PARSEURL_PROTO_HTTP;
                        *port = 80; /* default port is 80 for HTTP */
                        *itemtype = GOPHER_ITEM_HTML;
                    } else {
                        protocol = PARSEURL_PROTO_UNKNOWN;
                    }
                    break;
                }
            }
            protocol = PARSEURL_PROTO_GOPHER;
            break;
        }
    }
    /* start reading the url */
    curtoken = url;
    for (; parserstate < 4; url += 1) {
        switch (parserstate) {
            case 0:  /* reading host */
                if (*url == ':') { /* a port will follow */
                    *host = 0;
                    curtoken = url + 1;
                    parserstate = 1;
                } else if (*url == '/') { /* gopher type will follow */
                    *host = 0;
                    parserstate = 2;
                } else if (*url == 0) { /* end of url */
                    *host = 0;
                    parserstate = 4;
                } else { /* still part of the host */
                    *host = *url;
                    host += 1;
                }
                break;
            case 1:  /* reading port */
                if (*url == 0) { /* end of url */
                    *port = atoi(curtoken);
                    parserstate = 4;
                } else if (*url == '/') {
                    *url = 0; /* temporary end of string */
                    *port = atoi(curtoken);
                    *url = '/'; /* restore the original char */
                    parserstate = 2; /* gopher type follows */
                }
                break;
            case 2:  /* reading itemtype */
                if (protocol == PARSEURL_PROTO_GOPHER) { /* if non-Gopher, skip the itemtype */
                    if (*url != 0) {
                        *itemtype = *url;
                        parserstate = 3;
                    } else {
                        parserstate = 4;
                    }
                    url += 1;
                }
                parserstate = 3; /* go right to the url part now */
            case 3:
                if (*url == 0) {
                    *selector = 0;
                    parserstate = 4;
                } else {
                    *selector = *url;
                    selector += 1;
                }
                break;
        }
    }

    return protocol;
}

static int build_http_url(char *res, int maxlen, const char *host, int port, const char *selector)
{
    static const char *protoname = "http://";
    int i;

    maxlen--;

    for (i = 0; protoname[i] && (i < maxlen); i++)
        res[i] = protoname[i];

    while (*host && (i < maxlen))
        res[i++] = *host++;

    /* port (optional, only if not 80) */
    if (port != 80 && (i + 6 < maxlen)) {
        res[i++] = ':';
        i += int2str(&res[i], port);
    }

    if (i < maxlen)
        res[i++] = '/';

    while (*selector && (i < maxlen))
        res[i++] = *selector++;

    res[i] = '\0';
    return i;
}

static int build_gopher_url(char *res, int maxlen, const char *host, int port, char itemtype, char *selector)
{
    static const char *protoname = "gopher://";
    int i = 0;

    maxlen--;

    if (maxlen < 2) return -1;
    if ((port < 1) || (port > 65535)) return -1;
    if (!host || !res || !selector) return -1;
    if (itemtype < 33) return -1;

    /* detect special hURL links */
    if (itemtype == GOPHER_ITEM_HTML) {
        if ((strstr(selector, "URL:") == selector) || (strstr(selector, "/URL:") == selector)) {
            if (selector[0] == '/') selector++;
            selector += 4;
            i = 0;
            while (*selector && (i < maxlen))
                res[i++] = *selector++;
        }
    } else {
        /* this is a classic gopher location */
        for (i = 0; protoname[i] && (i < maxlen); i++)
            res[i] = protoname[i];

        /* if empty host, return only the gopher:// string */
        if (!host[0]) {
            res[i] = '\0';
            return i;
        }

        /* build the url string */
        while (*host && (i < maxlen))
            res[i++] = *host++;

        if (port != 70 && (i + 6 < maxlen)) {
            res[i++] = ':';
            i += int2str(&res[i], port);
        }

        if (i < maxlen)
            res[i++] = '/';

        if (i < maxlen)
            res[i++] = itemtype;

        for (; *selector && (i < maxlen); selector++) {
            if (((unsigned)*selector <= 0x1F) || ((unsigned)*selector >= 0x80)) { /* encode unsafe chars - RFC 1738: */
                if (i + 2 < maxlen) {                         /* URLs are written only with the graphic printable characters of the  */
                    res[i++] = '%';                           /* US-ASCII coded character set. The octets 80-FF hexadecimal are not  */
                    res[i++] = '0' + (*selector >> 4);        /* used in US-ASCII, and the octets 00-1F and 7F hexadecimal represent */
                    res[i++] = '0' + (*selector & 0x0F);      /* control characters; these must be encoded. */
                }
            } else {
                res[i++] = *selector;
            }
        }
    }

    res[i] = '\0';
    return i;
}

/* computes an URL string from exploded gopher parts, and returns its length. Returns -1 on error. */
int build_url(char *res, int maxlen, int protocol, char *host, int port, char itemtype, char *selector)
{
    return (protocol == PARSEURL_PROTO_HTTP)
        ? build_http_url(res, maxlen, host, port, selector)
        : build_gopher_url(res, maxlen, host, port, itemtype, selector);
}
