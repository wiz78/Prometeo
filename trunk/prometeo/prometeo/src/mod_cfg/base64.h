/***************************************************************************
                                  base64.h
                             -------------------
	revision             : $Id: base64.h,v 1.1.1.1 2002-10-10 09:59:23 tellini Exp $
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

#ifndef BASE64_H
#define BASE64_H

extern void Base64Decode( const char *from, char *to, int size );

#endif
