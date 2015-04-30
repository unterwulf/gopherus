/*
 * This file is part of the Gopherus project.
 * Copyright (C) Mateusz Viste 2013
 */

#ifndef HISTORY_H
#define HISTORY_H

struct historytype {
    long cachesize;
    char *host;
    char *selector;
    char *cache;
    struct historytype *next;
    unsigned int port;
    char protocol;
    char itemtype;
    int displaymemory[2];  /* used by some display plugins to remember how the item was displayed. this is always initialized to -1 values */
};

/* remove the last visited page from history (goes back to the previous one) */
void history_back(struct historytype **history);

/* adds a new node to the history list. Returns 0 on success, non-zero otherwise. */
int history_add(struct historytype **history, char protocol, char *host, unsigned int port, char itemtype, char *selector);

/* free cache content past latest maxallowedcache bytes */
void history_cleanupcache(struct historytype *history);

/* flush all history, freeing memory */
void history_flush(struct historytype *history);

#endif
