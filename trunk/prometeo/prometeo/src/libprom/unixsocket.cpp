/***************************************************************************
                               unixsocket.cpp
                             -------------------
	revision             : $Id: unixsocket.cpp,v 1.1.1.1 2002-10-10 09:59:22 tellini Exp $
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

#include <sys/socket.h>
#include <sys/un.h>
#include <sys/stat.h>

#include "unixsocket.h"

//---------------------------------------------------------------------------
UnixSocket::UnixSocket() : Socket( PF_UNIX, SOCK_STREAM, 0 )
{
	PeerMode = 0;
	PeerUID  = 0;
}
//---------------------------------------------------------------------------
UnixSocket::UnixSocket( int fd ) : Socket( fd )
{
	PeerMode = 0;
	PeerUID  = 0;
}
//---------------------------------------------------------------------------
UnixSocket::~UnixSocket()
{
}
//---------------------------------------------------------------------------
bool UnixSocket::Bind( const char *name )
{
	struct sockaddr_un	addr;

	unlink( name );

	memset( &addr, 0, sizeof( addr ));

	strcpy( addr.sun_path, name );

	addr.sun_family = AF_UNIX;
#ifdef HAVE_SOCKADDR_UN_LEN
	addr.sun_len    = sizeof( addr.sun_len ) + sizeof( addr.sun_family )
					  + strlen( addr.sun_path ) + 1;
#endif

	return( bind( FD, (struct sockaddr *)&addr, sizeof( addr )) >= 0 );
}
//---------------------------------------------------------------------------
Socket *UnixSocket::Accept( void )
{
	struct sockaddr_un	addr;
	socklen_t			len = sizeof( addr );
	int					sock = accept( FD, (struct sockaddr *)&addr, &len );

	return( new UnixSocket( sock ));
}
//---------------------------------------------------------------------------
bool UnixSocket::NameToAddr( const char *name, struct sockaddr **addr, socklen_t *len )
{
	memset( &AddrBuf, 0, sizeof( AddrBuf ));

	strcpy( AddrBuf.sun_path, name );

	AddrBuf.sun_family = AF_UNIX;
#ifdef HAVE_SOCKADDR_UN_LEN
	AddrBuf.sun_len = sizeof( AddrBuf.sun_len ) + sizeof( AddrBuf.sun_family )
					  + strlen( AddrBuf.sun_path ) + 1;
#endif

	*addr = (struct sockaddr *)&AddrBuf;
	*len  = sizeof( AddrBuf );

	return( true );
}
//---------------------------------------------------------------------------
char *UnixSocket::GetPeerName( void )
{
	socklen_t	len = sizeof( AddrBuf );

	AddrBuf.sun_path[ 0 ] = '\0';

	getpeername( FD, (struct sockaddr *)&AddrBuf, &len );

#ifdef HAVE_SOCKADDR_UN_LEN
	len -= sizeof( AddrBuf.sun_len ) + sizeof( sun_family );

	AddrBuf.sun_path[ len ] = '\0'; // make sure it's null terminated
#endif

	return( AddrBuf.sun_path );
}
//---------------------------------------------------------------------------
char *UnixSocket::GetLocalName( void )
{
	socklen_t	len = sizeof( AddrBuf );

	AddrBuf.sun_path[ 0 ] = '\0';

	getsockname( FD, (struct sockaddr *)&AddrBuf, &len );

#ifdef HAVE_SOCKADDR_UN_LEN
	len -= sizeof( AddrBuf.sun_len ) + sizeof( sun_family );

	AddrBuf.sun_path[ len ] = '\0'; // make sure it's null terminated
#endif

	return( AddrBuf.sun_path );
}
//---------------------------------------------------------------------------
bool UnixSocket::DoStat( void )
{
	bool ret = true;

	if( !PeerMode ) {
		struct stat	statbuf;

		ret = stat( GetPeerName(), &statbuf ) >= 0;

		if( ret ) {

			PeerMode = statbuf.st_mode;
			PeerUID  = statbuf.st_uid;
		}
	}

	return( ret );
}
//---------------------------------------------------------------------------
mode_t UnixSocket::GetPeerPerms( void )
{
	DoStat();

	return( PeerMode );
}
//---------------------------------------------------------------------------
uid_t UnixSocket::GetPeerUID( void )
{
	DoStat();

	return( PeerUID );
}
//---------------------------------------------------------------------------
