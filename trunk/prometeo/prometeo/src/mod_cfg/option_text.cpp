/***************************************************************************
                               option_text.cpp
                             -------------------
    revision             : $Id: option_text.cpp,v 1.2 2002-11-21 18:36:55 tellini Exp $
    copyright            : (C) 2002 by Simone Tellini
    email                : tellini@users.sourceforge.net

    description          : text option class
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

#include "option_text.h"

//---------------------------------------------------------------------------
TextOption::TextOption( const string page ) : StringOption( page )
{
	Type = T_TEXT;
}
//---------------------------------------------------------------------------
void TextOption::Render( string& result )
{
	string				value = GetValue();
	string::size_type	pos;

	pos = value.find( "<" );

	while( pos != string::npos ) {

		value.insert( pos + 1, "&lt;" );
		value.erase( pos, 1 );

		pos = value.find( "<", pos );
	}

	pos = value.find( ">" );

	while( pos != string::npos ) {

		value.insert( pos + 1, "&gt;" );
		value.erase( pos, 1 );

		pos = value.find( ">", pos );
	}

	result += "<tr>"
			  "  <td class=\"label\">" + Label + "</td>"
			  "  <td class=\"value\">"
			  "    <textarea rows=5 name=\"" + Name + "\">" + value + "</textarea>"
			  "  </td>"
			  "</tr>"
			  "<tr>"
			  "  <td colspan=\"2\" class=\"help\">" + Descr + "</td>"
			  "</tr>";
}
//---------------------------------------------------------------------------
