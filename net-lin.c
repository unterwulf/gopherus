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

static int g_sk;

unsigned long net_dnsresolve(const char *name)
{
    struct hostent *hent = gethostbyname(name);
    return (hent) ? htonl(*((uint32_t *)(hent->h_addr))) : 0;
}

int net_init(void)
{
    return 0;
}

int net_connect(unsigned long ipaddr, unsigned short port)
{
    struct sockaddr_in remote;
    char ipstr[64];

    sprintf(ipstr, "%lu.%lu.%lu.%lu", (ipaddr >> 24) & 0xFF, (ipaddr >> 16) & 0xFF, (ipaddr >> 8) & 0xFF, ipaddr & 0xFF);

    g_sk = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (g_sk < 0)
        return -1;

    remote.sin_family = AF_INET;  /* Proto family (IPv4) */
    inet_pton(AF_INET, ipstr, (void *)(&remote.sin_addr.s_addr)); /* set dst IP address */
    remote.sin_port = htons(port); /* set the dst port */

    if (connect(g_sk, (struct sockaddr *)&remote, sizeof remote) < 0) {
        close(g_sk);
        return -1;
    }

    return 0;
}

int net_send(const char *buf, int len)
{
    return send(g_sk, buf, len, 0);
}

int net_recv(char *buf, int maxlen)
{
    int res;
    fd_set rfds;
    struct timeval tv;

    /* Use select() to wait up to 100ms if nothing awaits on the socket (spares some CPU time) */
    FD_ZERO(&rfds);
    FD_SET(g_sk, &rfds);
    tv.tv_sec = 0;
    tv.tv_usec = 100000;

    res = select(g_sk + 1, &rfds, NULL, NULL, &tv);
    if (res < 0)
        return -1;
    if (res == 0)
        return 0;

    /* read the stuff now (if any) */
    res = recv(g_sk, buf, maxlen, MSG_DONTWAIT);
    if (res < 0) {
        if (errno == EAGAIN) return 0;
        if (errno == EWOULDBLOCK) return 0;
    }

    if (res == 0)
        return -1; /* the peer performed an orderly shutdown */

    return res;
}

void net_close(void)
{
    close(g_sk);
}

void net_abort(void)
{
    net_close();
}
