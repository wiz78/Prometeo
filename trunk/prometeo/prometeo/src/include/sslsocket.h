/***************************************************************************
                                sslsocket.h
                             -------------------
    revision             : $Id: sslsocket.h,v 1.6 2003-02-07 14:10:58 tellini Exp $
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

#if USE_SSL

#ifndef PROM_SSLSOCKET_H
#define PROM_SSLSOCKET_H

#include <openssl/ssl.h>

#include "tcpsocket.h"
#include "bitfield.h"
#include "sslctx.h"

class SSLSocket : public TcpSocket
{
public:
					SSLSocket( SSLCtx *ctx );
					SSLSocket( SSLCtx *ctx, int fd, bool dontclose = true );
					SSLSocket( SSLCtx *ctx, TcpSocket *sock, bool dontclose = true );
	virtual			~SSLSocket();

	bool			SSLInitSession( SSLCtx::SessionType type );
	void			SSLEndSession( void );

	const char		*SSLGetErrorStr( void );

	X509			*SSLGetPeerCert( void ) { return( SSL_get_peer_certificate( Ssl )); }
	STACK_OF(X509)	*SSLGetPeerCertChain( void ) { return( SSL_get_peer_cert_chain( Ssl )); }

	void			SSLSetCert( X509 *cert ) { SSL_use_certificate( Ssl, cert ); }

	// flags is ignored unless it's MSG_PEEK, which is emulated
	// buffering data
	virtual bool	Send( const void *data, int size, int flags = 0 );
	// flags is ignored, timeout too at the moment
	virtual int		Recv( void *buffer, int size, int flags = 0, int timeout = -1 );

private:
	BitField		SSLFlags;
	SSL				*Ssl;
	SSLCtx			*SslCtx;

	void			Setup( void );
};

#define SSLF_DONT_CLOSE			(1 << 0)

#endif

#endif /* USE_SSL */
