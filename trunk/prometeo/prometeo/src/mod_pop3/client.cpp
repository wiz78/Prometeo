/***************************************************************************
                                 client.cpp
                             -------------------
    revision             : $Id: client.cpp,v 1.2 2003-05-24 12:31:09 tellini Exp $
    copyright            : (C) 2003 by Simone Tellini
    email                : tellini@users.sourceforge.net

    description          : this class handles an user connection
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
#include <ctype.h>

#include "registry.h"
#include "iodispatcher.h"
#include "unixsocket.h"
#include "tcpsocket.h"
#include "dnscache.h"
#include "stringlist.h"
#include "client.h"

#define TIMEOUT_CMD			30
#define TIMEOUT_REPLY		60
#define TIMEOUT_SPAMD		60
#define TIMEOUT_IDLE		120

//---------------------------------------------------------------------------
namespace mod_pop3
{
//---------------------------------------------------------------------------
static void SocketCB( SOCKREF sock, Prom_SC_Reason reason, int data, void *userdata )
{
	((Client *)userdata )->SocketEvent( sock, reason, data );
}
//---------------------------------------------------------------------------
Client::Client( const string& key ) : Process()
{
	CfgKey     = key;
	User       = NULL;
	Server     = NULL;
}
//---------------------------------------------------------------------------
Client::~Client()
{
	Cleanup();
}
//---------------------------------------------------------------------------
void Client::OnFork( void )
{
	// avoid killing the process on deletion
	Flags.Clear( PROM_PROCF_RUNNING );

	Cleanup();

	delete Socket;

	Socket = NULL;
}
//---------------------------------------------------------------------------
void Client::Setup( void )
{
	string key = CfgKey + "/spamd";
		
	if( App->Cfg->OpenKey( key.c_str(), false )) {

		CFlags.Set( POPF_CFG_SPAMD, App->Cfg->GetInteger( "enabled", true ));

		SpamdHost    = App->Cfg->GetString( "host", "localhost" );
		SpamdPort    = App->Cfg->GetInteger( "port", 783 );
		SpamdMaxSize = App->Cfg->GetInteger( "maxsize", 256000 );

		App->Cfg->CloseKey();
	}
}
//---------------------------------------------------------------------------
void Client::Cleanup( void )
{
	delete User;
	delete Server;

	User       = NULL;
	Server     = NULL;
}
//---------------------------------------------------------------------------
void Client::ReloadCfg( void )
{
	App->Cfg->Load();
	Setup();
}
//---------------------------------------------------------------------------
void Client::Serve( TcpSocket *sock, bool forked )
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
void Client::WaitRequest( void )
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

		Prom_set_ps_display( "user's logging in" );

		Dispatch();

		// signal that we're idle
		num = 0;

		Socket->Send( &num, sizeof( num ));

		CFlags.Clear( ~POPF_CFG_MASK );

		Prom_set_ps_display( "idle" );
	}
}
//---------------------------------------------------------------------------
bool Client::RecvCommand( void )
{
	char	buf[512];
	bool	ok;

	ok = User->RecvLine( buf, sizeof( buf ), TIMEOUT_CMD ) > 0;

	if( ok ) {
		char	*ptr = buf;

		// convert the first word to uppercase
		while( *ptr && ( *ptr != ' ' ))
			*ptr++ = toupper( *ptr );

		if( *ptr )
			*ptr++ = '\0';

		Command = buf;
		Args    = ptr;
		
	} else
		CFlags.Clear( POPF_CONNECTED );

	return( ok );
}
//---------------------------------------------------------------------------
void Client::Dispatch( void )
{
	time_t		timeout;
	Prom_Addr	addr;
	short		port;

	User->UseDispatcher( App->IO );
	User->SetAsyncCallback( SocketCB, this );

	App->IO->AddFD( User, PROM_IOF_READ );

	CFlags.Set( POPF_CONNECTED );

	timeout = time( NULL ) + TIMEOUT_IDLE;

	if( User->GetOriginalDest( &addr, &port )) {

		if( DoConnect( &addr, port )) {

			CFlags.Set( POPF_TRANSPARENT );

			ForwardReply( false );
				
				
		} else
			User->Printf( "-ERR Cannot connect to the mail server\r\n" );
	}
	

	do {

		if( !App->IO->WaitEvents( TIMEOUT_IDLE )){

		 	if( time( NULL ) > timeout ) {

				User->Printf( "-ERR Connection idle for %d seconds. Get lost.\r\n",
							  TIMEOUT_IDLE );

				CFlags.Clear( POPF_CONNECTED );
			}

		} else
			timeout = time( NULL ) + TIMEOUT_IDLE;

	} while( CFlags.IsSet( POPF_CONNECTED ));

	Cleanup();
}
//---------------------------------------------------------------------------
void Client::DispatchCmd( void )
{
	if( RecvCommand() ) {

		if( CFlags.IsSet( POPF_CFG_SPAMD ) && ( Command == "RETR" ))
			FilterEMail();
		else
			ForwardCmd();
	}
}
//---------------------------------------------------------------------------
void Client::SocketEvent( SOCKREF sock, Prom_SC_Reason reason, int data )
{
	switch( reason ) {

		case PROM_SOCK_READ:
			if( sock == User )
				DispatchCmd();
			break;

		case PROM_SOCK_TIMEOUT:
		case PROM_SOCK_ERROR:
			HandleError((TcpSocket *)sock, data );
			break;
	}
}
//---------------------------------------------------------------------------
void Client::ConnectToServer( void )
{
	string::size_type	pos;
	string				server, user;
	short				port = 110;
	Prom_Addr			addr;

	pos    = Args.find( "%" );
	server = Args.substr( 0, pos );
	user   = Args.substr( pos + 1 );
	pos    = server.rfind( ":" );

	if( pos != string::npos ) {

		port = atoi( server.substr( pos + 1 ).c_str() );

		server.erase( pos );
	}

	CFlags.Clear( POPF_CONNECTED );

	if( App->DNS->Resolve( server.c_str(), &addr )) {
			
		if( DoConnect( &addr, port )) {
			char	buf[512];
			
			Server->RecvLine( buf, sizeof( buf ), TIMEOUT_REPLY ); // skip the greeting
			Server->Printf( "USER %s\r\n", user.c_str() );
		    ForwardReply( false );
		}
		
	} else
		User->Printf( "-ERR Cannot resolve the requested hostname\r\n" );
}
//---------------------------------------------------------------------------
bool Client::DoConnect( Prom_Addr *addr, int port )
{
	Server = new TcpSocket();

	Prom_set_ps_display( "connecting to server..." );

	if( Server->Connect( addr, port )) {
				
		CFlags.Set( POPF_CONNECTED );
		
		Prom_set_ps_display( "connected" );

	} else
		User->Printf( "-ERR Cannot connect to the requested host\r\n" );

	return( CFlags.IsSet( POPF_CONNECTED ));
}
//---------------------------------------------------------------------------
void Client::ForwardReply( bool multiline )
{
	bool loop;
	int	 line = 0;

	do {
		char	buf[1204];
		int		ret;

		loop = false;
		ret  = Server->RecvLine( buf, sizeof( buf ), TIMEOUT_REPLY );

		if( ret > 0 ) {

			User->Printf( "%s\r\n", buf );
				
			if( multiline )
				loop = ( !line && ( buf[0] == '+' )) ||
					   ( line && (( buf[0] != '.' ) || buf[1] ));

		} else
			User->Printf( "-ERR No response from the remote server\r\n" );

		line++;

	} while( loop );
}
//---------------------------------------------------------------------------
void Client::ForwardCmd( void )
{
	static const char *Multi[] = {
		"LIST", "RETR", "TOP", "UIDL"
	};
#define NUM_CMDS	( sizeof( Multi ) / sizeof( Multi[ 0 ] ))
	bool multiline = false;
		
	Server->Printf( "%s", Command.c_str() );

	if( !Args.empty() )
		Server->Printf( " %s", Args.c_str() );
		
	Server->Send( "\r\n", 2 );

	for( int i = 0; !multiline && ( i < NUM_CMDS ); i++ )
		if( Command == Multi[ i ] )
			multiline = true;
	
	ForwardReply( multiline );
#undef NUM_CMDS
}
//---------------------------------------------------------------------------
void Client::FilterEMail( void )
{
	char	buf[512];

	Server->Printf( "LIST %s\r\n", Args.c_str() );

	if(( Server->RecvLine( buf, sizeof( buf ), TIMEOUT_REPLY ) > 0 ) && ( buf[0] == '+' )) {
		char	*ptr = strchr( buf, ' ' );
		int 	size = SpamdMaxSize + 1;

		if( ptr && ( ptr = strchr( ptr + 1, ' ' )))
			size = atoi( ptr + 1 );
		
		if( size <= SpamdMaxSize ) {

			Server->Printf( "RETR %s\r\n", Args.c_str() );

			if(( Server->RecvLine( buf, sizeof( buf ), TIMEOUT_REPLY ) > 0 ) && ( buf[0] == '+' ))		
				FilterWithSpamd();
			else
				User->Printf( "%s\r\n", buf );

		} else
			ForwardCmd();

	} else {

		App->Log->Log( LOG_ERR, "mod_pop3: cannot LIST message %s, forwarding as-is", Args.c_str() );

		ForwardCmd();
	}
}
//---------------------------------------------------------------------------
bool Client::CreateTmpDir( string& dir )
{
	bool	ret;
	char	path[] = "mod_pop3-XXXXXX";
	
	ret = mkdtemp( path ) != NULL;

	if( ret ) {

		dir = path;

		chmod( path, S_IRWXU | S_IRWXG );
			
	} else
		App->Log->Log( LOG_ERR, 
					   "mod_pop3: cannot created temporary directory \"%s\" (%d: %s)", 
					   path, errno, strerror( errno ));

	return( ret );
}
//---------------------------------------------------------------------------
void Client::FilterWithSpamd( void )
{
	TcpSocket  *sock = new TcpSocket();
	Prom_Addr	addr;

	if( sock->IsValid() && 
		App->DNS->Resolve( SpamdHost.c_str(), &addr ) &&
		sock->Connect( &addr, SpamdPort )) {
		bool loop = true;
		char buf[ 1024 ];

		sock->Printf( "PROCESS SPAMC/1.0\r\n" );

		// forward the message to spamd
		while( loop && ( Server->RecvLine( buf, sizeof( buf ), TIMEOUT_REPLY ) > 0 )) {
	
			if( buf[0] == '.' ) {
								
				if( buf[1] == '\0' )
					loop = false;
				else {
					sock->Send( &buf[1], strlen( &buf[1] ));
					sock->Send( "\r\n", 2 );
				}
						
			} else {
					
				sock->Send( buf, strlen( buf ));
				sock->Send( "\r\n", 2 );
			}
		}

		if( loop )
			User->Printf( "-ERR Connection error while retrieving the message\r\n" );
		else {
			// forward from spamd to the client
			
			sock->Shutdown( 1 ); // send EOF to spamd

			if(( sock->RecvLine( buf, sizeof( buf ), TIMEOUT_SPAMD ) > 0 ) &&
			   !strncmp( buf, "SPAMD/", 6 )) {

				User->Printf( "+OK\r\n" );

				while( sock->RecvLine( buf, sizeof( buf ), TIMEOUT_SPAMD ) > 0 ) {
	
					if( buf[0] == '.' )
						User->Send( ".", 1 );

					User->Send( buf, strlen( buf ));
					User->Send( "\r\n", 2 );
				}

				User->Send( ".\r\n", 3 );

			} else
				User->Printf( "-ERR wrong answer from spamd\r\n" );
		}
			
	} else {
			
		App->Log->Log( LOG_ERR, 
					   "mod_pop3: cannot connect to spamd on %s:%d (%d: %s)", 
					   SpamdHost.c_str(), SpamdPort, errno, strerror( errno ));

		ForwardReply( true );
	}
	
	delete sock;
}
//---------------------------------------------------------------------------
void Client::HandleError( TcpSocket *sock, int err )
{
	if( sock == Server )
		User->Printf( "-ERR The server connection dropped.\r\n" );

	CFlags.Clear( POPF_CONNECTED );
}
//---------------------------------------------------------------------------
}; // namespace
//---------------------------------------------------------------------------
