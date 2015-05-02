/*
 * This file is part of the Gopherus project
 * Copyright (C) Mateusz Viste 2013
 */

#include <stdio.h>
#include "wordwrap.h"

int main(int argc, char **argv)
{
    char *strptr;
    char res[128];
    if (argc != 2) {
        puts("Usage: wraptest teststring");
        return 0;
    }
    for (strptr = wordwrap(res, argv[1], 16); ; strptr = wordwrap(res, strptr, 16)) {
        printf("|%s|\r\n", res);
        if (strptr == NULL) break;
    }
    return 0;
}
