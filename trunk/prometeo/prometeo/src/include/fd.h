/***************************************************************************
                                    fd.h
                             -------------------
	revision             : $Id: fd.h,v 1.1.1.1 2002-10-10 09:59:10 tellini Exp $
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

#ifndef FD_H
#define FD_H

#include "api.h"
#include "bitfield.h"

#include <sys/types.h>

class IODispatcher;

class Fd
{
public:
						Fd();
						~Fd();

	int					GetFD( void ) const { return( FD ); }

	bool				IsValid( void ) const { return( FD >= 0 ); }

	virtual void		HandleRead( void ) {};
	virtual void		HandleWrite( void ) {};
	virtual void		HandleExcept( void ) {};
	virtual void		HandleTimeout( void ); // this just resets the timeout

	void				SetTimeout( int seconds );
	bool				HasTimedOut( time_t now ) const { return( Timeout && ( now > Timeout )); }

	bool				SetNonBlocking( bool dontblock );
	bool				IsNonBlocking( void ) const;

	void				UseDispatcher( IODispatcher *iod );

	void				SetUserData( void *userdata, Prom_Destructor destructor );
	void				*GetUserData( void ) const { return( UserData ); }

protected:
	int					FD;
	time_t				Timeout;
	IODispatcher		*Dispatcher;
	void				*UserData;
	Prom_Destructor		UserDataDestructor;
	BitField			FdFlags;
};

// Flags
#define PROM_FDF_NONBLOCKING	(1 << 0)

#endif
