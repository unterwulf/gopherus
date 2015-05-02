/*
 * This file is part of the Gopherus project.
 * Copyright (C) Mateusz Viste 2013
 */

#include <stdlib.h>    /* NULL */
#include "wordwrap.h"

char *wordwrap(char *dst, const char *src, int width)
{
    int i, lastspace = 0;

    for (i = 0; i < width; i++) {
        dst[i] = src[i];
        if (src[i] == '\0')
            return NULL;
        if (dst[i] == '\t')
            dst[i] = ' '; /* replace all TABs with spaces */
        if (dst[i] == ' ')
            lastspace = i;
        if (src[i] == '\n') {
            /* if it's part of a CR/LF couple, delete them both */
            if ((i > 0) && (src[i - 1] == '\r'))
                dst[i - 1] = '\0';
            dst[i] = '\0';
            return (char *)src + i + 1;
        }
    }

    if (lastspace == 0 || src[width] == ' ')
        lastspace = width;

    dst[lastspace] = '\0';
    src += lastspace;
    while (*src == ' ')
        src++;

    return (*src == '\0') ? NULL : (char *)src;
}
