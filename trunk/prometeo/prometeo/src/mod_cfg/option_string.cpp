/***************************************************************************
                              option_string.cpp
                             -------------------
    revision             : $Id: option_string.cpp,v 1.1 2002-11-20 22:53:43 tellini Exp $
    copyright            : (C) 2002 by Simone Tellini
    email                : tellini@users.sourceforge.net

    description          : string option class
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

#include "option_string.h"

//---------------------------------------------------------------------------
StringOption::StringOption( const string page ) : Option( T_STRING, page )
{
}
//---------------------------------------------------------------------------
void StringOption::Render( string& result )
{
	result += "<tr>"
			  "  <td class=\"label\">" + Label + "</td>"
			  "  <td class=\"value\">"
			  "    <input type=\"text\" name=\"" + Name + "\" value=\"" + GetValue() + "\"";

	if( !IsEditable() )
		result += " readonly";

	result += ">"
			  "  </td>"
			  "</tr>"
			  "<tr>"
			  "  <td colspan=\"2\" class=\"help\">" + Descr + "</td>"
			  "</tr>";
}
//---------------------------------------------------------------------------
string StringOption::GetValue( void )
{
	string				value = Default, key = Key;
	string::size_type	pos;

	pos = key.rfind( "/" );

	if( pos != string::npos ) {

		if( App->Cfg->OpenKey( key.substr( 0, pos - 1 ).c_str(), false )) {

			value = App->Cfg->GetString( key.substr( pos + 1 ).c_str(), value.c_str() );

			App->Cfg->CloseKey();
		}
	}

	return( value );
}
//---------------------------------------------------------------------------
