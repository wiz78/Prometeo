/***************************************************************************
                                 option.cpp
                             -------------------
    revision             : $Id: option.cpp,v 1.1 2002-11-20 22:53:43 tellini Exp $
    copyright            : (C) 2002 by Simone Tellini
    email                : tellini@users.sourceforge.net

    description          : base option class
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "option.h"
#include "option_string.h"
#include "option_text.h"
#include "option_bool.h"
#include "option_integer.h"
#include "option_list.h"
#include "option_select.h"

//---------------------------------------------------------------------------
Option::Option( OptionType type, const string page, const string path )
{
	Type     = type;
	Page     = page;
	Path     = path;
	Editable = true;
}
//---------------------------------------------------------------------------
void Option::SetName( const string str )
{
	string				tmp = Page;
	string::size_type	pos, start = 0;

	Name = str;

	if( tmp.substr( 0, 4 ) == "mod/" )
		tmp.erase( 0, 4 );

	Path = "/Page[ @name = '" + tmp + "' ]/";

	tmp  = Name;
	pos  = tmp.find( "/" );

	while( pos != string::npos ) {

		Path += "Option[ @name = '" + tmp.substr( start, pos ) + "' ]/";

		start = pos + 1;
		pos   = tmp.find( "/", start );
	}

	Path += "Option[ @name = '" + tmp.substr( start ) + "' ]/";
}
//---------------------------------------------------------------------------
void Option::ReadParams( SablotSituation sit, SDOM_Node ctx )
{
	SetName( ValueOf( "@name", ctx, sit ));
	SetKey( ValueOf( "Key/@name", ctx, sit ));
	SetLabel( ValueOf( "Label", ctx, sit ));
	SetDescr( ValueOf( "Descr", ctx, sit ));
	SetDefault( ValueOf( "@default", ctx, sit ));
	SetEditable( ValueOf( "@editable", ctx, sit ) != "no" );
	SetVisible( ValueOf( "@show", ctx, sit ) != "no" );
}
//---------------------------------------------------------------------------
void Option::GetOptionList( SablotSituation sit, SDOM_Node ctx,
							const string query, List& Options, const string page )
{
	SDOM_NodeList	options;

	if( SDOM_xql( sit, query.c_str(), ctx, &options ) == SDOM_OK ) {
		SDOM_NodeList	types;
		int				numopts = 0;

		SDOM_getNodeListLength( sit, options, &numopts );

		Options.SetAllocBy( numopts );

		for( int i = 0; i < numopts; i++ ) {
			SDOM_Node	option;
			Option		*opt = NULL;
			string		type;

			SDOM_getNodeListItem( sit, options, i, &option );

			type = ValueOf( "@type", option, sit );

			if( type == "string" )
				opt = new StringOption( page );
			else if( type == "integer" )
				opt = new IntegerOption( page );
			else if( type == "bool" )
				opt = new BoolOption( page );
			else if( type == "text" )
				opt = new TextOption( page );
			else if( type == "list" )
				opt = new ListOption( page );
			else if( type == "select" )
				opt = new SelectOption( page );

			if( opt ) {

				opt->ReadParams( sit, option );

				Options.Add( opt );
			}
		}

		SDOM_disposeNodeList( sit, options );
	}
}
//---------------------------------------------------------------------------
