/***************************************************************************
                                 dnscache.h
                             -------------------
	revision             : $Id: dnscache.h,v 1.2 2002-10-29 18:01:13 tellini Exp $
    copyright            : (C) 2002 by Simone Tellini
    email                : tellini@users.sourceforge.net

	description          : a cache for DNS queries
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef PROM_DNSCACHE_H
#define PROM_DNSCACHE_H

#include "main.h"
#include "processgroup.h"
#include "strhash.h"

#include <sys/socket.h>

class DNSItem;
class IODispatcher;

class DNSCache : private ProcessGroup
{
public:
					DNSCache( IODispatcher *io );
					~DNSCache();

	// empty the cache, close the sockets
	void			OnFork();

	void			AsyncResolve( const char *hostname, Prom_Addr *addr,
								  Prom_DNS_Callback callback, void *userdata );
	bool			Resolve( const char *hostname, Prom_Addr *addr );

	// removes the pending request that matches the args
	void			CancelRequest( const char *hostname, Prom_DNS_Callback callback, void *userdata );

	void 			IPCAnswer( void *answer, int len );

	void			ReloadCfg( void );
	void			Flush( time_t now );

private:
	unsigned int	TTL;
	unsigned int	Slots;
	unsigned int	FreeSlot;
	DNSItem			*Items;
	StrHash			Hash;
	StrHash			Pending;

	void			Clear( void );

	void			Cache( const char *hostname, Prom_Addr *addr );
	void			Remove( DNSItem *item );
	bool 			FindCached( const char *hostname, Prom_Addr *addr );

	bool			AddPendingRequest( const char *hostname, Prom_Addr *addr,
									   Prom_DNS_Callback callback, void *userdata );
};

class DNSItem
{
public:
				DNSItem();
				~DNSItem();

	void		Set( const char *host, Prom_Addr *addr, unsigned int ttl );
	void		Clear( void );

	bool		HasExpired( time_t now );

	Prom_Addr	*GetAddr( void ) { return( &Addr ); }
	char		*GetHostname( void ) { return( Hostname ); }

	DNSItem&	operator =( DNSItem& item );

private:
	time_t		Expire;
	Prom_Addr	Addr;
	char		*Hostname;
};

#endif
