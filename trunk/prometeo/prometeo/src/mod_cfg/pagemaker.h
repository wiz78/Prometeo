/***************************************************************************
                                 pagemaker.h
                             -------------------
    revision             : $Id: pagemaker.h,v 1.6 2002-11-15 20:22:25 tellini Exp $
    copyright            : (C) 2002 by Simone Tellini
    email                : tellini@users.sourceforge.net

    description          : builds the HTML pages
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef XSLT_H
#define XSLT_H

#include <string>
#include <sablot.h>
#include <sdom.h>

#include "cfgdata.h"
#include "stringlist.h"

class ListData
{
public:
	string			Page;
	string			List;
	string			Path;
	string			Descr;
	string			BaseKey;
	string			KeyName;
	string			KeyLabel;
	string			KeyDescr;
	StringList		Fields;
};

class PageMaker
{
public:
					PageMaker( CfgData *cfg );
					~PageMaker();

	void			SetModuleManifest( const char *manifest );

	void			BuildPage( const string& page, string& result );

	StringList&		GetOptions( const string& page );

private:
	CfgData			*Cfg;
	SablotHandle	Processor;
	SablotSituation	Situation;
	SDOM_Document	DocDOM;
	string			Doc;
	StringList		Options;

	void			LoadFile( const char *str );

	bool			ParseDoc( void );

	string			ValueOf( const string query, SDOM_Node context );
	void			GetOptionList( const string query, StringList& list );

	void			AddPageHeader( const string page, string& result );
	void			AddPageFooter( string& result, bool closeform = true );

	void			BeginOptionsTable( const string caption, string& result );
	void			EndOptionsTable( string& result );

	void			AddOption( const string& page, const char *opt, string& result );
	void			AddTextOption( const StringList& args, string& result, bool edit );
	void			AddTextAreaOption( const StringList& args, string& result );
	void			AddIntegerOption( const StringList& args, string& result );
	void			AddBoolOption( const StringList& args, string& result );
	void			AddListOption( const StringList& args, string& result, const string& page );

	void			BuildModsPage( string& result );
	void			BuildApplyPage( string& result );
	void			BuildAclPage( string& result );
	void			BuildAclUserPage( string& result );
	void			BuildListEditPage( string& result );
	void			BuildListItemPage( string& result );

	void			GetListData( ListData& data );
	int				AddListHeaders( const ListData& data, string& result );
	void			AddListRow( const char *item, const ListData& data, string& result );
	void			SaveListItem( const ListData& data );
	void			DeleteListItem( string& result );

	string			UrlEncode( const string& str );
};

// options
enum { OP_TYPE = 0, OP_NAME, OP_KEY, OP_LABEL, OP_DESCR, OP_DEFAULT };

#endif

