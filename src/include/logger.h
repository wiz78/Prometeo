/***************************************************************************
                                  logger.h
                             -------------------
	revision             : $Id: logger.h,v 1.1.1.1 2002-10-10 09:59:12 tellini Exp $
    copyright            : (C) 2002 by Simone Tellini
    email                : tellini@users.sourceforge.net
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef LOGGER_H
#define LOGGER_H

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifdef HAVE_SYSLOG_H
#	include <syslog.h>
#else
#	ifdef HAVE_SYS_SYSLOG_H
#		include <sys/syslog.h>
#	endif
#endif
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include <stdarg.h>

class Registry;

class Logger
{
public:
					Logger();
					~Logger();

	void			Log( int level, char *fmt, ... );
	void			VLog( int level, char *fmt, va_list ap );

	void			Setup( Registry *reg );
};

#endif
