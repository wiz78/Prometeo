/***************************************************************************
                                mod_tunnel.cpp
                             -------------------
	revision             : $Id: mod_tunnel.cpp,v 1.1.1.1 2002-10-10 09:59:55 tellini Exp $
    copyright            : (C) 2002 by Simone Tellini
    email                : tellini@users.sourceforge.net

	description          : a very simple module that forwards packets to/from
                           a defined address
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
#include "registry.h"
#include "dnscache.h"

#include "mod_tunnel.h"

#define CONNECT_TIMEOUT		5

static HANDLE SetupModule( const char *key );
static BOOL CleanupModule( HANDLE mod );
static const char *GetManifest( const char *key, const char *name );
static void CfgChanged( HANDLE mod );
static void OnFork( HANDLE mod );
static void SocketCB( SOCKREF sock, Prom_SC_Reason reason, int data, void *userdata );
static void UserDataDestructor( void *userdata );
static void DNSCallback( int addrlen, void *userdata );

//---------------------------------------------------------------------------
PROM_MODULE =
{
	1,						// API version supported by this module
	"mod_tunnel",
	PROM_MT_CUSTOM,

	GetManifest,

	SetupModule,
	CleanupModule,

	CfgChanged,
	OnFork,
};
//---------------------------------------------------------------------------
static const char *GetManifest( const char *key, const char *name )
{
	static string		Manifest;
	string				basekey = string( key ) + "/", mod = name;

	Manifest =	"<Page name=\"" + mod + "\">"
				"	<Label>" + mod + " options</Label>"

				"	<Option type=\"integer\" name=\"srcport\">"
				"		<Label>Local port</Label>"
				"		<Descr>The source port of the tunnel</Descr>"
				"		<Key name=\"" + basekey + "srcport\"/>"
				"	</Option>"

				"	<Option type=\"string\" name=\"dsthost\">"
				"		<Label>Remote host</Label>"
				"		<Descr>The destination host of the tunnel</Descr>"
				"		<Key name=\"" + basekey + "dsthost\"/>"
				"	</Option>"

				"	<Option type=\"integer\" name=\"dstport\">"
				"		<Label>Remote port</Label>"
				"		<Descr>The destination port of the tunnel</Descr>"
				"		<Key name=\"" + basekey + "dstport\"/>"
				"	</Option>"

				"</Page>";

	return( Manifest.c_str() );
}
//---------------------------------------------------------------------------
static HANDLE SetupModule( const char *key )
{
	return( new Tunnel( key ));
}
//---------------------------------------------------------------------------
static BOOL CleanupModule( HANDLE mod )
{
	BOOL ret = TRUE;

	if( mod ) {
		Tunnel *tunnel = (Tunnel *)mod;

		ret = tunnel->Cleanup();

		if( ret )
			delete tunnel;
	}

	return( ret );
}
//---------------------------------------------------------------------------
static void CfgChanged( HANDLE mod )
{
	if( mod )
		((Tunnel *)mod )->ReloadCfg();
}
//---------------------------------------------------------------------------
static void OnFork( HANDLE mod )
{
	if( mod )
		((Tunnel *)mod )->OnFork();
}
//---------------------------------------------------------------------------
Tunnel::Tunnel( const char *key )
{
	Key             = key;
	TargetPort      = 0;
	SrcPort         = 0;
	ListeningSocket = NULL;

	ReloadCfg();
}
//---------------------------------------------------------------------------
Tunnel::~Tunnel()
{
	delete ListeningSocket;
}
//---------------------------------------------------------------------------
void Tunnel::ReloadCfg( void )
{
	if( App->Cfg->OpenKey( Key.c_str(), false )) {

		TargetHost = strdup( App->Cfg->GetString( "dsthost", "" ));
		TargetPort = App->Cfg->GetInteger( "dstport", 0 );
		SrcPort    = App->Cfg->GetInteger( "srcport", 0 );

		App->Cfg->CloseKey();
	}

	Setup();
}
//---------------------------------------------------------------------------
void Tunnel::Setup( void )
{
	if( !ListeningSocket || ( ListeningSocket->GetLocalPort() != SrcPort )) {

		delete ListeningSocket;

		ListeningSocket = new TcpSocket();

		ListeningSocket->SetAsyncCallback( SocketCB, this );
		ListeningSocket->UseDispatcher( App->IO );

		if( ListeningSocket->Bind( SrcPort )) {

			if( !ListeningSocket->Listen())
				App->Log->Log( LOG_ERR,
							   "mod_tunnel: cannot listen on port %d - %s",
							   SrcPort, strerror( errno ));

		} else
			App->Log->Log( LOG_ERR,
						   "mod_tunnel: cannot bind on port %d - %s",
						   SrcPort, strerror( errno ));
	}
}
//---------------------------------------------------------------------------
bool Tunnel::Cleanup( void )
{
	while( !Sockets.IsEmpty() ) {
		TunnelData *data = (TunnelData *)Sockets.GetHead();

		// we must make sure that we don't leave potential
		// callbacks around
		if( data->State == TS_RESOLVING )
			App->DNS->CancelRequest( TargetHost.c_str(), DNSCallback, data );

		delete data->Src;
	}

	return( true );
}
//---------------------------------------------------------------------------
void Tunnel::OnFork( void )
{
	delete ListeningSocket;

	ListeningSocket = NULL;

	Cleanup();
}
//---------------------------------------------------------------------------
void Tunnel::SocketEvent( Socket *sock, Prom_SC_Reason reason, int data )
{
	switch( reason ) {

		case PROM_SOCK_ACCEPT:
			Accept((TcpSocket *)data );
			break;

		case PROM_SOCK_ERROR:
		case PROM_SOCK_TIMEOUT:
			Error(( TunnelData * )sock->GetUserData(), data );
			break;

		case PROM_SOCK_READ:
			Forward(( TunnelData * )sock->GetUserData(), data, sock );
			break;

		case PROM_SOCK_CONNECTED:
			if( data )
				Error(( TunnelData * )sock->GetUserData(), data );
			else
				Connected(( TunnelData * )sock->GetUserData() );
			break;
	}
}
//---------------------------------------------------------------------------
static void SocketCB( SOCKREF sock, Prom_SC_Reason reason, int data, void *userdata )
{
	((Tunnel *)userdata )->SocketEvent( sock, reason, data );
}
//---------------------------------------------------------------------------
void Tunnel::Accept( TcpSocket *sock )
{
	if( sock->IsValid() ) {
		TunnelData *data = new TunnelData();

		App->Log->Log( LOG_INFO, "mod_tunnel: incoming connection from %s", sock->GetPeerName() );

		Sockets.AddTail( data );

		data->TunnelObj = this;
		data->Src       = sock;
		data->Dest      = NULL;
		data->DestPort  = TargetPort;
		data->FreeMe	= true;
		data->State     = TS_RESOLVING;

		sock->SetUserData( data, UserDataDestructor );
		sock->UseDispatcher( App->IO );
		sock->SetAsyncCallback( SocketCB, this );

		App->DNS->AsyncResolve( TargetHost.c_str(), &data->Addr, DNSCallback, data );

	} else
		delete sock;
}
//---------------------------------------------------------------------------
static void UserDataDestructor( void *userdata )
{
	TunnelData *data = (TunnelData *)userdata;

	if( data ) {

		data->Unlink();

		if( data->FreeMe )
			delete data->Dest;

		delete data;
	}
}
//---------------------------------------------------------------------------
static void DNSCallback( int addrlen, void *userdata )
{
	TunnelData *data = (TunnelData *)userdata;

	data->TunnelObj->Resolved( data, addrlen );
}
//---------------------------------------------------------------------------
void Tunnel::Resolved( TunnelData *data, int addrlen )
{
	if( addrlen  ) {
		char 		*str = TcpSocket::AddrToName( &data->Addr );
    	TunnelData	*data2 = new TunnelData();

		App->Log->Log( LOG_INFO, "mod_tunnel: connecting to %s", str );

		free( str );

		data->Dest = new TcpSocket();

		data->Dest->SetUserData( data2, UserDataDestructor );

		Sockets.AddTail( data2 );

	    if( data2 && data->Dest->IsValid() ) {

			data2->TunnelObj = this;
			data2->Src       = data->Src;
			data2->Dest      = data->Dest;
			data2->DestPort  = data->DestPort;
			data2->FreeMe    = false;
			data2->State     = TS_CONNECTING;
			data->State      = TS_CONNECTING;

			data->Dest->UseDispatcher( App->IO );
			data->Dest->SetAsyncCallback( SocketCB, data->TunnelObj );

			data->Dest->AsyncConnect( &data->Addr, data->DestPort, CONNECT_TIMEOUT );

		} else
			delete data->Src;

	} else
		delete data->Src;
}
//---------------------------------------------------------------------------
void Tunnel::Connected( TunnelData *data )
{
	App->Log->Log( LOG_INFO, "mod_tunnel: forwarding %s:%d <-> %s:%d",
				   data->Src->GetPeerName(), SrcPort, data->Dest->GetPeerName(), data->DestPort );

	data->State = TS_FORWARDING;

	data->Dest->AsyncRecv( data->Buf, sizeof( data->Buf ));

	// now fetch the userdata of the other socket as we need
	// the corresponding buffer
	data        = (TunnelData *)data->Src->GetUserData();
	data->State = TS_FORWARDING;

	data->Src->AsyncRecv( data->Buf, sizeof( data->Buf ));
}
//---------------------------------------------------------------------------
void Tunnel::Error( TunnelData *data, int err )
{
	App->Log->Log( LOG_INFO, "mod_tunnel: socket error while tunnelling from %s - %s",
				   data->Src->GetPeerName(),  err ? strerror( err ) : "timeout" );

	delete data->Src;
}
//---------------------------------------------------------------------------
void Tunnel::Forward( TunnelData *data, int len, Socket *sock )
{
	if( len == 0 ) {

		App->Log->Log( LOG_INFO, "mod_tunnel: closing tunnel %s:%d <-> %s:%d",
					   data->Src->GetPeerName(), SrcPort, data->Dest->GetPeerName(), data->DestPort );

		delete data->Src;

	} else if( sock == data->Src ) {

		data->Dest->AsyncSend( data->Buf, len );
		data->Src->AsyncRecv( data->Buf, sizeof( data->Buf ));

	} else {

		data->Src->AsyncSend( data->Buf, len );
		data->Dest->AsyncRecv( data->Buf, sizeof( data->Buf ));
	}
}
//---------------------------------------------------------------------------
