/*
 * This file is part of the Gopherus project.
 * Copyright (c) 2015 Vitaly Sinilin
 *
 * This snprintf() implements only a subset of standard snprintf()
 * features that is used in gopherus.
 */

#include <ctype.h>
#include <stdarg.h>
#include <string.h>
#include "snprintf.h"

#define FL_ZERO 0x1
#define FL_LEFT 0x2
#define FL_PREC 0x4
#define FL_INT  0x8

struct print_fmt {
    int width;
    int precision;
    unsigned int flags;
};

int vsnprintf(char *str, size_t size, const char *fmt, va_list ap)
{
    const char *pos;
    const char *lastspec = NULL;
    int is_empty_str = (size == 0);
    size_t ret = size;
    enum {
        ln_normal, ln_long
    } length = ln_normal;
    enum {
        st_escape, st_normal, st_flags, st_width, st_prec, st_length, st_spec
    } state = st_normal;
    struct print_fmt pfmt;

#define append_char(c) { if (size > 0) { *str++ = c; size--; } else ret++; }

    for (pos = fmt; *pos; pos++) {
        switch (state) {
            case st_normal:
                switch (*pos) {
                    case '\\': state = st_escape; break;
                    case '%':  memset(&pfmt, '\0', sizeof pfmt);
                               lastspec = pos;
                               length = ln_normal;
                               state = st_flags;
                               break;
                    default:   append_char(*pos);
                }
                break;

            case st_escape:
                switch (*pos) {
                    case 'n': append_char('\n'); break;
                    case 'r': append_char('\r'); break;
                    case 't': append_char('\t'); break;
                    case '\\': append_char('\\'); break;
                    /* process it again */
                    default:  append_char('\\'); pos--;
                }
                state = st_normal;
                break;

            case st_flags:
                switch (*pos) {
                    case '0': pfmt.flags |= FL_ZERO; break;
                    case '-': pfmt.flags |= FL_LEFT; break;
                    default:  pos--; state = st_width;
                }
                break;

            case st_width:
                if (isdigit(*pos)) {
                    pfmt.width = 10*pfmt.width + (*pos - '0');
                } else if (*pos == '.') {
                    state = st_prec;
                    pfmt.flags |= FL_PREC;
                } else {
                    state = st_length;
                    pos--; /* process it again */
                }
                break;

            case st_prec:
                if (*pos == '*') {
                    pfmt.precision = va_arg(ap, int);
                    state = st_length;
                } else if (isdigit(*pos)) {
                    pfmt.precision = 10*pfmt.precision + (*pos - '0');
                } else {
                    state = st_length;
                    pos--; /* process it again */
                }
                break;

            case st_length:
                if (*pos == 'l')
                    length = ln_long;
                else
                    pos--; /* process it again */
                state = st_spec;
                break;

            case st_spec:
                switch (*pos) {
                    case 'u': {
                        char buf[33] = "";
                        char *ptr = buf + sizeof(buf) - 1;
                        size_t arg_len = 0;
                        size_t towrite = 0;
                        unsigned arg = va_arg(ap, unsigned);

                        do {
                            *--ptr = '0' + arg % 10;
                            arg_len++;
                        } while ((arg /= 10) != 0);

                        towrite = arg_len > size ? size : arg_len;

                        if (towrite > 0) {
                            memcpy(str, ptr, towrite);
                            str += towrite;
                            size -= towrite;
                        }

                        ret += arg_len - towrite;
                        break;
                    }
                    case 's': {
                        char *arg = va_arg(ap, char *);
                        size_t arg_len = strlen(arg);
                        size_t towrite = 0;

                        if ((pfmt.flags & FL_PREC) && arg_len > pfmt.precision)
                            arg_len = pfmt.precision;

                        towrite = arg_len > size ? size : arg_len;

                        if (towrite > 0) {
                            memcpy(str, arg, towrite);
                            str += towrite;
                            size -= towrite;
                        }

                        ret += arg_len - towrite;
                        break;
                    }
                    case 'c':
                        append_char(va_arg(ap, char));
                        break;
                    default:
                        append_char('%');
                        pos = lastspec + 1;
                        break;
                }

                state = st_normal;
                break;
        }
    }

    if (size > 0)
        *str = '\0';
    else if (!is_empty_str)
        *(str-1) = '\0';

    return ret - size;
}

int snprintf(char *str, size_t size, const char *fmt, ...)
{
    int ret;
    va_list ap;
    va_start(ap, fmt);
    ret = vsnprintf(str, size, fmt, ap);
    va_end(ap);
    return ret;
}
