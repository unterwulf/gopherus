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

int net_connect(unsigned long ipaddr, unsigned short port)
{
    return 0;
}

int net_send(const char *buf, int len)
{
    return -1;
}

int net_recv(char *buf, int maxlen)
{
    return -1;
}

void net_close(void)
{
}

void net_abort(void)
{
}
