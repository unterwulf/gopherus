/*
 * This file is part of the Gopherus project.
 * Copyright (C) Mateusz Viste 2013
 */

#ifndef PARSEURL_H
#define PARSEURL_H

#define PARSEURL_PROTO_GOPHER 1
#define PARSEURL_PROTO_HTTP 2
#define PARSEURL_PROTO_UNKNOWN -1

/* explodes a URL into parts, and return the protocol id, or a negative value on error */
int parsegopherurl(char *url, char *host, int *port, char *itemtype, char *selector);

/* builds a URL from exploded parts */
int buildgopherurl(char *res, int maxlen, int protocol, char *host, int port, char itemtype, char *selector);

#endif
