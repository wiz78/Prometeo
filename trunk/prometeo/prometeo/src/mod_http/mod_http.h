/***************************************************************************
                                 mod_http.h
                             -------------------
	revision             : $Id: mod_http.h,v 1.3 2002-11-21 18:36:55 tellini Exp $
    copyright            : (C) 2002 by Simone Tellini
    email                : tellini@users.sourceforge.net

	description          : caching HTTP proxy
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef MOD_HTTP_H
#define MOD_HTTP_H

#define MOD_VERSION	"1.0"

#include <sys/types.h>
#include <string>

using namespace std;

#include "api.h"
#include "tcpsocket.h"
#include "linkedlist.h"
#include "bitfield.h"
#include "stringlist.h"

#include "http.h"
#include "connectionmgr.h"
#include "url.h"
#include "cache.h"
#include "hostmapper.h"
#include "filtermgr.h"

class HTTPData;
class CacheObj;

class HTTPProxy
{
public:
					HTTPProxy( const char *key );
					~HTTPProxy();

	bool			Cleanup();
	void			ReloadCfg( void );
	void			OnFork( void );
	void			OnTimer( time_t now );

	void			SocketEvent( Socket *sock, Prom_SC_Reason reason, int data );
	void			Resolved( HTTPData *data, int addrlen );

	void			CacheObjListener( HTTPData *data, char *buf, int len );

private:
	string			Key;
	short			Port;
	BitField		Flags;
	TcpSocket		*ListeningSocket;
	LinkedList		Sockets;
	ConnectionMgr	Connections;
	Cache			CacheMgr;
	time_t			LastPruneTime;
	time_t			LastIndexTime;
	unsigned int	MaxObjectSize;
	HostMapper		*HostMap;
	FilterMgr		*Filters;

	void			Setup( void );

	void			Accept( TcpSocket *sock );
	void			Error( HTTPData *data, int err, TcpSocket *sock );
	void			Connected( HTTPData *data );
	void			GotData( HTTPData *data, int len, Socket *sock );
	void			SocketClosed( HTTPData *data, Socket *sock );
	void			Written( HTTPData *data, int len, Socket *sock );

	void			SendError( HTTPData *data, int code, const char *text );
	void			SendRequest( const char *method, HTTPData *data );

	void			ResetConnection( HTTPData *data );
	bool			ResetServerConnection( HTTPData *data );
	bool			CloseClientSocket( HTTPData *data );
	bool			CloseServerSocket( HTTPData *data );
	bool			FreeHTTPData( HTTPData *data );

	void			ConnectToServer( HTTPData *data );
	void			StartTunneling( HTTPData *data );

	void			HandleRequest( HTTPData *data );
	void			HandleResponse( HTTPData *data );
	void			Handle_GET_HEAD( HTTPData *data );
	void			HandleBody( HTTPData *data, int len );

	void			ProcessFirstData( HTTPData *data );
	void			CopyServerHeaders( HTTPData *data, StringList& headers );
	void			PrepareHeaders( HTTPData *data, StringList& headers );

	void			ForwardResponse( HTTPData *data );
	void			DownloadAndCache( HTTPData *data );
	void			SendFromCache( HTTPData *data );

	bool			IsCacheable( HTTPData *data );

	enum {
		S_REQUEST,			// receiving the request from the client
		S_RESOLVING,		// resolving the host to connect to
		S_CONNECTING,		// connecting to the requested host
		S_TUNNELING,		// forwarding data client<->server
		S_CLOSING,			// waiting for all data to be sent in order
							// to close the client socket
		S_SERVER_RESPONSE,	// receiving the headers from the server
		S_BODY,				// dealing with the resource body
	};
};

// Flags
#define MODF_COMPRESS			(1 << 0)	// enable compression
#define MODF_LOG_REQUESTS		(1 << 1)

class HTTPData : public LinkedListNode
{
public:
	HTTPProxy	*Proxy;
	TcpSocket	*ClientSock;
	TcpSocket	*ServerSock;
	HTTP		Client;
	HTTP		Server;
	short		State;
	short		ServerState;
	Prom_Addr	Addr;
	char		ClientBuf[ 1 * 1024 ];
	char		ServerBuf[ 4 * 1024 ];
	CacheObj	*Cached;
};

#endif
