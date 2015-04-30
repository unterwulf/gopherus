/*
 * This file is part of the gopherus project.
 */

#include <string.h>   /* strstr() */
#include <stdlib.h>   /* atoi() */
#include "int2str.h"  /* int2str() is used to convert the port into a string */
#include "parseurl.h" /* include self for control */

int parsegopherurl(char *url, char *host, int *port, char *itemtype, char *selector)
{
    int parserstate = 0;
    int protocol = PARSEURL_PROTO_GOPHER, x;
    char *curtoken;
    /* set default values */
    *port = 70;
    *itemtype = '1';
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
                        *itemtype = 'h';
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

/* computes an URL string from exploded gopher parts, and returns its length. Returns -1 on error. */
int buildgopherurl(char *res, int maxlen, int protocol, char *host, int port, char itemtype, char *selector)
{
    int x = 0;
    char *protoname_gopher = "gopher://";
    char *protoname_http = "http://";
    char *protoname = protoname_gopher;
    maxlen -= 1;

    if (protocol == PARSEURL_PROTO_HTTP) { /* http URL */
        protoname = protoname_http;
        for (; *protoname != 0; protoname++) { /* protocol */
            if (x == maxlen) goto maxlenreached;
            res[x++] = *protoname;
        }
        for (; *host != 0; host++) { /* hostname */
            if (x == maxlen) goto maxlenreached;
            res[x++] = *host;
        }
        if (port != 80) { /* port (optional, only if not 80) */
            if (x + 6 >= maxlen) goto maxlenreached;
            res[x++] = ':';
            x += int2str(&res[x], port);
        }
        if (x == maxlen) goto maxlenreached; /* / delimiter */
        res[x++] = '/';
        for (; *selector != 0; selector++) { /* url */
            if (x == maxlen) goto maxlenreached;
            res[x++] = *selector;
        }
        res[x] = 0;
        return x;
    }
    /* The proto is gopher -- validate input data */
    if (maxlen < 2) return -1;
    if ((port < 1) || (port > 65535)) return -1;
    if ((host == NULL) || (res == NULL) || (selector == NULL)) return -1;
    if (itemtype < 33) return -1;
    /* detect special hURL links */
    if (itemtype == 'h') {
        if ((strstr(selector, "URL:") == selector) || (strstr(selector, "/URL:") == selector)) {
            if (selector[0] == '/') selector += 1;
            selector += 4;
            for (; *selector != 0; selector += 1) {
                if (x == maxlen) goto maxlenreached;
                res[x++] = *selector;
            }
            res[x] = 0;
            return x;
        }
    }
    /* this is a classic gopher location */
    for (; *protoname != 0; protoname += 1) {
        if (x == maxlen) goto maxlenreached;
        res[x++] = *protoname;
    }
    /* if empty host, return only the gopher:// string */
    if (host[0] == 0) {
        res[x] = 0;
        return x;
    }
    /* build the url string */
    for (; *host != 0; host += 1) {
        if (x == maxlen) goto maxlenreached;
        res[x++] = *host;
    }
    if (port != 70) {
        if (x + 6 >= maxlen) goto maxlenreached;
        res[x++] = ':';
        x += int2str(&res[x], port);
    }
    if (x == maxlen) goto maxlenreached;
    res[x++] = '/';
    if (x == maxlen) goto maxlenreached;
    res[x++] = itemtype;
    for (; *selector != 0; selector += 1) {
        if (x == maxlen) goto maxlenreached;
        if (((unsigned)*selector <= 0x1F) || ((unsigned)*selector >= 0x80)) { /* encode unsafe chars - RFC 1738: */
            if (x + 3 > maxlen) goto maxlenreached;       /* URLs are written only with the graphic printable characters of the  */
            res[x++] = '%';                               /* US-ASCII coded character set. The octets 80-FF hexadecimal are not  */
            res[x++] = '0' + (*selector >> 4);            /* used in US-ASCII, and the octets 00-1F and 7F hexadecimal represent */
            res[x++] = '0' + (*selector & 0x0F);          /* control characters; these must be encoded. */
        } else {
            res[x++] = *selector;
        }
    }

maxlenreached:
    res[x] = 0;
    return x;
}
