/***************************************************************************
                                   main.h
                             -------------------
	revision             : $Id: main.h,v 1.1.1.1 2002-10-10 09:59:13 tellini Exp $
    copyright            : (C) 2002 by Simone Tellini
    email                : tellini@users.sourceforge.net

    description          : include this file to get the declarations of
	                       global variables, common includes and functions
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef MAIN_H
#define MAIN_H

// XXX
#define DEBUG	1

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif

#include <stdlib.h>
#include <unistd.h>
#ifdef HAVE_STRING_H
#include <string.h>
#endif
#ifdef HAVE_STRINGS_H
#include <strings.h>
#endif
#include <errno.h>

#ifndef EWOULDBLOCK
#define EWOULDBLOCK EAGAIN
#endif

#if TIME_WITH_SYS_TIME
# include <sys/time.h>
# include <time.h>
#else
# if HAVE_SYS_TIME_H
#  include <sys/time.h>
# else
#  include <time.h>
# endif
#endif

#include "core.h"
#include "api.h"
#include "logger.h"

// application's main object
extern Core *App;

// from setproctitle.cpp
extern void save_ps_display_args(int argc, char *argv[]);

// debug stuff
#ifdef DEBUG
#define DBG(x)		{ x; }
#else
#define DBG(x)
#endif

#endif

