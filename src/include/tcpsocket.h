/***************************************************************************
                                tcpsocket.h
                             -------------------
	revision             : $Id: tcpsocket.h,v 1.1.1.1 2002-10-10 09:59:19 tellini Exp $
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

#ifndef PROM_TCPSOCKET_H
#define PROM_TCPSOCKET_H

#include "main.h"

#include <sys/socket.h>
#include <netinet/in.h>

#include "socket.h"

class TcpSocket : public Socket
{
public:
						TcpSocket();
						TcpSocket( int fd );

	bool				Bind( unsigned short port );
	virtual Socket		*Accept( void );

	bool				Connect( Prom_Addr *addr, short port );
	bool				AsyncConnect( Prom_Addr *addr, short port, int timeout = -1 );

	virtual char		*GetPeerName( void );	// human readable name
	virtual char		*GetLocalName( void );
	int					GetPeerPort( void );
	int					GetLocalPort( void );
	
	static char			*AddrToName( Prom_Addr *addr );

protected:
#if HAVE_IPV6
	struct sockaddr_in6	AddrBuf;
	char				NameBuf[ INET6_ADDRSTRLEN ];
#else
	struct sockaddr_in	AddrBuf;
	char				NameBuf[ 16 ];
#endif

	virtual bool		NameToAddr( const char *name, struct sockaddr **addr, socklen_t *len );
};

#endif
