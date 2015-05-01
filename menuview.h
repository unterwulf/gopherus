/*
 * This file is part of the Gopherus project.
 * Copyright (c) 2013 Mateusz Viste
 */

#ifndef MENUVIEW_H
#define MENUVIEW_H

#include "common.h"
#include "history.h"

int display_menu(struct historytype **history, struct gopherusconfig *cfg, char *buffer, char *statusbar);

#endif
