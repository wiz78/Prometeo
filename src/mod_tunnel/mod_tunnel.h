/***************************************************************************
                                mod_tunnel.h
                             -------------------
	revision             : $Id: mod_tunnel.h,v 1.1.1.1 2002-10-10 09:59:55 tellini Exp $
    copyright            : (C) 2002 by Simone Tellini
    email                : tellini@users.sourceforge.net

	description          : a very simple module that forwards packets to/from
                           a defined addres
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef MOD_TUNNEL_H
#define MOD_TUNNEL_H

#include "tcpsocket.h"
#include "linkedlist.h"

#include <string>

class TunnelData;

class Tunnel
{
public:
				Tunnel( const char *key );
				~Tunnel();

	bool		Cleanup();
	void		ReloadCfg( void );
	void		OnFork( void );

	void		SocketEvent( Socket *sock, Prom_SC_Reason reason, int data );
	void		Resolved( TunnelData *data, int addrlen );

private:
	string		Key;
	string		TargetHost;
	short		TargetPort;
	short		SrcPort;
	TcpSocket	*ListeningSocket;
	LinkedList	Sockets;

	void		Setup( void );

	void		Accept( TcpSocket *sock );
	void		Error( TunnelData *data, int err );
	void		Connected( TunnelData *data );
	void		Forward( TunnelData *data, int len, Socket *sock );
};

class TunnelData : public LinkedListNode
{
public:
	Tunnel		*TunnelObj;
	TcpSocket	*Src;
	TcpSocket	*Dest;
	short		DestPort;
	Prom_Addr	Addr;
	char		Buf[ 512 ];
	bool		FreeMe;
	short		State;
};

enum {
	TS_RESOLVING,
	TS_CONNECTING,
	TS_FORWARDING
};

#endif
