/*
 * This file is part of the Gopherus project.
 * It provides a set of basic network-related functions.
 *
 * Copyright (C) Mateusz Viste 2013
 *
 * Provides all network functions used by Gopherus, wrapped around the WatTCP TCP/IP stack.
 */

#include <stdlib.h>
#include <string.h>
#include <time.h>

/* WatTCP includes */
#include <tcp.h>

#include "net.h"

#define SKBUF_SIZE 2048

static tcp_Socket g_sk;
static char *g_skbuf;

static int is_int_pending_adapter(void *sock)
{
    return is_int_pending();
}

unsigned long net_dnsresolve(const char *name)
{
    return lookup_host(name, NULL);
}

static int dummy_printf(const char * format, ...)
{
    if (format == NULL) return -1;
    return 0;
}

int net_init(void)
{
    tzset();
    _printf = dummy_printf;  /* this is to avoid watt32 printing its stuff to console */
    g_skbuf = malloc(SKBUF_SIZE);
    return sock_init();
}

int net_connect(unsigned long ipaddr, unsigned short port)
{
    int status = 0;

    if (!tcp_open(&g_sk, 0, ipaddr, port, NULL))
        return -1;

    sock_setbuf(&g_sk, g_skbuf, SKBUF_SIZE);
    sock_wait_established(&g_sk, sock_delay, &is_int_pending_adapter, NULL);
    sock_tick(&g_sk, &status); /* in case they sent reset */
    return 0;
sock_err:
    return -1;
}

int net_send(const char *buf, int len)
{
    int status = 0;
    int res = sock_write(&g_sk, buf, len);
    sock_tick(&g_sk, &status); /* call this to let WatTCP handle its internal stuff */
    return res;
sock_err:
    return -1;
}

int net_recv(char *buf, int maxlen)
{
    int status = 0;
    sock_tick(&g_sk, &status); /* call this to let WatTCP handle its internal stuff */
    return sock_fastread(&g_sk, buf, maxlen);
sock_err:
    return -1;
}

void net_close(void)
{
    sock_close(&g_sk);
    sock_wait_closed(&g_sk, sock_delay, &is_int_pending_adapter, NULL);
sock_err:
    return;
}

void net_abort(void)
{
    sock_abort(&g_sk);
}
