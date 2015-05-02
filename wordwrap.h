/*
 * This file is part of the Gopherus project.
 * Copyright (C) Mateusz Viste 2013
 */

#ifndef WORDWRAP_H
#define WORDWRAP_H

/* Fills *dst with part or totality of original *src and return a pointer
 * where to start next iteration. dst shall point to a buffer of size of
 * at least width + 1 chars. */
char *wordwrap(char *dst, const char *src, int width);

#endif
