/***************************************************************************
                          logger.cpp  -  description
                             -------------------
	revision             : $Id: logger.cpp,v 1.1 2002-10-10 10:22:59 tellini Exp $
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

#include "main.h"

#ifndef HAVE_VSYSLOG
#include <stdio.h>
#endif

#include "registry.h"
/*
LOG_EMERG
LOG_ALERT
LOG_CRIT
LOG_ERR
LOG_WARNING
LOG_NOTICE
LOG_INFO
LOG_DEBUG
*/
//--------------------------------------------------------------------------
Logger::Logger()
{
	openlog( PACKAGE, LOG_PID | LOG_NDELAY, LOG_DAEMON );
}
//--------------------------------------------------------------------------
Logger::~Logger()
{
	closelog();
}
//--------------------------------------------------------------------------
void Logger::Log( int level, char *fmt, ... )
{
	va_list		ap;

	va_start( ap, fmt );

	VLog( level, fmt, ap );

	va_end( ap );
}
//--------------------------------------------------------------------------
void Logger::VLog( int level, char *fmt, va_list ap )
{
#ifdef HAVE_VSYSLOG
	vsyslog( level, fmt, ap );
#else
	char		buf[ 512 ];

	vsnprintf( buf, sizeof( buf ), fmt, ap );
	syslog( level, "%s", buf );
#endif
}
//--------------------------------------------------------------------------
void Logger::Setup( Registry *reg )
{
	if( reg->OpenKey( "root/Log", false )) {
		const char	*ident;

		ident = reg->GetString( "ident", PACKAGE );

		closelog();
		openlog( ident, LOG_PID | LOG_NDELAY, LOG_DAEMON );

		reg->CloseKey();
	}
}
//--------------------------------------------------------------------------
