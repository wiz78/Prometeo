/***************************************************************************
                                resolver.cpp
                             -------------------
	revision             : $Id: resolver.cpp,v 1.4 2002-11-08 14:32:32 tellini Exp $
    copyright            : (C) 2002 by Simone Tellini
    email                : tellini@users.sourceforge.net

	description          : a DNS resolver class. It spawns a new process
	                       to handle dns queries asynchronously
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

#include <sys/types.h>
#include <sys/signal.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdlib.h>

#include "resolver.h"
#include "unixsocket.h"

//---------------------------------------------------------------------------
void Resolver::OnFork( void )
{
	// avoid killing the process on deletion
	Flags.Clear( PROM_PROCF_RUNNING );

	delete Socket;

	Socket = NULL;
}
//---------------------------------------------------------------------------
void Resolver::WaitRequest( void )
{
	signal( SIGHUP, SIG_IGN );

	Process::WaitRequest();
}
//---------------------------------------------------------------------------
void Resolver::Dispatch( void )
{
	struct ResolverRequest *req = (struct ResolverRequest *)Data.GetData();
	bool					ok;
	Prom_Addr				addr;
	unsigned int			len;

	Prom_set_ps_display( "resolving %s", req->HostName );

	ok = Resolve( req->HostName, &addr, req->Family );

	len = sizeof( addr ) + strlen( req->HostName ) + 1 + sizeof( len );
	Socket->Send( &len, sizeof( len ));

	len = sizeof( addr );

#if HAVE_IPV6
	// Prom_Addr is struct in6_addr
	if( req->Family == AF_INET )
		len = sizeof( struct in_addr );
#endif

	Socket->Send( &len, sizeof( len ));
	Socket->Send( req->HostName, strlen( req->HostName ) + 1 );
	Socket->Send( &addr, len );

	if( !ok )
		App->Log->Log( LOG_ERR, "resolver: failed to resolve %s (%d: %s)",
					   req->HostName, errno, strerror( errno ));
}
//---------------------------------------------------------------------------
bool Resolver::Resolve( const char *host, Prom_Addr *addr, int family )
{
	bool	ok = false;

	if( family == AF_INET ) {
		unsigned int	len;
		struct hostent	*ent;

		if( ent = gethostbyname( host )) {

			memcpy( &((struct in_addr *)addr )->s_addr, ent->h_addr, sizeof( ent->h_addr ));

			ok = true;

		} else
			((struct in_addr *)addr )->s_addr = INADDR_NONE;

	} else {
#if HAVE_IPV6
		struct in6_addr	tmpaddr = IN6ADDR_ANY_INIT;
		unsigned int	len;
		struct hostent	*ent;
		int				error_num;

#if HAVE_GETIPNODEBYNAME
		if( ent = getipnodebyname( host, AF_INET6, AI_DEFAULT, &error_num )) {

			memcpy(( char * )&tmpaddr.s6_addr, ent->h_addr, sizeof( tmpaddr.s6_addr ));
			freehostent( ent );

			ok = true;
		}
#else /* !HAVE_GETIPNODEBYNAME */
		struct addrinfo	*res;

		if( getaddrinfo( host, NULL, NULL, &res ) == 0 ) {

			switch( res->ai_family ) {

				case AF_INET:
					// create an IPv6 V4MAPPED address
					memcpy( &((uint32_t *)tmpaddr.s6_addr)[3],
							&((struct sockaddr_in *)res->ai_addr )->sin_addr,
							sizeof( uint32_t ));

					((uint32_t *)&tmpaddr.s6_addr)[2] = htonl( 0xffff );
					break;

				case AF_INET6:
					memcpy( &tmpaddr.s6_addr,
							&((struct sockaddr_in6 *)res->ai_addr )->sin6_addr,
							sizeof( tmpaddr.s6_addr ));
					break;
			}

			freeaddrinfo( res );

			ok = true;
		}
#endif /* HAVE_GETIPNODEBYNAME */

		if( ok )
			memcpy( addr, &tmpaddr, sizeof( tmpaddr ));
#endif /* HAVE_IPV6 */
	}

	return( ok );
}
//---------------------------------------------------------------------------
