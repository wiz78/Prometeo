/***************************************************************************
                                   api.cpp
                             -------------------
	revision             : $Id: api.cpp,v 1.1 2002-10-10 10:22:59 tellini Exp $
    copyright            : (C) 2002 by Simone Tellini
    email                : tellini@users.sourceforge.net
	
	description			 : this files contains the entry points for
						 : the functions exported for use by modules.
						 ; C++ modules can also access the objects directly
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

#include <stdarg.h>

#include "dnscache.h"

extern Core *App;

//---------------------------------------------------------------------------
API_EXPORT( void ) Prom_Log( int level, char *fmt, ... )
{
	va_list	ap;

	va_start( ap, fmt );

	App->Log->VLog( level, fmt, ap );

	va_end( ap );
}
//---------------------------------------------------------------------------
API_EXPORT( pid_t ) Prom_Fork( char *ident )
{
	return( App->Fork( ident ));
}
//---------------------------------------------------------------------------
API_EXPORT( BOOL ) Prom_Resolve( char *hostname, Prom_Addr *addr )
{
	return( App->DNS->Resolve( hostname, addr ));
}
//---------------------------------------------------------------------------
API_EXPORT( void ) Prom_AsyncResolve( char *hostname, Prom_Addr *addr,
									  Prom_DNS_Callback callback, void *userdata )
{
	App->DNS->AsyncResolve( hostname, addr, callback, userdata );
}
//---------------------------------------------------------------------------
