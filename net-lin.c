/*
 * This file is part of the Gopherus project
 * Copyright (C) Mateusz Viste 2013
 *
 * Provides all network functions used by Gopherus, wrapped around POSIX (BSD) sockets.
 */

#include <stdlib.h>  /* NULL */
#include <sys/socket.h> /* socket() */
#include <arpa/inet.h>
#include <netdb.h>
#include <stdio.h> /* sprintf() */
#include <unistd.h> /* close() */
#include <errno.h>
#include <stdint.h> /* uint32_t */

#include "net.h"

struct netwrap {
    int fd;
};

unsigned long net_dnsresolve(const char *name)
{
    struct hostent *hent = gethostbyname(name);
    return (hent) ? htonl(*((uint32_t *)(hent->h_addr))) : 0;
}

int net_init()
{
    return 0;
}

struct net_tcpsocket *net_connect(unsigned long ipaddr, int port)
{
    struct netwrap *s;
    struct sockaddr_in remote;
    struct net_tcpsocket *result;
    char ipstr[64];

    sprintf(ipstr, "%lu.%lu.%lu.%lu", (ipaddr >> 24) & 0xFF, (ipaddr >> 16) & 0xFF, (ipaddr >> 8) & 0xFF, ipaddr & 0xFF);
    s = malloc(sizeof *s);

    if (s == NULL)
        return NULL;

    s->fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (s->fd < 0) {
        free(s);
        return NULL;
    }

    result = malloc(sizeof *result);
    if (result == NULL) {
        close(s->fd);
        free(s);
        return NULL;
    }

    remote.sin_family = AF_INET;  /* Proto family (IPv4) */
    inet_pton(AF_INET, ipstr, (void *)(&remote.sin_addr.s_addr)); /* set dst IP address */
    remote.sin_port = htons(port); /* set the dst port */

    if (connect(s->fd, (struct sockaddr *)&remote, sizeof remote) < 0) {
        close(s->fd);
        free(s);
        free(result);
        return NULL;
    }

    result->sock = s;

    return result;
}

int net_send(struct net_tcpsocket *socket, char *line, int len)
{
    return send(((struct netwrap *)(socket->sock))->fd, line, len, 0);
}

int net_recv(struct net_tcpsocket *socket, char *buff, int maxlen)
{
    int res;
    fd_set rfds;
    struct timeval tv;
    int realsocket = ((struct netwrap *)socket->sock)->fd;

    /* Use select() to wait up to 100ms if nothing awaits on the socket (spares some CPU time) */
    FD_ZERO(&rfds);
    FD_SET(realsocket, &rfds);
    tv.tv_sec = 0;
    tv.tv_usec = 100000;

    res = select(realsocket + 1, &rfds, NULL, NULL, &tv);
    if (res < 0)
        return -1;
    if (res == 0)
        return 0;

    /* read the stuff now (if any) */
    res = recv(realsocket, buff, maxlen, MSG_DONTWAIT);
    if (res < 0) {
        if (errno == EAGAIN) return 0;
        if (errno == EWOULDBLOCK) return 0;
    }

    if (res == 0)
        return -1; /* the peer performed an orderly shutdown */

    return res;
}

void net_close(struct net_tcpsocket *socket)
{
    close(((struct netwrap *)(socket->sock))->fd);
    free(socket->sock);
}

void net_abort(struct net_tcpsocket *socket)
{
    net_close(socket);
}
