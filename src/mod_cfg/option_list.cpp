/***************************************************************************
                               option_list.cpp
                             -------------------
    revision             : $Id: option_list.cpp,v 1.2 2002-11-21 18:36:55 tellini Exp $
    copyright            : (C) 2002 by Simone Tellini
    email                : tellini@users.sourceforge.net

    description          : list option class
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

#include "option_list.h"
#include "option_select.h"
#include "misc.h"

//---------------------------------------------------------------------------
ListOption::ListOption( const string page ) : Option( T_LIST, page )
{
}
//---------------------------------------------------------------------------
void ListOption::Render( string& result )
{
	string	url;

	url = "/listedit?page=" + UrlEncode( Page ) + "&list=" + UrlEncode( GetPathName() );

	result += "<tr>"
			  "  <td class=\"label\">" + Label + "</td>"
			  "  <td class=\"value\">"
			  "    <a href=\"" + url + "\">Edit</a>"
			  "  </td>"
			  "</tr>"
			  "<tr>"
			  "  <td colspan=\"2\" class=\"help\">" + Descr + "</td>"
			  "</tr>";
}
//---------------------------------------------------------------------------
void ListOption::ReadParams( SablotSituation sit, SDOM_Node ctx )
{
	Option::ReadParams( sit, ctx );

	KeyName  = ValueOf( "ListKey/@name", ctx, sit );
	KeyLabel = ValueOf( "ListKey/Label", ctx, sit );
	KeyDescr = ValueOf( "ListKey/Descr", ctx, sit );

	GetOptionList( sit, ctx, "Fields/Option", Fields, Page );

	for( int i = 0; i < Fields.Count(); i++ ) {
		Option	*opt = GetField( i );

		if( opt->GetType() == T_LIST )
			((ListOption *)opt )->SetParentName( GetPathName() );
	}
}
//---------------------------------------------------------------------------
Option *ListOption::GetField( int i )
{
	Option	*ret;

	try {
		ret = (Option *)Fields[ i ];
	}
	catch(...) {
		ret = NULL;
	}

	return( ret );
}
//---------------------------------------------------------------------------
void ListOption::PrintListTable( string& result )
{
	char	span[32];

	result += "<table width=\"100%\">";

	sprintf( span, "%d", AddListHeaders( result ));

	if( App->Cfg->OpenKey( Key.c_str(), false )) {
		int num = 0;

		while( const char *item = App->Cfg->EnumKeys( num++ ))
			if( App->Cfg->OpenKey( item, false )) {

				AddListRow( item, result );

				App->Cfg->CloseKey();
			}

		App->Cfg->CloseKey();
	}

	result +=	"<tr>"
				"  <td align=\"center\" colspan=\"" + string( span ) + "\">"
				"    <form action=\"/listitem\" method=\"POST\">"
				"      <input type=\"hidden\" name=\"page\" value=\"" + Page + "\">"
				"      <input type=\"hidden\" name=\"list\" value=\"" + GetPathName() + "\">"
				"      <input type=\"hidden\" name=\"" + KeyName + "\" value=\"\">"
				"      <input type=\"submit\" value=\"Add a new item\" class=\"maxwidth\">"
				"    </form>"
				"  </td>"
				"</tr>";

	result +=	"<tr>"
				"  <td align=\"center\" colspan=\"" + string( span ) + "\">"
				"    <form action=\"/" + Page + "\" method=\"GET\">"
				"      <input type=\"submit\" value=\"Back to the previous page\" class=\"maxwidth\">"
				"    </form>"
				"  </td>"
				"</tr>";

	result += "</table>";
}
//---------------------------------------------------------------------------
int ListOption::AddListHeaders( string& result )
{
	int columns = 3;

	result += 	"<tr>"
				"  <th>" + KeyLabel + "</th>";

	for( int i = 0; i < Fields.Count(); i++ ) {
		Option	*opt = (Option *)Fields[ i ];

		if( opt->IsVisible() ) {

			result += "<th>" + opt->GetLabel() + "</th>";
			columns++;
		}
	}

	result += "  <th width=\"2%\">&nbsp;</th>"
			  "  <th width=\"2%\">&nbsp;</th>"
			  "</tr>";

	return( columns );
}
//---------------------------------------------------------------------------
void ListOption::AddListRow( const char *item, string& result )
{
	result +=	"<tr>"
				"  <td>" + string( item ) + "</td>";

	for( int i = 0; i < Fields.Count(); i++ ) {
		Option	*opt = (Option *)Fields[ i ];

		if( opt->IsVisible() ) {
			string		value;
			const char	*key = opt->GetKey().c_str();

			switch( opt->GetType() ) {

				case T_STRING:
				case T_TEXT:
					value = App->Cfg->GetString( key, "" );
					break;

				case T_INTEGER: {
					char num[ 32 ];

					sprintf( num, "%d", App->Cfg->GetInteger( key, 0 ));
					value = num;
				}	break;

				case T_BOOL:
					value = App->Cfg->GetInteger( key, 0 ) ? "Yes" : "No";
					break;

				case T_LIST:
					value = "<a href=\"/listedit?page=" + UrlEncode( Page ) +
							"&list=" + UrlEncode(((ListOption *)opt )->GetPathName() ) + 
							"\">edit this list</a>";
					break;

				case T_SELECT:
					value = ((SelectOption *)opt )->GetValueLabel( App->Cfg->GetString( key, "" ));
					break;

				default:
					value = "(datatype not supported yet here, edit the config manually)";
					break;
			}

			result += "<td>" + value + "</td>";
		}
	}

	result +=	"  <td>"
				"    <form action=\"listitem\" method=\"POST\">"
				"      <input type=\"hidden\" name=\"" + KeyName + "\" value=\"" + string( item ) + "\">"
				"      <input type=\"hidden\" name=\"page\" value=\"" + Page + "\">"
				"      <input type=\"hidden\" name=\"list\" value=\"" + GetPathName() + "\">"
				"      <input type=\"submit\" value=\"Edit\" class=\"maxwidth\">"
				"    </form>"
				"  </td>"
				"  <td>"
				"    <form action=\"listitem/del\" method=\"POST\">"
				"      <input type=\"hidden\" name=\"item\" value=\"" + string( item ) + "\">"
				"      <input type=\"hidden\" name=\"page\" value=\"" + Page + "\">"
				"      <input type=\"hidden\" name=\"list\" value=\"" + GetPathName() + "\">"
				"      <input type=\"submit\" value=\"Delete\" class=\"maxwidth\">"
				"    </form>"
				"  </td>"
				"</tr>";
}
//---------------------------------------------------------------------------
ListOption *ListOption::FindList( string name )
{
	ListOption *ret = NULL;

	for( int i = 0; !ret && ( i < Fields.Count() ); i++ ) {
		ListOption	*opt = (ListOption *)Fields[ i ];

		if( opt->GetType() == T_LIST ) {

			if( opt->GetPathName() == name )
				ret = opt;
			else
				ret = opt->FindList( name );
		}
	}

	return( ret );
}
//---------------------------------------------------------------------------
