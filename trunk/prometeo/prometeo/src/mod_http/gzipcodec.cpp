/***************************************************************************
                                gzipcodec.cpp
                             -------------------
    revision             : $Id: gzipcodec.cpp,v 1.2 2003-02-14 15:50:33 tellini Exp $
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

#include "main.h"

#if HAVE_ZLIB_H

#include <strings.h>

#include "gzipcodec.h"

#ifndef OS_CODE
#define OS_CODE 0
#endif

#define DEF_MEM_LEVEL 8

#define MAGIC1	'\037'
#define MAGIC2	'\213'

/* gzip flag byte */
#define ASCII_FLAG   0x01 /* bit 0 set: file probably ascii text */
#define HEAD_CRC     0x02 /* bit 1 set: header CRC present */
#define EXTRA_FIELD  0x04 /* bit 2 set: extra field present */
#define ORIG_NAME    0x08 /* bit 3 set: original file name present */
#define COMMENT      0x10 /* bit 4 set: file comment present */

//---------------------------------------------------------------------------
GZipCodec::GZipCodec()
{
	memset( &State, 0, sizeof( State ));

	Flags.Set( GZC_F_INIT );
}
//---------------------------------------------------------------------------
GZipCodec::~GZipCodec()
{
	if( Flags.IsSet( GZC_F_INFLATE ))
		inflateEnd( &State );
	else if( Flags.IsSet( GZC_F_DEFLATE ))
		deflateEnd( &State );
}
//---------------------------------------------------------------------------
bool GZipCodec::InflateData( const char *data, int len )
{
	bool	ok = true;

	if( Flags.IsSet( GZC_F_INIT )) {
			
		ok = inflateInit2( &State, -MAX_WBITS ) == Z_OK;

		Flags.Clear( GZC_F_INIT );
		Flags.Set( GZC_F_INFLATE );
	}

	Data.Clear();

	if( ok )
		ok = Inflate( data, len );

	return( ok );
}
//---------------------------------------------------------------------------
bool GZipCodec::DeflateData( const char *data, int len )
{
	bool	ok = true;

	if( Flags.IsSet( GZC_F_INIT )) {
			
		ok = deflateInit2( &State, Z_DEFAULT_COMPRESSION, Z_DEFLATED, 
						   -MAX_WBITS, DEF_MEM_LEVEL, Z_DEFAULT_STRATEGY ) == Z_OK;
	
		Flags.Clear( GZC_F_INIT );
		Flags.Set( GZC_F_DEFLATE );
	}

	Data.Clear();

	if( ok )
		ok = Deflate( data, len );

	return( ok );
}
//---------------------------------------------------------------------------
bool GZipCodec::GZipDecodeData( const char *data, int len )
{
	bool	ok = true, head;
		
	if( Flags.IsSet( GZC_F_INIT )) {

		Flags.Clear( GZC_F_INIT );
		Flags.Set( GZC_F_HEADER | GZC_F_INFLATE );
		
		StillInTmpBuf = 0;
		CRC           = crc32( 0, Z_NULL, 0 );

		ok = inflateInit2( &State, -MAX_WBITS ) == Z_OK;
	}

	head = Flags.IsSet( GZC_F_HEADER );
	
	if( ok && head )
		ok = CheckGZipHeader( data, len );
	
	Data.Clear();
	
	if( ok && !Flags.IsSet( GZC_F_HEADER | GZC_F_DONE )) {

		if( StillInTmpBuf > 0 ) {
			// this avoids passing overlapping buffers to inflate()
			Buffer	tmp( DataStart, StillInTmpBuf );

			ok = Inflate( tmp.GetData(), StillInTmpBuf );

			StillInTmpBuf = 0;
		}

		// if head = true, data has been moved in TmpBuf and processed above
		if( ok && !head )
			ok = Inflate( data, len );
		
		if( Data.GetSize() )
			CRC = crc32( CRC, (Bytef *)Data.GetData(), Data.GetSize() );

//		if( Flags.IsSet( GZC_F_DONE )) {
			// XXX TODO we don't check the CRC, oh well...
//		}
	}

	return( ok );
}
//---------------------------------------------------------------------------
bool GZipCodec::CheckGZipHeader( const char *data, int len )
{
	bool	ok = true;
		
	memcpy( &TmpBuf[ StillInTmpBuf ], data, len );

	StillInTmpBuf += len;

	if( StillInTmpBuf > 10 ) {
		
		if(( TmpBuf[0] != MAGIC1 ) || ( TmpBuf[1] != MAGIC2 ))
			Flags.Set( GZC_F_NOT_GZIPPED );
		
		else if( TmpBuf[2] == Z_DEFLATED ) {
			char	*ptr = TmpBuf + 10;
			int		len = StillInTmpBuf - 10;
			bool	extra, name, comm, crc;

			extra = ( TmpBuf[3] & EXTRA_FIELD ) != 0;
			name  = ( TmpBuf[3] & ORIG_NAME ) != 0;
			comm  = ( TmpBuf[3] & COMMENT ) != 0;
			crc   = ( TmpBuf[3] & HEAD_CRC ) != 0;
				
			// skip the extra field
			if( extra ) {
					
				len -= 2;
				ptr += 2;

				if( len > 0 ) {
					int n = TmpBuf[10] + ( TmpBuf[11] << 8 );

					ptr += n;
					len -= n;
				}
			}

			if( len > 0 ) {

				extra = false;
						
				// skip original file name
				while( name && ( len-- > 0 ))
					if( !*ptr++ )
						name = false;

				if( !name && ( len > 0 )) {

					// skip comment
					while( comm && ( len-- > 0 ))
						if( !*ptr++ )
							comm = false;

					// skip header crc						
					if( !comm && ( len > 0 ) && crc ) {
						crc  = false;
						len -= 2;
						ptr += 2;
					}
				}
			}

			// are we done with the header?
			if(( len > 0 ) && !( extra || name || comm || crc )) {
					
				Flags.Clear( GZC_F_HEADER );
				
				DataStart     = ptr;
				StillInTmpBuf = len;
			}
					
		} else
			ok = false;
	}

	return( ok );
}
//---------------------------------------------------------------------------
bool GZipCodec::GZipEncodeData( const char *data, int len )
{
	bool	ok = true;

	Data.Clear();
		
	if( Flags.IsSet( GZC_F_INIT )) {
			
		AddGZipHeader();
		
		Flags.Clear( GZC_F_INIT );
		Flags.Set( GZC_F_DEFLATE );

		CRC = crc32( 0, Z_NULL, 0 );
		ok  = deflateInit2( &State, Z_DEFAULT_COMPRESSION, Z_DEFLATED, 
							-MAX_WBITS, DEF_MEM_LEVEL, Z_DEFAULT_STRATEGY ) == Z_OK;
	}
	
	if( ok ) {

		if( len > 0 )
			CRC = crc32( CRC, (Bytef *)data, len );

		ok = Deflate( data, len );
			
		if( len == 0 ) {
			Data.Append((char *)&CRC, sizeof( CRC ));
			Data.Append((char *)&State.total_in, sizeof( State.total_in ));
		}
	}

	return( ok );
}
//---------------------------------------------------------------------------
void GZipCodec::AddGZipHeader( void )
{
	char	*data;

	Data.Resize( 10 );

	data = Data.GetData();

	*data++ = MAGIC1;
	*data++ = MAGIC2;
	*data++ = Z_DEFLATED;
	*data++ = 0;			// flags

	time((time_t *)data );

	data += 4;

	*data++ = 0;			// deflated flags
	*data   = OS_CODE;		// OS code
}
//--------------------------------------------------------------------------
bool GZipCodec::Deflate( const char *data, int len )
{
	bool	ok = true, notdone;
	int		flush = len ? Z_PARTIAL_FLUSH : Z_FINISH;

	State.next_in   = (Bytef *)data;
	State.avail_in  = len;
	State.next_out  = (Bytef *)TmpBuf;
	State.avail_out = sizeof( TmpBuf );

	do {
		int count, err;

		notdone = false;

		err = deflate( &State, flush );
		
		switch( err ) {

			case Z_OK:
			case Z_BUF_ERROR:
				count   = sizeof( TmpBuf ) - State.avail_out;
				notdone = true;

				if( count > 0 )
					Data.Append( TmpBuf, count );
				else if( err == Z_BUF_ERROR )
					notdone = false;

				State.next_out  = (Bytef *)TmpBuf;
				State.avail_out = sizeof( TmpBuf );
				break;

			case Z_STREAM_END:
				count = sizeof( TmpBuf ) - State.avail_out;

				if( count > 0 )
					Data.Append( TmpBuf, count );

				Flags.Set( GZC_F_DONE );
				break;

			default:
				ok = false;
				break;
		}

	} while( notdone );

	return( ok );
}
//---------------------------------------------------------------------------
bool GZipCodec::Inflate( const char *data, int len )
{
	bool	ok = true;

	if( Flags.IsSet( GZC_F_NOT_GZIPPED )) {

		Data.Append( data, len );
			
	} else {
		bool	notdone;

		State.next_in   = (Bytef *)data;
		State.avail_in  = len;
		State.next_out  = (Bytef *)TmpBuf;
		State.avail_out = sizeof( TmpBuf );

		do { // inflate loop
			int count, err;

			notdone = false;
			err     = inflate( &State, Z_PARTIAL_FLUSH );

			switch( err ) {

				case Z_OK:
					count = sizeof( TmpBuf ) - State.avail_out;

					if( count > 0 )
						Data.Append( TmpBuf, count );

					State.next_out  = (Bytef *)TmpBuf;
					State.avail_out = sizeof( TmpBuf );
				
					notdone = true;
					break;

				case Z_STREAM_END:
					count = sizeof( TmpBuf ) - State.avail_out;

					if( count > 0 )
						Data.Append( TmpBuf, count );

					Flags.Set( GZC_F_DONE );
					break;

				case Z_BUF_ERROR: // no progress possible, finished?
					break;

				case Z_DATA_ERROR:
					if( inflateSync( &State ) == Z_OK )
						notdone = true;
					break;

				default:
					ok = false;
					break;
			}

		} while( notdone );
	}

	return( ok );
}
//---------------------------------------------------------------------------

#endif /* HAVE_ZLIB_H */

