/***************************************************************************
                                sslsocket.h
                             -------------------
    revision             : $Id: sslsocket.h,v 1.1 2002-10-30 16:53:01 tellini Exp $
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

#ifndef PROM_SSLSOCKET_H
#define PROM_SSLSOCKET_H

#include <openssl/ssl.h>

#include "tcpsocket.h"
#include "bitfield.h"

class SSLSocket : public TcpSocket
{
public:
	typedef enum { CLIENT, SERVER } SocketType;

					SSLSocket();
					SSLSocket( int fd, bool dontclose = true );
					SSLSocket( TcpSocket *sock, bool dontclose = true );
					~SSLSocket();

	bool			SSLInitSession( SocketType type );
	void			SSLEndSession( void );

	// flags is ignored
	virtual bool	Send( const void *data, int size, int flags = 0 );
	// flags is ignored, timeout too at the moment
	virtual int		Recv( void *buffer, int size, int flags = 0, int timeout = -1 );

private:
	BitField		SSLFlags;
	SSL				*Ssl;
	SSL_CTX			*SslCtx;

	void			Setup( void );

	SSL_CTX			*InitClient( void );
	SSL_CTX			*InitServer( void );
};

#define SSLF_DONT_CLOSE			(1 << 0)

#endif
