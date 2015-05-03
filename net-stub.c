/*
 * This file is part of the Gopherus project.
 * It provides a set of stubs for basic network-related functions.
 *
 * Copyright (c) 2015 Vitaly Sinilin
 */

#include <stdlib.h>
#include "net.h"

unsigned long net_dnsresolve(const char *name)
{
    return 0;
}

int net_init(void)
{
    return 0;
}

struct net_tcpsocket *net_connect(unsigned long ipaddr, int port)
{
    return NULL;
}

int net_send(struct net_tcpsocket *socket, char *line, int len)
{
    return -1;
}

int net_recv(struct net_tcpsocket *socket, char *buff, int maxlen)
{
    return -1;
}

void net_close(struct net_tcpsocket *socket)
{
}

void net_abort(struct net_tcpsocket *socket)
{
}
