/***************************************************************************
                                  misc.cpp
                             -------------------
    revision             : $Id: misc.cpp,v 1.1 2002-11-21 12:43:26 tellini Exp $
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

#include <cctype>

#include "misc.h"

//---------------------------------------------------------------------------
string UrlEncode( const string& str )
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
string UrlDecode( const string val )
{
	const char	*src;
	char		*dst, *tmp;
	string		ret;

	tmp = new char[ val.length() + 1 ];
	src = val.c_str();
	dst = tmp;

	while( *src ) {
		char ch = *src++;

		switch( ch ) {

			case '+':
				*dst++ = ' ';
				break;

			case '%': // %xx - hex value to decode
				char v1, v2;

				if(( v1 = *src ) && ( v2 = *++src )) {
					static const char 	hex[] = "0123456789abcdef";
					const char			*ptr;

					if( ptr = strchr( hex, tolower( v1 ))) {

						v1 = ptr - hex;

						if( ptr = strchr( hex, tolower( v2 ))) {

							v2 = ptr - hex;

							*dst++ = ( v1 << 4 ) | v2;
						}
					}

					src++;
				}
				break;

			default:
				*dst++ = ch;
				break;
		}
	}

	*dst = '\0';

	ret = tmp;

	delete[] tmp;

	return( ret );
}
//---------------------------------------------------------------------------
string ValueOf( const string query, SDOM_Node context, SablotSituation sit )
{
	SDOM_NodeList	list;
	string			res;

	if( SDOM_xql( sit, query.c_str(), context, &list ) == SDOM_OK ) {
		int num = 0;

		SDOM_getNodeListLength( sit, list, &num );

		if( num >= 1 ) {
			SDOM_Node		node;
			SDOM_NodeType	type;
			SDOM_char		*str;

			SDOM_getNodeListItem( sit, list, 0, &node );
			SDOM_getNodeType( sit, node, &type );

			// our elements should only have one PCDATA child
			if( type == SDOM_ELEMENT_NODE )
				SDOM_getFirstChild( sit, node, &node );

			if(( SDOM_getNodeValue( sit, node, &str ) == SDOM_OK ) && str ) {

				res = str;

				SablotFree( str );
			}
		}

		SDOM_disposeNodeList( sit, list );
	}

	return( res );
}
//---------------------------------------------------------------------------
