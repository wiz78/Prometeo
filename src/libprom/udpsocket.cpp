/***************************************************************************
                                udpsocket.cpp
                             -------------------
	revision             : $Id: udpsocket.cpp,v 1.1 2003-10-23 17:31:04 tellini Exp $
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
#if HAVE_NETINET_IP_COMPAT_H
#include <netinet/ip_compat.h>
#endif
#if HAVE_NETINET_IP_FIL_COMPAT_H
#include <netinet/ip_fil_compat.h>
#endif
#if HAVE_NETINET_IP_FIL_H
#include <netinet/ip_fil.h>
#endif
#if HAVE_NETINET_IP_NAT_H
#include <netinet/ip_nat.h>
#endif
#if HAVE_LINUX_NETFILTER_IPV4_H
#include <linux/netfilter_ipv4.h>
#endif

#include "udpsocket.h"

//---------------------------------------------------------------------------
UdpSocket::UdpSocket() : TcpSocket( SOCK_DGRAM, IPPROTO_UDP )
{
}
//---------------------------------------------------------------------------
UdpSocket::UdpSocket( int fd ) : TcpSocket( fd )
{
}
//---------------------------------------------------------------------------
int UdpSocket::SendTo( const void *msg, size_t len, int flags, Prom_Addr *addr )
{
	return( sendto( FD, msg, len, int flags, (struct sockaddr *)addr, sizeof( Prom_Addr )));
}
//---------------------------------------------------------------------------

