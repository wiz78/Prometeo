/***************************************************************************
                                tcpsocket.cpp
                             -------------------
	revision             : $Id: tcpsocket.cpp,v 1.2 2002-11-13 15:42:12 tellini Exp $
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
#if defined(HAVE_NETINET_IP_COMPAT_H)
#include <netinet/ip_compat.h>
#endif
#if defined(HAVE_NETINET_IP_FIL_COMPAT_H)
#include <netinet/ip_fil_compat.h>
#endif
#if defined(HAVE_NETINET_IP_FIL_H)
#include <netinet/ip_fil.h>
#endif
#if defined(HAVE_NETINET_IP_NAT_H)
#include <netinet/ip_nat.h>
#endif
#if defined(HAVE_LINUX_NETFILTER_IPV4_H)
#include <linux/netfilter_ipv4.h>
#endif

#include "tcpsocket.h"

#ifdef HAVE_IPV6
#define PF	PF_INET6
#else
#define PF	PF_INET
#endif

//---------------------------------------------------------------------------
TcpSocket::TcpSocket() : Socket( PF, SOCK_STREAM, IPPROTO_TCP )
{
#if HAVE_IPV6
	Family = AF_INET6;
#endif
}
//---------------------------------------------------------------------------
TcpSocket::TcpSocket( int fd ) : Socket( fd )
{
#if HAVE_IPV6
	socklen_t	len = sizeof( Family );

	Family = AF_INET6;

	getsockopt( FD, IPPROTO_IPV6, IPV6_ADDRFORM, &Family, &len );
#endif
}
//---------------------------------------------------------------------------
#if HAVE_IPV6
bool TcpSocket::MakeIPv4( void )
{
	bool		ok = false;
	int			fam = AF_INET;

	if( !setsockopt( FD, IPPROTO_IPV6, IPV6_ADDRFORM, &fam, sizeof( fam ))) {
		Family = AF_INET;
		ok = true;
	}

	return( ok );
}
#endif
//---------------------------------------------------------------------------
bool TcpSocket::Bind( unsigned short port )
{
	bool	ok;

#if HAVE_IPV6
	if( Family == AF_INET6 ) {
		struct sockaddr_in6 sockadr;

		memset( &sockadr, 0, sizeof( sockadr ));

		sockadr.sin6_family   = AF_INET6;
		sockadr.sin6_port     = htons( port );
		sockadr.sin6_addr     = in6addr_any;

		ok = bind( FD, (struct sockaddr *)&sockadr, sizeof( sockadr )) >= 0;

	} else
#endif
	{
		struct sockaddr_in sockadr;

		memset( &sockadr, 0, sizeof( sockadr ));

		sockadr.sin_family = AF_INET;
		sockadr.sin_port   = htons( port );

		ok = bind( FD, (struct sockaddr *)&sockadr, sizeof( sockadr )) >= 0;
	}

	SetLinger( false );

	return( ok );
}
//---------------------------------------------------------------------------
Socket *TcpSocket::Accept( void )
{
	int	sock;

#if HAVE_IPV6
	if( Family == AF_INET6 ) {
		struct sockaddr_in6	addr;
		socklen_t			len = sizeof( addr );

		sock = accept( FD, (struct sockaddr *)&addr, &len );

	} else
#endif
	{
		struct sockaddr_in	addr;
		socklen_t			len = sizeof( addr );

		sock = accept( FD, (struct sockaddr *)&addr, &len );
	}

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

#if HAVE_IPV6
	*addr = (struct sockaddr *)&AddrBuf6;
	*len  = sizeof( AddrBuf6 );

	ret = inet_pton( AF_INET6, name, &AddrBuf6.sin6_addr ) > 0;
#else
	*addr = (struct sockaddr *)&AddrBuf;
	*len  = sizeof( AddrBuf );

	ret = inet_aton( name, &AddrBuf.sin_addr ) != 0;
#endif

	return( ret );
}
//---------------------------------------------------------------------------
bool TcpSocket::NameToAddr( const char *name, Prom_Addr *addr )
{
	bool	ret;

#if HAVE_IPV6
	if( strchr( name, ':' )) {

		ret = inet_pton( AF_INET6, name, addr ) > 0;

	} else {
		// create an IPv6 V4MAPPED address
		struct in_addr	tmp;

		ret = inet_aton( name, &tmp ) != 0;

		if( ret ) {

			memset( addr, 0, sizeof( *addr ));

			memcpy( &((uint32_t *)addr->s6_addr)[3], &tmp, sizeof( uint32_t ));

			((uint32_t *)&addr->s6_addr)[2] = htonl( 0xffff );
		}
	}
#else
	ret = inet_aton( name, addr ) != 0;
#endif

	return( ret );
}
//---------------------------------------------------------------------------
char *TcpSocket::AddrToName( Prom_Addr *addr )
{
	char *name;

#if HAVE_IPV6
	if( name = (char *)malloc( INET6_ADDRSTRLEN )) {

		if( !inet_ntop( AF_INET6, addr, name, INET6_ADDRSTRLEN ))
			strcpy( name, "***ERROR***" );

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
	bool	ok = false;

#if HAVE_IPV6
	if( Family == AF_INET6 ) {
		socklen_t	len = sizeof( AddrBuf6 );

		if(( getpeername( FD, (struct sockaddr *)&AddrBuf6, &len ) == 0 ) &&
		   inet_ntop( AF_INET6, &AddrBuf6.sin6_addr, NameBuf, sizeof( NameBuf )))
			ok = true;

	} else
#endif
	{
		socklen_t	len = sizeof( AddrBuf );

		ok = getpeername( FD, (struct sockaddr *)&AddrBuf, &len ) == 0;

		if( ok )
			strcpy( NameBuf, inet_ntoa( AddrBuf.sin_addr ));
	}

	if( !ok )
		strcpy( NameBuf, "ERROR" );

	return( NameBuf );
}
//---------------------------------------------------------------------------
char *TcpSocket::GetLocalName( void )
{
	bool	ok = false;

#if HAVE_IPV6
	if( Family == AF_INET6 ) {
		socklen_t	len = sizeof( AddrBuf6 );

		if(( getsockname( FD, (struct sockaddr *)&AddrBuf6, &len ) == 0 ) &&
			inet_ntop( AF_INET6, &AddrBuf6.sin6_addr, NameBuf, sizeof( NameBuf )))
			ok = true;

	} else
#endif
	{
		socklen_t	len = sizeof( AddrBuf );

		ok = getsockname( FD, (struct sockaddr *)&AddrBuf, &len ) == 0;

		if( ok )
			strcpy( NameBuf, inet_ntoa( AddrBuf.sin_addr ));
	}

	if( !ok )
		strcpy( NameBuf, "ERROR" );

	return( NameBuf );
}
//---------------------------------------------------------------------------
int TcpSocket::GetPeerPort( void )
{
	int	port = 0;

#if HAVE_IPV6
	if( Family == AF_INET6 ) {
		socklen_t	len = sizeof( AddrBuf6 );

		if( getpeername( FD, (struct sockaddr *)&AddrBuf6, &len ) == 0 )
			port = ntohs( AddrBuf6.sin6_port );

	} else
#endif
	{
		socklen_t	len = sizeof( AddrBuf );

		if( getpeername( FD, (struct sockaddr *)&AddrBuf, &len ) == 0 )
			port = ntohs( AddrBuf.sin_port );
	}

	return( port );
}
//---------------------------------------------------------------------------
int TcpSocket::GetLocalPort( void )
{
	int	port = 0;

#if HAVE_IPV6
	if( Family == AF_INET6 ) {
		socklen_t	len = sizeof( AddrBuf6 );

		if( getsockname( FD, (struct sockaddr *)&AddrBuf6, &len ) == 0 )
			port = ntohs( AddrBuf6.sin6_port );

	} else
#endif
	{
		socklen_t	len = sizeof( AddrBuf );

		if( getsockname( FD, (struct sockaddr *)&AddrBuf, &len ) == 0 )
			port = ntohs( AddrBuf.sin_port );
	}

	return( port );
}
//---------------------------------------------------------------------------
bool TcpSocket::GetOriginalDest( Prom_Addr *addr, short *port )
{
	bool	ret = false;

#if !HAVE_IPV6

	struct sockaddr_in name;
	socklen_t          len = sizeof( name );

	if( getsockname( FD, (struct sockaddr *)&name, &len ) == 0 ) {

#if defined(HAVE_LINUX_NETFILTER_IPV4_H) && defined(SO_ORIGINAL_DST)
		struct sockaddr_in	dest;

		// Linux netfilter
		len = sizeof( dest );

		ret = getsockopt( FD, SOL_IP, SO_ORIGINAL_DST, &dest, &len ) == 0;

		// loop?
		if(( name.sin_port == dest.sin_port ) &&
		( name.sin_addr.s_addr == dest.sin_addr.s_addr ))
			ret = false;

		*((struct in_addr *)addr) = dest.sin_addr;
		*port 					  = dest.sin_port;

#elif defined(HAVE_NETINET_IP_NAT_H) && defined(SIOCGNATL)
		int 		natfd;
		natlookup_t	natlook, *nlptr = &natlook;

		// BSD ipnat table lookup
		nat_fd = open( IPL_NAT, O_RDONLY, 0 );

		if( natfd >= 0 ) {
			struct sockaddr_in	peer;
			int					rc;

			len = sizeof( peer );

			getpeername( FD, (struct sockaddr *)&peer, &len );

			memset( &natlook, 0, sizeof( natlook ));

			natlook.nl_flags        = IPN_TCP;
			natlook.nl_inip.s_addr  = name.sin_addr.s_addr;
			natlook.nl_inport       = name.sin_port;
			natlook.nl_outip.s_addr = peer.sin_addr.s_addr;
			natlook.nl_outport      = peer.sin_port;

			// handle versions differences...
			rc = 0;

			if( 63 == ( SIOCGNATL & 0xff ))
				rc = ioctl( nat_fd, SIOCGNATL, &nlptr );
			else
				rc = ioctl( nat_fd, SIOCGNATL, &natlook );

			close( nat_fd );

			ret = rc >= 0;

			if( ret ) {

				// loop?
				if(( name.sin_port == natlook.nl_realport ) &&
				( name.sin_addr.s_addr == natlook.nl_realip.s_addr ))
					ret = false;

				if( ret ) {

					*((struct in_addr *)addr) = natlook.nl_realip;
					*port                     = natlook.nl_realport;
				}
			}
		}

#else /* !BSD-IPNAT */

		// IP-Chains uses getsockname, as "transparent address"

		ret = true;

		*((struct in_addr *)addr) = name.sin_addr;
		*port                     = name.sin_port;
#endif
	}

#endif /* HAVE_IPV6 */

	return( ret );
}
//---------------------------------------------------------------------------
