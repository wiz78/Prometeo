/***************************************************************************
                               option_bool.cpp
                             -------------------
    revision             : $Id: option_bool.cpp,v 1.1 2002-11-20 22:53:43 tellini Exp $
    copyright            : (C) 2002 by Simone Tellini
    email                : tellini@users.sourceforge.net

    description          : boolean option class
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

#include "option_bool.h"

//---------------------------------------------------------------------------
BoolOption::BoolOption( const string page ) : IntegerOption( page )
{
	Type = T_BOOL;
}
//---------------------------------------------------------------------------
void BoolOption::Render( string& result )
{
	result += "<tr>"
			  "  <td class=\"label\">" + Label + "</td>"
			  "  <td class=\"value\">"
			  "    <input type=\"checkbox\" name=\"" + Name + "\"";

	if( GetValue() )
		result += " checked";

	result += ">"
			  "  </td>"
			  "</tr>"
			  "<tr>"
			  "  <td colspan=\"2\" class=\"help\">" + Descr + "</td>"
			  "</tr>";
}
//---------------------------------------------------------------------------
