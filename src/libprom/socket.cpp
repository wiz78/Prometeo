/***************************************************************************
                                  socket.cpp
                             -------------------
	revision             : $Id: socket.cpp,v 1.2 2002-10-14 19:36:16 tellini Exp $
    copyright            : (C) 2002 by Simone Tellini
    email                : tellini@users.sourceforge.net

	description          : socket interface wrapper
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

#include <sys/socket.h>
#include <fcntl.h>
#include <errno.h>
#include <stdarg.h>
#include <stdio.h>

#include "socket.h"
#include "buffer.h"
#include "iodispatcher.h"

#ifndef EWOULDBLOCK
#define EWOULDBLOCK EAGAIN
#endif

//---------------------------------------------------------------------------
Socket::Socket( int family, int type, int proto )
{
	Init();

	FD = socket( family, type, proto );
}
//---------------------------------------------------------------------------
Socket::Socket( int fd )
{
	Init();

	FD	= fd;

	if( IsValid() && ( fcntl( FD, F_GETFL, 0 ) & O_NONBLOCK ))
		FdFlags.Set( PROM_FDF_NONBLOCKING );
}
//---------------------------------------------------------------------------
Socket::~Socket()
{
	if( IsValid() ) {
		shutdown( FD, 2 );
		close( FD );
	}
}
//---------------------------------------------------------------------------
void Socket::Init( void )
{
	AsyncCallback	= NULL;
	AsyncReadBuffer = NULL;
}
//---------------------------------------------------------------------------
bool Socket::Connect( const char *host )
{
	bool			ret = false;
	struct sockaddr	*addr;
	socklen_t		len;

	if( NameToAddr( host, &addr, &len ))
		ret = Connect( addr, len );

	return( ret );
}
//---------------------------------------------------------------------------
bool Socket::Connect( struct sockaddr *addr, socklen_t len )
{
	bool ret;

	SetNonBlocking( false );

	ret = connect( FD, addr, len ) == 0;

	return( ret );
}
//---------------------------------------------------------------------------
bool Socket::Send( const void *data, int size, int flags )
{
	int len;

	SetNonBlocking( false );

	len = send( FD, data, size, flags | MSG_NOSIGNAL );

	return( len == size );
}
//---------------------------------------------------------------------------
bool Socket::Printf( const char *fmt, ... )
{
	char	buffer[1024];
	va_list	ap;

	va_start( ap, fmt );

	vsnprintf( buffer, sizeof( buffer ), fmt, ap );

	va_end( ap );

	return( Send( buffer, strlen( buffer ), 0 ));
}
//---------------------------------------------------------------------------
int Socket::Recv( void *buffer, int size, int flags, int timeout )
{
	SetNonBlocking( false );

	return( recv( FD, buffer, size, flags | MSG_NOSIGNAL ));
}
//---------------------------------------------------------------------------
int Socket::RecvLine( char *buffer, int size, int timeout )
{
	int read = 0;

	buffer[0] = '\0';
	size--;             // ending '\0'

	while( size ) {
		int num;

		num = Recv( buffer, size, MSG_PEEK, timeout );

		if( num == 0 ) {

			size = 0; // connection has been closed

		} else if( num > 0 ) {
			int     left = num;
			char   *ptr = buffer;

			while( size && left ) {

				switch( *ptr++ ) {

					case '\n':
					case '\0':
						num  = ptr - buffer;
						size = 0;
						break;

					default:
						left--;
						size--;
						break;
				}
			}

			Recv( buffer, num ); // remove data from the input queue

			buffer += num;
			read   += num;

		} else {

			if( read > 0 )
				buffer[ -1 ] = '\0';

			read = num;
			size = 0;
		}
	}

	// make sure to terminate the string
	if( read > 0 )
		buffer[ (( read >= 2 ) && ( buffer[ -2 ] == '\r' )) ? -2 : -1 ] = '\0';

	return( read );
}
//---------------------------------------------------------------------------
void Socket::SetAsyncCallback( Prom_SockCallback callback, void *userdata )
{
	AsyncCallback = callback;
	AsyncUserData = userdata;
}
//---------------------------------------------------------------------------
bool Socket::AsyncConnect( char *host, int timeout )
{
	bool			ret = false;
	struct sockaddr	*addr;
	socklen_t		len;

	if( NameToAddr( host, &addr, &len ))
		ret = AsyncConnect( addr, len, timeout );

	return( ret );
}
//---------------------------------------------------------------------------
bool Socket::AsyncConnect( struct sockaddr *addr, socklen_t len, int timeout )
{
	bool	ret = false;

	if( SetNonBlocking( true )) {
		int			err = 0;
		socklen_t	errlen = sizeof( err );

		ret = connect( FD, addr, len ) >= 0;

		if( ret ) {

			if( getsockopt( FD, SOL_SOCKET, SO_ERROR, &err, &errlen ) == 0 )
				errno = err;
			else
				errno = ENOTCONN;

			ret = errno == 0;
		}

		if( !ret && ( errno == EINPROGRESS )) {

			Flags.Set( PROM_SOCKF_CONNECTING );
			SetTimeout( timeout );

			if( Dispatcher )
				Dispatcher->AddFD( this, PROM_IOF_WRITE );

			ret = true;

		} else if( ret )
			Callback( PROM_SOCK_CONNECTED, 0 );
		else
			Callback( PROM_SOCK_ERROR, errno );
	}

	return( ret );
}
//---------------------------------------------------------------------------
bool Socket::AsyncSend( const void *data, int size, int flags )
{
	bool ret = true;

	if( Flags.IsSet( PROM_SOCKF_SENDING ))
		Data.Append(( char * )data, size );

	else if( SetNonBlocking( true )) {

		// don't try to send anything here: it's common to perform
		// several AsyncSend()'s in a row and if one fails the error
		// callback is invoked; if that happens to delete some instance
		// data used by the routine which is calling AsyncSend() you
		// will get 1. nice fireworks, 2. a programmer going mad ;-)

		ret             = true;
		WriteOffset     = 0;
		AsyncWriteFlags = flags | MSG_NOSIGNAL;

		Data.SetContent((char *)data, size );
		Flags.Set( PROM_SOCKF_SENDING );

		if( Dispatcher )
			Dispatcher->AddFD( this, PROM_IOF_WRITE );

	} else
		ret = false;

	return( ret );
}
//---------------------------------------------------------------------------
bool Socket::AsyncRecv( void *data, int size, int flags, int timeout )
{
	bool ret = SetNonBlocking( true );

	if( Dispatcher )
		Dispatcher->RemFD( this, PROM_IOF_READ );

	if( ret ) {
		int len;

		flags |= MSG_NOSIGNAL;

		len = recv( FD, data, size, flags );

		if( len >= 0 )
			Callback( PROM_SOCK_READ, len );

		else if(( errno != EWOULDBLOCK ) && ( errno != EINTR )) {

			Callback( PROM_SOCK_ERROR, errno );

			ret = false;

		} else if( errno == EWOULDBLOCK ) {

			AsyncReadBuffer = data;
			AsyncReadFlags	= flags;
			ReadSize        = size;

			SetTimeout( timeout );

			if( Dispatcher )
				Dispatcher->AddFD( this, PROM_IOF_READ );
		}
	}

	return( ret );
}
//---------------------------------------------------------------------------
bool Socket::AsyncPrintf( const char *fmt, ... )
{
	char	buffer[1024];
	va_list	ap;

	va_start( ap, fmt );

	vsnprintf( buffer, sizeof( buffer ), fmt, ap );

	va_end( ap );

	return( AsyncSend( buffer, strlen( buffer ), 0 ));
}
//---------------------------------------------------------------------------
void Socket::NotifyOnDisconnect( int timeout )
{
	if( timeout >= 0 ) {

		Flags.Set( PROM_SOCKF_NOTIFY_DISCONN );

		SetTimeout( timeout );

		if( Dispatcher )
			Dispatcher->AddFD( this, PROM_IOF_READ );

	} else {

		SetTimeout( 0 );

		if( Dispatcher )
			Dispatcher->RemFD( this, PROM_IOF_READ );
	}
}
//---------------------------------------------------------------------------
bool Socket::Listen( int backlog )
{
	bool ret = listen( FD, backlog ) >= 0;

	if( ret ) {

		Flags.Set( PROM_SOCKF_LISTEN );

		if( Dispatcher )
			Dispatcher->AddFD( this, PROM_IOF_READ );
	}

	return( ret );
}
//---------------------------------------------------------------------------
bool Socket::SetLinger( bool on, int timeout )
{
    struct linger L;

    L.l_onoff  = on;
    L.l_linger = timeout;

    return( setsockopt( FD, SOL_SOCKET, SO_LINGER, &L, sizeof( L )) == 0 );
}
//---------------------------------------------------------------------------
bool Socket::SetReuseAddr( bool on )
{
	int reuse = on;

    return( setsockopt( FD, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof( reuse )) == 0 );
}
//---------------------------------------------------------------------------
void Socket::HandleRead( void )
{
	if( Flags.IsSet( PROM_SOCKF_LISTEN )) {

		Callback( PROM_SOCK_ACCEPT, (int)Accept() );

	} else if( Flags.IsSet( PROM_SOCKF_NOTIFY_DISCONN )) {
		
		if( !StillConnected() ) {

			if( Dispatcher )
				Dispatcher->RemFD( this, PROM_IOF_READ );

			Callback( PROM_SOCK_READ, 0 );
		}

	} else {
		int len;

		if( Dispatcher )
			Dispatcher->RemFD( this, PROM_IOF_READ );

		len = recv( FD, AsyncReadBuffer, ReadSize, AsyncReadFlags );

		if( len >= 0 )
			Callback( PROM_SOCK_READ, len );

		else if(( errno != EWOULDBLOCK ) && ( errno != EINTR ))
			Callback( PROM_SOCK_ERROR, errno );

		else if( Dispatcher )
			Dispatcher->AddFD( this, PROM_IOF_READ );
	}
}
//---------------------------------------------------------------------------
void Socket::HandleWrite( void )
{
	bool keepwaiting = false;

	if( Dispatcher )
		Dispatcher->RemFD( this, PROM_IOF_WRITE );

	Flags.Clear( PROM_SOCKF_SENDING );

	if( Flags.IsSet( PROM_SOCKF_CONNECTING )) {
		int			result;
		socklen_t	len = sizeof( result );

		Flags.Clear( PROM_SOCKF_CONNECTING );

		if( getsockopt( FD, SOL_SOCKET, SO_ERROR, &result, &len ) != 0 )
			result = ENOTCONN;

		Callback( PROM_SOCK_CONNECTED, result );

	} else if( WriteOffset < Data.GetSize() ) {
		int len;

		len = send( FD, Data.GetData() + WriteOffset, Data.GetSize() - WriteOffset, AsyncWriteFlags );

		if( len > 0 ) {

			WriteOffset += len;

			if( WriteOffset >= Data.GetSize() ) {

				Data.Clear();
				Callback( PROM_SOCK_WRITTEN, 0 );

			} else
				keepwaiting = true;

		} else if(( len <= 0 ) && ( errno != EWOULDBLOCK ) && ( errno != EINTR )) {

			Data.Clear();
			Callback( PROM_SOCK_ERROR, errno );

		} else
			keepwaiting = true;
	}

	if( keepwaiting ) {

		Flags.Set( PROM_SOCKF_SENDING );

		if( Dispatcher )
			Dispatcher->AddFD( this, PROM_IOF_WRITE );
	}
}
//---------------------------------------------------------------------------
void Socket::HandleExcept( void )
{
	Callback( PROM_SOCK_EXCEPT, 0 );
}
//---------------------------------------------------------------------------
void Socket::HandleTimeout( void )
{
	Fd::HandleTimeout();

	Data.Clear();

	Flags.Clear( PROM_SOCKF_SENDING | PROM_SOCKF_CONNECTING | PROM_SOCKF_NOTIFY_DISCONN );

	if( Dispatcher )
		Dispatcher->RemFD( this, PROM_IOF_READ | PROM_IOF_WRITE | PROM_IOF_EXCEPT );
	
	Callback( PROM_SOCK_TIMEOUT, 0 );
}
//---------------------------------------------------------------------------
void Socket::Callback( Prom_SC_Reason reason, int data )
{
	if( reason != PROM_SOCK_WRITTEN )
		SetTimeout( 0 );
		
	if( AsyncCallback )
		( *AsyncCallback )( this, reason, data, AsyncUserData );
}
//---------------------------------------------------------------------------
bool Socket::StillConnected( void )
{
	bool		conn = false;
	int			err;
	socklen_t	errlen = sizeof( err );
	char		foo[1];

	SetNonBlocking( true );

	err = recv( FD, foo, sizeof( foo ), MSG_PEEK | MSG_NOSIGNAL );

	if(( err > 0 ) || ( errno == EWOULDBLOCK )) {

		conn = true;

		// perhaps there's an error, even though the recv buffer isn't empty?
		if(( getsockopt( FD, SOL_SOCKET, SO_ERROR, &err, &errlen ) == 0 ) &&
		   ( err != EINTR ) && ( err != EWOULDBLOCK ))
			conn = false;
	}

	return( conn );
}
//---------------------------------------------------------------------------
