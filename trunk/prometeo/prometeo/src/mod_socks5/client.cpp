/***************************************************************************
                                 client.cpp
                             -------------------
    revision             : $Id: client.cpp,v 1.1 2003-10-23 17:27:17 tellini Exp $
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
namespace mod_socks5
{
//---------------------------------------------------------------------------
static void SocketCB( SOCKREF sock, Prom_SC_Reason reason, int data, void *userdata )
{
	reinterpret_cast<Client *>( userdata )->SocketEvent( sock, reason, data );
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
}
//---------------------------------------------------------------------------
void Client::Cleanup( void )
{
	delete User;
	delete Server;

	User   = NULL;
	Server = NULL;

	CFlags.Clear( SOCKS5F_CONNECTED );
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

		Prom_set_ps_display( "busy" );

		Dispatch();

		// signal that we're idle
		num = 0;

		Socket->Send( &num, sizeof( num ));

		Prom_set_ps_display( "idle" );
	}
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

	State  = STATE_METHOD;
	CmdLen = 0;
	
	CFlags.Set( SOCKS5F_CONNECTED );

	timeout = time( NULL ) + TIMEOUT_IDLE;

	do {

		if( !App->IO->WaitEvents( TIMEOUT_IDLE )) {

		 	if( time( NULL ) > timeout )
				CFlags.Clear( SOCKS5F_CONNECTED );

		} else
			timeout = time( NULL ) + TIMEOUT_IDLE;

	} while( CFlags.IsSet( SOCKS5F_CONNECTED ));

	Cleanup();
}
//---------------------------------------------------------------------------
void Client::SocketEvent( SOCKREF sock, Prom_SC_Reason reason, int data )
{
	switch( reason ) {

		case PROM_SOCK_READ:
			HandleData( reinterpret_cast<TcpSocket *>( sock ));
			break;

		case PROM_SOCK_TIMEOUT:
		case PROM_SOCK_ERROR:
			CFlags.Clear( SOCKS5F_CONNECTED );
			break;
	}
}
//---------------------------------------------------------------------------
void Client::HandleData( TcpSocket *sock )
{
	switch( State ) {

		case STATE_METHOD:
			ReadMethods();
			break;

		case STATE_CMD:
			ReadRequest();
			break;

		case STATE_LISTEN:
			AcceptConnection();
			break;

		case STATE_FORWARD:
			ForwardData( sock );
			break;
	}
}
//---------------------------------------------------------------------------
void Client::ReadMethods( void )
{
	int len;
		
	if( CmdLen < 2 )
		len = User->Recv( &CmdBuffer[ CmdLen ], 2 - CmdLen );
	else
		len = User->Recv( &CmdBuffer[ CmdLen ], CmdBuffer[ SOCKS5_F_NMETHODS ] - CmdLen + 2 );

	if( len > 0 ) {

		CmdLen += len;

		if(( CmdLen >= 2 ) && ( CmdLen - 2 == CmdBuffer[ SOCKS5_F_NMETHODS ])) {
			unsigned char method = SOCKS5_AUTH_NO_ACCEPTABLE_METHODS;
	
			if( CmdBuffer[ SOCKS5_F_VER ] == SOCKS5_VERSION )
				for( int i = SOCKS5_F_METHODS; i < CmdLen; i++ )
					if( CmdBuffer[ i ] == SOCKS5_AUTH_NO_AUTH_REQUIRED ) {
						method = SOCKS5_AUTH_NO_AUTH_REQUIRED;
						break;
					}

			CmdBuffer[ 0 ] = SOCKS5_VERSION;
			CmdBuffer[ 1 ] = method;

			User->Send( CmdBuffer, 2 );

			if( method != SOCKS5_AUTH_NO_ACCEPTABLE_METHODS ) {
				State  = STATE_CMD;
				CmdLen = 0;
			} else
				Cleanup();
		}
			
	} else
		Cleanup();
}
//---------------------------------------------------------------------------
void Client::ReadRequest( void )
{
	int len = -1, cmdlen;

	if( CmdLen < 5 )
		cmdlen = 5;
	else {
		
		cmdlen = GetAddressLen() + 6;

		if( cmdlen > sizeof( CmdBuffer ))
			cmdlen = -1;	
	}

	if( cmdlen > 0 ) {
			
		len = User->Recv( &CmdBuffer[ CmdLen ], cmdlen - CmdLen );

		if( len > 0 ) {

			CmdLen += len;

			if(( CmdLen > 6 ) && ( CmdLen >= cmdlen ))
				HandleRequest();
		}
	} 
	
	if( len <= 0 ) {
		SendReply( SOCKS5_REP_GENERAL_FAILURE );
		Cleanup();
	}
}
//---------------------------------------------------------------------------
int Client::GetAddressLen( void )
{
	int len = 0;
		
	switch( CmdBuffer[ SOCKS5_F_ATYP ] ) {

		case SOCKS5_ATYP_IPV4:
			len = 4;
			break;

		case SOCKS5_ATYP_IPV6:
			len = 16;
			break;

		case SOCKS5_ATYP_DOMAINNAME:
			len = CmdBuffer[ SOCKS5_F_DST_ADDR ] + 1;
			break;
	}

	return( len );
}
//---------------------------------------------------------------------------
void Client::GetAddress( Prom_Addr *addr, short *port )
{
	switch( CmdBuffer[ SOCKS5_F_ATYP ] ) {

		case SOCKS5_ATYP_IPV4:
#if HAVE_IPV6
			TcpSocket::MapIPv4toIPv6((struct in_addr *)&CmdBuffer[ SOCKS5_F_DST_ADDR ], addr );
#else
			memcpy( addr, &CmdBuffer[ SOCKS5_F_DST_ADDR ], 4 );
#endif
			if( port )
				*port = ntohs( *((short *)&CmdBuffer[ SOCKS5_F_DST_ADDR + 4 ]));
			break;

#if HAVE_IPV6
		case SOCKS5_ATYP_IPV6:
			memcpy( addr, &CmdBuffer[ SOCKS5_F_DST_ADDR ], 16 );

			if( port )
				*port = ntohs( *((short *)&CmdBuffer[ SOCKS5_F_DST_ADDR + 16 ]));
			break;
#endif
	}
}
//---------------------------------------------------------------------------
void Client::SendReply( char reply, char atype, Prom_Addr *addr, short port )
{
	char	buf[ 22 ];
	int		addrlen, len;
	
	addrlen = ( atype == SOCKS5_ATYP_IPV4 ) ? 4 : 16;
	len     = 6 + addrlen;

	memset( buf, 0, len );

	buf[ SOCKS5_F_VER  ] = SOCKS5_VERSION;
	buf[ SOCKS5_F_REP  ] = reply;
	buf[ SOCKS5_F_ATYP ] = atype;

	if( addr )
		memcpy( &buf[ SOCKS5_F_DST_ADDR ], addr, addrlen );

	port = htons( port );

	memcpy( &buf[ SOCKS5_F_DST_ADDR + addrlen ], &port, 2 );
	
	User->Send( buf, len );
}
//---------------------------------------------------------------------------
void Client::HandleRequest( void )
{
#if !HAVE_IPV6
	if(( CmdBuffer[ SOCKS5_F_ATYP ] != SOCKS5_ATYP_IPV4 ) &&
	   ( CmdBuffer[ SOCKS5_F_ATYP ] != SOCKS5_ATYP_DOMAINNAME )) {
		SendReply( SOCKS5_REP_ATYPE_NOT_SUPPORTED );
		Cleanup();
	} else {
#endif

	switch( CmdBuffer[ SOCKS5_F_CMD ] ) {

		case SOCKS5_CMD_CONNECT:
			HandleConnect();
			break;

		case SOCKS5_CMD_BIND:
			HandleBind();
			break;

		case SOCKS5_CMD_ASSOCIATE:
			HandleAssociate();
			break;

		default:
			SendReply( SOCKS5_REP_CMD_NOT_SUPPORTED );
			Cleanup();
			break;
	}

#if !HAVE_IPV6
	}
#endif
}
//---------------------------------------------------------------------------
void Client::HandleConnect( void )
{
	if( CmdBuffer[ SOCKS5_F_ATYP ] == SOCKS5_ATYP_DOMAINNAME ) {
		char	host[ 256 ];
		short	port;

		strncpy( host, &CmdBuffer[ SOCKS5_F_DST_ADDR + 1 ], sizeof( host ));

		host[ CmdBuffer[ SOCKS5_F_DST_ADDR ] ] = '\0';

		port = ntohs( *((short *)&CmdBuffer[ SOCKS5_F_DST_ADDR + 1 + CmdBuffer[ SOCKS5_F_DST_ADDR ] ]));

		ConnectToServer( host, port );

	} else {
		Prom_Addr	addr;
		short		port;

		GetAddress( &addr, &port );
		DoConnect( &addr, port );
	}
}
//---------------------------------------------------------------------------
void Client::HandleBind( void )
{
	Prom_Addr	addr;
	short		port;

	GetAddress( &addr, &port );
	
	Server = new TcpSocket();

	if( Server->IsValid() && Server->Bind( port ) && Server->Listen() ) {

		Server->UseDispatcher( App->IO );
		Server->SetAsyncCallback( SocketCB, this );

		App->IO->AddFD( Server, PROM_IOF_READ );

		TcpSocket::NameToAddr( Server->GetLocalName(), &addr );
			
#if HAVE_IPV6
		SendReply( SOCKS5_REP_SUCCEEDED, SOCKS5_ATYP_IPV6, &addr, Server->GetLocalPort() );
#else
		SendReply( SOCKS5_REP_SUCCEEDED, SOCKS5_ATYP_IPV4, &addr, Server->GetLocalPort() );
#endif

		State = STATE_LISTEN;
			
	} else {

		SendReply( SOCKS5_REP_GENERAL_FAILURE ); 
		Cleanup();
	}
}
//---------------------------------------------------------------------------
void Client::HandleAssociate( void )
{
	SendReply( SOCKS5_REP_CMD_NOT_SUPPORTED );
	Cleanup();
}
//---------------------------------------------------------------------------
void Client::AcceptConnection( void )
{
	TcpSocket *sock = reinterpret_cast<TcpSocket *>( Server->Accept() );

	if( sock->IsValid() ) {
		Prom_Addr	addr;
			
		delete Server;

		Server = sock;
		State  = STATE_FORWARD;

		Server->UseDispatcher( App->IO );
		Server->SetAsyncCallback( SocketCB, this );

		App->IO->AddFD( Server, PROM_IOF_READ );

		TcpSocket::NameToAddr( Server->GetPeerName(), &addr );

#if HAVE_IPV6
		SendReply( SOCKS5_REP_SUCCEEDED, SOCKS5_ATYP_IPV6, &addr, Server->GetPeerPort() );
#else
		SendReply( SOCKS5_REP_SUCCEEDED, SOCKS5_ATYP_IPV4, &addr, Server->GetPeerPort() );
#endif

	} else
		Cleanup();
}
//---------------------------------------------------------------------------
void Client::ConnectToServer( const char *server, short port )
{
	Prom_Addr	addr;

	CFlags.Clear( SOCKS5F_CONNECTED );

	if( App->DNS->Resolve( server, &addr ))
		DoConnect( &addr, port );
	else
		SendReply( SOCKS5_REP_GENERAL_FAILURE );
}
//---------------------------------------------------------------------------
bool Client::DoConnect( Prom_Addr *addr, short port )
{
	Server = new TcpSocket();

	Prom_set_ps_display( "connecting to server..." );

	if( Server->Connect( addr, port )) {
		Prom_Addr local;

		Server->UseDispatcher( App->IO );
		Server->SetAsyncCallback( SocketCB, this );

		App->IO->AddFD( Server, PROM_IOF_READ );
				
		CFlags.Set( SOCKS5F_CONNECTED );

		State = STATE_FORWARD;
		
		TcpSocket::NameToAddr( Server->GetPeerName(), &local );

#if HAVE_IPV6
		SendReply( SOCKS5_REP_SUCCEEDED, SOCKS5_ATYP_IPV6, &local, Server->GetLocalPort() );
#else
		SendReply( SOCKS5_REP_SUCCEEDED, SOCKS5_ATYP_IPV4, &local, Server->GetLocalPort() );
#endif

		Prom_set_ps_display( "tunneling" );

	} else {
		int rep = SOCKS5_REP_GENERAL_FAILURE;

		switch( errno ) {

			case ENETUNREACH:
				rep = SOCKS5_REP_NETWORK_UNREACHABLE;
				break;

			case ECONNREFUSED:
				rep = SOCKS5_REP_CONNECTION_REFUSED;
				break;

			case EHOSTUNREACH:
				rep = SOCKS5_REP_HOST_UNREACHABLE;
				break;

			case EPERM:
				rep = SOCKS5_REP_NOT_ALLOWED;
				break;
		}

		SendReply( rep );
	}

	return( CFlags.IsSet( SOCKS5F_CONNECTED ));
}
//---------------------------------------------------------------------------
void Client::ForwardData( TcpSocket *sock )
{
	char    DataBuffer[ 4096 ];
	int		len;

	if( sock == Server )
		len = Server->Recv( DataBuffer, sizeof( DataBuffer ));
	else
		len = User->Recv( DataBuffer, sizeof( DataBuffer ));

	if( len <= 0 )
		Cleanup();
	else if( sock == Server )
		User->Send( DataBuffer, len );
	else
		Server->Send( DataBuffer, len );
}
//---------------------------------------------------------------------------
}; // namespace
//---------------------------------------------------------------------------
