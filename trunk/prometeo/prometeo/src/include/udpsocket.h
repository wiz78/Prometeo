/***************************************************************************
                                udpsocket.h
                             -------------------
	revision             : $Id: udpsocket.h,v 1.1 2003-10-23 17:30:34 tellini Exp $
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

#ifndef PROM_UDPSOCKET_H
#define PROM_UDPSOCKET_H

#include "main.h"

#include <sys/socket.h>
#include <netinet/in.h>

#include "tcpsocket.h"

class UdpSocket : public TcpSocket
{
public:
						UdpSocket();
						UdpSocket( int fd );

	int					SendTo( const void *msg, size_t len, int flags, Prom_Addr *addr );
};

#endif
