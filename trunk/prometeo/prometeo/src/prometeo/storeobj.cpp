/***************************************************************************
                                storeobj.cpp
                             -------------------
    revision             : $Id: storeobj.cpp,v 1.3 2002-10-29 18:01:16 tellini Exp $
    copyright            : (C) 2002 by Simone Tellini
    email                : tellini@users.sourceforge.net

    description          : this class handles a disk-based storage object
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include <errno.h>
#include <sys/stat.h>

#include "main.h"
#include "storeobj.h"
#include "storage.h"

#define PROM_LOAD_CHUNK_SIZE	(2 * 1024)

//---------------------------------------------------------------------------
StoreObj::StoreObj( Storage *store, const char *name, bool create, bool async )
{
	int					flags = async ? PROM_FILEF_NONBLOCK : 0;
	string::size_type	pos;

	RefCount       = 1;
	Store          = store;
	AvailSize      = 0;
	ReadReq        = 0;
	ErrorCallback  = NULL;
	Name           = name;
	flags         |= create ? PROM_FILEF_WRITE : PROM_FILEF_READ;

	UseDispatcher( App->IO );

	if( create ) // signal that this obj is receiving data
		StoreObj::Flags.Set( PROM_STOREF_READING );

	if( !Open( name, flags ) &&
		(( pos = Name.find( "/" )) != string::npos )) {
		string	dir, rest = Name;

		// try creating the directory where the file should reside
		do {

			dir += rest.substr( 0, pos );
			rest = rest.substr( pos + 1 );

			// don't care for the return code: a path part might
			// already exists, so some mkdir()'s might fail
			mkdir( dir.c_str(), 0700 );

			pos  = rest.find( "/" );
			dir += "/";

		} while( pos != string::npos );

		Open( name, flags );
	}
}
//---------------------------------------------------------------------------
StoreObj::~StoreObj()
{
	while( Listener *lis = (Listener *)Listeners.RemTail() )
		delete lis;

	Close();
}
//---------------------------------------------------------------------------
void StoreObj::Callback( Prom_FC_Reason reason, int data )
{
	switch( reason ) {

		case PROM_FILE_READ:
			Acquire();
			NotifyListeners( Data.GetData() + AvailSize, data );

			AvailSize += data;

			if( ReadReq > 0 ) {

				ReadReq -= data;

				if( ReadReq > 0 )
					Read( Data.GetData() + AvailSize, ReadReq );
				else
					NotifyListeners( NULL, 0 ); // EOF
			}

			Release();
			break;

		case PROM_FILE_ERROR:
			if( ErrorCallback )
				switch( data ) {

					case ENOSPC:
						( *ErrorCallback )( this, PROM_STORE_FULL, ErrorUserdata );
						break;

					default:
						( *ErrorCallback )( this, PROM_STORE_ERROR, ErrorUserdata );
						break;
				}
			break;
	}
}
//---------------------------------------------------------------------------
void StoreObj::Load( void )
{
	ReadReq = File::GetSize();

	if( ReadReq > 0 ) {

		AvailSize = 0;

		Data.Clear();
		Data.Resize( ReadReq );

		Read( Data.GetData(), ReadReq );

	} else
		NotifyListeners( NULL, 0 ); // EOF
}
//---------------------------------------------------------------------------
void StoreObj::Write( const void *data, int len )
{
	Acquire();

	Data.Append((char *)data, len );

	NotifyListeners( Data.GetData() + AvailSize, len );

	AvailSize += len;

	if( !StoreObj::Flags.IsSet( PROM_STOREF_DONT_WRITE )) {

		File::Write( data, len );

		Store->UpdateSize( len );
	}

	Release();
}
//---------------------------------------------------------------------------
void StoreObj::Write( const string& str )
{
	unsigned int	len = str.length();
	char			*ptr;

	Acquire();

	Data.Append((char *)&len, sizeof( len ));
	Data.Append( str.c_str(), len );

	len       += sizeof( len );
	ptr        = Data.GetData() + AvailSize;
	AvailSize += len;

	NotifyListeners( ptr, len );

	if( !StoreObj::Flags.IsSet( PROM_STOREF_DONT_WRITE )) {

		File::Write( ptr, len );

		Store->UpdateSize( len );
	}

	Release();
}
//---------------------------------------------------------------------------
bool StoreObj::Release( void )
{
	bool del;

	if( RefCount > 0 )
		--RefCount;

	del = RefCount <= 0;

	if( del )
		Store->Close( this );

	return( del );
}
//---------------------------------------------------------------------------
void StoreObj::SetErrorCallback( Prom_Storage_Callback callback, void *userdata )
{
	ErrorCallback = callback;
	ErrorUserdata = userdata;
}
//---------------------------------------------------------------------------
void StoreObj::AddListener( Prom_Listener callback, void *userdata )
{
	Listener	*lis = new Listener();

	lis->Callback = callback;
	lis->Userdata = userdata;

	Listeners.AddTail( lis );
}
//---------------------------------------------------------------------------
void StoreObj::RemListener( Prom_Listener callback, void *userdata )
{
	Listener	*lis = (Listener *)Listeners.GetHead();

	while( Listeners.IsNode( lis )) {

		if(( lis->Callback == callback ) && ( lis->Userdata == userdata )) {

			delete lis;

			break;
		}

		lis = (Listener *)lis->GetSucc();
	}
}
//---------------------------------------------------------------------------
void StoreObj::NotifyListeners( char *data, int len )
{
	Listener	*lis = (Listener *)Listeners.GetHead();

	// prevent from being freed by a listener
	Acquire();

	while( Listeners.IsNode( lis )) {
		Listener	*next = (Listener *)lis->GetSucc();

		( *lis->Callback )( lis->Userdata, data, len );

		lis = next;
	}

	Release();
}
//---------------------------------------------------------------------------
void StoreObj::WriteComplete( bool ok )
{
	StoreObj::Flags.Clear( PROM_STOREF_READING );

	NotifyListeners( NULL, ok ? 0 : -1 );
}
//---------------------------------------------------------------------------
