/***************************************************************************
                                   ctrlipc.cpp
                             -------------------
    revision             : $Id: ctrlipc.cpp,v 1.2 2002-11-03 17:28:46 tellini Exp $
    copyright            : (C) 2002 by Simone Tellini
    email                : tellini@users.sourceforge.net

    description          : this class handles the administration interface
                           of the server. Basicly, it parses and dispacthes
                           commands received by prometeoctl
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

#include <sys/stat.h>
#include <pwd.h>

#include "ctrlipc.h"
#include "loader.h"
#include "registry.h"
#include "acl.h"

static void SocketCB( SOCKREF sock, Prom_SC_Reason reason, int data, void *userdata );

//--------------------------------------------------------------------------
CtrlIPC::CtrlIPC()
{
	if( IsValid() ) {

		UseDispatcher( App->IO );
		SetAsyncCallback( SocketCB, this );

		if( Bind( PROM_CTRL_SOCKNAME )) {

			chmod( PROM_CTRL_SOCKNAME, S_IRWXU | S_IRWXG | S_IRWXO );

			if( !Listen( 1 ))
				App->Log->Log( LOG_ERR,
							   "IPC: coulnd't listen on "PROM_CTRL_SOCKNAME" - %s",
							   strerror( errno ));

		} else
			App->Log->Log( LOG_ERR,
						   "IPC: couldn't bind the socket to "PROM_CTRL_SOCKNAME" - %s",
						   strerror( errno ));

	} else
		App->Log->Log( LOG_ERR,
					   "IPC: couldn't create a unix domain socket - %s",
					   strerror( errno ));
}
//--------------------------------------------------------------------------
CtrlIPC::~CtrlIPC()
{
}
//--------------------------------------------------------------------------
void CtrlIPC::Cleanup( void )
{
	unlink( PROM_CTRL_SOCKNAME );
}
//--------------------------------------------------------------------------
void CtrlIPC::SocketEvent( Socket *sock, Prom_SC_Reason reason, int data )
{
	switch( reason ) {

		case PROM_SOCK_ACCEPT:
			NewConnection((UnixSocket *)data );
			break;
	}
}
//--------------------------------------------------------------------------
static void SocketCB( SOCKREF sock, Prom_SC_Reason reason, int data, void *userdata )
{
	((CtrlIPC *)userdata )->SocketEvent( sock, reason, data );
}
//---------------------------------------------------------------------------
void CtrlIPC::NewConnection( UnixSocket *sock )
{
	if( sock->IsValid() ) {
		mode_t	mode = sock->GetPeerPerms();

		App->Log->Log( LOG_INFO, "IPC: connection from %s, uid %d", sock->GetPeerName(), sock->GetPeerUID() );
		
		if( mode ) {

#ifdef S_ISSOCK
			if( S_ISSOCK( mode )) {
#endif
			uid_t	uid = sock->GetPeerUID();

			if( CheckAuth( uid, "admin/prometeoctl/connect" ))
				HandleRequest( sock );
			else {

				sock->Printf( "Sorry, you don't have the required authorization.\n" );
						
				App->Log->Log( LOG_ALERT,
							   "IPC: unauthorized connection attempted from uid %d",
							   uid );
			}

#ifdef S_ISSOCK
			} else
				App->Log->Log( LOG_ERR,
							   "IPC: %s doesn't appear to be a socket",
							   sock->GetPeerName() );
#endif

		} else
			App->Log->Log( LOG_ERR,
						   "IPC: stat( %s ) failed - %s",
						   sock->GetPeerName(), strerror( errno ));

	} else
		App->Log->Log( LOG_ERR,
					   "IPC: failed to accept incoming connection - %s",
					   strerror( errno ));

	delete sock;
}
//--------------------------------------------------------------------------
bool CtrlIPC::CheckAuth( int uid, char *auth )
{
	bool	ok = ( uid == 0 ) ||		// root is always ok
				 ( uid == getuid() );	// as is the user which ran us

	if( !ok ) {
		struct passwd	*pw;

		pw = getpwuid( uid );

		if( pw )
			ok = App->ACL->UserPermission( pw->pw_name, auth );
	}

	return( ok );
}
//--------------------------------------------------------------------------
void CtrlIPC::HandleRequest( UnixSocket *sock )
{
	char	req[ PROM_IPC_MAX_REQ_LEN ];

	if( ReadLine( sock, req, sizeof( req ))) {
		char	*args;

		args = strchr( req, ' ' );

		if( args )
			*args++ = '\0';

		if( !strcmp( req, "stop" )) {

			sock->Printf( "Quitting...\n" );
			App->Quit();

		} else if( !strcmp( req, "load" ))
			CmdLoad( sock, args );

		else if( !strcmp( req, "unload" ))
			CmdUnload( sock, args );

		else if( !strcmp( req, "lsmod" ))
			CmdLsMod( sock );

		else if( !strcmp( req, "help" ))
			CmdHelp( sock );

		else
			sock->Printf( "Unknown command. See 'prometeoctl help' for more info.\n" );

	} else
		App->Log->Log( LOG_ERR, "IPC: invalid request from uid %d", sock->GetPeerUID() );
}
//--------------------------------------------------------------------------
bool CtrlIPC::ReadLine( UnixSocket *sock, char *req, int maxlen )
{
	int		reqlen = 0;
	bool	done = false, ok = true;

	do {
		int len = sock->Recv( &req[ reqlen ], maxlen - reqlen );

		if( len > 0 ) {

			reqlen += len;

			if( req[ reqlen - 1 ] == '\0' )
				done = true;

		} else
			ok = false;

		// getting too many data?
		if(( reqlen >= maxlen ) && !done )
			ok = false;

	} while( ok && !done );

	return( ok );
}
//--------------------------------------------------------------------------
void CtrlIPC::CmdLoad( UnixSocket *sock, char *req )
{
	if( req ) {

		if( App->Mods->Load( req, true ))
			sock->Printf( "module %s has been loaded.\n", req );
		else
			sock->Printf( "cannot load %s! Make sure it has been defined in the configuration file.\n", req );

	} else
		sock->Printf( "missing argument\n" );
}
//--------------------------------------------------------------------------
void CtrlIPC::CmdUnload( UnixSocket *sock, char *req )
{
	if( req ) {

		App->Mods->Unload( req );

		sock->Printf( "done.\n" );

	} else
		sock->Printf( "missing argument\n" );
}
//--------------------------------------------------------------------------
void CtrlIPC::CmdLsMod( UnixSocket *sock )
{
	sock->Printf( "--------------------------------------------------------\n" 
				  " %-8.8s | %s\n"
				  "--------------------------------------------------------\n",
				  "Status", "Name" );

	if( App->Cfg->OpenKey( "root/Modules", false )) {
		int i = 0;
	
		while( const char *mod = App->Cfg->EnumKeys( i++ ))
			sock->Printf( " %-8.8s | %s\n", 
						  App->Mods->IsRunning( mod ) ? "Enabled" : "Disabled", mod );

		App->Cfg->CloseKey();
	}
}
//--------------------------------------------------------------------------
void CtrlIPC::CmdHelp( UnixSocket *sock )
{
	sock->Printf( "Available commands:\n"
				  "\n"
				  "  load module     - load the module with the specified name\n"
				  "                    (it must have already been defined in the\n"
				  "                    configuration file)\n"
				  "  unload module   - unload the module\n"
				  "  lsmod           - list the configured modules\n"
				  "  stop            - quit prometeo\n"
				  "  help            - guess it?\n" );
}
//--------------------------------------------------------------------------

