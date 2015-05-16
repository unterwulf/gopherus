/*
 * This file is part of the Gopherus project.
 * Copyright (c) 2015 Vitaly Sinilin
 */

#ifndef SNPRINTF_H
#define SNPRINTF_H

#ifndef HAVE_SNPRINTF
#include <stdarg.h>
#include <stddef.h>
int vsnprintf(char *str, size_t size, const char *fmt, va_list ap);
int snprintf(char *str, size_t size, const char *fmt, ...);
#else
#include <stdio.h>
#endif

#endif
