/***************************************************************************
                                 gzipcodec.h
                             -------------------
    revision             : $Id: gzipcodec.h,v 1.1.1.1 2002-10-10 09:59:36 tellini Exp $
    copyright            : (C) 2002 by Simone Tellini
    email                : tellini@users.sourceforge.net

    description          : handles gzip/deflate content encodings
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef GZIPCODEC_H
#define GZIPCODEC_H

#include <zlib.h>

#include "buffer.h"
#include "bitfield.h"

class GZipCodec
{
public:
					GZipCodec();
					~GZipCodec();

	bool			InflateData( const char *data, int len );
	bool			DeflateData( const char *data, int len );

	bool			GZipDecodeData( const char *data, int len );
	bool			GZipEncodeData( const char *data, int len );

	int				GetSize( void ) const { return( Data.GetSize() ); }
	const char		*GetData( void ) { return( Data.GetData() ); }

private:
	z_stream		State;
	char			TmpBuf[ 16 * 1024 ];
	BitField		Flags;
	Buffer			Data;
	uLong			CRC;
	int				StillInTmpBuf;
	char			*DataStart;

	bool			CheckGZipHeader( const char *data, int len );
	void			AddGZipHeader( void );

	bool			Inflate( const char *data, int len );
	bool			Deflate( const char *data, int len );
};

// Flags
#define GZC_F_INIT			(1 << 0)
#define GZC_F_INFLATE		(1 << 1)
#define GZC_F_DEFLATE		(1 << 2)
#define GZC_F_HEADER		(1 << 3)
#define GZC_F_NOT_GZIPPED	(1 << 4)
#define GZC_F_DONE			(1 << 5)

#endif
