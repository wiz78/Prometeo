/***************************************************************************
                                pagemaker.cpp
                             -------------------
    revision             : $Id: pagemaker.cpp,v 1.9 2003-01-06 12:30:26 tellini Exp $
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
#include "loader.h"
#include "stringlist.h"
#include "pagemaker.h"
#include "option.h"
#include "option_string.h"
#include "option_list.h"

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
	ClearOptions();

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
bool PageMaker::ReadOptions( const string& page )
{
	ClearOptions();

	if( ParseDoc() ) {
		string	query, page2 = page;

		if( page2.substr( 0, 4 ) == "mod/" )
			page2.erase( 0, 4 );

		query = "//Page[ @name = '" + UrlDecode( page2 ) + "' ]/Option";

		Option::GetOptionList( Situation, (SDOM_Node)DocDOM, query, Options, page );
	}

	return( Options.Count() > 0 );
}
//---------------------------------------------------------------------------
void PageMaker::ClearOptions( void )
{
	int	i = 0;

	while( Option *opt = GetOption( i++ ))
		delete opt;

	Options.Clear();
}
//---------------------------------------------------------------------------
Option *PageMaker::GetOption( int i )
{
	Option	*ret;

	try {
		ret = (Option *)Options[ i ];
	}
	catch(...) {
		ret = NULL;
	}

	return( ret );
}
//---------------------------------------------------------------------------
void PageMaker::AddPageHeader( string page, string& result )
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

	if( page.substr( 0, 4 ) == "mod/" )
		page = "mod/" + UrlEncode( page.substr( 4 ));
	else
		page = UrlEncode( page );
		
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
void PageMaker::BuildPage( const string& page, string& result )
{
	ReadOptions( page );
		
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
				int i = 0;

				AddPageHeader( page, result );
				BeginOptionsTable( ValueOf( "Label", node ).c_str(), result );

				while( Option *opt = GetOption( i++ ))
					opt->Render( result );

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
	StringOption	opt( "mods" );

	AddPageHeader( "mods", result );
	BeginOptionsTable( "Modules administration", result );

	opt.SetName( "path" );
	opt.SetKey( "root/Modules/path" );
	opt.SetLabel( "Modules path" );
	opt.SetDescr( "The directory containing your modules" );
	opt.SetDefault( MOD_DIR );

	opt.Render( result );

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
	ListOption	*opt = GetListData();

	if( opt ) {

		AddPageHeader( "listedit", result );
		BeginOptionsTable( opt->GetDescr(), result );

		result += "</form><tr><td colspan=2>";

		opt->PrintListTable( result );

		result += "</td></tr>";

		EndOptionsTable( result );
		AddPageFooter( result, false );
	}
}
//---------------------------------------------------------------------------
ListOption *PageMaker::GetListData( void )
{
	string		page, list;
	ListOption	*ret = NULL;

	page = Cfg->DecodeArg( "page" );
	list = Cfg->DecodeArg( "list" );

	ReadOptions( page );

	for( int i = 0; !ret && ( i < Options.Count() ); i++ ) {
		ListOption *opt = (ListOption *)GetOption( i );

		if( opt->GetType() == Option::T_LIST ) {
		
			if( list == opt->GetPathName() )
				ret = opt;
			else
				ret = opt->FindList( list );
		}
	}

	return( ret );
}
//---------------------------------------------------------------------------
void PageMaker::BuildListItemPage( string& result )
{
	ListOption	*opt = GetListData();
	string	item;

	if( opt ) {
		StringOption	str( opt->GetPage() );
		int				i;
		Option			*field;

		if( Cfg->DecodeArg( "save" ) == "1" )
			SaveListItem( opt );

		AddPageHeader( "listitem", result );
		BeginOptionsTable( opt->GetDescr(), result );

		result +=	"<input type=\"hidden\" name=\"page\" value=\"" + opt->GetPage() + "\">"
					"<input type=\"hidden\" name=\"list\" value=\"" + opt->GetPathName() + "\">"
					"<input type=\"hidden\" name=\"save\" value=\"1\">";

		item = Cfg->DecodeArg( opt->GetKeyName().c_str() );

		if( !item.empty() )
			str.SetEditable( false );

		str.SetName( opt->GetKeyName() );
		str.SetLabel( opt->GetKeyLabel() );
		str.SetDescr( opt->GetKeyDescr() );
		str.SetDefault( item );

		str.Render( result );

		i = 0;

		while( field = opt->GetField( i++ )) {
			field->SetKey( opt->GetKey() + "/" + item + "/" + field->GetKey() );
			field->Render( result );
		}

		EndOptionsTable( result );

		result += 	"  <div align=\"center\"><input type=\"submit\" value=\"Save\"></div>"
					"</form>"
					"<form action=\"/listedit\" method=\"GET\">"
					"  <input type=\"hidden\" name=\"page\" value=\"" + opt->GetPage() + "\">"
					"  <input type=\"hidden\" name=\"list\" value=\"" + opt->GetPathName() + "\">"
					"  <input type=\"submit\" value=\"Back to the list page\">"
					"</form>";

		AddPageFooter( result, false );
	}
}
//---------------------------------------------------------------------------
void PageMaker::DeleteListItem( string& result )
{
	ListOption	*opt = GetListData();

	if( opt ) {
		string	key;

		key = opt->GetKey() + "/" + Cfg->DecodeArg( opt->GetKeyName().c_str() );

		App->Cfg->DeleteKey( key.c_str() );
	}

	BuildListEditPage( result );
}
//---------------------------------------------------------------------------
void PageMaker::SaveListItem( ListOption *opt )
{
	string	key = opt->GetKey() + "/" + Cfg->DecodeArg( opt->GetKeyName().c_str() );
	int		i = 0;

	while( Option *field = opt->GetField( i++ ))
		Cfg->UpdateKey( key, field->GetKey(), field->GetType(),
						Cfg->DecodeArg( field->GetName().c_str() ));
}
//---------------------------------------------------------------------------
