/***************************************************************************
                                  client.h
                             -------------------
    revision             : $Id: client.h,v 1.9 2002-11-02 17:19:11 tellini Exp $
    copyright            : (C) 2002 by Simone Tellini
    email                : tellini@users.sourceforge.net

    description          : this class handles an FTP user connection
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

class Client : public Process
{
public:
						Client( const string& key );
						~Client();

	virtual void		OnFork( void );
	virtual void		ReloadCfg( void );

	void				Serve( TcpSocket *sock, bool forked );
	void				SocketEvent( SOCKREF sock, Prom_SC_Reason reason, int data );

private:
	string				CfgKey;
	TcpSocket			*User;
	TcpSocket			*Server;
	TcpSocket			*UserData;
	TcpSocket			*ServerData;
#if USE_SSL
	SSLCtx				*ClientCtx;
	SSLCtx				*ServerCtx;
#endif
	string				Command;
	string				Args;
	string				LastReply;
	BitField			FTPFlags;
	int					PortPort;
	char				PortAddress[ 32 ];

	void				Setup();
	void				Cleanup();

	virtual void		WaitRequest( void );

	bool				RecvCommand( void );
	int					RecvStatus( void );

	void				ForwardReply( void );
	bool				ForwardCmd( void );

	bool				SetupServerDataConnection( void );
	bool				OpenServerDataConnection( void );
	bool				OpenClientDataConnection( void );
	bool				WaitClientDataConnection( void );
	bool				DataConnectionCmd( void );
	void				ForwardData( TcpSocket *sock, int len );
	void				HandleError( TcpSocket *sock, int err );

	virtual void		Dispatch( void );

	void				DispatchCmd( void );

	void				ConnectToServer( void );
	bool				ServerLogin( const string& user );
	bool				AttemptTLSLogin( void );

	void				AcceptUserData( TcpSocket *sock );
	void				AcceptServerData( TcpSocket *sock );

	void				CmdAuth( void );
	void				CmdPass( void );
	void				CmdFeat( void );
	void				CmdPbsz( void );
	void				CmdProt( void );
	void				CmdPasv( void );
	void				CmdEpsv( void );
	void				CmdPort( void );
	void				CmdEprt( void );
	void				CmdAbor( void );
};

#define FTPF_CONNECTED			(1 << 0)
#define FTPF_LOGGED_IN			(1 << 1)
#define FTPF_CLI_DATA_TLS		(1 << 2)	// TLS required for data channel
#define FTPF_PBSZ				(1 << 3)
#define FTPF_SRV_TLS			(1 << 4)
#define FTPF_SRV_DATA_TLS		(1 << 5)
#define FTPF_CLIENT_PASV		(1 << 6)
#define FTPF_CLIENT_PORT		(1 << 7)
#define FTPF_BLOCK_DATA_CONN	(1 << 8)	// EPSV ALL
#define FTPF_SERVER_ACCEPT		(1 << 9)
#define FTPF_CLIENT_ACCEPT		(1 << 10)

#define FTPF_CFG_TRY_TLS		(1 << 31)
#define FTPF_CFG_REQUIRE_TLS	(1 << 30)
#define FTPF_CFG_DATA_TLS		(1 << 29)

#define FTPF_CFG_MASK			( FTPF_CFG_TRY_TLS | FTPF_CFG_REQUIRE_TLS | FTPF_CFG_DATA_TLS )

#endif
