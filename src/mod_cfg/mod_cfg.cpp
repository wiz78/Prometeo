/***************************************************************************
                                 mod_cfg.cpp
                             -------------------
	revision             : $Id: mod_cfg.cpp,v 1.1.1.1 2002-10-10 09:59:27 tellini Exp $
    copyright            : (C) 2002 by Simone Tellini
    email                : tellini@users.sourceforge.net

	description          : configuration editor via web
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

#include "mod_cfg.h"
#include "cfgdata.h"

static const char *GetManifest( const char *key, const char *name );
static HANDLE SetupModule( const char *key );
static BOOL CleanupModule( HANDLE mod );
static void CfgChanged( HANDLE mod );
static void OnFork( HANDLE mod );
static void SocketCB( SOCKREF sock, Prom_SC_Reason reason, int data, void *userdata );

//---------------------------------------------------------------------------
PROM_MODULE =
{
	1,						// API version supported by this module
	"mod_cfg",
	PROM_MT_CUSTOM,

	GetManifest,

	SetupModule,
	CleanupModule,

	CfgChanged,
	OnFork,
	NULL, //OnTimer,
};
//---------------------------------------------------------------------------
static const char *GetManifest( const char *key, const char *name )
{
	static string		Manifest;
	string				basekey = string( key ) + "/", mod = name;

	Manifest =	"<Page name=\"" + mod + "\">"
				"	<Label>" + mod + " options</Label>"

				"	<Option type=\"integer\" name=\"port\" default=\"8081\">"
				"		<Label>Port</Label>"
				"		<Descr>The port to listen on.</Descr>"
				"		<Key name=\"" + basekey + "port\"/>"
				"	</Option>"

				"</Page>";

	return( Manifest.c_str() );
}
//---------------------------------------------------------------------------
static HANDLE SetupModule( const char *key )
{
	return( new CfgEditor( key ));
}
//---------------------------------------------------------------------------
static BOOL CleanupModule( HANDLE mod )
{
	BOOL ret = TRUE;

	if( mod ) {
		CfgEditor *cfg = (CfgEditor *)mod;

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
		((CfgEditor *)mod )->ReloadCfg();
}
//---------------------------------------------------------------------------
static void OnFork( HANDLE mod )
{
	if( mod )
		((CfgEditor *)mod )->Cleanup();
}
//---------------------------------------------------------------------------
CfgEditor::CfgEditor( const char *key )
{
	Key             = key;
	Port            = 8081;
	ListeningSocket = NULL;

	ReloadCfg();
}
//---------------------------------------------------------------------------
void CfgEditor::ReloadCfg( void )
{
	if( App->Cfg->OpenKey( Key.c_str(), false )) {

		Port = App->Cfg->GetInteger( "port", Port );

		App->Cfg->CloseKey();
	}
	
	Setup();
}
//---------------------------------------------------------------------------
void CfgEditor::Setup( void )
{
	if( !ListeningSocket || ( ListeningSocket->GetLocalPort() != Port )) {

		delete ListeningSocket;

		ListeningSocket = new TcpSocket();

		ListeningSocket->SetAsyncCallback( SocketCB, this );
		ListeningSocket->UseDispatcher( App->IO );

		if( ListeningSocket->Bind( Port )) {

			if( !ListeningSocket->Listen())
				App->Log->Log( LOG_ERR,
							   "mod_cfg: cannot listen on port %d - %s",
							   Port, strerror( errno ));

		} else
			App->Log->Log( LOG_ERR,
						   "mod_cfg: cannot bind on port %d - %s",
						   Port, strerror( errno ));
	}
}
//---------------------------------------------------------------------------
bool CfgEditor::Cleanup( void )
{
	while( CfgData *data = (CfgData *)Sockets.RemTail() )
		delete data;

	delete ListeningSocket;
	
	ListeningSocket = NULL;

	return( true );
}
//---------------------------------------------------------------------------
static void SocketCB( SOCKREF sock, Prom_SC_Reason reason, int data, void *userdata )
{
	if( reason == PROM_SOCK_ACCEPT )
		((CfgEditor *)userdata )->Accept((TcpSocket *)data );
}
//---------------------------------------------------------------------------
void CfgEditor::Accept( TcpSocket *sock )
{
	if( sock->IsValid() ) {
		CfgData *data = new CfgData( this, sock );

		Sockets.AddTail( data );

	} else
		delete sock;
}
//---------------------------------------------------------------------------

