/***************************************************************************
                                  client.h
                             -------------------
    revision             : $Id: client.h,v 1.1 2003-10-23 17:27:17 tellini Exp $
    copyright            : (C) 2003 by Simone Tellini
    email                : tellini@users.sourceforge.net

    description          : this class handles a SOCKS5 connection
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef CLIENT_H
#define CLIENT_H

using namespace std;

#include <string>

#include "process.h"
#include "bitfield.h"
#include "socks5.h"

class TcpSocket;

namespace mod_socks5
{

class Client : public Process
{
public:
						Client( const string& key );
	virtual				~Client();

	virtual void		OnFork( void );
	virtual void		ReloadCfg( void );

	void				Serve( TcpSocket *sock, bool forked );
	void				SocketEvent( SOCKREF sock, Prom_SC_Reason reason, int data );

private:
	string				CfgKey;
	TcpSocket			*User;
	TcpSocket			*Server;
	BitField			CFlags;
	enum {
		STATE_METHOD, STATE_AUTH, STATE_CMD, STATE_LISTEN, STATE_FORWARD
	}					State;
	char				CmdBuffer[ 300 ];
	int					CmdLen;

	void				Setup();
	void				Cleanup();

	virtual void		WaitRequest( void );
	virtual	void		Dispatch( void );
	
	void				HandleData( TcpSocket *sock );
	
	void				ReadMethods( void );
	void				ReadRequest( void );
	
	int					GetAddressLen( void );
	void				GetAddress( Prom_Addr *addr, short *port );
	
	void				SendReply( char reply, char atype = SOCKS5_ATYP_IPV4, Prom_Addr *addr = NULL, short port = 0 );

	void				HandleRequest( void );
	void				HandleConnect( void );
	void				HandleBind( void );
	void				HandleAssociate( void );
	void				AcceptConnection( void );

	void				ConnectToServer( const char *host, short port );
	bool				DoConnect( Prom_Addr *addr, short port );

	void				ForwardData( TcpSocket *sock );
};

#define SOCKS5F_CONNECTED		(1 << 0) // client connected

}; // namespace

using namespace mod_socks5;

#endif
