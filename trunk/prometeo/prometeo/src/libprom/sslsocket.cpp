/***************************************************************************
                                sslsocket.cpp
                             -------------------
    revision             : $Id: sslsocket.cpp,v 1.4 2002-11-09 18:25:12 tellini Exp $
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

	// first check whether we already have some data
	if( TmpData.GetSize() > 0 ) {

		nread = TmpData.GetSize();

		if( size < nread )
			nread = size;

		memcpy( buffer, TmpData.GetData(), nread );

		if( nread < TmpData.GetSize() )
			TmpData.SetContent( TmpData.GetData() + nread, TmpData.GetSize() - nread );
		else
			TmpData.Clear();

	} else {

		SetNonBlocking( false );

		nread = SSL_read( Ssl, buffer, size );
	}

	if( flags & MSG_PEEK )
		TmpData.SetContent((char *)buffer, nread );

	return( nread );
}
//---------------------------------------------------------------------------
const char *SSLSocket::SSLGetErrorStr( void )
{
	return( ERR_error_string( ERR_get_error(), NULL ));
}
//---------------------------------------------------------------------------
#endif /* USE_SSL */
