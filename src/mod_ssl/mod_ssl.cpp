/***************************************************************************
                                 mod_ssl.cpp
                             -------------------
    revision             : $Id: mod_ssl.cpp,v 1.1 2002-11-09 18:25:12 tellini Exp $
    copyright            : (C) 2002 by Simone Tellini
    email                : tellini@users.sourceforge.net

    description          : simple TCP tunnel with SSL transport
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
#include "tcpsocket.h"

#include "mod_ssl.h"
#include "client.h"

static const char *GetManifest( const char *key, const char *name );
static HANDLE SetupModule( const char *key );
static BOOL CleanupModule( HANDLE mod );
static void CfgChanged( HANDLE mod );
static void OnFork( HANDLE mod );
static void OnTimer( HANDLE mod, time_t now );
static void SocketCB( SOCKREF sock, Prom_SC_Reason reason, int data, void *userdata );

//---------------------------------------------------------------------------
PROM_MODULE =
{
	1,						// API version supported by this module
	"mod_ssl",
	PROM_MT_CUSTOM,

	GetManifest,

	SetupModule,
	CleanupModule,

	CfgChanged,
	OnFork,
	OnTimer,
};
//---------------------------------------------------------------------------
static const char *GetManifest( const char *key, const char *name )
{
	static string		Manifest;
	string				basekey = string( key ) + "/", mod = name;

	Manifest =	"<Page name=\"" + mod + "\">"
				"	<Label>" + mod + " options</Label>"

				"	<Option type=\"integer\" name=\"srcport\" default=\"8639\">"
				"		<Label>Local port</Label>"
				"		<Descr>The port to listen on.</Descr>"
				"		<Key name=\"" + basekey + "srcport\"/>"
				"	</Option>"

				"	<Option type=\"string\" name=\"dsthost\" default=\"\">"
				"		<Label>Target host</Label>"
				"		<Descr>The host to connect to.</Descr>"
				"		<Key name=\"" + basekey + "dsthost\"/>"
				"	</Option>"

				"	<Option type=\"integer\" name=\"dstport\" default=\"80\">"
				"		<Label>Target port</Label>"
				"		<Descr>The port to connect to.</Descr>"
				"		<Key name=\"" + basekey + "dstport\"/>"
				"	</Option>"

				"	<Option type=\"integer\" name=\"minchildren\" default=\"0\">"
				"		<Label>Min children #</Label>"
				"		<Descr>The number of processes to keep ready to serve client requests.</Descr>"
				"		<Key name=\"" + basekey + "minchildren\"/>"
				"	</Option>"

				"	<Option type=\"integer\" name=\"maxchildren\" default=\"20\">"
				"		<Label>Max children #</Label>"
				"		<Descr>The maximum number of clients that can be served at once.</Descr>"
				"		<Key name=\"" + basekey + "maxchildren\"/>"
				"	</Option>"

				"	<Option type=\"integer\" name=\"childrenttl\" default=\"60\">"
				"		<Label>Children TTL</Label>"
				"		<Descr>The lifetime of idle children, in seconds.</Descr>"
				"		<Key name=\"" + basekey + "childrenttl\"/>"
				"	</Option>"

				"</Page>";

	return( Manifest.c_str() );
}
//---------------------------------------------------------------------------
static HANDLE SetupModule( const char *key )
{
	return( new SSLProxy( key ));
}
//---------------------------------------------------------------------------
static BOOL CleanupModule( HANDLE mod )
{
	BOOL ret = TRUE;

	if( mod ) {
		SSLProxy *cfg = (SSLProxy *)mod;

		ret = cfg->Cleanup();

		if( ret )
			delete cfg;
	}

	return( ret );
}
//---------------------------------------------------------------------------
static void CfgChanged( HANDLE mod )
{
	if( mod )
		((SSLProxy *)mod )->ReloadCfg();
}
//---------------------------------------------------------------------------
static void OnFork( HANDLE mod )
{
	if( mod )
		((SSLProxy *)mod )->OnFork();
}
//---------------------------------------------------------------------------
static void OnTimer( HANDLE mod, time_t now )
{
	if( mod )
		((SSLProxy *)mod )->OnTimer( now );
}
//---------------------------------------------------------------------------
SSLProxy::SSLProxy( const char *key ) : Children( key, App->IO )
{
	Key             = key;
	Port            = 8639;
	ListeningSocket = NULL;

	ReloadCfg();
}
//---------------------------------------------------------------------------
void SSLProxy::ReloadCfg( void )
{
	if( App->Cfg->OpenKey( Key.c_str(), false )) {

		Port = App->Cfg->GetInteger( "srcport", Port );

		App->Cfg->CloseKey();
	}

	Setup();

	Children.ReloadCfg();
}
//---------------------------------------------------------------------------
void SSLProxy::Setup( void )
{
	if( !ListeningSocket || ( ListeningSocket->GetLocalPort() != Port )) {

		delete ListeningSocket;

		ListeningSocket = new TcpSocket();

		ListeningSocket->SetAsyncCallback( SocketCB, this );
		ListeningSocket->UseDispatcher( App->IO );

		if( ListeningSocket->Bind( Port )) {

			if( !ListeningSocket->Listen())
				App->Log->Log( LOG_ERR,
							   "mod_ssl: cannot listen on port %d - %s",
							   Port, strerror( errno ));

		} else
			App->Log->Log( LOG_ERR,
						   "mod_ssl: cannot bind on port %d - %s",
						   Port, strerror( errno ));
	}
}
//---------------------------------------------------------------------------
bool SSLProxy::Cleanup( void )
{
	delete ListeningSocket;

	ListeningSocket = NULL;

	// we must wait until all the clients have been satisfied
	return( Children.AreIdle() );
}
//---------------------------------------------------------------------------
static void SocketCB( SOCKREF sock, Prom_SC_Reason reason, int data, void *userdata )
{
	if( reason == PROM_SOCK_ACCEPT )
		((SSLProxy *)userdata )->Accept((TcpSocket *)data );
}
//---------------------------------------------------------------------------
void SSLProxy::Accept( TcpSocket *sock )
{
	if( sock->IsValid() )
		Children.ServeClient( sock );

	delete sock;
}
//---------------------------------------------------------------------------
void SSLProxy::OnTimer( time_t now )
{
	Children.Flush( now );
}
//---------------------------------------------------------------------------
void SSLProxy::OnFork( void )
{
	Children.OnFork();
	Cleanup();
}
//---------------------------------------------------------------------------
