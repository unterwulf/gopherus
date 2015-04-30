/*
 * This file is part of the Gopherus project.
 * Copyright (C) Mateusz Viste 2013
 */

#ifndef INT2STR_H
#define INT2STR_H

/* converts an integer from the range 0..9999999 into a string. returns the length of the computed string, or -1 on error. */
int int2str(char *res, int x);

#endif
