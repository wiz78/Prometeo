/***************************************************************************
                                tcpsocket.cpp
                             -------------------
	revision             : $Id: tcpsocket.cpp,v 1.1 2002-10-10 10:22:59 tellini Exp $
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

#include "main.h"

#include <string.h>
#include <arpa/inet.h>

#include "tcpsocket.h"

#if HAVE_IPV6
#define PF	PF_INET6
#else
#define PF	PF_INET
#endif

//---------------------------------------------------------------------------
TcpSocket::TcpSocket() : Socket( PF, SOCK_STREAM, IPPROTO_TCP )
{
}
//---------------------------------------------------------------------------
TcpSocket::TcpSocket( int fd ) : Socket( fd )
{
}
//---------------------------------------------------------------------------
bool TcpSocket::Bind( unsigned short port )
{           
#if HAVE_IPV6
	struct sockaddr_in6 sockadr;

	memset( &sockadr, 0, sizeof( sockadr ));

	sockadr.sin6_family   = AF_INET6;
	sockadr.sin6_port     = htons( port );
	sockadr.sin6_addr     = in6addr_any;
#else
	struct sockaddr_in sockadr;

	memset( &sockadr, 0, sizeof( sockadr ));

	sockadr.sin_family = AF_INET;
	sockadr.sin_port   = htons( port );
#endif

	SetLinger( false );

	return( bind( FD, (struct sockaddr *)&sockadr, sizeof( sockadr )) >= 0 );
}
//---------------------------------------------------------------------------
Socket *TcpSocket::Accept( void )
{
#if HAVE_IPV6
	struct sockaddr_in6	addr;
#else
	struct sockaddr_in	addr;
#endif
	socklen_t			len = sizeof( addr );
	int					sock = accept( FD, (struct sockaddr *)&addr, &len );

	return( new TcpSocket( sock ));
}
//---------------------------------------------------------------------------
bool TcpSocket::Connect( Prom_Addr *addr, short port )
{
#if HAVE_IPV6
	struct sockaddr_in6	sockaddr;
#else
	struct sockaddr_in	sockaddr;
#endif

	memset( &sockaddr, 0, sizeof( sockaddr ));

#if HAVE_IPV6
	sockaddr.sin6_family = AF_INET6;
	sockaddr.sin6_port   = htons( port );

	memcpy( &sockaddr.sin6_addr, addr, sizeof( *addr ));
#else
	sockaddr.sin_family = AF_INET;
	sockaddr.sin_addr   = *addr;
	sockaddr.sin_port   = htons( port );
#endif

	return( Socket::Connect(( struct sockaddr * )&sockaddr, sizeof( sockaddr )));
}
//---------------------------------------------------------------------------
bool TcpSocket::AsyncConnect( Prom_Addr *addr, short port, int timeout )
{
#if HAVE_IPV6
	struct sockaddr_in6	sockaddr;
#else
	struct sockaddr_in	sockaddr;
#endif

	memset( &sockaddr, 0, sizeof( sockaddr ));

#if HAVE_IPV6
	sockaddr.sin6_family = AF_INET6;
	sockaddr.sin6_port   = htons( port );

	memcpy( &sockaddr.sin6_addr, addr, sizeof( *addr ));
#else
	sockaddr.sin_family = AF_INET;
	sockaddr.sin_addr   = *addr;
	sockaddr.sin_port   = htons( port );
#endif

	return( Socket::AsyncConnect(( struct sockaddr * )&sockaddr, sizeof( sockaddr ), timeout ));
}
//---------------------------------------------------------------------------
bool TcpSocket::NameToAddr( const char *name, struct sockaddr **addr, socklen_t *len )
{
	bool	ret;

	*addr = (struct sockaddr *)&AddrBuf;
	*len  = sizeof( AddrBuf );

#if HAVE_IPV6
	ret = inet_pton( AF_INET6, name, &AddrBuf.sin6_addr );
#else
	ret = inet_aton( name, &AddrBuf.sin_addr );
#endif

	return( ret );
}
//---------------------------------------------------------------------------
char *TcpSocket::AddrToName( Prom_Addr *addr )
{
	char *name;

#if HAVE_IPV6
	if( name = (char *)malloc( INET6_ADDRSTRLEN )) {

		inet_ntop( AF_INET6, addr, name, INET6_ADDRSTRLEN );

	} else
		name = strdup( "***ERROR***" );
#else
	name = strdup( inet_ntoa( *addr ));
#endif

	return( name );
}
//---------------------------------------------------------------------------
char *TcpSocket::GetPeerName( void )
{
	socklen_t	len = sizeof( AddrBuf );

	if( getpeername( FD, (struct sockaddr *)&AddrBuf, &len ) == 0 ) {

#if HAVE_IPV6
		if( !inet_ntop( AF_INET6, &AddrBuf, NameBuf, sizeof( NameBuf )))
			strcpy( NameBuf, "ERROR" );
#else
		strcpy( NameBuf, inet_ntoa( AddrBuf.sin_addr ));
#endif
	} else
		strcpy( NameBuf, "ERROR" );

	return( NameBuf );
}
//---------------------------------------------------------------------------
char *TcpSocket::GetLocalName( void )
{
	socklen_t	len = sizeof( AddrBuf );

	if( getsockname( FD, (struct sockaddr *)&AddrBuf, &len ) == 0 ) {

#if HAVE_IPV6
		if( !inet_ntop( AF_INET6, &AddrBuf, NameBuf, sizeof( NameBuf )))
			strcpy( NameBuf, "ERROR" );
#else
		strcpy( NameBuf, inet_ntoa( AddrBuf.sin_addr ));
#endif
	} else
		strcpy( NameBuf, "ERROR" );

	return( NameBuf );
}
//---------------------------------------------------------------------------
int TcpSocket::GetPeerPort( void )
{
	socklen_t	len = sizeof( AddrBuf );
	int			port = 0;

	if( getpeername( FD, (struct sockaddr *)&AddrBuf, &len ) == 0 ) {
#if HAVE_IPV6
		port = ntohs( AddrBuf.sin6_port );
#else
		port = ntohs( AddrBuf.sin_port );
#endif
	}

	return( port );
}
//---------------------------------------------------------------------------
int TcpSocket::GetLocalPort( void )
{
	socklen_t	len = sizeof( AddrBuf );
	int			port = 0;

	if( getsockname( FD, (struct sockaddr *)&AddrBuf, &len ) == 0 ) {
#if HAVE_IPV6
		port = ntohs( AddrBuf.sin6_port );
#else
		port = ntohs( AddrBuf.sin_port );
#endif
	}

	return( port );
}
//---------------------------------------------------------------------------
