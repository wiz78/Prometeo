/***************************************************************************
                                 sslctx.cpp
                             -------------------
    revision             : $Id: sslctx.cpp,v 1.1 2002-11-01 22:25:19 tellini Exp $
    copyright            : (C) 2002 by Simone Tellini
    email                : tellini@users.sourceforge.net

    description          : SSL context wrapper
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

#include "sslctx.h"

//---------------------------------------------------------------------------
static int UseCount = 0;
//---------------------------------------------------------------------------
SSLCtx::SSLCtx( SessionType type )
{
	Type = type;

	if( Type == DUMMY )
		return;

	if( UseCount++ == 0 ) {

		SSL_load_error_strings();
		SSL_library_init();

#if ZLIB
		{
			COMP_METHOD *cm = COMP_zlib();

			if( cm && ( cm->type != NID_undef ))
        		SSL_COMP_add_compression_method( 0xe0, cm ); // Eric Young's ZLIB ID
		}
#endif
	}

	switch( type ) {

		case CLIENT:
			InitClient();
			break;

		case SERVER:
			InitServer();
			break;
	}

	if( IsValid() ) {

		// TODO make paths/certs configurable
		SSL_CTX_set_default_verify_paths( Ctx );
	}
}
//---------------------------------------------------------------------------
SSLCtx::~SSLCtx()
{
	if( Type != DUMMY ) {

		SSL_CTX_free( Ctx );

		if( --UseCount <= 0 ) {

			ERR_free_strings();
			ERR_remove_state( 0 );
		}
	}
}
//---------------------------------------------------------------------------
void SSLCtx::InitClient( void )
{
	Ctx = SSL_CTX_new( SSLv23_client_method() );

	if( Ctx ) {

		/* Set up session caching. */
		SSL_CTX_set_session_cache_mode( Ctx, SSL_SESS_CACHE_CLIENT );
		SSL_CTX_set_session_id_context( Ctx, (unsigned char *)"1", 1 );
	}
}
//---------------------------------------------------------------------------
void SSLCtx::InitServer( void )
{
	Ctx = SSL_CTX_new( SSLv23_server_method() );

	if( Ctx ) {

		/* Set up session caching. */
		SSL_CTX_set_session_cache_mode( Ctx, SSL_SESS_CACHE_SERVER );
		SSL_CTX_set_session_id_context( Ctx, (unsigned char *)"1", 1 );
	}
}
//---------------------------------------------------------------------------
#endif /* USE_SSL */
