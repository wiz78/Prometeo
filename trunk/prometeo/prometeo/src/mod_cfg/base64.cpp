/***************************************************************************
                                 base64.cpp
                             -------------------
	revision             : $Id: base64.cpp,v 1.1.1.1 2002-10-10 09:59:23 tellini Exp $
    copyright            : (C) 2002 by Simone Tellini
    email                : tellini@users.sourceforge.net

	description          : base 64 decoder
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include <string.h>

#include "base64.h"

static const char Base64Table[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

//---------------------------------------------------------------------------
void Base64Decode( const char *from, char *to, int size )
{
	int		step = 0;
	char	newbyte;

	size--; // final '\0'

	while( *from && ( *from != '=' ) && size ) {
		char   *ptr;

		if( ptr = strchr( Base64Table, *from++ )) {
			char	value;

			value = ptr - Base64Table;

			switch( step++ ) {

				case 0:
					newbyte = value << 2;
					break;

				case 1:
					*to++   = newbyte | (( value & 0x30 ) >> 4 );
					newbyte = ( value & 0x0F ) << 4;
					size--;
					break;

				case 2:
					*to++   = newbyte | (( value & 0x3C ) >> 2 );
					newbyte = ( value & 0x03 ) << 6;
					size--;
					break;

				case 3:
					*to++ = newbyte | ( value & 0x3F );
					size--;
					break;
			}

			if( step >= 4 )
				step = 0;
		}
	}

	*to = '\0';
}
//---------------------------------------------------------------------------

