/***************************************************************************
                                sslsocket.cpp
                             -------------------
    revision             : $Id: sslsocket.cpp,v 1.2 2002-10-31 19:04:14 tellini Exp $
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

#if HAVE_ZLIB_H
#define ZLIB 1
#endif

#include <openssl/err.h>
#include <openssl/comp.h>

#include "sslsocket.h"

//---------------------------------------------------------------------------
SSLSocket::SSLSocket() : TcpSocket()
{
	Setup();
}
//---------------------------------------------------------------------------
SSLSocket::SSLSocket( int fd, bool dontclose ) : TcpSocket( fd )
{
	if( dontclose )
		SSLFlags.Set( SSLF_DONT_CLOSE );

	Setup();
}
//---------------------------------------------------------------------------
SSLSocket::SSLSocket( TcpSocket *sock, bool dontclose ) : TcpSocket( sock->GetFD() )
{
	if( dontclose )
		SSLFlags.Set( SSLF_DONT_CLOSE );

	Setup();
}
//---------------------------------------------------------------------------
SSLSocket::~SSLSocket()
{
	SSLEndSession();

	free( SslCtx );

	ERR_free_strings();
	ERR_remove_state( 0 );

	if( SSLFlags.IsSet( SSLF_DONT_CLOSE ))
		FD = -1;
}
//---------------------------------------------------------------------------
void SSLSocket::Setup( void )
{
	Ssl    = NULL;
	SslCtx = NULL;
}
//---------------------------------------------------------------------------
bool SSLSocket::SSLInitSession( SocketType type )
{
	bool	ok = false;

	SSL_load_error_strings();
	SSL_library_init();

#if ZLIB
	{
		COMP_METHOD *cm = COMP_zlib();

		if( cm && ( cm->type != NID_undef ))
        	SSL_COMP_add_compression_method( 0xe0, cm ); // Eric Young's ZLIB ID
	}
#endif

	if( type == CLIENT )
		SslCtx = InitClient();
	else
		SslCtx = InitServer();

	if( SslCtx ) {

		// TODO make paths/certs configurable
		SSL_CTX_set_default_verify_paths( SslCtx );

		Ssl = SSL_new( SslCtx );

		if( Ssl ) {
			int res;

			SSL_set_fd( Ssl, FD );

			if( type == CLIENT )
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

		free( Ssl );

		Ssl = NULL;
	}
}
//---------------------------------------------------------------------------
SSL_CTX *SSLSocket::InitClient( void )
{
	SSL_CTX	*ctx = SSL_CTX_new( SSLv23_client_method() );

	if( ctx ) {

		/* Set up session caching. */
		SSL_CTX_set_session_cache_mode( ctx, SSL_SESS_CACHE_CLIENT );
		SSL_CTX_set_session_id_context( ctx, (unsigned char *)"1", 1 );
	}

	return( ctx );
}
//---------------------------------------------------------------------------
SSL_CTX *SSLSocket::InitServer( void )
{
	SSL_CTX	*ctx = SSL_CTX_new( SSLv23_server_method() );

	if( ctx ) {

		/* Set up session caching. */
		SSL_CTX_set_session_cache_mode( ctx, SSL_SESS_CACHE_SERVER );
		SSL_CTX_set_session_id_context( ctx, (unsigned char *)"1", 1 );
	}

	return( ctx );
}
//---------------------------------------------------------------------------
bool SSLSocket::Send( const void *data, int size, int flags )
{
	return( SSL_write( Ssl, data, size ) == size );
}
//---------------------------------------------------------------------------
int SSLSocket::Recv( void *buffer, int size, int flags, int timeout )
{
	int nread;

	nread = SSL_read( Ssl, buffer, size );

	return( nread );
}
//---------------------------------------------------------------------------
#endif /* USE_SSL */