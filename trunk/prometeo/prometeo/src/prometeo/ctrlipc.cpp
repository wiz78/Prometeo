/***************************************************************************
                                   ctrlipc.cpp
                             -------------------
    revision             : $Id: ctrlipc.cpp,v 1.4 2002-11-15 16:26:46 tellini Exp $
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
#include <cctype>

#include "ctrlipc.h"
#include "loader.h"
#include "module.h"
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
		uid_t	uid = sock->GetPeerUID();

		App->Log->Log( LOG_INFO, "IPC: connection from %s, uid %d", sock->GetPeerName(), uid );

		if( mode ) {

#ifdef S_ISSOCK
			if( S_ISSOCK( mode )) {
#endif

			if( CheckAuth( uid, "admin/prometeoctl" ))
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
			while( isspace( *args ))
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
bool CtrlIPC::SplitArgs( UnixSocket *sock, char *req, StringList& args )
{
	bool	ret = true, in = false;

	while( *req ) {
		char	delimiter = '\0', *end;

		while( isspace( *req ))
			req++;

		switch( *req ) {

			case '"':
			case '\'':
				delimiter = *req++;
				in        = true;
				break;
		}

		end = req;

		while( *end && ( *end != delimiter ))
			end++;

		if( *end == delimiter )
			in = false;

		*end++ = '\0';

		args.Add( "%s", req );

		req = end;
	}

	if( in ) {
		ret = false;
		sock->Printf( "syntax error\n" );
	}

	return( ret );
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
void CtrlIPC::CmdMod( UnixSocket *sock, char *req )
{
	if( req ) {
		StringList	args;

		if( SplitArgs( sock, req, args )) {
			Module		*mod;

			mod = App->Mods->FindModule( args[0] );

			if( mod ) {
				char	result[ 4096 ];

				mod->ParseCmd( args[1], result, sizeof( result ));

				sock->Send( result, strlen( result ));
				sock->Printf( "\n" );

			} else
				sock->Printf( "module %s isn't running.\n", args[0] );
		}

	} else
		sock->Printf( "missing argument\n" );
}
//--------------------------------------------------------------------------
void CtrlIPC::CmdHelp( UnixSocket *sock )
{
	sock->Printf( PACKAGE" v"VERSION" - (c) by Simone Tellini <tellini@users.sourceforge.net>\n"
				  "\n"
				  "This program is free software; you can redistribute it and/or modify\n"
				  "it under the terms of the GNU General Public License as published by\n"
				  "the Free Software Foundation; either version 2 of the License, or\n"
				  "(at your option) any later version.\n"
				  "\n"
				  "Available commands:\n"
				  "\n"
				  "  load <module>      - load the module with the specified name\n"
				  "                       (it must have already been defined in\n"
				  "                       the configuration file)\n"
				  "  unload <module>    - unload the module\n"
				  "  lsmod              - list the configured modules\n"
				  "  mod <module> <cmd> - send <cmd> to <module>, if it's running\n"
				  "  stop               - quit prometeo\n"
				  "  help               - guess it?\n" );
}
//--------------------------------------------------------------------------

