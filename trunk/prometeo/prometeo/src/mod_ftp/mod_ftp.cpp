/***************************************************************************
                                 mod_ftp.cpp
                             -------------------
    revision             : $Id: mod_ftp.cpp,v 1.2 2002-11-01 22:23:50 tellini Exp $
    copyright            : (C) 2002 by Simone Tellini
    email                : tellini@users.sourceforge.net

    description          : FTP proxy with TLS support
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

#include "mod_ftp.h"
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
	"mod_ftp",
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

				"	<Option type=\"integer\" name=\"port\" default=\"8021\">"
				"		<Label>Port</Label>"
				"		<Descr>The port to listen on.</Descr>"
				"		<Key name=\"" + basekey + "port\"/>"
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
				
				"	<Option type=\"bool\" name=\"trytls\" default=\"1\">"
				"		<Label>Try TLS/SSL auth</Label>"
				"		<Descr>If set, mod_ftp will try to setup a TLS connection to remote sites.</Descr>"
				"		<Key name=\"" + basekey + "trytls\"/>"
				"	</Option>"
				
				"	<Option type=\"bool\" name=\"requiretls\" default=\"0\">"
				"		<Label>Require TLS/SSL</Label>"
				"		<Descr>If set (and if the above switch is set too), you won't be able to connect"
				" to servers which don't support the TLS/SSL authorization method.</Descr>"
				"		<Key name=\"" + basekey + "requiretls\"/>"
				"	</Option>"
				
				"	<Option type=\"bool\" name=\"datatls\" default=\"0\">"
				"		<Label>Secure data channel</Label>"
				"		<Descr>If TLS is in use and this is set, the data channel will be secured. Otherwise"
				" only the control channel will use TLS.</Descr>"
				"		<Key name=\"" + basekey + "datatls\"/>"
				"	</Option>"

				"</Page>";

	return( Manifest.c_str() );
}
//---------------------------------------------------------------------------
static HANDLE SetupModule( const char *key )
{
	return( new Proxy( key ));
}
//---------------------------------------------------------------------------
static BOOL CleanupModule( HANDLE mod )
{
	BOOL ret = TRUE;

	if( mod ) {
		Proxy *cfg = (Proxy *)mod;

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
		((Proxy *)mod )->ReloadCfg();
}
//---------------------------------------------------------------------------
static void OnFork( HANDLE mod )
{
	if( mod )
		((Proxy *)mod )->OnFork();
}
//---------------------------------------------------------------------------
static void OnTimer( HANDLE mod, time_t now )
{
	if( mod )
		((Proxy *)mod )->OnTimer( now );
}
//---------------------------------------------------------------------------
Proxy::Proxy( const char *key ) : Children( key, App->IO )
{
	Key             = key;
	Port            = 8021;
	ListeningSocket = NULL;

	ReloadCfg();
}
//---------------------------------------------------------------------------
void Proxy::ReloadCfg( void )
{
	if( App->Cfg->OpenKey( Key.c_str(), false )) {

		Port = App->Cfg->GetInteger( "port", Port );

		App->Cfg->CloseKey();
	}

	Setup();

	Children.ReloadCfg();
}
//---------------------------------------------------------------------------
void Proxy::Setup( void )
{
	if( !ListeningSocket || ( ListeningSocket->GetLocalPort() != Port )) {

		delete ListeningSocket;

		ListeningSocket = new TcpSocket();

		ListeningSocket->SetAsyncCallback( SocketCB, this );
		ListeningSocket->UseDispatcher( App->IO );

		if( ListeningSocket->Bind( Port )) {

			if( !ListeningSocket->Listen())
				App->Log->Log( LOG_ERR,
							   "mod_ftp: cannot listen on port %d - %s",
							   Port, strerror( errno ));

		} else
			App->Log->Log( LOG_ERR,
						   "mod_ftp: cannot bind on port %d - %s",
						   Port, strerror( errno ));
	}
}
//---------------------------------------------------------------------------
bool Proxy::Cleanup( void )
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
		((Proxy *)userdata )->Accept((TcpSocket *)data );
}
//---------------------------------------------------------------------------
void Proxy::Accept( TcpSocket *sock )
{
	if( sock->IsValid() )
		Children.ServeClient( sock );

	delete sock;
}
//---------------------------------------------------------------------------
void Proxy::OnTimer( time_t now )
{
	Children.Flush( now );
}
//---------------------------------------------------------------------------
void Proxy::OnFork( void )
{
	Children.OnFork();
	Cleanup();
}
//---------------------------------------------------------------------------
