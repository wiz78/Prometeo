/***************************************************************************
                                 pagemaker.h
                             -------------------
    revision             : $Id: pagemaker.h,v 1.7 2002-11-20 22:53:43 tellini Exp $
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

#include "list.h"

#include "cfgdata.h"
#include "misc.h"

class Option;
class ListOption;

class PageMaker
{
public:
					PageMaker( CfgData *cfg );
					~PageMaker();

	void			SetModuleManifest( const char *manifest );

	void			BuildPage( const string& page, string& result );

	bool			ReadOptions( const string& page );
	Option			*GetOption( int i );

private:
	CfgData			*Cfg;
	SablotHandle	Processor;
	SablotSituation	Situation;
	SDOM_Document	DocDOM;
	string			Doc;
	List			Options;

	void			LoadFile( const char *str );

	bool			ParseDoc( void );

	string			ValueOf( const string query, SDOM_Node context ) { return( ::ValueOf( query, context, Situation )); }
	void			ClearOptions( void );

	void			AddPageHeader( string page, string& result );
	void			AddPageFooter( string& result, bool closeform = true );

	void			BeginOptionsTable( const string caption, string& result );
	void			EndOptionsTable( string& result );

	void			BuildModsPage( string& result );
	void			BuildApplyPage( string& result );
	void			BuildAclPage( string& result );
	void			BuildAclUserPage( string& result );
	void			BuildListEditPage( string& result );
	void			BuildListItemPage( string& result );

	ListOption		*GetListData( void );
	void			SaveListItem( ListOption *opt );
	void			DeleteListItem( string& result );
};

// options
enum { OP_TYPE = 0, OP_NAME, OP_KEY, OP_LABEL, OP_DESCR, OP_DEFAULT };

#endif

