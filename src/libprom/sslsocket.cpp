/***************************************************************************
                                sslsocket.cpp
                             -------------------
    revision             : $Id: sslsocket.cpp,v 1.5 2003-01-18 21:58:23 tellini Exp $
    copyright            : (C) 2002 by Simone Tellini
    email                : tellini@users.sourceforge.net

    description          : TLS/SSL enabled TCP socket
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

#if USE_SSL

#include <openssl/err.h>

#include "sslsocket.h"

//---------------------------------------------------------------------------
SSLSocket::SSLSocket( SSLCtx *ctx ) : TcpSocket()
{
	SslCtx = ctx;

	Setup();
}
//---------------------------------------------------------------------------
SSLSocket::SSLSocket( SSLCtx *ctx, int fd, bool dontclose ) : TcpSocket( fd )
{
	SslCtx = ctx;

	if( dontclose )
		SSLFlags.Set( SSLF_DONT_CLOSE );

	Setup();
}
//---------------------------------------------------------------------------
SSLSocket::SSLSocket( SSLCtx *ctx, TcpSocket *sock, bool dontclose ) : TcpSocket( sock->GetFD() )
{
	SslCtx = ctx;

	if( dontclose )
		SSLFlags.Set( SSLF_DONT_CLOSE );

	Setup();
}
//---------------------------------------------------------------------------
SSLSocket::~SSLSocket()
{
	SSLEndSession();

	if( SSLFlags.IsSet( SSLF_DONT_CLOSE ))
		FD = -1;
}
//---------------------------------------------------------------------------
void SSLSocket::Setup( void )
{
	Ssl = NULL;
}
//---------------------------------------------------------------------------
bool SSLSocket::SSLInitSession( SSLCtx::SessionType type )
{
	bool	ok = false;

	if( SslCtx->IsValid() ) {

		Ssl = SSL_new( SslCtx->GetCtx() );

		if( Ssl ) {
			int res;

			SSL_set_fd( Ssl, FD );

			if( type == SSLCtx::CLIENT )
				res = SSL_connect( Ssl );
			else
				res = SSL_accept( Ssl );

			ok = res != -1;
		}
	}

	return( ok );
}
//---------------------------------------------------------------------------
void SSLSocket::SSLEndSession( void )
{
	if( Ssl ) {

		if( SSL_shutdown( Ssl ) == 0 )
			SSL_shutdown( Ssl );

		SSL_free( Ssl );

		Ssl = NULL;
	}
}
//---------------------------------------------------------------------------
bool SSLSocket::Send( const void *data, int size, int flags )
{
	SetNonBlocking( false );

	return( SSL_write( Ssl, data, size ) == size );
}
//---------------------------------------------------------------------------
int SSLSocket::Recv( void *buffer, int size, int flags, int timeout )
{
	int nread;

	if( flags & MSG_PEEK )
		nread = SSL_peek( Ssl, buffer, size );
	else
		nread = SSL_read( Ssl, buffer, size );

	return( nread );
}
//---------------------------------------------------------------------------
const char *SSLSocket::SSLGetErrorStr( void )
{
	return( ERR_error_string( ERR_get_error(), NULL ));
}
//---------------------------------------------------------------------------
#endif /* USE_SSL */
