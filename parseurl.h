/*
 * This file is part of the Gopherus project.
 * Copyright (C) Mateusz Viste 2013
 */

#ifndef PARSEURL_H
#define PARSEURL_H

#define PARSEURL_PROTO_GOPHER 1
#define PARSEURL_PROTO_HTTP 2
#define PARSEURL_PROTO_UNKNOWN -1

struct url {
    char *host;
    char *selector;
    unsigned short port;
    char protocol;
    char itemtype;
};

/* Explodes a URL into parts, and return 0 on success, or a negative value on
 * error. Modifies original string and uses it as a storage. */
int parse_url(char *url_str, struct url *url);

/* builds a URL from exploded parts */
int build_url(char *res, int maxlen, const struct url *url);

#endif
