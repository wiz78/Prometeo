/***************************************************************************
                                  socket.h
                             -------------------
	revision             : $Id: socket.h,v 1.6 2003-02-07 14:10:58 tellini Exp $
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

#ifndef PROM_SOCKET_H
#define PROM_SOCKET_H

#include <sys/socket.h>
#include <sys/un.h>
#include <netinet/in.h>

#include "api.h"
#include "fd.h"
#include "bitfield.h"
#include "buffer.h"

class IODispatcher;

// Flags
#define PROM_SOCKF_LISTEN			(1 << 0)
#define PROM_SOCKF_CONNECTING		(1 << 1)
#define PROM_SOCKF_SENDING			(1 << 2)
#define PROM_SOCKF_RECVING			(1 << 3)
#define PROM_SOCKF_NOTIFY_DISCONN	(1 << 4)

class Socket : public Fd
{
public:
						Socket( int family, int type, int proto );
						Socket( int fd );
	virtual				~Socket();

	bool				Shutdown( int how = 2 );

	// synchronous functions
	bool				Connect( const char *host );	// host is a dotted form name
														// no name resolution is performed
	bool				Connect( struct sockaddr *addr, socklen_t len );
	virtual bool		Send( const void *data, int size, int flags = 0 );
	virtual int			Recv( void *buffer, int size, int flags = 0, int timeout = -1 );
	// receive a CRLF, LF or \0 terminated line
	int					RecvLine( char *buffer, int size, int timeout = -1 );
	bool				Printf( const char *fmt, ... );

	// asynchronous ones
	// in case of error they return false AND the
	// callback is called with PROM_SOCK_ERROR reason
	bool				AsyncConnect( const char *host, int timeout = -1 ); // see comment on Connect( char *host )
	bool				AsyncConnect( struct sockaddr *addr, socklen_t len, int timeout = -1 );
	bool				AsyncSend( const void *data, int size, int flags = 0 );
	bool				AsyncRecv( void *data, int size, int flags = 0, int timeout = -1 );
	bool				AsyncPrintf( const char *fmt, ... );
	// send PROM_SOCK_READ, 0 when the peer disconnects
	// call it when no AsyncRecv()'s are pending
	// call it with timeout = -1 to reset it
	void				NotifyOnDisconnect( int timeout = 0 );
	bool				WritePending( void ) const { return( Flags.IsSet( PROM_SOCKF_SENDING )); }

	// miscellaneous functions
	bool				Listen( int backlog = 5 );

	// a callback MUST be set if using any async call, including Listen()
	void				SetAsyncCallback( Prom_SockCallback callback, void *userdata );

	// socket-family dependant functions
	virtual Socket		*Accept( void ) = 0;
	virtual char		*GetPeerName( void ) = 0;	// human readable name
	virtual char		*GetLocalName( void ) = 0;

	bool				SetLinger( bool on, int timeout = 0 );
	bool				SetReuseAddr( bool on );

	// try to guess whether the socket is still connected to the other end
	bool				StillConnected( void );

	// Fd interface implementation
	virtual void		HandleRead( void );
	virtual void		HandleWrite( void );
	virtual void		HandleExcept( void );
	virtual void		HandleTimeout( void );

protected:
	BitField			Flags;
	Prom_SockCallback	AsyncCallback;
	void				*AsyncUserData;
	void				*AsyncReadBuffer;
	int					AsyncReadFlags;
	int					AsyncWriteFlags;
	Buffer				Data;
	unsigned int		ReadSize;	// how much of Data we've already sent/recvd
	unsigned int		WriteOffset;

	void				Init( void );

	void				Callback( Prom_SC_Reason reason, int data );

	virtual bool		NameToAddr( const char *name, struct sockaddr **addr, socklen_t *len ) = 0;
};

#endif
