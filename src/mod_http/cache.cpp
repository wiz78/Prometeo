/***************************************************************************
                                  cache.cpp
                             -------------------
    revision             : $Id: cache.cpp,v 1.4 2002-10-22 17:43:22 tellini Exp $
    copyright            : (C) 2002 by Simone Tellini
    email                : tellini@users.sourceforge.net

    description          : this class handles the disk-based cache for
                           mod_http
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include <stdio.h>

#include "main.h"
#include "cache.h"
#include "md5.h"

#define INDEX_VERSION	0

//---------------------------------------------------------------------------
Cache::Cache() : Store( "mod_http" ), Hash( 4019 )
{
	MaxSize  = 500 * 1024 * 1024;
	Modified = false;

	LoadIndex();
}
//---------------------------------------------------------------------------
Cache::~Cache()
{
	while( CacheObj *obj = (CacheObj *)Objects.RemTail() )
		delete obj;
}
//---------------------------------------------------------------------------
CacheObj *Cache::Create( const char *id )
{
	unsigned int	key = 0;
	CacheObj		*obj = NULL;
	StoreObj		*sto;
	unsigned char	*ptr = (unsigned char *)&key;
	char			buf[ 46 ];
	const char		*str = id;
	unsigned int	md5[4];

	// use MD5 to be sufficiently sure that 2 different ids with
	// the same hash key won't collide on the same file
	CalcMD5( id, strlen( id ), md5 );

	// hash the filename, we use this key to decide where to put the file
	while( *str )
		key = ( key << 5 ) + *str++;
	
	// spread the files among a 4-levels deep tree to
	// limit the length of each directory and hopefully
	// avoid the filesystem to slow things down too much
	sprintf( buf, "%02x/%02x/%02x/%02x/%08x%08x%08x%08x",
			 ptr[0], ptr[1], ptr[2], ptr[3],
			 md5[0], md5[1], md5[2], md5[3] );

	sto = Store.Open( buf, true, true );

	if( sto ) {

		obj = new CacheObj( this, sto, buf );

		obj->SetID( id );

		Hash.Add( id, obj );
		Objects.AddTail( obj );

		Modified = true;
	}

	return( obj );
}
//---------------------------------------------------------------------------
CacheObj *Cache::Find( const char *id )
{
	CacheObj *obj = (CacheObj *)Hash.FindData( id );

	if( obj ) {

		obj->Acquire();
		obj->SetLastAccess();

		// mantain the LRU objects in the first part of the list
		obj->Unlink();
		Objects.AddTail( obj );
	}

	return( obj );
}
//---------------------------------------------------------------------------
void Cache::Delete( const char *id )
{
	CacheObj	*obj = (CacheObj *)Hash.FindData( id );

	if( obj )
		Delete( obj );
}
//---------------------------------------------------------------------------
void Cache::Delete( CacheObj *obj )
{
	if( !obj->IsDeleted() ) {

		Store.Delete( obj->GetFileName() );
		Store.UpdateSize( -obj->GetSize() );

		Hash.Remove( obj->GetID() );

		obj->Delete();

		Modified = true;
	}
}
//---------------------------------------------------------------------------
bool Cache::Prune( void )
{
	bool	deleted = false;

	DBG( App->Log->Log( LOG_INFO, "mod_http: cache size = %lld, MaxSize = %lld", Store.GetSize(), MaxSize ));

	if( Store.GetSize() > MaxSize ) {
		CacheObj	*obj = (CacheObj *)Objects.GetHead();
		bool		loop;

		do {

			// stop looping when no item can be deleted,
			// even if MaxSize is exceeded
			loop = false;

			while( Objects.IsNode( obj ) && ( obj->IsInUse() || obj->IsDeleted() ))
				obj = (CacheObj *)obj->GetSucc();

			DBG( App->Log->Log( LOG_INFO, "obj = %08x", obj ));
			
			if( Objects.IsNode( obj )) {
				CacheObj	*next = (CacheObj *)obj->GetSucc();

				DBG( App->Log->Log( LOG_INFO, "deleting %08x", obj ));
				Delete( obj );
				DBG( App->Log->Log( LOG_INFO, "next = %08x", next ));

				deleted = true;
				loop    = true;
				obj     = next;
			}

		} while( loop && ( Store.GetSize() > MaxSize ));
	}
	
	DBG( App->Log->Log( LOG_INFO, "mod_http: Cache::Prune() returns %d", deleted ));
	
	return( deleted );
}
//---------------------------------------------------------------------------
void Cache::LoadIndex( void )
{
	StoreObj *sto = Store.Open( "cache.idx", false, false );

	if( sto ) {
		int	ver;

		sto->Read( &ver, sizeof( ver ));

		if( ver <= INDEX_VERSION ) {
			int num;

			sto->Read( &num, sizeof( num ));

			while( num-- > 0 ) {
				CacheObj *obj = new CacheObj( this );

				obj->LoadFromIdx( sto, ver );

				Hash.Add( obj->GetID(), obj );
				Objects.AddTail( obj );

				Store.UpdateSize( obj->GetSize() );
			}

		} else
			App->Log->Log( LOG_ERR, "mod_http: cannot load the cache index: unsupported version." );

		Store.Close( sto );
	}

	DBG( App->Log->Log( LOG_INFO, "mod_http: Cache::LoadIndex() - loaded %d items, size = %lld", 
						Objects.Count(), Store.GetSize() ));
}
//---------------------------------------------------------------------------
void Cache::WriteIndex( void )
{
	if( Modified ) {
		StoreObj	*sto = Store.Open( "cache.idx", true, false );
		
		if( sto ) {
			CacheObj	*obj = (CacheObj *)Objects.GetHead();
			int			ver = INDEX_VERSION, num = 0;
			bool		allsaved = true;

			while( Objects.IsNode( obj )) {

				if( obj->IsComplete() && !obj->IsDeleted() )
					num++;
				else
					allsaved = false;

				obj = (CacheObj *)obj->GetSucc();
			}

			sto->Write( &ver, sizeof( ver ));
			sto->Write( &num, sizeof( num ));

			obj = (CacheObj *)Objects.GetHead();

			while( Objects.IsNode( obj )) {

				if( obj->IsComplete() && !obj->IsDeleted() )
					obj->SaveToIdx( sto );

				obj = (CacheObj *)obj->GetSucc();
			}

			sto->FlushWriteBuffers();

			Store.Close( sto );

			if( allsaved )
				Modified = false;

		} else
			App->Log->Log( LOG_ERR, "mod_http: failed to create the cache index file - %s", strerror( errno ));
	}
}
//---------------------------------------------------------------------------

