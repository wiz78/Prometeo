/***************************************************************************
                                 client.cpp
                             -------------------
    revision             : $Id: client.cpp,v 1.8 2002-11-02 17:19:11 tellini Exp $
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

#include "main.h"
#include "registry.h"
#include "iodispatcher.h"
#include "unixsocket.h"
#include "tcpsocket.h"
#include "sslsocket.h"
#include "dnscache.h"
#include "stringlist.h"
#include "client.h"

#define TIMEOUT_CMD			30
#define TIMEOUT_REPLY		60
#define TIMEOUT_IDLE		120
#define TIMEOUT_DATA_CONN	60

#define MAX_REPLY_SIZE	(1024 * 1024)

static const char *ForwardableCmds[] = {
	"ACCT", "CWD", "CDUP",
	"SMNT", "REIN", "TYPE",
	"STRU", "MODE", "ALLO",
	"REST", "RNFR", "RNTO",
	"DELE", "RMD", "PASS",
	"MKD", "PWD", "SITE",
	"SYST", "STAT", "NOOP",
};

#define NUM_FWD_CMDS	( sizeof( ForwardableCmds ) / sizeof( ForwardableCmds[0] ))

enum { TO_CLIENT, TO_SERVER };

static struct {
	const char *cmd;
	int			dir;
} DataCmds[] = {
	{ "LIST", TO_CLIENT },
	{ "NLST", TO_CLIENT },
	{ "RETR", TO_CLIENT },
	{ "STOR", TO_SERVER },
	{ "APPE", TO_SERVER },
	{ "STOU", TO_SERVER },
};

#define NUM_DATA_CMDS	( sizeof( DataCmds ) / sizeof( DataCmds[0] ))

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
	UserData   = NULL;
	ServerData = NULL;
#if USE_SSL
	ClientCtx  = new SSLCtx( SSLCtx::CLIENT );
	ServerCtx  = new SSLCtx( SSLCtx::SERVER );
#endif

	FTPFlags.Set( FTPF_CFG_TRY_TLS );
}
//---------------------------------------------------------------------------
Client::~Client()
{
	Cleanup();

#if USE_SSL
	delete ClientCtx;
	delete ServerCtx;
#endif
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
	if( App->Cfg->OpenKey( CfgKey.c_str(), false )) {

		FTPFlags.Set( FTPF_CFG_TRY_TLS,     App->Cfg->GetInteger( "trytls", true ));
		FTPFlags.Set( FTPF_CFG_REQUIRE_TLS, App->Cfg->GetInteger( "requiretls", false ));
		FTPFlags.Set( FTPF_CFG_DATA_TLS,    App->Cfg->GetInteger( "datatls", false ));

		App->Cfg->CloseKey();
	}
}
//---------------------------------------------------------------------------
void Client::Cleanup( void )
{
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

		FTPFlags.Clear( ~FTPF_CFG_MASK );

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
		FTPFlags.Clear( FTPF_CONNECTED );

	return( ok );
}
//---------------------------------------------------------------------------
int Client::RecvStatus( void )
{
	bool	loop, info = false;
	int		status = 221;

	LastReply.erase();

	do {
		char	buf[512];
		int		len;

		len  = Server->RecvLine( buf, sizeof( buf ), TIMEOUT_REPLY );
		loop = len > 0;

		if( loop ) {

			LastReply += buf;
			LastReply += "\r\n";

			if( info ) {

				if( buf[0] != ' ' ) {
					status = atoi( buf );
					loop   = false;
				}

			} else if(( len >= 4 ) && ( buf[3] == '-' ))
				info = true;
			else {
				status = atoi( buf );
				loop   = false;
			}

		} else
			status = 221;

	} while( loop && ( LastReply.length() <= MAX_REPLY_SIZE ));

	return( status );
}
//---------------------------------------------------------------------------
void Client::Dispatch( void )
{
	time_t	timeout;

	User->Printf( "220 Welcome\r\n" );

	User->UseDispatcher( App->IO );
	User->SetAsyncCallback( SocketCB, this );

	App->IO->AddFD( User, PROM_IOF_READ );

	FTPFlags.Set( FTPF_CONNECTED );

	timeout = time( NULL ) + TIMEOUT_IDLE;

	do {

		if( !App->IO->WaitEvents( TIMEOUT_IDLE )){

		 	if( time( NULL ) > timeout ) {

				User->Printf( "221 Connection idle for %d seconds. Get lost.\r\n",
							  TIMEOUT_IDLE );

				FTPFlags.Clear( FTPF_CONNECTED );
			}

		} else
			timeout = time( NULL ) + TIMEOUT_IDLE;

	} while( FTPFlags.IsSet( FTPF_CONNECTED ));

	Cleanup();
}
//---------------------------------------------------------------------------
void Client::DispatchCmd( void )
{
	if( RecvCommand() ) {

		if( Command == "AUTH" ) {

			CmdAuth();

		} else if( Command == "USER" ) {

			if( Args.find( "@" ) != string::npos )
				ConnectToServer();
			else
				User->Printf( "501 This is a proxy: you must login with user@host[:port] as username\r\n" );

		} else if( Command == "PASS" ) {

			CmdPass();

		} else if( Command == "QUIT" ) {

			User->Printf( "221 Goodbye.\r\n" );
			FTPFlags.Clear( FTPF_CONNECTED );

		} else if( FTPFlags.IsSet( FTPF_LOGGED_IN )) {

			if( Command == "FEAT" )
				CmdFeat();
#if USE_SSL
			else if( Command == "PBSZ" )
				CmdPbsz();
			else if( Command == "PROT" )
				CmdProt();
#endif
			else if( Command == "PASV" )
				CmdPasv();
			else if( Command == "EPSV" )
				CmdEpsv();
			else if( Command == "PORT" )
				CmdPort();
			else if( Command == "EPRT" )
				CmdEprt();
			else if( Command == "ABOR" )
				CmdAbor();
			else if( !ForwardCmd() && !DataConnectionCmd() )
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
			else if(( sock == ServerData ) || ( sock == UserData ))
				ForwardData((TcpSocket *)sock, data );
			break;


		case PROM_SOCK_ACCEPT:
			if( sock == UserData )
				AcceptUserData((TcpSocket *)data );
			else
				AcceptServerData((TcpSocket *)data );
			break;

		case PROM_SOCK_TIMEOUT:
		case PROM_SOCK_ERROR:
			HandleError((TcpSocket *)sock, data );
			break;
	}
}
//---------------------------------------------------------------------------
void Client::AcceptUserData( TcpSocket *sock )
{
	FTPFlags.Set( FTPF_CLIENT_ACCEPT );

	delete UserData;

#if USE_SSL
	if( sock->IsValid() && FTPFlags.IsSet( FTPF_CLI_DATA_TLS )) {
		SSLSocket	*ssl = new SSLSocket( ServerCtx, sock );

		sock->SetFD( -1 );

		if( ssl->SSLInitSession( SSLCtx::SERVER )) {

			delete sock;

			sock = ssl;

		} else
			delete ssl;
	}
#endif

	if( sock->IsValid() ) {

		sock->UseDispatcher( App->IO );
		sock->SetAsyncCallback( SocketCB, this );

		UserData = sock;

		App->IO->AddFD( UserData, PROM_IOF_READ );

	} else {

		delete sock;

		UserData = NULL;
	}
}
//---------------------------------------------------------------------------
void Client::AcceptServerData( TcpSocket *sock )
{
	FTPFlags.Set( FTPF_SERVER_ACCEPT );

	delete ServerData;

#if USE_SSL
	if( sock->IsValid() && FTPFlags.IsSet( FTPF_SRV_DATA_TLS )) {
		SSLSocket	*ssl = new SSLSocket( ClientCtx, sock );

		sock->SetFD( -1 );

		if( ssl->SSLInitSession( SSLCtx::CLIENT )) {

			delete sock;

			sock = ssl;

		} else
			delete ssl;
	}
#endif

	if( sock->IsValid() ) {

		sock->UseDispatcher( App->IO );
		sock->SetAsyncCallback( SocketCB, this );

		ServerData = sock;

		App->IO->AddFD( ServerData, PROM_IOF_READ );

	} else {

		delete sock;

		ServerData = NULL;
	}
}
//---------------------------------------------------------------------------
void Client::ConnectToServer( void )
{
	string::size_type	pos;
	string				server, user;
	short				port = 21;
	Prom_Addr			addr;

	pos    = Args.find( "@" );
	user   = Args.substr( 0, pos );
	server = Args.substr( pos + 1 );
	pos    = server.find( ":" );

	if( pos != string::npos ) {

		port = atoi( server.substr( pos + 1 ).c_str() );

		server.erase( pos );
	}

	FTPFlags.Clear( FTPF_CONNECTED );

	if( App->DNS->Resolve( server.c_str(), &addr )) {

		Server = new TcpSocket();

		Prom_set_ps_display( "connecting to server..." );

		if( Server->Connect( &addr, port )) {

			Prom_set_ps_display( "connected" );

			if( ServerLogin( user ))
				FTPFlags.Set( FTPF_CONNECTED );

		} else
			User->Printf( "421 Cannot connect to the requested host\r\n" );

	} else
		User->Printf( "421 Cannot resolve the requested hostname\r\n" );
}
//---------------------------------------------------------------------------
bool Client::ServerLogin( const string& user )
{
	bool	ret = false;

	if( RecvStatus() == 220 ) {

		if( !FTPFlags.IsSet( FTPF_CFG_TRY_TLS ) || AttemptTLSLogin() ) {

			Server->Printf( "USER %s\r\n", user.c_str() );

			ret = true;

			switch( RecvStatus() ) {

				case 331:
					User->Printf( "331 Password required for %s.\r\n", user.c_str() );
					break;

				case 230:
					User->Send( LastReply.c_str(), LastReply.length() );
					FTPFlags.Set( FTPF_LOGGED_IN );
					break;

				default:
					User->Send( LastReply.c_str(), LastReply.length() );
					break;
			}

			ret = true;
		}
	}

	return( ret );
}
//---------------------------------------------------------------------------
bool Client::AttemptTLSLogin( void )
{
	bool	ret = false;

#if USE_SSL
	Server->Printf( "AUTH TLS\r\n" );

	if( RecvStatus() == 234 ) {
		SSLSocket *ssl = new SSLSocket( ClientCtx, Server );

		if( ssl->SSLInitSession( SSLCtx::CLIENT )) {

			// avoid closing the socket
			Server->SetFD( -1 );

			delete Server;

			Server = (TcpSocket *)ssl;
			ret    = true;

			FTPFlags.Set( FTPF_SRV_TLS );

			if( FTPFlags.IsSet( FTPF_CFG_DATA_TLS )) {

				Server->Printf( "PBSZ 0\r\n" );

				if( RecvStatus() == 200 ) {

					Server->Printf( "PROT P\r\n" );

					if( RecvStatus() == 200 )
						FTPFlags.Set( FTPF_SRV_DATA_TLS );
				}
			}

		} else {

			// grab return code from the server
			RecvStatus();

			ssl->SetFD( -1 );

			delete ssl;
		}
	}

	if( !ret ) {

		if( FTPFlags.IsSet( FTPF_CFG_REQUIRE_TLS ))
			User->Printf( "421 Cannot setup TLS connection to the remote server, give up.\r\n" );
		else
			ret = true;
	}
#else

	ret = true;

#endif /* USE_SSL */

	return( ret );
}
//---------------------------------------------------------------------------
void Client::ForwardReply( void )
{
	bool loop = false, info = false;

	do {
		char	buf[1204];
		int		ret;

		ret = Server->RecvLine( buf, sizeof( buf ), TIMEOUT_REPLY ) > 0;

		if( ret > 0 ) {

			User->Printf( "%s\r\n", buf );

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
void Client::CmdAuth( void )
{
#if USE_SSL
	if(( Args == "TLS" ) || ( Args == "TLS-C" ) || ( Args == "TLS-P" ) ||
	   ( Args == "SSL" )) {
		SSLSocket	*ssl;

		User->Printf( "234 AUTH %s accepted.\r\n", Args.c_str() );

		ssl = new SSLSocket( ServerCtx, User );

		if( ssl->SSLInitSession( SSLCtx::SERVER )) {

			// avoid closing the socket
			User->SetFD( -1 );

			delete User;

			User = (TcpSocket *)ssl;

		} else {

			User->Printf( "550 TLS handshake failed\r\n" );

			ssl->SetFD( -1 );

			delete ssl;
		}

	} else
#endif
		User->Printf( "504 AUTH %s is not supported.\r\n", Args.c_str() );
}
//---------------------------------------------------------------------------
void Client::CmdPass( void )
{
	Server->Printf( "PASS %s\r\n", Args.c_str() );

	if( RecvStatus() == 230 )
		FTPFlags.Set( FTPF_LOGGED_IN );

	User->Send( LastReply.c_str(), LastReply.length() );
}
//---------------------------------------------------------------------------
void Client::CmdFeat( void )
{
	User->Printf( "211-Extensions supported\r\n"
#if USE_SSL
				  " AUTH TLS\r\n"
				  " PBSZ\r\n"
				  " PROT\r\n"
#endif
				  " EPRT\r\n"
				  " EPSV\r\n"
				  "211 END\r\n" );
}
//---------------------------------------------------------------------------
void Client::CmdPbsz( void )
{
	if( Args == "0" ) {

		FTPFlags.Set( FTPF_PBSZ );
		User->Printf( "200 Yessir.\r\n" );

	} else
		User->Printf( "504 Only PBSZ 0 is accepted.\r\n" );
}
//---------------------------------------------------------------------------
void Client::CmdProt( void )
{
	if( FTPFlags.IsSet( FTPF_PBSZ )) {

		FTPFlags.Set( FTPF_CLI_DATA_TLS, toupper( Args.c_str()[0] ) == 'P' );
		FTPFlags.Clear( FTPF_PBSZ );

		User->Printf( "200 Yessir.\r\n" );

	} else
		User->Printf( "503 PBSZ not issued yet.\r\n" );
}
//---------------------------------------------------------------------------
void Client::CmdPasv( void )
{
	char	*ip = User->GetLocalName();

	if( strchr( ip, ':' )) {
		// uh-oh, is the user connected from an IPv6 interface
		// or via an IPv4-Mapped socket?

		if( !strncmp( ip, "::ffff:", 7 ))
			ip += 7;

		else if( !strcmp( ip, "::1" ))
			strcpy( ip, "127,0,0,1" );

		else {

			User->Printf( "425 Sorry, I can't find a way to listen on an IPv4 interface for you.\r\n" );

			*ip = '\0';
		}
	}

	if( *ip && SetupServerDataConnection() ) {
		char	*ptr;

		ptr = ip;

		while( ptr = strchr( ptr, '.' ))
			*ptr++ = ',';

		delete UserData;

		UserData = new TcpSocket();

		UserData->UseDispatcher( App->IO );
		UserData->SetAsyncCallback( SocketCB, this );

		if( UserData->IsValid() &&
			UserData->Bind() && UserData->Listen( 1 )) {
			int		port = UserData->GetLocalPort();

			User->Printf( "227 Entering passive mode (%s,%d,%d).\r\n",
						  ip, port >> 8, port & 0xff );

			FTPFlags.Set( FTPF_CLIENT_PASV );

		} else {

			User->Printf( "425 Cannot setup the socket for data connection.\r\n" );

			delete UserData;
			UserData = NULL;
		}

	} else if( *ip )
		User->Printf( "425 Cannot setup the data connection to the server.\r\n" );
}
//---------------------------------------------------------------------------
void Client::CmdEpsv( void )
{
	delete UserData;

	if( Args == "ALL" )
		FTPFlags.Set( FTPF_BLOCK_DATA_CONN );
	else {
		int	family = AF_INET;

		if( Args.length() )
			family = atoi( Args.c_str() );

		if(( family == AF_INET )
#if HAVE_IPV6
			|| ( family == AF_INET6 )
#endif
			) {

			if( SetupServerDataConnection() ) {

				UserData = new TcpSocket();

				UserData->UseDispatcher( App->IO );
				UserData->SetAsyncCallback( SocketCB, this );

				if( UserData->IsValid() && UserData->Bind() && UserData->Listen( 1 )) {

					User->Printf( "229 Entering extended passive mode (|||%d|)\r\n",
								  UserData->GetLocalPort() );

					FTPFlags.Set( FTPF_CLIENT_PASV );

				} else {

					User->Printf( "425 Cannot setup the socket for data connection.\r\n" );

					delete UserData;
					UserData = NULL;
				}

			} else
				User->Printf( "425 Cannot setup the data connection to the server.\r\n" );

		} else
#if HAVE_IPV6
			User->Printf( "522 Unsupported protocol family (%d,%d)", AF_INET, AF_INET6 );
#else
			User->Printf( "522 Unsupported protocol family (%d)", AF_INET );
#endif
	}
}
//---------------------------------------------------------------------------
void Client::CmdPort( void )
{
	int	a1, a2, a3, a4, p1, p2;

	if( sscanf( Args.c_str(), "%d,%d,%d,%d,%d,%d",
				&a1, &a2, &a3, &a4, &p1, &p2 ) == 6 ) {

		sprintf( PortAddress, "%d.%d.%d.%d", a1, a2, a3, a4 );

		PortPort = ( p1 << 8 ) | p2;

		if( SetupServerDataConnection() ) {

			FTPFlags.Set( FTPF_CLIENT_PORT );

			User->Printf( "200 Ok\r\n" );

		} else
			User->Printf( "451 Cannot setup data connection to server\r\n" );

	} else
		User->Printf( "501 Cannot parse the argument\r\n" );
}
//---------------------------------------------------------------------------
void Client::CmdEprt( void )
{
	StringList	list;

	list.Explode( Args, Args.substr( 0, 1 ).c_str() );

	if( list.Count() == 5 ) {
		int	family;

		strncpy( PortAddress, list[2], sizeof( PortAddress ));
		PortAddress[ sizeof( PortAddress ) - 1 ] = '\0';

		PortPort = atoi( list[3] );
		family   = atoi( list[1] );

		if(( family == AF_INET )
#if HAVE_IPV6
			|| ( family == AF_INET6 )
#endif
			) {

			if( SetupServerDataConnection() ) {

				FTPFlags.Set( FTPF_CLIENT_PORT );

				User->Printf( "200 Ok\r\n" );

			} else
				User->Printf( "451 Cannot setup data connection to server\r\n" );

		} else
#if HAVE_IPV6
			User->Printf( "522 Unsupported protocol family (%d,%d)", AF_INET, AF_INET6 );
#else
			User->Printf( "522 Unsupported protocol family (%d)", AF_INET );
#endif

	} else
		User->Printf( "501 Cannot parse the argument\r\n" );
}
//---------------------------------------------------------------------------
void Client::CmdAbor( void )
{
	FTPFlags.Clear( FTPF_CLIENT_PORT | FTPF_CLIENT_PASV );

	if( Server ) {
		Server->Printf( "ABOR\r\n" );
		RecvStatus();
	}

	delete UserData;
	delete ServerData;

	UserData   = NULL;
	ServerData = NULL;
}
//---------------------------------------------------------------------------
bool Client::SetupServerDataConnection( void )
{
	bool	ok = false;

	ServerData = new TcpSocket();

	ServerData->UseDispatcher( App->IO );
	ServerData->SetAsyncCallback( SocketCB, this );

	if( ServerData->IsValid() &&
		ServerData->Bind() &&
		ServerData->Listen( 1 )) {
		char	*ip = Server->GetLocalName();
		int		port = ServerData->GetLocalPort();

#if HAVE_IPV6
		if( strchr( ip, ':' ) && strncmp( ip, "::ffff:", 7 ))
			Server->Printf( "EPRT |%d|%s|%d|\r\n",
							AF_INET6, ip, port );

		else
#endif
		{
			char	*ptr;

			if( ip[0] == ':' )
				ip += 7;

			ptr = ip;

			while( ptr = strchr( ptr, '.' ))
				*ptr++ = ',';

			Server->Printf( "PORT %s,%d,%d\r\n",
							ip, port >> 8, port & 0xff );
		}

		ok = RecvStatus() == 200;
	}

	if( !ok ) {
		delete ServerData;
		ServerData = NULL;
	}

	return( ok );
}
//---------------------------------------------------------------------------
bool Client::OpenServerDataConnection( void )
{
	bool	ok = false, loop = true;

	Server->Printf( "%s %s\r\n", Command.c_str(), Args.c_str() );

	if( RecvStatus() == 150 ) {
		time_t	timeout = time( NULL ) + TIMEOUT_DATA_CONN;

		// wait for data connection
		do {

			if( !App->IO->WaitEvents( TIMEOUT_DATA_CONN ) &&
			   ( time( NULL ) > timeout ))
				loop = false;

			else if( FTPFlags.IsSet( FTPF_SERVER_ACCEPT )) {

				loop = false;
				ok   = ServerData != NULL;

				FTPFlags.Clear( FTPF_SERVER_ACCEPT );
			}

		} while( ServerData && loop );

		if( !ok ) {
			Server->Printf( "ABOR\r\n" );
			delete ServerData;
			ServerData = NULL;
		}
	}

	return( ok );
}
//---------------------------------------------------------------------------
bool Client::OpenClientDataConnection( void )
{
	bool		ok;
	Prom_Addr	addr;

	delete UserData;

	UserData = new TcpSocket();

	UserData->UseDispatcher( App->IO );
	UserData->SetAsyncCallback( SocketCB, this );

	ok = TcpSocket::NameToAddr( PortAddress, &addr ) &&
		 UserData->IsValid() &&
		 UserData->Connect( &addr, PortPort );

#if USE_SSL
	if( ok && FTPFlags.IsSet( FTPF_CLI_DATA_TLS )) {
		SSLSocket	*ssl = new SSLSocket( ServerCtx, UserData );

		UserData->SetFD( -1 );

		if( ssl->SSLInitSession( SSLCtx::SERVER )) {

			delete UserData;

			UserData = ssl;

		} else {

			ok = false;

			delete ssl;
		}
	}
#endif

	if( ok )
		App->IO->AddFD( UserData, PROM_IOF_READ );

	return( ok );
}
//---------------------------------------------------------------------------
bool Client::WaitClientDataConnection( void )
{
	bool	ok;

	if( FTPFlags.IsSet( FTPF_CLIENT_ACCEPT ))
		ok = UserData != NULL;

	else {
		bool	loop = true;
		time_t	timeout = time( NULL ) + TIMEOUT_DATA_CONN;

		ok = false;

		// wait for data connection
		do {

			if( !App->IO->WaitEvents( TIMEOUT_DATA_CONN ) &&
			   ( time( NULL ) > timeout ))
				loop = false;

			else if( FTPFlags.IsSet( FTPF_CLIENT_ACCEPT )) {

				loop = false;
				ok   = UserData != NULL;
			}

		} while( UserData && loop );
	}

	FTPFlags.Clear( FTPF_CLIENT_ACCEPT );

	return( ok );
}
//---------------------------------------------------------------------------
bool Client::DataConnectionCmd( void )
{
	bool	ret = false;
	int		dir;

	for( int i = 0; !ret && ( i < NUM_DATA_CMDS ); i++ )
		if( Command == DataCmds[ i ].cmd ) {
			ret = true;
			dir = DataCmds[ i ].dir;
		}

	if( ret ) {

		if( FTPFlags.IsSet( FTPF_BLOCK_DATA_CONN ))
			User->Printf( "425 Cannot open data connection after EPSV ALL\r\n" );

		else if( FTPFlags.IsSet( FTPF_CLIENT_PORT | FTPF_CLIENT_PASV )) {

			if( !OpenServerDataConnection() )
				User->Printf( "425 Cannot open data connection\r\n" );

			else {
				bool ok = false;

				if( FTPFlags.IsSet( FTPF_CLIENT_PORT ))
					ok = OpenClientDataConnection();
				else if( FTPFlags.IsSet( FTPF_CLIENT_PASV ))
					ok = WaitClientDataConnection();

				if( ok ) {

					User->Printf( "150 Data connection open\r\n" );

				} else {

					Server->Printf( "ABOR\r\n" );
					RecvStatus();

					User->Printf( "425 Cannot open data connection\r\n" );

					delete ServerData;
					delete UserData;

					ServerData = NULL;
					UserData   = NULL;
				}
			}

		} else
			User->Printf( "503 No PORT nor PASV has been issued.\r\n" );
	}

	return( ret );
}
//---------------------------------------------------------------------------
void Client::ForwardData( TcpSocket *sock, int len )
{
	char	DataBuffer[ 4096 ];

	if( sock == ServerData )
		len = ServerData->Recv( DataBuffer, sizeof( DataBuffer ));
	else
		len = UserData->Recv( DataBuffer, sizeof( DataBuffer ));

	if( len <= 0 ) {

		delete ServerData;
		delete UserData;

		RecvStatus();
		User->Printf( "226 Closing data connection\r\n" );

		ServerData = NULL;
		UserData   = NULL;

	} else if( sock == ServerData )
		UserData->Send( DataBuffer, len );
	else
		ServerData->Send( DataBuffer, len );
}
//---------------------------------------------------------------------------
void Client::HandleError( TcpSocket *sock, int err )
{
	if(( sock == UserData ) || ( sock == ServerData ))
		ForwardData( sock, 0 );
	else {

		if( sock == Server )
			User->Printf( "221 The server connection dropped.\r\n" );

		FTPFlags.Clear( FTPF_CONNECTED );
	}
}
//---------------------------------------------------------------------------
