/***************************************************************************
                                 storage.cpp
                             -------------------
    revision             : $Id: storage.cpp,v 1.3 2002-10-30 14:48:50 tellini Exp $
    copyright            : (C) 2002 by Simone Tellini
    email                : tellini@users.sourceforge.net

    description          : this class handles a disk-based storage
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

#include <sys/stat.h>

#include "registry.h"
#include "storage.h"
#include "storeobj.h"

//---------------------------------------------------------------------------
Storage::Storage( const char *storeid ) : OpenHash( 127 )
{
	const char *spooldir = "/var/spool/prometeo";

	Size = 0;
	
	if( App->Cfg->OpenKey( "root/General", false )) {

		spooldir = App->Cfg->GetString( "spooldir", spooldir );

		App->Cfg->CloseKey();
	}

	mkdir( spooldir, 0700 );

	Directory  = spooldir;
	Directory += "/";
	Directory += storeid;

	mkdir( Directory.c_str(), 0700 );

	Directory += "/";
}
//---------------------------------------------------------------------------
Storage::~Storage()
{
	while( StoreObj *obj = (StoreObj *)OpenObjs.RemHead() ) {

		obj->FlushWriteBuffers();
			
		delete obj;
	}
}
//---------------------------------------------------------------------------
StoreObj *Storage::Open( const char *name, bool create, bool async )
{
	string		file = Directory + name;
	StoreObj	*obj = new StoreObj( this, file.c_str(), create, async );

	if( obj->IsValid() ) {

		OpenObjs.AddTail( obj );
		OpenHash.Add( file.c_str(), obj );

	} else {

		delete obj;
		obj = NULL;
	}

	return( obj );
}
//---------------------------------------------------------------------------
void Storage::Close( StoreObj *obj )
{
	OpenHash.Remove( obj->GetName() );

	if( !obj->Close() ) {

		obj->SetClosing();

		// move it to the head of the list, so we can
		// find it quickly later
		obj->Unlink();
		OpenObjs.AddHead( obj );

	} else
		delete obj;
}
//---------------------------------------------------------------------------
StoreObj *Storage::Find( const char *name )
{
	StoreObj *obj = (StoreObj *)OpenHash.FindData( name );

	if( obj )
		obj->Acquire();

	return( obj );
}
//---------------------------------------------------------------------------
void Storage::Delete( const char *name )
{
	string file = Directory + name;

	unlink( file.c_str() );
}
//---------------------------------------------------------------------------
void Storage::Flush( void )
{
	StoreObj	*obj = (StoreObj *)OpenObjs.GetHead();

	// keep walking the list until the first non-closing obj is found
	// we keep them grouped in the first part of the list
	while( OpenObjs.IsNode( obj ) && obj->IsClosing() ) {
		StoreObj	*next = (StoreObj *)obj->GetSucc();

		if( obj->Close() )
			delete obj;

		obj = next;
	}
}
//---------------------------------------------------------------------------
