/***************************************************************************
                               connectionmgr.h
                             -------------------
	revision             : $Id: connectionmgr.h,v 1.1.1.1 2002-10-10 09:59:34 tellini Exp $
    copyright            : (C) 2002 by Simone Tellini
    email                : tellini@users.sourceforge.net

	description          : manages connections to HTTP 1.1 servers
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef CONNECTIONMGR_H
#define CONNECTIONMGR_H

#include <sys/types.h>

#include "strhash.h"
#include "linkedlist.h"

class TcpSocket;
class Connection;

class ConnectionMgr
{
public:
				ConnectionMgr() : Hash( 127 ) {};
				~ConnectionMgr();

	void		AddConnection( const char *server, TcpSocket *sock );
	void		RemoveConnection( const char *server, TcpSocket *sock );

	TcpSocket	*FindConnection( const char *server );
	void		ReleaseConnection( const char *server, TcpSocket *sock );

	void		Clear( void );

	void		SocketEvent( TcpSocket *sock, Prom_SC_Reason reason, int data );

private:
	StrHash		Hash;			// to find them quickly
	LinkedList	Connections;	// to check for expiration quickly

	void		SetupCheck( Connection *conn, TcpSocket *sock );
};

#endif
