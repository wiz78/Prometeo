/***************************************************************************
                                pagemaker.cpp
                             -------------------
    revision             : $Id: pagemaker.cpp,v 1.2 2002-11-07 14:49:40 tellini Exp $
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
		SDOM_NodeList	options;

		query = "//Page[ @name = '" + page + "' ]/Option";

		if( SDOM_xql( Situation, query.c_str(), (SDOM_Node)DocDOM, &options ) == SDOM_OK ) {
			SDOM_NodeList	types;
			int				numopts = 0;

			SDOM_getNodeListLength( Situation, options, &numopts );

			for( int i = 0; i < numopts; i++ ) {
				SDOM_Node	option;

				SDOM_getNodeListItem( Situation, options, i, &option );

				Options.Add( "%s|%s|%s|%s|%s|%s",
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

	return( Options );
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
void PageMaker::AddTextOption( const StringList& args, string& result )
{
	string				value, key = args[ OP_KEY ];
	string::size_type	pos;

	pos = key.rfind( "/" );

	if( pos != string::npos ) {

		if( App->Cfg->OpenKey( key.substr( 0, pos - 1 ).c_str(), false )) {

			value = App->Cfg->GetString( key.substr( pos + 1 ).c_str(), args[ OP_DEFAULT ] );

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
void PageMaker::AddIntegerOption( const StringList& args, string& result )
{
	string				value, key = args[ OP_KEY ];
	string::size_type	pos;

	pos = key.rfind( "/" );

	if( pos != string::npos ) {

		if( App->Cfg->OpenKey( key.substr( 0, pos - 1 ).c_str(), false )) {
			char	buf[32];

			sprintf( buf, "%d",
					 App->Cfg->GetInteger( key.substr( pos + 1 ).c_str(),
					 					   atoi( args[ OP_DEFAULT ] )));

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
	bool				value = false;
	string				key = args[ OP_KEY ];
	string::size_type	pos;

	pos = key.rfind( "/" );

	if( pos != string::npos ) {

		if( App->Cfg->OpenKey( key.substr( 0, pos - 1 ).c_str(), false )) {

			value = App->Cfg->GetInteger( key.substr( pos + 1 ).c_str(), atoi( args[ OP_DEFAULT ]) );

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
void PageMaker::BuildPage( const string& page, string& result )
{
	if( page == "mods" )
		BuildModsPage( result );

	else if( page == "apply" )
		BuildApplyPage( result );

	else if( page == "listedit" )
		BuildListEditPage( result );

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

				for( int i = 0; i < options.Count(); i++ ) {
					StringList	tmp;

					tmp.Explode( options[ i ], "|" );

					if( !strcmp( tmp[ OP_TYPE ], "string" ))
						AddTextOption( tmp, result );
					else if( !strcmp( tmp[ OP_TYPE ], "integer" ))
						AddIntegerOption( tmp, result );
					else if( !strcmp( tmp[ OP_TYPE ], "bool" ))
						AddBoolOption( tmp, result );
					else if( !strcmp( tmp[ OP_TYPE ], "list" ))
						AddListOption( tmp, result, page );
				}

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

	AddTextOption( args, result );

	result += "<tr>"
			  "  <td colspan=2 align=\"center\">"
		      "    <input type=\"submit\" value=\"Save\">"
			  "    </form>";

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
void PageMaker::BuildListEditPage( string& result )
{
	string	page, list, descr;

	page = Cfg->DecodeArg( "page" );
	list = Cfg->DecodeArg( "list" );

	StringList&	options = GetOptions( page );

	for( int i = 0; i < options.Count(); i++ ) {
		StringList	tmp;

		tmp.Explode( options[ i ], "|" );

		if( list == tmp[ OP_NAME ] ) {

			descr = tmp[ OP_DESCR ];

			break;
		}
	}

	AddPageHeader( "listedit", result );

	BeginOptionsTable( descr, result );

	result +=	"<tr>"
				"  <td>"
				"    <form action=\"/listitem\" method=\"POST\">"
				"      <input type=\"hidden\" name=\"page\" value=\"" + page + "\">"
				"      <input type=\"hidden\" name=\"item\" value=\"\">"
				"      <table class=\"modules\" width=\"100%\">"
				"        <tr>"
				"          <td align=\"center\"><input type=\"submit\" value=\"Add a new item\" class=\"maxwidth\"></td>"
				"          <td> &nbsp; </td>"
				"        </tr>"
				"      </table>"
				"    </form>";
				"  </td>"
				"</tr>";

	EndOptionsTable( result );
	AddPageFooter( result, false );
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
