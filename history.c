/*
 * This file is part of the Gopherus project.
 * Copyright (C) Mateusz Viste 2013
 */

#include <stdlib.h>  /* malloc(), NULL */
#include <string.h>  /* strcasecmp(), ... */
#include "history.h" /* include self for control and type declaration */


#define MAXALLOWEDCACHE 1024*1024*2

static void history_free_node(struct historytype *node) {
  if (node->cache != NULL) free(node->cache);
  if (node->selector != NULL) free(node->selector);
  if (node->host != NULL) free(node->host);
  free(node);
}


/* remove the last visited page from history (goes back to the previous one) */
void history_back(struct historytype **history) {
  struct historytype *victim;
  if (*history != NULL) {
    if ((*history)->next != NULL) {
      victim = *history;
      *history = (*history)->next;
      history_free_node(victim);
    }
  }
  /* check if the last request was a query, and if not in cache, put a message instead to avoid reloading a query again */
  if (((*history)->itemtype == '7') && ((*history)->cache == NULL)) {
    char *msg = "3Query not in cache\ni\niThis location is not avaiable in the local cache. Gopherus is not reissuing custom queries automatically. If you wish to force a reload, press F5.\n";
    (*history)->cachesize = strlen(msg);
    (*history)->cache = malloc((*history)->cachesize + 1);
    if ((*history)->cache == NULL) { /* oops, out of memory! */
      (*history)->cachesize = 0;
      return;
    }
    memcpy((*history)->cache, msg, (*history)->cachesize + 1);
  }
}


/* adds a new node to the history list. Returns 0 on success, non-zero otherwise. */
int history_add(struct historytype **history, char protocol, char *host, unsigned int port, char itemtype, char *selector) {
  struct historytype *result;
  int tmplen;

  /* shortcut - if the new node is identical to the previous page, the user is doing a 'back' action */
  if (*history != NULL) { /* do we have any history at all? */
    if ((*history)->next != NULL) { /* is there a 'previous' position? */
      if (protocol == (*history)->next->protocol) { /* same protocol */
        if (strcasecmp(host, (*history)->next->host) == 0) { /* same host */
          if (port == (*history)->next->port) { /* same port */
            if (itemtype == (*history)->next->itemtype) { /* same itemtype */
              if (strcmp(selector, (*history)->next->selector) == 0) { /* same resource */
                history_back(history); /* do a 'back' action in the history list */
                return(0);  /* return success */
              }
            }
          }
        }
      }
    }
  }
  /* add the node */
  result = malloc(sizeof(struct historytype));
  if (result == NULL) return(-1);
  tmplen = strlen(host) + 1;
  result->host = malloc(tmplen);
  if (result->host == NULL) {
    free(result);
    return(-1);
  }
  memcpy(result->host, host, tmplen);
  result->protocol = protocol;
  result->port = port;
  result->itemtype = itemtype;
  result->displaymemory[0] = -1;
  result->displaymemory[1] = -1;
  tmplen = strlen(selector) + 1;
  result->selector = malloc(tmplen);
  if (result->selector == NULL) {
    free(result->host);
    free(result);
    return(-1);
  }
  memcpy(result->selector, selector, tmplen);
  result->cache = NULL;
  result->cachesize = 0;
  result->next = *history;
  *history = result;
  return(0);
}


/* free cache content past latest MAXALLOWEDCACHE bytes */
void history_cleanupcache(struct historytype *history) {
  unsigned long totalcache = 0;
  for (; history != NULL; history = history->next) {
    totalcache += history->cachesize;
    if (totalcache > MAXALLOWEDCACHE) {
      if (history->cache != NULL) {
        free(history->cache);
        history->cache = NULL;
        history->cachesize = 0;
      }
    }
  }
}


/* flush all history, freeing memory */
void history_flush(struct historytype *history) {
  struct historytype *victim;
  while (history != NULL) {
    victim = history;
    history = history->next;
    history_free_node(victim);
  }
}
