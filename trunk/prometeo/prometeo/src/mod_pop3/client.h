/***************************************************************************
                                  client.h
                             -------------------
    revision             : $Id: client.h,v 1.1 2003-05-24 12:28:53 tellini Exp $
    copyright            : (C) 2003 by Simone Tellini
    email                : tellini@users.sourceforge.net

    description          : this class handles a POP3 user connection
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

class TcpSocket;

namespace mod_pop3
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
	string				SpamdHost;
	int					SpamdPort;
	int					SpamdMaxSize;
	TcpSocket			*User;
	TcpSocket			*Server;
	BitField			CFlags;
	string				Command;
	string				Args;

	void				Setup();
	void				Cleanup();

	virtual void		WaitRequest( void );

	bool				RecvCommand( void );

	void				ForwardReply( bool multiline );
	void				ForwardCmd( void );

	virtual void		Dispatch( void );

	void				DispatchCmd( void );

	void				ConnectToServer( void );
	bool				DoConnect( Prom_Addr *addr, int port );
	
	void				HandleError( TcpSocket *sock, int err );

	void				FilterEMail( void );
	void				FilterWithSpamd( void );
	bool				CreateTmpDir( string& dir );
};

#define POPF_CONNECTED			(1 << 0)
#define POPF_TRANSPARENT		(1 << 1)

#define POPF_CFG_SPAMD			(1 << 31)

#define POPF_CFG_MASK			( POPF_CFG_SPAMD )

}; // namespace

using namespace mod_pop3;

#endif
