/***************************************************************************
                                tcpsocket.h
                             -------------------
	revision             : $Id: tcpsocket.h,v 1.3 2002-10-29 18:01:14 tellini Exp $
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

#if HAVE_IPV6
	bool				MakeIPv4( void );
#else
	bool				MakeIPv4( void ) { return( true ); }
#endif
	bool				IsIPv4( void ) const { return( Family == AF_INET ); }

	bool				Bind( unsigned short port = 0 );
	virtual Socket		*Accept( void );

	bool				Connect( Prom_Addr *addr, short port );
	bool				AsyncConnect( Prom_Addr *addr, short port, int timeout = -1 );

	virtual char		*GetPeerName( void );	// human readable name
	virtual char		*GetLocalName( void );
	int					GetPeerPort( void );
	int					GetLocalPort( void );

	static char			*AddrToName( Prom_Addr *addr );
	static bool			NameToAddr( const char *name, Prom_Addr *addr );

protected:
#if HAVE_IPV6
	int					Family;
	struct sockaddr_in6	AddrBuf6;
#endif
	struct sockaddr_in	AddrBuf;
#ifdef HAVE_IPV6
	char				NameBuf[ INET6_ADDRSTRLEN ];
#else
	char				NameBuf[ 16 ];
#endif

	virtual bool		NameToAddr( const char *name, struct sockaddr **addr, socklen_t *len );
};

#endif
