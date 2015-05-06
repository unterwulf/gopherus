/*
 * This file is part of the Gopherus project
 * Copyright (C) Mateusz Viste 2013
 *
 * Provides all network functions used by Gopherus, wrapped around POSIX (BSD) sockets.
 */

#include <stdlib.h>  /* NULL */
#include <sys/socket.h> /* socket() */
#include <fcntl.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <stdio.h> /* sprintf() */
#include <unistd.h> /* close() */
#include <errno.h>
#include <stdint.h> /* uint32_t */

#include "net.h"

#define POLLING_TIMEOUT_USEC 125000

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

    g_sk = socket(AF_INET, SOCK_STREAM | O_NONBLOCK, IPPROTO_TCP);
    if (g_sk < 0)
        return -1;

    remote.sin_family = AF_INET;
    remote.sin_addr.s_addr = htonl(ipaddr);
    remote.sin_port = htons(port);

    if (connect(g_sk, (struct sockaddr *)&remote, sizeof remote) < 0) {
        if (errno == EINPROGRESS) {
            while (!is_int_pending()) {
                int ret;
                fd_set wfd;
                struct timeval tv;

                tv.tv_sec = 0;
                tv.tv_usec = POLLING_TIMEOUT_USEC;
                FD_ZERO(&wfd);
                FD_SET(g_sk, &wfd);

                ret = select(g_sk + 1, NULL, &wfd, NULL, &tv);

                if (ret == 1) {
                    return 0;
                } else if (ret < 0 && errno != EINTR) {
                    break;
                }
            }
        }
        close(g_sk);
        return -1;
    }

    return 0;
}

int net_send(const char *buf, int len)
{
    int ret;

    do {
        ret = send(g_sk, buf, len, 0);

        if (ret < 0) {
            if (errno == EINTR ||
                errno == EAGAIN ||
                errno == EWOULDBLOCK) {
                usleep(POLLING_TIMEOUT_USEC);
            } else {
                return ret;
            }
        }
    } while (!is_int_pending() && ret < 0);

    return ret;
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
