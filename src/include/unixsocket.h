/***************************************************************************
                                unixsocket.h
                             -------------------
	revision             : $Id: unixsocket.h,v 1.1.1.1 2002-10-10 09:59:19 tellini Exp $
    copyright            : (C) 2002 by Simone Tellini
    email                : tellini@users.sourceforge.net

	description          : socket interface wrapper
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef PROM_UNIXSOCKET_H
#define PROM_UNIXSOCKET_H

#include <sys/socket.h>
#include <sys/un.h>

#include "socket.h"

class UnixSocket : public Socket
{
public:
						UnixSocket();
						UnixSocket( int fd );
						~UnixSocket();

	bool				Bind( const char *name );
	virtual Socket		*Accept( void );

	virtual char		*GetPeerName( void );	// human readable name
	virtual char		*GetLocalName( void );

	mode_t				GetPeerPerms( void );
	uid_t				GetPeerUID( void );

protected:
	struct sockaddr_un	AddrBuf;
	mode_t				PeerMode;
	uid_t				PeerUID;

	virtual bool		NameToAddr( const char *name, struct sockaddr **addr, socklen_t *len );
	bool				DoStat( void );
};

#endif
