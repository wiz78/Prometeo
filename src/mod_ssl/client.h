/***************************************************************************
                                  client.h
                             -------------------
    revision             : $Id: client.h,v 1.2 2003-02-07 14:10:59 tellini Exp $
    copyright            : (C) 2002 by Simone Tellini
    email                : tellini@users.sourceforge.net

    description          : this class handles an SSL tunnel
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
#include "sslctx.h"

class TcpSocket;
class SSLSocket;

class SSLClient : public Process
{
public:
						SSLClient( const string& key );
	virtual				~SSLClient();

	virtual void		OnFork( void );
	virtual void		ReloadCfg( void );

	void				Serve( TcpSocket *sock, bool forked );
	void				SocketEvent( SOCKREF sock, Prom_SC_Reason reason, int data );

private:
	string				CfgKey;
	TcpSocket			*User;
	SSLSocket			*Server;
	SSLCtx				*ClientCtx;
	string				TargetHost;
	int					TargetPort;

	void				Setup();
	void				Cleanup();

	virtual void		WaitRequest( void );

	void				ForwardData( TcpSocket *sock, int len );
	void				HandleError( TcpSocket *sock, int err );

	virtual void		Dispatch( void );
};

#endif
