/***************************************************************************
                                dnscache.cpp
                             -------------------
	revision             : $Id: dnscache.cpp,v 1.2 2002-10-15 13:03:42 tellini Exp $
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

#include "main.h"

#include <signal.h>
#include <sys/socket.h>

#include "dnscache.h"
#include "registry.h"
#include "resolver.h"

#define DNS_CHILDREN	4

static void IPCCallback( void *answer, int len, void *userdata );
static void ResolverCallback( int status, void *userdata );

class PendingReq
{
public:
						PendingReq( const char *host, Prom_Addr *addr,
									Prom_DNS_Callback cb, void *data )
						{
							Hostname = strdup( host );
							Callback = cb;
							Userdata = data;
							Addr     = addr;
							Next     = NULL;
						}

						~PendingReq()
						{
							free( Hostname );
							delete Next;
						}

	PendingReq			*Next;
	char 				*Hostname;
	Prom_Addr			*Addr;
	Prom_DNS_Callback 	Callback;
	void 				*Userdata;
};

//---------------------------------------------------------------------------
DNSCache::DNSCache( IODispatcher *io ) : ProcessGroup( DNS_CHILDREN, io ), Hash( 1027 ), Pending( 53 )
{
	int children = DNS_CHILDREN;

	TTL   = 300;
	Slots = 500;

	if( App->Cfg->OpenKey( "root/DNSCache", false )) {

		children = App->Cfg->GetInteger( "children", children );
		TTL      = App->Cfg->GetInteger( "ttl", TTL );
		Slots    = App->Cfg->GetInteger( "slots", Slots );

		App->Cfg->CloseKey();
	}

	Children.SetAllocBy( children );

	for( int i = 0; i < children; i++ ) {
		Resolver	*res = new Resolver();

		if( res->Spawn( "resolver" ))
			AddChild( res );
		else
			App->Log->Log( LOG_ERR, "DNS cache: couldn't spawn a new process" );
	}

	Items    = new DNSItem[ Slots ];
	FreeSlot = 0;
}
//---------------------------------------------------------------------------
DNSCache::~DNSCache()
{
	Clear();
}
//---------------------------------------------------------------------------
void DNSCache::Clear( void )
{
	delete[] Items;

	Items = NULL;

	while( Pending.Count() > 0 ) {
		PendingReq *req = (PendingReq *)Pending.GetData( 0 );

		Pending.Remove( req->Hostname );
		
		delete req;
	}
}
//---------------------------------------------------------------------------
void DNSCache::OnFork( void )
{
	for( int i = 0; i < Children.Count(); i++ ) {
		Resolver	*proc = (Resolver *)Children[ i ];

		proc->OnFork();

		delete proc;
	}

	Children.Clear();
	Clear();
}
//---------------------------------------------------------------------------
void DNSCache::Cache( const char *hostname, Prom_Addr *addr )
{
	Remove( &Items[ FreeSlot ]);

	Items[ FreeSlot ].Set( hostname, addr, TTL );

	Hash.Add( hostname, &Items[ FreeSlot ]);

	if( ++FreeSlot >= Slots )
		FreeSlot = 0;
}
//---------------------------------------------------------------------------
bool DNSCache::FindCached( const char *hostname, Prom_Addr *addr )
{
	bool	ret = false;
	DNSItem	*item;

	if( item = (DNSItem *)Hash.FindData( hostname )) {
		Prom_Addr	*sa = item->GetAddr();

		memcpy( addr, sa, sizeof( *sa ));

		ret = true;
	}

	return( ret );
}
//---------------------------------------------------------------------------
void DNSCache::Remove( DNSItem *item )
{
	if( char *host = item->GetHostname() ) {
		Hash.Remove( host );
		item->Clear();
	}
}
//---------------------------------------------------------------------------
void DNSCache::AsyncResolve( const char *hostname, Prom_Addr *addr,
							 Prom_DNS_Callback callback, void *userdata )
{
	if( FindCached( hostname, addr ))
		( *callback )( sizeof( *addr  ), userdata );

	else if( AddPendingRequest( hostname, addr, callback, userdata )) {
		Buffer					*req = new Buffer();
		struct ResolverRequest	*resreq;

		req->Resize( sizeof( struct ResolverRequest ) + strlen( hostname ) + 1 );

		resreq = (struct ResolverRequest *)req->GetData();

#if HAVE_IPV6
		resreq->Family = AF_INET6;
#else
		resreq->Family = AF_INET;
#endif
		strcpy( resreq->HostName, hostname );

		SendRequest( req, IPCCallback, this );
	}
}
//---------------------------------------------------------------------------
bool DNSCache::AddPendingRequest( const char *hostname, Prom_Addr *addr,
								  Prom_DNS_Callback callback, void *userdata )
{
	bool		send = false;
	PendingReq	*req = new PendingReq( hostname, addr, callback, userdata );
	PendingReq	*req2 = (PendingReq *)Pending.FindData( hostname );

	if( req2 ) {
		// there's another pending request for the same host
		// thus there's no need to ask again, just wait for the answer

		req->Next = req2;

		Pending.Replace( hostname, req );

	} else {

		Pending.Add( hostname, req );

		send = true;
	}

	return( send );
}
//---------------------------------------------------------------------------
void DNSCache::CancelRequest( const char *hostname, Prom_DNS_Callback callback, void *userdata )
{
	PendingReq	*req = (PendingReq *)Pending.FindData( hostname );
	PendingReq	*pred = NULL;

	// search for this request
	while( req && (( req->Callback != callback ) || ( req->Userdata != userdata ))) {
		pred = req;
		req  = req->Next;
	}

	if( req ) {

		if( pred )
			pred->Next = req->Next;
		else {

			if( req->Next )
				Pending.Replace( hostname, req->Next );
			else
				Pending.Remove( hostname );
		}

		req->Next = NULL; // otherwise it'll be deleted by the destructor

		delete req;
	}
}
//---------------------------------------------------------------------------
void DNSCache::IPCAnswer( void *answer, int len )
{
	PendingReq	*req;
	char		*host = (char *)answer + sizeof( int );

	Resolving = false;

	req = (PendingReq *)Pending.FindData( host );

	if( req ) {
		int 		addrlen = *((int *)answer );
		void		*addr = host + strlen( host ) + 1;
		PendingReq	*req2 = req;

		Pending.Remove( host );

		if( addrlen ) {
			bool			ok;
#if HAVE_IPV6
			struct in6_addr any = IN6ADDR_ANY_INIT;
#endif			
			memcpy( req->Addr, addr, addrlen );

#if HAVE_IPV6
			ok = memcmp( &any, addr, sizeof( any ));
#else
			ok = *((unsigned int *)req->Addr ) != INADDR_NONE;
#endif
			if( ok )
				Cache( host, req->Addr );
		}

		do {
			PendingReq	*next = req->Next;

			( *req->Callback )( addrlen, req->Userdata );

			req = next;

		} while( req );

		delete req2;

	} else
		App->Log->Log( LOG_ERR,
					   "DNS cache: unexpected message from child" );
}
static void IPCCallback( void *answer, int len, void *userdata )
{
	((DNSCache *)userdata )->IPCAnswer( answer, len );
}
//---------------------------------------------------------------------------
bool DNSCache::Resolve( const char *hostname, Prom_Addr *addr )
{
	Resolving = true;

	AsyncResolve( hostname, addr, ResolverCallback, this );

	while( Resolving )
		pause();

	return( ResolverStatus == 0 );
}
static void ResolverCallback( int status, void *userdata )
{
	((DNSCache *)userdata )->SetResolverStatus( status );
}
//---------------------------------------------------------------------------
void DNSCache::SetResolverStatus( int status )
{
	ResolverStatus = status;

	raise( SIGUSR1 );
}
//---------------------------------------------------------------------------
void DNSCache::ReloadCfg( void )
{
	int 	children = Children.Count(), oldslots = Slots, num;
	DNSItem	*newitems;

	if( App->Cfg->OpenKey( "root/DNSCache", false )) {

		children = App->Cfg->GetInteger( "children", children );
		TTL      = App->Cfg->GetInteger( "ttl", 300 );
		Slots    = App->Cfg->GetInteger( "slots", 500 );

		App->Cfg->CloseKey();
	}

	if( children < Children.Count() ) {
		Process *proc;

		while(( children < Children.Count() ) && ( proc = FindIdleProcess() )) {
			Children.Remove( proc );
			delete proc;
		}

	} else if( children > Children.Count() ) {

		Children.SetAllocBy( children - Children.Count() );

		while( children > Children.Count() ) {
			Resolver	*res = new Resolver();

			// XXX da qui torna a MainLoop() nel processo nuovo... why?
			if( res->Spawn( "resolver" ))
				AddChild( res );
			else
				App->Log->Log( LOG_ERR, "DNS cache: couldn't spawn a new process" );
		}
	}

	if( oldslots != Slots ) {

		FreeSlot = 0;
		newitems = new DNSItem[ Slots ];

		num = ( Slots > oldslots ) ? oldslots : Slots;

		for( int i = 0; i < num; i++ )
			if( char *host = Items[ i ].GetHostname() ) {

				newitems[ FreeSlot ] = Items[ i ];

				Hash.Replace( host, &newitems[ FreeSlot ]);

				FreeSlot++;
			}

		// remove the remaining items from the hash table
		for( ; num < oldslots; num++ )
			Remove( &Items[ num ]);

		if( FreeSlot >= Slots )
			FreeSlot = 0;

		delete[] Items;

		Items = newitems;
	}

	RunQueue();
}
//---------------------------------------------------------------------------
void DNSCache::Flush( time_t now )
{
	for( int i = 0; i < Slots; i++ ) {
		DNSItem	*item = &Items[ i ];

		if( item->GetHostname() && item->HasExpired( now ))
			Remove( item );
	}
}
//---------------------------------------------------------------------------
DNSItem::DNSItem()
{
	Hostname = NULL;
}
//---------------------------------------------------------------------------
DNSItem::~DNSItem()
{
	Clear();
}
//---------------------------------------------------------------------------
void DNSItem::Clear()
{
	free( Hostname );
	Hostname = NULL;
}
//---------------------------------------------------------------------------
void DNSItem::Set( const char *host, Prom_Addr *addr, unsigned int ttl )
{
	Hostname = strdup( host );
	Expire   = time( NULL ) + ttl;

	memcpy( &Addr, addr, sizeof( Addr ));
}
//---------------------------------------------------------------------------
bool DNSItem::HasExpired( time_t now )
{
	return( now > Expire );
}
//---------------------------------------------------------------------------
DNSItem& DNSItem::operator =( DNSItem& item )
{
	Hostname = item.Hostname ? strdup( item.Hostname ) : NULL;
	Expire   = item.Expire;

	memcpy( &Addr, &item.Addr, sizeof( Addr ));

	return( *this );
}
//---------------------------------------------------------------------------
