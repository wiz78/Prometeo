/***************************************************************************
                                 pagemaker.h
                             -------------------
    revision             : $Id: pagemaker.h,v 1.1.1.1 2002-10-10 09:59:30 tellini Exp $
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

#include "stringlist.h"

class PageMaker
{
public:
					PageMaker();
					~PageMaker();

	void			SetModuleManifest( const char *manifest );

	void			BuildPage( const string& page, string& result );

	StringList&		GetOptions( const string& page );

private:
	SablotHandle	Processor;
	SablotSituation	Situation;
	SDOM_Document	DocDOM;
	string			Doc;
	StringList		Options;

	void			LoadFile( const char *str );

	bool			ParseDoc( void );

	string			ValueOf( const string query, SDOM_Node context );

	void			AddPageHeader( const string page, string& result );
	void			AddPageFooter( string& result, bool closeform = true );

	void			BeginOptionsTable( const string caption, string& result );
	void			EndOptionsTable( string& result );

	void			AddTextOption( const StringList& args, string& result );
	void			AddIntegerOption( const StringList& args, string& result );
	void			AddBoolOption( const StringList& args, string& result );

	void			BuildModsPage( string& result );
	void			BuildApplyPage( string& result );

	string			UrlEncode( const string& str );
};

// options
enum { OP_TYPE = 0, OP_NAME, OP_KEY, OP_LABEL, OP_DESCR, OP_DEFAULT };

#endif

