/***************************************************************************
                                 filter.cpp
                             -------------------
    revision             : $Id: filter.cpp,v 1.1 2002-11-21 18:39:42 tellini Exp $
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

#include <regex.h>

#include "registry.h"
#include "stringlist.h"
#include "filter.h"

//---------------------------------------------------------------------------
Filter::Filter( const char *regkey )
{
	Action = F_NOOP;

	if( App->Cfg->OpenKey( regkey, false )) {
		string	tmp;

		PrepareURLs( App->Cfg->GetString( "url", "" ));

		tmp = App->Cfg->GetString( "action", "noaction" );

		if( tmp == "nocache" )
			Action = F_DONT_CACHE;
		else if( tmp == "forbid" )
			Action = F_FORBID;
		else if( tmp == "redirect" )
			Action = F_REDIRECT;

		switch( Action ) {

			case F_REDIRECT:
				Target = App->Cfg->GetString( "redirect", "" );
				break;
		}

		App->Cfg->CloseKey();
	}
}
//---------------------------------------------------------------------------
Filter::~Filter()
{
	for( int i = 0; i < URLs.Count(); i++ ) {
		regex_t *reg = (regex_t *)URLs[ i ];

		regfree( reg );

		delete reg;
	}
}
//---------------------------------------------------------------------------
void Filter::PrepareURLs( string urls )
{
	string::size_type	pos;
	StringList			list;

	pos = urls.length();

	while(( pos = urls.rfind( "\r", pos )) != string::npos )
		urls.erase( pos, pos + 1 );

	list.Explode( urls, "\n" );

	for( int i = 0; i < list.Count(); i++ ) {
		regex_t *reg = new regex_t;
		int		err;

		err = regcomp( reg, list[ i ], REG_EXTENDED | REG_ICASE | REG_NOSUB );

		if( !err )
			URLs.Add( reg );
		else {
			char	errstr[2048];

			regerror( err, reg, errstr, sizeof( errstr ));

			App->Log->Log( LOG_ERR,
						"mod_http: wrong regular expression among filters (%s - %s)",
						list[ i ], errstr );

			delete reg;
		}
	}
}
//---------------------------------------------------------------------------
bool Filter::Match( const char *url )
{
	bool	ret = false;

	for( int i = 0; !ret && ( i < URLs.Count() ); i++ )
		if( regexec((regex_t *)URLs[ i ], url, 0, NULL, 0 ) != REG_NOMATCH )
			ret = true;

	return( ret );
}
//---------------------------------------------------------------------------
