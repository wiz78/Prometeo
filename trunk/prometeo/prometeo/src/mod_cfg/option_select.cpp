/***************************************************************************
                              option_select.cpp
                             -------------------
    revision             : $Id: option_select.cpp,v 1.1 2002-11-20 22:53:43 tellini Exp $
    copyright            : (C) 2002 by Simone Tellini
    email                : tellini@users.sourceforge.net

    description          : select option class
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

#include "option_select.h"
#include "misc.h"

//---------------------------------------------------------------------------
SelectOption::SelectOption( const string page ) : StringOption( page )
{
	Type     = T_SELECT;
	NumItems = 0;
	Items    = NULL;
}
//---------------------------------------------------------------------------
SelectOption::~SelectOption()
{
	delete[] Items;
}
//---------------------------------------------------------------------------
void SelectOption::Render( string& result )
{
	string	value = GetValue();

	result += "<tr>"
			  "  <td class=\"label\">" + Label + "</td>"
			  "  <td class=\"value\">"
			  "    <select name=\"" + Name + "\">";

	for( int i = 0; i < NumItems; i++ ) {
		SelectItem	*item = &Items[ i ];
		bool		sel = item->Value == value;

		result += "<option value=\"" + item->Value + "\"";

		if( sel )
			result += " selected";

		result += ">" + item->Label + "</option>";
	}

	result += "    </select>"
			  "  </td>"
			  "</tr>"
			  "<tr>"
			  "  <td colspan=\"2\" class=\"help\">" + Descr +
			  "</tr>";
}
//---------------------------------------------------------------------------
void SelectOption::ReadParams( SablotSituation sit, SDOM_Node ctx )
{
	SDOM_NodeList	list;

	Option::ReadParams( sit, ctx );

	if( SDOM_xql( sit, "Items/Item", ctx, &list ) == SDOM_OK ) {
		int num = 0;

		SDOM_getNodeListLength( sit, list, &num );

		Items    = new SelectItem[ num ];
		NumItems = num;

		for( int i = 0; i < num; i++ ) {
			SDOM_Node	node;
			SelectItem	*item = &Items[ i ];

			SDOM_getNodeListItem( sit, list, i, &node );

			item->Value = ValueOf( "@value", node, sit );
			item->Label = ValueOf( ".", node, sit );
		}

		SDOM_disposeNodeList( sit, list );
	}
}
//---------------------------------------------------------------------------
