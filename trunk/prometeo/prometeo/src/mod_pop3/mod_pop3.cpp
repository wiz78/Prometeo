/***************************************************************************
                                mod_pop3.cpp
                             -------------------
    revision             : $Id: mod_pop3.cpp,v 1.1 2003-05-24 12:28:53 tellini Exp $
    copyright            : (C) 2003 by Simone Tellini
    email                : tellini@users.sourceforge.net

    description          : POP3 proxy
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

#include "mod_pop3.h"
#include "client.h"

static const char *GetManifest( const char *key, const char *name );
static HANDLE SetupModule( const char *key );
static BOOL CleanupModule( HANDLE mod );
static void CfgChanged( HANDLE mod );
static void OnFork( HANDLE mod );
static void OnTimer( HANDLE mod, time_t now );

namespace mod_pop3 
{
static void SocketCB( SOCKREF sock, Prom_SC_Reason reason, int data, void *userdata );
};

//---------------------------------------------------------------------------
PROM_MODULE =
{
	1,						// API version supported by this module
	"mod_pop3",
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

				"	<Option type=\"integer\" name=\"port\" default=\"8110\">"
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
				
				"	<Option type=\"bool\" name=\"usespamd\" default=\"0\">"
				"		<Label>Filter with SpamAssassin's spamd</Label>"
				"		<Descr>If set, mod_pop3 will pass incoming emails to spamd for filtering.</Descr>"
				"		<Key name=\"" + basekey + "spamd/enabled\"/>"
				"	</Option>"
				
				"	<Option type=\"string\" name=\"spamdhost\" default=\"localhost\">"
				"		<Label>Spamd host</Label>"
				"		<Descr>The address of host running spamd.</Descr>"
				"		<Key name=\"" + basekey + "spamd/host\"/>"
				"	</Option>"
				
				"	<Option type=\"integer\" name=\"spamdport\" default=\"783\">"
				"		<Label>Spamd port</Label>"
				"		<Descr>The port spamd is listening on.</Descr>"
				"		<Key name=\"" + basekey + "spamd/port\"/>"
				"	</Option>"
				
				"	<Option type=\"integer\" name=\"spamdmaxsize\" default=\"256000\">"
				"		<Label>Filter messages up to this size</Label>"
				"		<Descr>Usually spam comes in small messages. To avoid wasting time you can skip filtering big emails.</Descr>"
				"		<Key name=\"" + basekey + "spamd/maxsize\"/>"
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
namespace mod_pop3
{
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
							   "mod_pop3: cannot listen on port %d - %s",
							   Port, strerror( errno ));

		} else
			App->Log->Log( LOG_ERR,
						   "mod_pop3: cannot bind on port %d - %s",
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
}; // namespace
//---------------------------------------------------------------------------

