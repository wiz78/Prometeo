/***************************************************************************
                                 client.cpp
                             -------------------
    revision             : $Id: client.cpp,v 1.2 2002-10-23 17:54:26 tellini Exp $
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

#include <ctype.h>

#include "iodispatcher.h"
#include "unixsocket.h"
#include "tcpsocket.h"
#include "dnscache.h"
#include "client.h"

#define TIMEOUT_CMD		30
#define TIMEOUT_REPLY	60

static const char *ForwardableCmds[] = {
	"ACCT", "CWD", "CDUP",
	"SMNT", "REIN", "TYPE",
	"STRU", "MODE", "ALLO",
	"REST", "RNFR", "RNTO",
	"ABOR", "DELE", "RMD",
	"MKD", "PWD", "SITE",
	"SYST", "STAT", "HELP",
	"NOOP"
};

#define NUM_FWD_CMDS	( sizeof( ForwardableCmds ) / sizeof( ForwardableCmds[0] ))

//---------------------------------------------------------------------------
static void SocketCB( SOCKREF sock, Prom_SC_Reason reason, int data, void *userdata )
{
	((Client *)userdata )->SocketEvent( sock, reason, data );
}
//---------------------------------------------------------------------------
Client::Client() : Process()
{
	User   = NULL;
	Server = NULL;
}
//---------------------------------------------------------------------------
Client::~Client()
{
	delete User;
	delete Server;
	delete UserData;
	delete ServerData;
}
//---------------------------------------------------------------------------
void Client::OnFork( void )
{
	// avoid killing the process on deletion
	Flags.Clear( PROM_PROCF_RUNNING );

	delete Socket;
	delete User;
	delete Server;
	delete UserData;
	delete ServerData;

	Socket     = NULL;
	User       = NULL;
	Server     = NULL;
	UserData   = NULL;
	ServerData = NULL;
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
	}

	return( ok );
}
//---------------------------------------------------------------------------
int Client::RecvStatus( void )
{
	bool	loop, info = false;
	int		status = 421;

	do {
		char	buf[512];

		loop = Server->RecvLine( buf, sizeof( buf ), TIMEOUT_REPLY ) > 4;

		if( loop ) {

			if( info ) {

				if( buf[0] != ' ' ) {
					status = atoi( buf );
					loop   = false;
				}

			} else if( buf[3] == '-' )
				info = true;
			else {
				status = atoi( buf );
				loop   = false;
			}
		}

	} while( loop );

	return( status );
}
//---------------------------------------------------------------------------
void Client::Dispatch( void )
{
	User->Printf( "220 Welcome\r\n" );

	User->UseDispatcher( App->IO );
	User->SetAsyncCallback( SocketCB, this );

	App->IO->AddFD( User, PROM_IOF_READ );

	Flags.Set( FTPF_CONNECTED );

	do {

		App->IO->WaitEvents();

	} while( Flags.IsSet( FTPF_CONNECTED ));

	delete User;
	delete Server;
	delete UserData;
	delete ServerData;

	User       = NULL;
	Server     = NULL;
	UserData   = NULL;
	ServerData = NULL;
}
//---------------------------------------------------------------------------
void Client::DispatchCmd( void )
{
	if( RecvCommand() ) {

		if( Command == "AUTH" ) {

//			TLSLogin();

		} else if( Command == "USER" ) {

			if( Args.find( "@" ) != string::npos )
				ConnectToServer();
			else
				User->Printf( "501 This is a proxy: you must login with user@host[:port] as username\r\n" );

		} else if( Command == "QUIT" ) {

			User->Printf( "221 Goodbye.\r\n" );
			Flags.Clear( FTPF_CONNECTED );

		} else if( Flags.IsSet( FTPF_LOGGED_IN )) {

			if( Command == "FEAT" )
				CmdFeat();
			else if( Command == "PBSZ" )
				CmdPbsz();
			else if( Command == "PROT" )
				CmdProt();
			else if( !ForwardCmd() )
				User->Printf( "502 Command not implemented\r\n" );

		} else
			User->Printf( "530 Please login.\r\n" );
	}
}
//---------------------------------------------------------------------------
void Client::SocketEvent( SOCKREF sock, Prom_SC_Reason reason, int data )
{
	switch( reason ) {

		case PROM_SOCK_READ:
			if( sock == User )
				DispatchCmd();
//			else if( sock == Server )
//				ForwardData();
			break;


		case PROM_SOCK_ACCEPT:
			if( sock == UserData )
				AcceptUserData((TcpSocket *)data );
			else
				AcceptServerData((TcpSocket *)data );
			break;

		case PROM_SOCK_TIMEOUT:
		case PROM_SOCK_ERROR:
//			HandleError( sock, data );
			break;
	}
}
//---------------------------------------------------------------------------
void Client::AcceptUserData( TcpSocket *sock )
{
	delete UserData;

	if( sock->IsValid() ) {

		sock->UseDispatcher( App->IO );
		sock->SetAsyncCallback( SocketCB, this );

		App->IO->AddFD( sock, PROM_IOF_READ );

		UserData = sock;

	} else
		UserData = NULL;
}
//---------------------------------------------------------------------------
void Client::AcceptServerData( TcpSocket *sock )
{
	delete ServerData;

	if( sock->IsValid() ) {

		sock->UseDispatcher( App->IO );
		sock->SetAsyncCallback( SocketCB, this );

		App->IO->AddFD( sock, PROM_IOF_READ );

		ServerData = sock;

	} else {

		// XXX report error to user

		ServerData = NULL;
	}
}
//---------------------------------------------------------------------------
void Client::ConnectToServer( void )
{
	string::size_type	pos;
	string				server, user;
	int					port = 21;
	Prom_Addr			addr;

	pos    = Args.find( "@" );
	user   = Args.substr( 0, pos );
	server = Args.substr( pos + 1 );
	pos    = server.find( ":" );

	if( pos != string::npos ) {

		port = atoi( server.substr( pos + 1 ).c_str() );

		server.erase( pos );
	}

	if( App->DNS->Resolve( server.c_str(), &addr )) {

		Server = new TcpSocket();

		Prom_set_ps_display( "connecting to server..." );

		if( Server->Connect( &addr, sizeof( addr ))) {

			Prom_set_ps_display( "connected" );

			if( !ServerLogin( user ))
				Flags.Clear( FTPF_CONNECTED );

		} else
			User->Printf( "421 Cannot connect to the requested host\r\n" );

	} else
		User->Printf( "421 Cannot resolve the requested hostname\r\n" );
}
//---------------------------------------------------------------------------
bool Client::ServerLogin( const string& user )
{
	bool	ret = false;

	if( Flags.IsSet( FTPF_TLS )) {
		bool	ok = false;

		Server->Printf( "AUTH TLS\r\n" );

		if( RecvStatus() == 234 ) {
		}

		if( !ok )
			User->Printf( "421 Cannot login.\r\n" );

	} else {

		Server->Printf( "USER %s\r\n", user.c_str() );

		ForwardReply();
	}

	return( ret );
}
//---------------------------------------------------------------------------
void Client::ForwardReply( void )
{
	bool loop = false, info = false;

	do {
		char	buf[1204];
		bool	ret;

		ret = Server->RecvLine( buf, sizeof( buf ), TIMEOUT_REPLY ) > 0;

		if( ret ) {

			User->Printf( "%s", buf );

			if( info )
				loop = buf[0] == ' ';
			else
				loop = buf[3] == '-';

		} else
			User->Printf( "421 No response from the remote server\r\n" );

	} while( loop );
}
//---------------------------------------------------------------------------
bool Client::ForwardCmd( void )
{
	bool	ok = false;

	for( int i = 0; !ok && ( i < NUM_FWD_CMDS ); i++ )
		if( Command == ForwardableCmds[ i ] )
			ok = true;

	if( ok ) {

		Server->Printf( "%s %s\r\n", Command.c_str(), Args.c_str() );

		ForwardReply();
	}

	return( ok );
}
//---------------------------------------------------------------------------
void Client::CmdFeat( void )
{
	User->Printf( "211-Extensions supported\r\n"
				  " AUTH TLS\r\n"
				  " PBSZ\r\n"
				  " PROT\r\n"
				  "211 END\r\n" );
}
//---------------------------------------------------------------------------
void Client::CmdPbsz( void )
{
	if( Args == "0" ) {

		Flags.Set( FTPF_PBSZ );
		User->Printf( "200 Yessir.\r\n" );

	} else
		User->Printf( "504 Only PBSZ 0 is accepted.\r\n" );
}
//---------------------------------------------------------------------------
void Client::CmdProt( void )
{
	if( Flags.IsSet( FTPF_PBSZ )) {

		Flags.Set( FTPF_DATA_TLS, toupper( Args.c_str()[0] ) == 'P' );
		Flags.Clear( FTPF_PBSZ );

		User->Printf( "200 Yessir.\r\n" );

	} else
		User->Printf( "503 PBSZ not issued yet.\r\n" );
}
//---------------------------------------------------------------------------
void Client::CmdPasv( void )
{
	UserData = new TcpSocket();

	UserData->UseDispatcher( App->IO );
	UserData->SetAsyncCallback( SocketCB, this );

	if( UserData->IsValid() && UserData->MakeIPv4() &&
		UserData->Bind() && UserData->Listen() ) {
		int		port = UserData->GetLocalPort();
		char	*ip = UserData->GetLocalName(), *ptr;

		ptr = ip;

		while( ptr = strchr( ptr, '.' ))
			*ptr++ = ',';

		User->Printf( "227 Entering passive mode (%s,%d,%d).\r\n",
					  ip, port >> 8, port & 0xff );

		Flags.Set( FTPF_CLIENT_PASV );

	} else {

		User->Printf( "425 Cannot setup the socket for data connection.\r\n" );

		delete UserData;
		UserData = NULL;
	}
}
//---------------------------------------------------------------------------
