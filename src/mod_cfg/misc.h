/***************************************************************************
                                   misc.h
                             -------------------
    revision             : $Id: misc.h,v 1.1 2002-11-21 12:43:26 tellini Exp $
    copyright            : (C) 2002 by Simone Tellini
    email                : tellini@users.sourceforge.net

    description          : miscellaneous support functions
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef MISC_H
#define MISC_H

using namespace std;

#include <string>
#include <sablot.h>
#include <sdom.h>

extern string UrlEncode( const string& str );
extern string UrlDecode( const string val );
extern string ValueOf( const string query, SDOM_Node context, SablotSituation sit );

#endif
