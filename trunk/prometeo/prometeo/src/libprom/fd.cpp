/***************************************************************************
                                   fd.cpp
                             -------------------
	revision             : $Id: fd.cpp,v 1.1.1.1 2002-10-10 09:59:20 tellini Exp $
    copyright            : (C) 2002 by Simone Tellini
    email                : tellini@users.sourceforge.net

	description          : this class wraps around a file descriptor
                           to be used in multiplexed I/O, holding
                           all the info needed and defining the interface
                           used by the dispatcher to notify of incoming
                           events
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

#include <fcntl.h>

#include "fd.h"
#include "iodispatcher.h"

//---------------------------------------------------------------------------
Fd::Fd()
{
	FD					= -1;
	Timeout				= 0;
	Dispatcher			= NULL;
	UserData			= NULL;
	UserDataDestructor	= NULL;
}
//---------------------------------------------------------------------------
Fd::~Fd()
{
	if( Dispatcher )
		Dispatcher->RemFD( this, PROM_IOF_READ | PROM_IOF_WRITE | PROM_IOF_EXCEPT );

	if( UserData && UserDataDestructor )
		( *UserDataDestructor )( UserData );
}
//---------------------------------------------------------------------------
void Fd::SetTimeout( int seconds )
{
	if( seconds > 0 ) {

		time( &Timeout );

		Timeout += seconds;

	} else
		Timeout = 0;
}
//---------------------------------------------------------------------------
void Fd::HandleTimeout( void )
{
	Timeout = 0;
}
//---------------------------------------------------------------------------
void Fd::UseDispatcher( IODispatcher *iod )
{
	Dispatcher = iod;
}
//---------------------------------------------------------------------------
void Fd::SetUserData( void *userdata, Prom_Destructor destructor )
{
	UserData           = userdata;
	UserDataDestructor = destructor;
}
//---------------------------------------------------------------------------
bool Fd::SetNonBlocking( bool dontblock )
{
	bool	ret = true;

	if( FdFlags.IsSet( PROM_FDF_NONBLOCKING ) != dontblock ) {
		int flags;

		flags = fcntl( FD, F_GETFL, 0 );

		if( dontblock )
			flags |= O_NONBLOCK;
		else
			flags &= ~O_NONBLOCK;

		ret = fcntl( FD, F_SETFL, flags ) == 0;

		if( ret )
			FdFlags.Set( PROM_FDF_NONBLOCKING, dontblock );
	}

	return( ret );
}
//---------------------------------------------------------------------------
bool Fd::IsNonBlocking( void ) const
{
	return( FdFlags.IsSet( PROM_FDF_NONBLOCKING ));
}
//---------------------------------------------------------------------------
