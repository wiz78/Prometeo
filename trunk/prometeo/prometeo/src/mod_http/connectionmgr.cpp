/***************************************************************************
                              connectionmgr.cpp
                             -------------------
	revision             : $Id: connectionmgr.cpp,v 1.1.1.1 2002-10-10 09:59:34 tellini Exp $
    copyright            : (C) 2002 by Simone Tellini
    email                : tellini@users.sourceforge.net

	description          : manages connections to HTTP 1.1 servers
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

#include <string>

#include "tcpsocket.h"
#include "connectionmgr.h"

#define CONN_EXPIRE_TIME	15 // keep connections alive for X seconds max
#define MAX_CONN_PER_SERVER	5

static void SocketCB( SOCKREF sock, Prom_SC_Reason reason, int data, void *userdata );

//---------------------------------------------------------------------------
class Connection : public LinkedListNode
{
public:
				Connection( const char *server, TcpSocket *sock );

	void		Clear( void );

	const char	*GetServer( void ) { return( Server.c_str() ); }

	TcpSocket	*GetSocket( void );
	bool		ReleaseSocket( TcpSocket *sock );

	bool		AddSocket( TcpSocket *sock );
	bool		RemSocket( TcpSocket *sock );

private:
	string		Server;
	TcpSocket	*Sockets[ MAX_CONN_PER_SERVER ];
	bool		Busy[ MAX_CONN_PER_SERVER ];
	int			Count;
};
//---------------------------------------------------------------------------
Connection::Connection( const char *server, TcpSocket *sock )
{
	memset( Sockets, 0, sizeof( Sockets ));
	memset( Busy, 0, sizeof( Busy ));

	Sockets[0] = sock;
	Count      = 1;
	Server     = server;
}
//---------------------------------------------------------------------------
void Connection::Clear( void )
{
	 for( int i = 0; i < MAX_CONN_PER_SERVER; i++ )
		delete Sockets[ i ];
}
//---------------------------------------------------------------------------
TcpSocket *Connection::GetSocket( void )
{
	for( int i = 0; i < MAX_CONN_PER_SERVER; i++ )
		if( !Busy[ i ] ) {

			Busy[ i ] = true;

			return( Sockets[ i ] );
		}

	return( NULL );
}
//---------------------------------------------------------------------------
bool Connection::ReleaseSocket( TcpSocket *sock )
{
	bool	found = false;

	for( int i = 0; i < MAX_CONN_PER_SERVER; i++ )
		if( Sockets[ i ] == sock ) {
			Busy[ i ] = false;
			found     = true;
			break;
		}

	return( found );
}
//---------------------------------------------------------------------------
bool Connection::AddSocket( TcpSocket *sock )
{
	bool	added = false;

	for( int i = 0; i < MAX_CONN_PER_SERVER; i++ )
		if( Sockets[ i ] == sock )
			return( true );

	for( int i = 0; i < MAX_CONN_PER_SERVER; i++ )
		if( !Sockets[ i ] ) {
			Sockets[ i ] = sock;
			Busy[ i ]    = false;
			added        = true;
			Count++;
			break;
		}

	if( !added )
		delete sock;

	return( added );
}
//---------------------------------------------------------------------------
bool Connection::RemSocket( TcpSocket *sock )
{
	for( int i = 0; i < MAX_CONN_PER_SERVER; i++ )
		if( Sockets[ i ] == sock ) {
			Sockets[ i ] = NULL;
			Count--;
			break;
		}

	return( Count <= 0 );
}
//---------------------------------------------------------------------------
ConnectionMgr::~ConnectionMgr()
{
	Clear();
}
//---------------------------------------------------------------------------
void ConnectionMgr::Clear( void )
{
	while( Connection *conn = (Connection *)Connections.RemTail() ) {

		Hash.Remove( conn->GetServer() );

		conn->Clear();

		delete conn;
	}
}
//---------------------------------------------------------------------------
void ConnectionMgr::AddConnection( const char *server, TcpSocket *sock )
{
	Connection	*conn = (Connection *)Hash.FindData( server );
	bool		ok = true;

	if( conn ) {

		ok = conn->AddSocket( sock );

	} else {

		conn = new Connection( server, sock );

		Hash.Add( server, conn );
		Connections.AddTail( conn );
	}

	if( ok )
		SetupCheck( conn, sock );
}
//---------------------------------------------------------------------------
void ConnectionMgr::RemoveConnection( const char *server, TcpSocket *sock )
{
	Connection	*conn = (Connection *)Hash.FindData( server );

	if( conn && conn->RemSocket( sock )) {

		Hash.Remove( server );

		delete conn;
	}
}
//---------------------------------------------------------------------------
TcpSocket *ConnectionMgr::FindConnection( const char *server )
{
	TcpSocket	*sock;
	bool		closed;

	do {
		Connection	*conn = (Connection *)Hash.FindData( server );

		sock = NULL;

		if( conn ) {

			sock = conn->GetSocket();

			if( sock ) {

				sock->NotifyOnDisconnect( -1 );
				sock->SetAsyncCallback( NULL, NULL );

				if( sock->StillConnected() )
					closed = false;
				else {

					RemoveConnection( server, sock );

					delete sock;

					closed = true;
				}
			}
		}

	} while( sock && closed );

	return( sock );
}
//---------------------------------------------------------------------------
void ConnectionMgr::ReleaseConnection( const char *server, TcpSocket *sock )
{
	Connection	*conn = (Connection *)Hash.FindData( server );

	if( conn && conn->ReleaseSocket( sock ))
		SetupCheck( conn, sock );
	else
		AddConnection( server, sock );
}
//---------------------------------------------------------------------------
void ConnectionMgr::SetupCheck( Connection *conn, TcpSocket *sock )
{
	// this may allow us to detect when the peer closes the connection
	sock->SetUserData( conn, NULL );
	sock->SetAsyncCallback( SocketCB, this );
	sock->NotifyOnDisconnect( CONN_EXPIRE_TIME );
}
//---------------------------------------------------------------------------
void ConnectionMgr::SocketEvent( TcpSocket *sock, Prom_SC_Reason reason, int data )
{
	switch( reason ) {

		case PROM_SOCK_ERROR:
		case PROM_SOCK_TIMEOUT:
			RemoveConnection(((Connection *)sock->GetUserData() )->GetServer(), sock );
			delete sock;
			break;

		case PROM_SOCK_READ:
			if( data == 0 ) {
				RemoveConnection(((Connection *)sock->GetUserData() )->GetServer(), sock );
				delete sock;
			}
			break;
	}
}
//---------------------------------------------------------------------------
static void SocketCB( SOCKREF sock, Prom_SC_Reason reason, int data, void *userdata )
{
	((ConnectionMgr *)userdata )->SocketEvent(( TcpSocket * )sock, reason, data );
}
//---------------------------------------------------------------------------
