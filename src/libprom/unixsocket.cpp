/***************************************************************************
                               unixsocket.cpp
                             -------------------
	revision             : $Id: unixsocket.cpp,v 1.4 2002-10-15 13:03:42 tellini Exp $
    copyright            : (C) 2002 by Simone Tellini
    email                : tellini@users.sourceforge.net

	description          : unix socket interface wrapper
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
#include <sys/uio.h>

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
// the following 2 functions derive from OpenSSH - monitor_fdpass.c
/*
 * Copyright 2001 Niels Provos <provos@citi.umich.edu>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
bool UnixSocket::SendFD( int fd )
{
	bool			ret = false;
#if defined(HAVE_SENDMSG) && (defined(HAVE_ACCRIGHTS_IN_MSGHDR) || defined(HAVE_CONTROL_IN_MSGHDR))
	struct msghdr	msg;
	struct iovec	vec;
	char			ch = '\0';
#ifndef HAVE_ACCRIGHTS_IN_MSGHDR
	char			tmp[ CMSG_SPACE( sizeof( int )) ];
	struct cmsghdr *cmsg;
#endif

	SetNonBlocking( false );

	memset( &msg, 0, sizeof( msg ));
#ifdef HAVE_ACCRIGHTS_IN_MSGHDR
	msg.msg_accrights    = (caddr_t)&fd;
	msg.msg_accrightslen = sizeof( fd );
#else
	msg.msg_control           = (caddr_t)tmp;
	msg.msg_controllen        = CMSG_LEN( sizeof( int ));
	cmsg                      = CMSG_FIRSTHDR( &msg );
	cmsg->cmsg_len            = CMSG_LEN( sizeof( int ));
	cmsg->cmsg_level          = SOL_SOCKET;
	cmsg->cmsg_type           = SCM_RIGHTS;
	*(int *)CMSG_DATA( cmsg ) = fd;
#endif

	vec.iov_base   = &ch;
	vec.iov_len    = 1;
	msg.msg_iov    = &vec;
	msg.msg_iovlen = 1;

	ret = sendmsg( FD, &msg, 0 ) == 1;
#endif

	return( ret );
}
//---------------------------------------------------------------------------
int UnixSocket::RecvFD( void )
{
	int				fd = -1;
#if defined(HAVE_RECVMSG) && (defined(HAVE_ACCRIGHTS_IN_MSGHDR) || defined(HAVE_CONTROL_IN_MSGHDR))
	struct msghdr	msg;
	struct iovec	vec;
	char			ch;
	int				n;
#ifndef HAVE_ACCRIGHTS_IN_MSGHDR
	char			tmp[ CMSG_SPACE( sizeof( int ))];
	struct cmsghdr *cmsg;
#endif

	SetNonBlocking( false );

	memset( &msg, 0, sizeof( msg ));

	vec.iov_base   = &ch;
	vec.iov_len    = 1;
	msg.msg_iov    = &vec;
	msg.msg_iovlen = 1;
#ifdef HAVE_ACCRIGHTS_IN_MSGHDR
	msg.msg_accrights    = (caddr_t)&fd;
	msg.msg_accrightslen = sizeof( fd );
#else
	msg.msg_control    = tmp;
	msg.msg_controllen = sizeof( tmp );
#endif

	n = recvmsg( FD, &msg, 0 );

	if( n == 1 ) {

#ifdef HAVE_ACCRIGHTS_IN_MSGHDR
		if( msg.msg_accrightslen != sizeof( fd ))
			fd = -1;
#else
		cmsg = CMSG_FIRSTHDR( &msg );

		if( cmsg->cmsg_type == SCM_RIGHTS )
			fd = (*(int *)CMSG_DATA( cmsg ));
#endif
	}

#endif

	return( fd );
}
//---------------------------------------------------------------------------
