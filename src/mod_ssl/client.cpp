/***************************************************************************
                                 client.cpp
                             -------------------
    revision             : $Id: client.cpp,v 1.1 2002-11-09 18:25:12 tellini Exp $
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

#include <ctype.h>

#include "main.h"
#include "registry.h"
#include "iodispatcher.h"
#include "unixsocket.h"
#include "tcpsocket.h"
#include "sslsocket.h"
#include "dnscache.h"
#include "stringlist.h"
#include "client.h"

//---------------------------------------------------------------------------
static void SocketCB( SOCKREF sock, Prom_SC_Reason reason, int data, void *userdata )
{
	((SSLClient *)userdata )->SocketEvent( sock, reason, data );
}
//---------------------------------------------------------------------------
SSLClient::SSLClient( const string& key ) : Process()
{
	CfgKey     = key;
	User       = NULL;
	Server     = NULL;
	ClientCtx  = new SSLCtx( SSLCtx::CLIENT );
}
//---------------------------------------------------------------------------
SSLClient::~SSLClient()
{
	Cleanup();

	delete ClientCtx;
}
//---------------------------------------------------------------------------
void SSLClient::OnFork( void )
{
	// avoid killing the process on deletion
	Flags.Clear( PROM_PROCF_RUNNING );

	Cleanup();

	delete Socket;

	Socket = NULL;
}
//---------------------------------------------------------------------------
void SSLClient::Setup( void )
{
	if( App->Cfg->OpenKey( CfgKey.c_str(), false )) {

		TargetHost = App->Cfg->GetString( "dsthost", "" );
		TargetPort = App->Cfg->GetInteger( "dstport", 80 );

		App->Cfg->CloseKey();
	}
}
//---------------------------------------------------------------------------
void SSLClient::Cleanup( void )
{
	delete User;
	delete Server;

	User   = NULL;
	Server = NULL;
}
//---------------------------------------------------------------------------
void SSLClient::ReloadCfg( void )
{
	App->Cfg->Load();
	Setup();
}
//---------------------------------------------------------------------------
void SSLClient::Serve( TcpSocket *sock, bool forked )
{
	// this method is called in the parent context
	int	fdnum = forked ? sock->GetFD() : -1;

	Flags.Set( PROM_PROCF_BUSY );

	Socket->SendFD( sock->GetFD() );
	Socket->Send( &fdnum, sizeof( fdnum ));

	Data.Resize( sizeof( int ));

	DataLen = 0;

	// wait for completion
	Socket->AsyncRecv( Data.GetData(), sizeof( int ));
}
//---------------------------------------------------------------------------
// methods below are called in the child context
void SSLClient::WaitRequest( void )
{
	int		FD = Socket->GetFD();
	fd_set	fds;

	Setup();

	FD_ZERO( &fds );
	FD_SET( FD, &fds );

	Prom_set_ps_display( "idle" );

	// loop until we recv some requests or we get killed
	while( select( FD + 1, &fds, NULL, NULL, NULL ) > 0 ) {
		int num, userfd;

		userfd = Socket->RecvFD();
		Socket->Recv( &num, sizeof( num ));

		// if we have just forked, we've inherited the
		// accepted socket, so we must close it
		if( num != -1 )
			close( num );

		User = new TcpSocket( userfd );

		Dispatch();

		// signal that we're idle
		num = 0;

		Socket->Send( &num, sizeof( num ));

		Prom_set_ps_display( "idle" );
	}
}
//---------------------------------------------------------------------------
void SSLClient::Dispatch( void )
{
	Prom_Addr	addr;

	Prom_set_ps_display( "resolving target host..." );

	if( App->DNS->Resolve( TargetHost.c_str(), &addr )) {

		Server = new SSLSocket( ClientCtx );

		Prom_set_ps_display( "connecting to server..." );

		if( Server->Connect( &addr, TargetPort )) {

			Prom_set_ps_display( "SSL handshake..." );

			if( Server->SSLInitSession( SSLCtx::CLIENT )) {

				User->UseDispatcher( App->IO );
				User->SetAsyncCallback( SocketCB, this );

				Server->UseDispatcher( App->IO );
				Server->SetAsyncCallback( SocketCB, this );

				App->IO->AddFD( User,   PROM_IOF_READ );
				App->IO->AddFD( Server, PROM_IOF_READ );

				Prom_set_ps_display( "tunnelling" );

				do {
					App->IO->WaitEvents();
				} while( User );

			} else
				App->Log->Log( LOG_ERR,
							   "mod_ssl: SSL handshake failed (%s)",
							   Server->SSLGetErrorStr() );

		} else
			App->Log->Log( LOG_ERR, "mod_ssl: couldn't connect to %s:%d",
						   TargetHost.c_str(), TargetPort );

	} else
		App->Log->Log( LOG_ERR, "mod_ssl: couldn't resolve %s", TargetHost.c_str() );

	Cleanup();
}
//---------------------------------------------------------------------------
void SSLClient::SocketEvent( SOCKREF sock, Prom_SC_Reason reason, int data )
{
	switch( reason ) {

		case PROM_SOCK_READ:
			ForwardData((TcpSocket *)sock, data );
			break;

		case PROM_SOCK_TIMEOUT:
		case PROM_SOCK_ERROR:
			HandleError((TcpSocket *)sock, data );
			break;
	}
}
//---------------------------------------------------------------------------
void SSLClient::ForwardData( TcpSocket *sock, int len )
{
	char	DataBuffer[ 4096 ];

	if( sock == Server )
		len = Server->Recv( DataBuffer, sizeof( DataBuffer ));
	else
		len = User->Recv( DataBuffer, sizeof( DataBuffer ));

	if( len <= 0 )
		Cleanup();
	else if( sock == Server )
		User->Send( DataBuffer, len );
	else
		Server->Send( DataBuffer, len );
}
//---------------------------------------------------------------------------
void SSLClient::HandleError( TcpSocket *sock, int err )
{
	Cleanup();
}
//---------------------------------------------------------------------------
