/***************************************************************************
                               hostmapper.cpp
                             -------------------
    revision             : $Id: hostmapper.cpp,v 1.1 2002-11-18 17:46:35 tellini Exp $
    copyright            : (C) 2002 by Simone Tellini
    email                : tellini@users.sourceforge.net

    description          : map hostnames to a host/port pair
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
#include "registry.h"
#include "hostmapper.h"

//---------------------------------------------------------------------------
HostMapper::HostMapper( const string key ) : HostMap( 127 )
{
	Key = key;
}
//---------------------------------------------------------------------------
HostMapper::~HostMapper()
{
	Clear();
}
//---------------------------------------------------------------------------
void HostMapper::Clear( void )
{
	for( int i = 0; i < HostMap.Count(); i++ )
		delete (Host *)HostMap.GetData( i );

	HostMap.Clear();
}
//---------------------------------------------------------------------------
void HostMapper::ReloadCfg( void )
{
	if( App->Cfg->OpenKey( Key.c_str(), false )) {
		int	i = 0;

		Clear();

		while( const char *host = App->Cfg->EnumKeys( i++ ))
			if( App->Cfg->OpenKey( host, false )) {

				HostMap.Add( host,
							 new Host( App->Cfg->GetString( "target", host ),
							 		   App->Cfg->GetInteger( "port", 80 )));

				App->Cfg->CloseKey();
			}

		App->Cfg->CloseKey();
	}
}
//---------------------------------------------------------------------------
const char	*HostMapper::Map( const char *host, short *port )
{
	const char	*ret = host;
	Host		*h = (Host *)HostMap.FindData( host );

	if( h ) {
		ret   = h->GetHost().c_str();
		*port = h->GetPort();
	}

	return( ret );
}
//---------------------------------------------------------------------------
