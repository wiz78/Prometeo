/***************************************************************************
                                 sslctx.h
                             -------------------
    revision             : $Id: sslctx.h,v 1.1 2002-11-01 22:25:14 tellini Exp $
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

#if USE_SSL

#ifndef PROM_SSLCTX_H
#define PROM_SSLCTX_H

#include <openssl/ssl.h>

class SSLCtx
{
public:
	typedef enum { CLIENT, SERVER, DUMMY } SessionType;

					SSLCtx( SessionType type );
					~SSLCtx();

	bool			IsValid( void ) const { return( Ctx != NULL ); }

	SSL_CTX			*GetCtx( void ) const { return( Ctx ); }

private:
	SSL_CTX			*Ctx;
	SessionType		Type;

	void			InitClient( void );
	void			InitServer( void );
};

#endif /* PROM_SSLCTX_H */

#endif /* USE_SSL */
