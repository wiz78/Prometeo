/***************************************************************************
                                filtermgr.cpp
                             -------------------
    revision             : $Id: filtermgr.cpp,v 1.1 2002-11-21 18:39:42 tellini Exp $
    copyright            : (C) 2002 by Simone Tellini
    email                : tellini@users.sourceforge.net

    description          : filter URL's
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
#include "filtermgr.h"
#include "filter.h"

//---------------------------------------------------------------------------
FilterMgr::FilterMgr( const string key )
{
	Key = key;
}
//---------------------------------------------------------------------------
FilterMgr::~FilterMgr()
{
	Clear();
}
//---------------------------------------------------------------------------
void FilterMgr::Clear( void )
{
	for( int i = 0; i < Filters.Count(); i++ )
		delete (Filter *)Filters[ i ];

	Filters.Clear();
}
//---------------------------------------------------------------------------
void FilterMgr::Load( void )
{
	Clear();

	if( App->Cfg->OpenKey( Key.c_str(), false )) {
		int i = 0;

		while( const char *filter = App->Cfg->EnumKeys( i++ ))
			Filters.Add( new Filter( filter ));

		App->Cfg->CloseKey();
	}
}
//---------------------------------------------------------------------------
Filter *FilterMgr::Match( const char *url )
{
	Filter	*ret = NULL;

	for( int i = 0; !ret && ( i < Filters.Count() ); i++ ) {
		Filter *fil = (Filter *)Filters[ i ];

		if( fil->Match( url ))
			ret = fil;
	}

	return( ret );
}
//---------------------------------------------------------------------------
