/***************************************************************************
                                pagemaker.cpp
                             -------------------
    revision             : $Id: pagemaker.cpp,v 1.6 2002-11-15 20:22:25 tellini Exp $
    copyright            : (C) 2002 by Simone Tellini
    email                : tellini@users.sourceforge.net

    description          : PageMaker engine wrapper
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
#include "buffer.h"
#include "mystring.h"
#include "loader.h"
#include "pagemaker.h"

#include <sdom.h>
#include <fstream>
#include <stdio.h>

static const struct {
		const char *link;
		const char *label;
} 	Pages[] = {
	{ "general", "General" },
	{ "acl",     "Access Control" },
	{ "dns",     "DNS Cache" },
	{ "mods",    "Modules" },
};

#define NUM_PAGES	( sizeof( Pages ) / sizeof( Pages[0] ))

//---------------------------------------------------------------------------
PageMaker::PageMaker( CfgData *cfg )
{
	Cfg    = cfg;
	DocDOM = NULL;

	SablotCreateProcessor( &Processor );
	SablotCreateSituation( &Situation );

	Doc = "<Manifest>";

	LoadFile( DATA_DIR"/core_options.xml" );

	Doc += "</Manifest>";
}
//---------------------------------------------------------------------------
PageMaker::~PageMaker()
{
	if( DocDOM )
		SablotDestroyDocument( Situation, DocDOM );

	SablotDestroySituation( Situation );
	SablotDestroyProcessor( Processor );
}
//---------------------------------------------------------------------------
void PageMaker::LoadFile( const char *str )
{
	ifstream	fh( str );

	if( fh ) {
		int	len;

		fh.seekg( 0, ios::end );
		len = fh.tellg();
		fh.seekg( 0, ios::beg );

		char *tmp = new char[ len + 1 ];

		fh.read( tmp, len );

		tmp[ len ] = '\0';

		Doc += tmp;

		delete[] tmp;

		fh.close();
	}
}
//---------------------------------------------------------------------------
void PageMaker::SetModuleManifest( const char *manifest )
{
	Doc  = "<Manifest>";
	Doc += manifest;
	Doc += "</Manifest>";
}
//---------------------------------------------------------------------------
bool PageMaker::ParseDoc( void )
{
	if( !DocDOM && ( SablotParseBuffer( Situation, Doc.c_str(), &DocDOM ) != SDOM_OK )) {
		if( DocDOM ) {
			SablotDestroyDocument( Situation, DocDOM );
			DocDOM = NULL;
		}
	}

	return( DocDOM != NULL );
}
//---------------------------------------------------------------------------
string PageMaker::ValueOf( const string query, SDOM_Node context )
{
	SDOM_NodeList	list;
	string			res;

	if( SDOM_xql( Situation, query.c_str(), context, &list ) == SDOM_OK ) {
		int num = 0;

		SDOM_getNodeListLength( Situation, list, &num );

		if( num >= 1 ) {
			SDOM_Node		node;
			SDOM_NodeType	type;
			SDOM_char		*str;

			SDOM_getNodeListItem( Situation, list, 0, &node );
			SDOM_getNodeType( Situation, node, &type );

			// our elements should only have one PCDATA child
			if( type == SDOM_ELEMENT_NODE )
				SDOM_getFirstChild( Situation, node, &node );

			if(( SDOM_getNodeValue( Situation, node, &str ) == SDOM_OK ) && str ) {

				res = str;

				SablotFree( str );
			}
		}

		SDOM_disposeNodeList( Situation, list );
	}

	return( res );
}
//---------------------------------------------------------------------------
StringList& PageMaker::GetOptions( const string& page )
{
	Options.Clear();

	if( ParseDoc() ) {
		string			query;

		query = "//Page[ @name = '" + page + "' ]/Option";

		GetOptionList( query, Options );
	}

	return( Options );
}
//---------------------------------------------------------------------------
void PageMaker::GetOptionList( const string query, StringList& list )
{
	SDOM_NodeList	options;

	if( SDOM_xql( Situation, query.c_str(), (SDOM_Node)DocDOM, &options ) == SDOM_OK ) {
		SDOM_NodeList	types;
		int				numopts = 0;

		SDOM_getNodeListLength( Situation, options, &numopts );

		for( int i = 0; i < numopts; i++ ) {
			SDOM_Node	option;

			SDOM_getNodeListItem( Situation, options, i, &option );

			list.Add( "%s|%s|%s|%s|%s|%s",
					  ValueOf( "@type", option ).c_str(),
					  ValueOf( "@name", option ).c_str(),
					  ValueOf( "Key/@name", option ).c_str(),
					  ValueOf( "Label", option ).c_str(),
					  ValueOf( "Descr", option ).c_str(),
					  ValueOf( "@default", option ).c_str() );
		}

		SDOM_disposeNodeList( Situation, options );
	}
}
//---------------------------------------------------------------------------
void PageMaker::AddPageHeader( const string page, string& result )
{
	result += "<!DOCTYPE html PUBLIC \"-//W3C//DTD HTML 4.01 Transitional//EN\">\n"
			  "<html>"
			  "  <head>"
			  "     <title> Prometeo - Administrative interface </title>"
			  "     <link rel=\"stylesheet\" type=\"text/css\" href=\"/files/style.css\">"
			  "  </head>"
			  "  <body>"
			  "    <table class=\"nav\" cellspacing=0>"
			  "      <tr>";

	for( int i = 0; i < NUM_PAGES; i++ ) {

		result += "<td";

		if( page == Pages[ i ].link )
			result += " class=\"curpage\"";

		result += "><a href=\"/";
		result += Pages[ i ].link;
		result += "\">";
		result += Pages[ i ].label;
		result += "</a></td>";
	}

	result += "        <td>"
			  "          <form action=\"/apply\" method=\"POST\">"
			  "            <input type=\"submit\" value=\"Apply changes\">"
			  "          </form>"
			  "        </td>"
			  "      </tr>"
			  "    </table>"
			  "    <form action=\"/" + page + "\" method=\"POST\">";
}
//---------------------------------------------------------------------------
void PageMaker::AddPageFooter( string& result, bool closeform )
{
	if( closeform )
		result += "      <div align=\"center\"><input type=\"submit\" value=\"Save\"></div>"
				  "    </form>";

	result += "    <div class=\"footer\">"
			  "      prometeo - mod_cfg<br>"
			  "      &copy;2002 by <a href=\"mailto:tellini@users.sourceforge.net\">Simone Tellini</a>"
			  "    </div>"
			  "  </body>"
			  "</html>";
}
//---------------------------------------------------------------------------
void PageMaker::BeginOptionsTable( const string caption, string& result )
{
	result += "<table class=\"options\" cellspacing=0>"
			  "  <tr><th colspan=2>" + caption + "</th></tr>"
			  "  <tr><td colspan=2 class=\"spacer\"></td></tr>";
}
//---------------------------------------------------------------------------
void PageMaker::EndOptionsTable( string& result )
{
	result += "</table>";
}
//---------------------------------------------------------------------------
void PageMaker::AddTextOption( const StringList& args, string& result, bool edit )
{
	string				value = args[ OP_DEFAULT ], key = args[ OP_KEY ];
	string::size_type	pos;

	pos = key.rfind( "/" );

	if( pos != string::npos ) {

		if( App->Cfg->OpenKey( key.substr( 0, pos - 1 ).c_str(), false )) {

			value = App->Cfg->GetString( key.substr( pos + 1 ).c_str(), value.c_str() );

			App->Cfg->CloseKey();
		}
	}

	result += "<tr>"
			  "  <td class=\"label\">" + string( args[ OP_LABEL ]) + "</td>"
			  "  <td class=\"value\">"
			  "    <input type=\"text\" name=\"" + string( args[ OP_NAME ]) + "\" value=\"" + value + "\"";

	if( !edit )
		result += " readonly";

	result += ">"
			  "  </td>"
			  "</tr>"
			  "<tr>"
			  "  <td colspan=\"2\" class=\"help\">" + string( args[ OP_DESCR ]) + "</td>"
			  "</tr>";
}
//---------------------------------------------------------------------------
void PageMaker::AddTextAreaOption( const StringList& args, string& result )
{
	string				value = args[ OP_DEFAULT ], key = args[ OP_KEY ];
	string::size_type	pos;

	pos = key.rfind( "/" );

	if( pos != string::npos ) {

		if( App->Cfg->OpenKey( key.substr( 0, pos - 1 ).c_str(), false )) {

			value = App->Cfg->GetString( key.substr( pos + 1 ).c_str(), value.c_str() );

			App->Cfg->CloseKey();
		}
	}

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

	pos = value.find( "\n" );

	while( pos != string::npos ) {

		value.insert( pos + 1, "<br>" );
		value.erase( pos, 1 );

		pos = value.find( "\n", pos );
	}

	result += "<tr>"
			  "  <td class=\"label\">" + string( args[ OP_LABEL ]) + "</td>"
			  "  <td class=\"value\">"
			  "    <textarea rows=5 name=\"" + string( args[ OP_NAME ]) + "\">" + value + "</textarea>"
			  "  </td>"
			  "</tr>"
			  "<tr>"
			  "  <td colspan=\"2\" class=\"help\">" + string( args[ OP_DESCR ]) + "</td>"
			  "</tr>";
}
//---------------------------------------------------------------------------
void PageMaker::AddIntegerOption( const StringList& args, string& result )
{
	string				value = args[ OP_DEFAULT ], key = args[ OP_KEY ];
	string::size_type	pos;

	pos = key.rfind( "/" );

	if( pos != string::npos ) {

		if( App->Cfg->OpenKey( key.substr( 0, pos - 1 ).c_str(), false )) {
			char	buf[32];

			sprintf( buf, "%d",
					 App->Cfg->GetInteger( key.substr( pos + 1 ).c_str(),
					 					   atoi( value.c_str() )));

			value = buf;

			App->Cfg->CloseKey();
		}
	}

	result += "<tr>"
			  "  <td class=\"label\">" + string( args[ OP_LABEL ]) + "</td>"
			  "  <td class=\"value\">"
			  "    <input type=\"text\" name=\"" + string( args[ OP_NAME ]) + "\" value=\"" + value + "\">"
			  "  </td>"
			  "</tr>"
			  "<tr>"
			  "  <td colspan=\"2\" class=\"help\">" + string( args[ OP_DESCR ]) + "</td>"
			  "</tr>";
}
//---------------------------------------------------------------------------
void PageMaker::AddBoolOption( const StringList& args, string& result )
{
	bool				value = atoi( args[ OP_DEFAULT ] );
	string				key = args[ OP_KEY ];
	string::size_type	pos;

	pos = key.rfind( "/" );

	if( pos != string::npos ) {

		if( App->Cfg->OpenKey( key.substr( 0, pos - 1 ).c_str(), false )) {

			value = App->Cfg->GetInteger( key.substr( pos + 1 ).c_str(), value );

			App->Cfg->CloseKey();
		}
	}

	result += "<tr>"
			  "  <td class=\"label\">" + string( args[ OP_LABEL ]) + "</td>"
			  "  <td class=\"value\">"
			  "    <input type=\"checkbox\" name=\"" + string( args[ OP_NAME ]) + "\"";

	if( value )
		result += " checked";

	result += ">"
			  "  </td>"
			  "</tr>"
			  "<tr>"
			  "  <td colspan=\"2\" class=\"help\">" + string( args[ OP_DESCR ]) + "</td>"
			  "</tr>";
}
//---------------------------------------------------------------------------
void PageMaker::AddListOption( const StringList& args, string& result, const string& page )
{
	string	url;

	url = "/listedit?page=" + UrlEncode( page ) + "&list=" + UrlEncode( args[ OP_NAME ]);

	result += "<tr>"
			  "  <td class=\"label\">" + string( args[ OP_LABEL ]) + "</td>"
			  "  <td class=\"value\">"
			  "    <a href=\"" + url + "\">Edit</a>"
			  "  </td>"
			  "</tr>"
			  "<tr>"
			  "  <td colspan=\"2\" class=\"help\">" + string( args[ OP_DESCR ]) + "</td>"
			  "</tr>";
}
//---------------------------------------------------------------------------
void PageMaker::AddOption( const string& page, const char *opt, string& result )
{
	StringList	tmp;

	tmp.Explode( opt, "|" );

	if( !strcmp( tmp[ OP_TYPE ], "string" ))
		AddTextOption( tmp, result, true );

	if( !strcmp( tmp[ OP_TYPE ], "string_noedit" ))
		AddTextOption( tmp, result, false );

	else if( !strcmp( tmp[ OP_TYPE ], "text" ))
		AddTextAreaOption( tmp, result );

	else if( !strcmp( tmp[ OP_TYPE ], "integer" ))
		AddIntegerOption( tmp, result );

	else if( !strcmp( tmp[ OP_TYPE ], "bool" ))
		AddBoolOption( tmp, result );

	else if( !strcmp( tmp[ OP_TYPE ], "list" ))
		AddListOption( tmp, result, page );
}
//---------------------------------------------------------------------------
void PageMaker::BuildPage( const string& page, string& result )
{
	if( page == "acl" )
		BuildAclPage( result );

	else if( page == "acl/user" )
		BuildAclUserPage( result );

	else if( page == "mods" )
		BuildModsPage( result );

	else if( page == "apply" )
		BuildApplyPage( result );

	else if( page == "listedit" )
		BuildListEditPage( result );

	else if( page == "listitem" )
		BuildListItemPage( result );

	else if( page == "listitem/del" )
		DeleteListItem( result );

	else if( ParseDoc() ) {
		string			query;
		SDOM_NodeList	queryres;
		string			page2 = page;

		if( page2.substr( 0, 4 ) == "mod/" )
			page2.erase( 0, 4 );

		query = "//Page[ @name = '" + page2 + "' ]";

		if( SDOM_xql( Situation, query.c_str(), (SDOM_Node)DocDOM, &queryres ) == SDOM_OK ) {
			SDOM_Node	node;
			int			num = 0;

			SDOM_getNodeListLength( Situation, queryres, &num );

			if(( num >= 1 ) &&
			   ( SDOM_getNodeListItem( Situation, queryres, 0, &node ) == SDOM_OK )) {
				StringList&	options = GetOptions( page2 );

				AddPageHeader( page, result );
				BeginOptionsTable( ValueOf( "Label", node ).c_str(), result );

				for( int i = 0; i < options.Count(); i++ )
					AddOption( page, options[ i ], result );

				EndOptionsTable( result );
				AddPageFooter( result );

			} else
				result = "Page '" + page2 + "' not found.";

			SDOM_disposeNodeList( Situation, queryres );

		} else
			result = "Page '" + page2 + "' not found.";

	} else
		result = "<html><body>Cannot parse the manifest!<pre>\n\n" + Doc + "</pre></body></html>";
}
//---------------------------------------------------------------------------
void PageMaker::BuildModsPage( string& result )
{
	StringList	args;

	AddPageHeader( "mods", result );
	BeginOptionsTable( "Modules administration", result );

	args.Add( "" );
	args.Add( "path" );
	args.Add( "root/Modules/path" );
	args.Add( "Modules path" );
	args.Add( "The directory containing your modules" );
	args.Add( MOD_DIR );

	AddTextOption( args, result, true );

	result += "<tr>"
			  "  <td colspan=2 align=\"center\">"
		      "    <input type=\"submit\" value=\"Save\">"
			  "    </form>"
			  "  </td>"
			  "</tr>";

	result += "<tr>"
			  "  <td colspan=2>"
			  "    <table class=\"modules\" width=\"100%\">"
			  "      <tr>"
			  "        <th> Name </th>"
			  "        <th> Module </th>"
			  "        <th> &nbsp; </th>"
			  "        <th> &nbsp; </th>"
			  "        <th> &nbsp; </th>"
			  "      </tr>";

	if( App->Cfg->OpenKey( "root/Modules", false )) {
		const char	*name;
		int			num = 0;

		while( name = App->Cfg->EnumKeys( num )) {
			string	file, en, run, mod, modnum;
			bool	enabled, running;
			char	tmp[32];

			sprintf( tmp, "%d", num );

			modnum = tmp;

			if( App->Cfg->OpenKey( name, false )) {

				file    = App->Cfg->GetString( "module", "" );
				enabled = App->Cfg->GetInteger( "enabled", true );

				App->Cfg->CloseKey();
			}

			mod     = name;
			running = App->Mods->IsRunning( name );

			en  = "<form action=\"/" + string( enabled ? "disable" : "enable" ) + "mod\" method=\"POST\">"
				  "  <input type=\"hidden\" name=\"mod\" value=\"" + mod + "\">"
				  "  <input type=\"submit\" value=\"" + string( enabled ? "DISABLE" : "ENABLE" ) + "\" class=\"maxwidth\">"
				  "</form>";

			run = "<form action=\"/" + string( running ? "unload" : "load" ) + "mod\" method=\"POST\">"
				  "  <input type=\"hidden\" name=\"mod\" value=\"" + mod + "\">"
				  "  <input type=\"submit\" value=\"" + string( running ? "UNLOAD" : "LOAD" ) + "\" class=\"maxwidth\">"
				  "</form>";

			result += "<tr>"
					  "  <td><a href=\"/mod/" + UrlEncode( mod ) + "\">" + mod + "</a></td>"
					  "  <td>" + file + "</td>"
					  "  <td align=\"center\">" + en + "</td>"
					  "  <td align=\"center\">" + run + "</td>"
					  "  <td align=\"center\">"
					  "    <form action=\"/deletemod\" method=\"POST\">"
					  "      <input type=\"hidden\" name=\"mod\" value=\"" + mod + "\">"
					  "      <input type=\"submit\" value=\"DELETE\" class=\"maxwidth\">"
					  "    </form>"
					  "  </td>"
					  "</tr>";

			num++;
		}

		App->Cfg->CloseKey();
	}

	result += "    </table>";

	result += 	"<form action=\"/addmod\" method=\"POST\">"
				"  <table class=\"modules\" width=\"100%\">"
				"    <tr>"
				"      <td><input type=\"text\" name=\"name\" class=\"maxwidth\"></td>"
				"      <td><input type=\"text\" name=\"file\" class=\"maxwidth\"></td>"
				"      <td>"
				"        Enable <input type=\"checkbox\" name=\"enabled\" checked>"
				"      </td>"
				"      <td><input type=\"submit\" value=\"ADD\" class=\"maxwidth\"></td>"
				"      <td> &nbsp; </td>"
				"    </tr>"
				"  </table>"
				"</form>";

	result += "  </td>"
			  "</tr>";

	EndOptionsTable( result );
	AddPageFooter( result, false );
}
//---------------------------------------------------------------------------
void PageMaker::BuildApplyPage( string& result )
{
	AddPageHeader( "apply", result );
	BeginOptionsTable( "Settings applied.", result );
	EndOptionsTable( result );
	AddPageFooter( result, false );
}
//---------------------------------------------------------------------------
void PageMaker::BuildAclPage( string& result )
{
	StringList	args;

	AddPageHeader( "acl", result );
	BeginOptionsTable( "Access control", result );

	result += "<tr>"
			  "  <td colspan=2>"
			  "    </form>"
			  "    <table class=\"acl\" width=\"100%\">"
			  "      <tr>"
			  "        <th> User </th>"
			  "        <th> &nbsp; </th>"
			  "        <th> &nbsp; </th>"
			  "      </tr>";

	if( App->Cfg->OpenKey( "root/ACL/Users", false )) {
		const char	*name;
		int			num = 0;

		while( name = App->Cfg->EnumKeys( num )) {

			result += "<tr>"
					  "  <td>" + string( name ) + "</td>"
					  "  <td align=\"center\">"
					  "    <form action=\"/acl/user\" method=\"POST\">"
					  "      <input type=\"hidden\" name=\"user\" value=\"" + name + "\">"
					  "      <input type=\"submit\" value=\"EDIT\" class=\"maxwidth\">"
					  "    </form>"
					  "  </td>"
					  "  <td align=\"center\">"
					  "    <form action=\"/acl/user/del\" method=\"POST\">"
					  "      <input type=\"hidden\" name=\"user\" value=\"" + name + "\">"
					  "      <input type=\"submit\" value=\"DELETE\" class=\"maxwidth\">"
					  "    </form>"
					  "  </td>"
					  "</tr>";

			num++;
		}

		App->Cfg->CloseKey();
	}

	result += 	"<form action=\"/acl/user/add\" method=\"POST\">"
				"  <tr>"
				"    <td><input type=\"text\" name=\"user\" class=\"maxwidth\"></td>"
				"    <td align=\"center\"><input type=\"submit\" value=\"ADD\" class=\"maxwidth\"></td>"
				"    <td> &nbsp; </td>"
				"  </tr>"
				"</form>";

	result += "  </td>"
			  "</tr>";

	EndOptionsTable( result );
	AddPageFooter( result, false );
}
//---------------------------------------------------------------------------
void PageMaker::BuildAclUserPage( string& result )
{
	string	user, key;
	 
	user = Cfg->DecodeArg( "user" );
	key  = "root/ACL/Users/" + user;

	AddPageHeader( "acl", result );
	BeginOptionsTable( "Access control - User: " + user, result );

	result += "<tr>"
			  "  <td colspan=2>"
			  "    </form>"
			  "    <table class=\"acl\" width=\"100%\">"
			  "      <tr>"
			  "        <th> Permission </th>"
			  "        <th> Granted? </th>"
			  "        <th> &nbsp; </th>"
			  "        <th> &nbsp; </th>"
			  "      </tr>";

	if( App->Cfg->OpenKey( key.c_str(), false )) {
		const char	*name;
		int			num = 0;

		while( name = App->Cfg->EnumValues( num )) {
			string	grant = App->Cfg->GetInteger( name, 0 ) ? "yes" : "no";

			result += "<tr>"
					  "  <td>" + string( name ) + "</td>"
					  "  <td align=\"center\">" + grant + "</td>"
					  "  <td align=\"center\">"
					  "    <form action=\"/acl/user/perm/toggle\" method=\"POST\">"
					  "      <input type=\"hidden\" name=\"user\" value=\"" + user + "\">"
					  "      <input type=\"hidden\" name=\"perm\" value=\"" + name + "\">"
					  "      <input type=\"submit\" value=\"TOGGLE\" class=\"maxwidth\">"
					  "    </form>"
					  "  </td>"
					  "  <td align=\"center\">"
					  "    <form action=\"/acl/user/perm/remove\" method=\"POST\">"
					  "      <input type=\"hidden\" name=\"user\" value=\"" + user + "\">"
					  "      <input type=\"hidden\" name=\"perm\" value=\"" + name + "\">"
					  "      <input type=\"submit\" value=\"REMOVE\" class=\"maxwidth\">"
					  "    </form>"
					  "  </td>"
					  "</tr>";

			num++;
		}

		App->Cfg->CloseKey();
	}

	result += 	"<form action=\"/acl/user/perm/add\" method=\"POST\">"
				"  <input type=\"hidden\" name=\"user\" value=\"" + user + "\">"
				"  <tr>"
				"    <td><input type=\"text\" name=\"perm\" class=\"maxwidth\"></td>"
				"    <td><input type=\"checkbox\" name=\"grant\" class=\"maxwidth\"></td>"
				"    <td><input type=\"submit\" value=\"ADD\" class=\"maxwidth\"></td>"
				"    <td> &nbsp; </td>"
				"  </tr>"
				"</form>";

	result += "  </td>"
			  "</tr>";

	EndOptionsTable( result );
	AddPageFooter( result, false );
}
//---------------------------------------------------------------------------
void PageMaker::BuildListEditPage( string& result )
{
	ListData	data;
	char		span[ 32 ];

	GetListData( data );

	AddPageHeader( "listedit", result );
	BeginOptionsTable( data.Descr, result );

	result += "</form><tr><td colspan=2><table width=\"100%\">";

	sprintf( span, "%d", AddListHeaders( data, result ));

	if( App->Cfg->OpenKey( data.BaseKey.c_str(), false )) {
		int num = 0;

		while( const char *item = App->Cfg->EnumKeys( num++ ))
			if( App->Cfg->OpenKey( item, false )) {

				AddListRow( item, data, result );

				App->Cfg->CloseKey();
			}

		App->Cfg->CloseKey();
	}

	result +=	"<tr>"
				"  <td align=\"center\" colspan=\"" + string( span ) + "\">"
				"    <form action=\"/listitem\" method=\"POST\">"
				"      <input type=\"hidden\" name=\"page\" value=\"" + data.Page + "\">"
				"      <input type=\"hidden\" name=\"list\" value=\"" + data.List + "\">"
				"      <input type=\"hidden\" name=\"" + data.KeyName + "\" value=\"\">"
				"      <input type=\"submit\" value=\"Add a new item\" class=\"maxwidth\">"
				"    </form>"
				"  </td>"
				"</tr>";

	result +=	"<tr>"
				"  <td align=\"center\" colspan=\"" + string( span ) + "\">"
				"    <form action=\"/" + data.Page + "\" method=\"GET\">"
				"      <input type=\"submit\" value=\"Back to the previous page\" class=\"maxwidth\">"
				"    </form>"
				"  </td>"
				"</tr>";

	result += "</table></td></tr>";

	EndOptionsTable( result );
	AddPageFooter( result, false );
}
//---------------------------------------------------------------------------
void PageMaker::GetListData( ListData& data )
{
	string	page2, query;

	data.Page = Cfg->DecodeArg( "page" );
	data.List = Cfg->DecodeArg( "list" );

	page2 = data.Page;

	if( page2.substr( 0, 4 ) == "mod/" )
		page2.erase( 0, 4 );

	StringList&	options = GetOptions( page2 );

	for( int i = 0; i < options.Count(); i++ ) {
		StringList	tmp;

		tmp.Explode( options[ i ], "|" );

		if( data.List == tmp[ OP_NAME ] ) {
			data.Descr   = tmp[ OP_DESCR ];
			data.BaseKey = tmp[ OP_KEY   ];
			break;
		}
	}

	data.Path = "//Page[ @name = '" + page2 + "' ]/Option[ @name = '" + data.List + "' ]/";
	query     = data.Path + "Fields/Option";

	data.KeyName  = ValueOf( data.Path + "ListKey/@name", (SDOM_Node)DocDOM );
	data.KeyLabel = ValueOf( data.Path + "ListKey/Label", (SDOM_Node)DocDOM );
	data.KeyDescr = ValueOf( data.Path + "ListKey/Descr", (SDOM_Node)DocDOM );

	GetOptionList( query, data.Fields );
}
//---------------------------------------------------------------------------
int PageMaker::AddListHeaders( const ListData& data, string& result )
{
	int columns = 3;

	result += 	"<tr>"
				"  <th>" + data.KeyLabel + "</th>";

	for( int i = 0; i < data.Fields.Count(); i++ ) {
		StringList	tmp;

		tmp.Explode( data.Fields[ i ], "|" );

		if( ValueOf( data.Path + "Fields/Option[ @name = '" +
					 string( tmp[ OP_NAME ] ) + "' ]/@show",
					 (SDOM_Node)DocDOM ) == "yes" ) {

			result += "<th>" + string( tmp[ OP_LABEL ] ) + "</th>";
			columns++;
		}
	}

	result += "  <th width=\"2%\">&nbsp;</th>"
			  "  <th width=\"2%\">&nbsp;</th>"
			  "</tr>";

	return( columns );
}
//---------------------------------------------------------------------------
void PageMaker::AddListRow( const char *item, const ListData& data, string& result )
{
	result +=	"<tr>"
				"  <td>" + string( item ) + "</td>";

	for( int i = 0; i < data.Fields.Count(); i++ ) {
		StringList	tmp;

		tmp.Explode( data.Fields[ i ], "|" );

		if( ValueOf( data.Path + "Fields/Option[ @name = '" +
					string( tmp[ OP_NAME ] ) + "' ]/@show",
					(SDOM_Node)DocDOM ) == "yes" ) {
			string value;

			if( !strcmp( tmp[ OP_TYPE ], "string" ))
				value = App->Cfg->GetString( tmp[ OP_KEY ], "" );

			else if( !strcmp( tmp[ OP_TYPE ], "integer" )) {
				char num[ 32 ];

				sprintf( num, "%d", App->Cfg->GetInteger( tmp[ OP_KEY ], 0 ));
				value = num;

			} else if( !strcmp( tmp[ OP_TYPE ], "bool" ))
				value = App->Cfg->GetInteger( tmp[ OP_KEY ], 0 ) ? "Yes" : "No";

			else if( !strcmp( tmp[ OP_TYPE ], "list" ))
				value = "(nested lists not supported yet, edit the config manually)";

			result += "<td>" + value + "</td>";
		}
	}

	result +=	"  <td>"
				"    <form action=\"listitem\" method=\"POST\">"
				"      <input type=\"hidden\" name=\"" + data.KeyName + "\" value=\"" + string( item ) + "\">"
				"      <input type=\"hidden\" name=\"page\" value=\"" + data.Page + "\">"
				"      <input type=\"hidden\" name=\"list\" value=\"" + data.List + "\">"
				"      <input type=\"submit\" value=\"Edit\" class=\"maxwidth\">"
				"    </form>"
				"  </td>"
				"  <td>"
				"    <form action=\"listitem/del\" method=\"POST\">"
				"      <input type=\"hidden\" name=\"item\" value=\"" + string( item ) + "\">"
				"      <input type=\"hidden\" name=\"page\" value=\"" + data.Page + "\">"
				"      <input type=\"hidden\" name=\"list\" value=\"" + data.List + "\">"
				"      <input type=\"submit\" value=\"Delete\" class=\"maxwidth\">"
				"    </form>"
				"  </td>"
				"</tr>";
}
//---------------------------------------------------------------------------
void PageMaker::BuildListItemPage( string& result )
{
	ListData	data;
	string		item, opt;

	GetListData( data );

	if( Cfg->DecodeArg( "save" ) == "1" )
		SaveListItem( data );

	AddPageHeader( "listitem", result );
	BeginOptionsTable( data.Descr, result );

	result +=	"</form>"
				"<form action=\"/listitem\" method=\"POST\">"
				"  <input type=\"hidden\" name=\"page\" value=\"" + data.Page + "\">"
				"  <input type=\"hidden\" name=\"list\" value=\"" + data.List + "\">"
				"  <input type=\"hidden\" name=\"save\" value=\"1\">";

	opt  = "string";
	item = Cfg->DecodeArg( data.KeyName.c_str() );

	if( !item.empty() )
		opt += "_noedit";

	// use a non-existant key, so it'll show the default (item)
	opt += "|" + data.KeyName + "||" +
		   data.KeyLabel + "|" + data.KeyDescr + "|" + item;

	AddOption( data.Page, opt.c_str(), result );

	for( int i = 0; i < data.Fields.Count(); i++ ) {
		StringList	args;

		args.Explode( data.Fields[ i ], "|" );
		args.Set( OP_KEY, data.BaseKey + "/" + item + "/" + string( args[ OP_KEY ]) );

		AddOption( data.Page, args.Implode( "|" ).c_str(), result );
	}

	EndOptionsTable( result );

	result += 	"      <div align=\"center\"><input type=\"submit\" value=\"Save\"></div>"
				"    </form>"
				"<form action=\"/listedit\" method=\"GET\">"
				"  <input type=\"hidden\" name=\"page\" value=\"" + data.Page + "\">"
				"  <input type=\"hidden\" name=\"list\" value=\"" + data.List + "\">"
				"  <input type=\"submit\" value=\"Back to the list page\">"
				"</form>";

	AddPageFooter( result, false );
}
//---------------------------------------------------------------------------
void PageMaker::DeleteListItem( string& result )
{
	ListData	data;
	string		key;

	GetListData( data );

	key = data.BaseKey + "/" + Cfg->DecodeArg( data.KeyName.c_str() );
	
	App->Cfg->DeleteKey( key.c_str() );
			
	BuildListEditPage( result );
}
//---------------------------------------------------------------------------
void PageMaker::SaveListItem( const ListData& data )
{
	string key = data.BaseKey + "/" + data.KeyName;

	for( int i = 0; i < data.Fields.Count(); i++ ) {
		StringList	args;

		args.Explode( data.Fields[ i ], "|" );

		Cfg->UpdateKey( key, args[ OP_KEY ], args[ OP_TYPE ],
						Cfg->DecodeArg( args[ OP_NAME ] ));
	}
}
//---------------------------------------------------------------------------
string PageMaker::UrlEncode( const string& str )
{
	string		ret;
	const char	*src;
	char		*dst, *tmp;

	src = str.c_str();
	tmp = new char[ ( str.length() * 3 ) + 1 ];
	dst = tmp;

	while( *src ) {
		char	ch = *src++;

		if((( ch >= '0' ) && ( ch <= '9' )) ||
		   (( ch >= 'A' ) && ( ch <= 'Z' )) ||
		   (( ch >= 'a' ) && ( ch <= 'z' )))
		   *dst++ = ch;
		else {

			sprintf( dst, "%%%02x", ch );

			dst += 3;
		}
	}

	*dst = '\0';
	ret  = tmp;

	delete[] tmp;

	return( ret );
}
//---------------------------------------------------------------------------
