/***************************************************************************
                                  http.cpp
                             -------------------
    revision             : $Id: http.cpp,v 1.7 2003-02-13 11:11:31 tellini Exp $
    copyright            : (C) 2002 by Simone Tellini
    email                : tellini@users.sourceforge.net

    description          : HTTP protocol implementation
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

#undef DBG
#define DBG(x)

#include <string.h>
#include <stdio.h>
#include <ctype.h>

#include "http.h"
#include "mod_http.h"
#include "stringlist.h"
#if HAVE_ZLIB_H
#include "gzipcodec.h"
#endif

static const struct
{
    int     	code;
    const char	*title;
}   Codes[] = {
        { 200, "OK" },
        { 201, "Created" },
        { 202, "Accepted" },
        { 203, "Non-Authoritative Information" },
        { 204, "No Content" },
        { 205, "Reset Content" },
        { 206, "Partial Content" },
        { 300, "Multiple Choices" },
        { 301, "Moved Permanently" },
        { 302, "Moved Temporarily" },
        { 303, "See Other" },
        { 304, "Not Modified" },
        { 305, "Use Proxy" },
        { 400, "Bad Request" },
        { 401, "Unauthorized" },
        { 402, "Payment Required" },
        { 403, "Forbidden" },
        { 404, "Not Found" },
        { 405, "Method Not Allowed" },
        { 406, "Not Acceptable" },
        { 407, "Proxy Authentication Required" },
        { 408, "Request Timeout" },
        { 409, "Conflict" },
        { 410, "Gone" },
        { 411, "Length Required" },
        { 412, "Precondition Failed" },
        { 413, "Request Entity Too Large" },
        { 414, "Request-URI Too Long" },
        { 415, "Unsupported Media Type" },
        { 500, "Internal Server Error" },
        { 501, "Not Implemented" },
        { 502, "Bad Gateway" },
        { 503, "Service Unavailable" },
        { 504, "Gateway Timeout" },
        { 505, "HTTP Version Not Supported" },
};

#define NUM_CODES   ( sizeof( Codes ) / sizeof( Codes[0] ))

//---------------------------------------------------------------------------
HTTP::HTTP()
{
#if HAVE_ZLIB_H
	GZipper = NULL;
#endif

	Reset();
}
//---------------------------------------------------------------------------
HTTP::~HTTP()
{
#if HAVE_ZLIB_H
	delete GZipper;
#endif
}
//---------------------------------------------------------------------------
void HTTP::Reset( void )
{
#if HAVE_ZLIB_H
	delete GZipper;

	GZipper = NULL;
#endif

	Method       = M_NONE;
	MethodStr    = "";
	Protocol     = "";
	MIMEType     = "text/html";
	Range        = "";
	Location     = "";
	ETag         = "";
	Size         = 0;
	Offset       = 0;
	LastModified = (time_t)0;
	MaxAge       = 0;

	EntityID.erase();

	MethodURL.Clear();
	Data.Clear();
	Flags.Clear();
	Headers.Clear();
	TmpBuf.Clear();
}
//---------------------------------------------------------------------------
bool HTTP::AddHeaderData( const char *data, int len )
{
	bool	complete = false;

	do {
		int	copy = 0;

		while(( copy < len ) && ( data[ copy++ ] != '\n' ));

		TmpBuf.Append( data, copy );

		data += copy;
		len  -= copy;

		if( data[ -1 ] == '\n' ) {
			char	*str = TmpBuf.GetData();
			int		size = TmpBuf.GetSize() - 1;

			// replace \r\n or \n with \0
			str[ size-- ] = '\0';

			if(( size >= 0 ) && ( str[ size ] == '\r' ))
				str[ size ] = '\0';

			if( Method == M_NONE ) {

				ParseMethod( str );

				if( Method == M_UNKNOWN )
					complete = true;

			} else if( !str[ 0 ] )
				complete = true;
			else
				ParseHeader( str );

			TmpBuf.Clear();
		}

	} while( !complete && ( len > 0 ));

	// if there's anything left, it's the entity body
	if( len > 0 )
		Data.SetContent( data, len );

	return( complete );
}
//---------------------------------------------------------------------------
void HTTP::ParseMethod( char *str )
{
	char *ptr;

	if( strncmp( str, "HTTP/", 5 ) == 0 ) {

		Method = M_RESPONSE;

		if( strncmp( &str[ 5 ], "1.1", 3 ) == 0 )
			Flags.Set( HTTPF_1_1 | HTTPF_KEEP_ALIVE );

		if( ptr = strchr( str, ' ' ))
			ReturnCode = atoi( ptr );

	} else {

		if( ptr = strchr( str, ' ' )) {
			char *prot;

			*ptr++ = '\0';

			if( prot = strchr( ptr, ' ' )) {

				*prot++  = '\0';
				Protocol = prot;

				if( Protocol == "HTTP/1.1" )
					Flags.Set( HTTPF_1_1 | HTTPF_KEEP_ALIVE );
			}

			MethodURL.Decode( ptr );
		}

		MethodStr = str;

		if( MethodStr == "GET" )
			Method = M_GET;
		else if( MethodStr == "HEAD" )
			Method = M_HEAD;
		else if( MethodStr == "POST" )
			Method = M_POST;
		else if( MethodStr == "PUT" )
			Method = M_PUT;
		else
			Method = M_UNKNOWN;
	}
}
//---------------------------------------------------------------------------
void HTTP::ParseHeader( char *str )
{
	char	*ptr, *args = "";
	string	orig;
	int		needargs = 0;

	DBG( App->Log->Log( LOG_ERR, "HTTP::ParseHeader( %s )", str ));

	Headers.Add( "%s", str );

	orig = str;

	for( char *ptr = str; *ptr; ptr++ ) {

		if( !needargs && ( *ptr == ':' )) {

			*ptr = '\0';

			needargs++;

		} else {

			*ptr = tolower( *ptr );

			if(( needargs == 1 ) && ( *ptr != ' ' )) {
				args = ptr;
				needargs++;
			}
		}
	}

	if( !strcmp( str, "host" )) {

		if( ptr = strchr( args, ':' )) {
			*ptr++ = '\0';
			MethodURL.SetPort( atoi( args ));
		}

		MethodURL.SetHost( args );

		if( !MethodURL.GetScheme()[0] )
			MethodURL.SetScheme( "http://" );

	} else if( !strcmp( str, "content-type" )) {

		if( ptr = strchr( orig.c_str(), ' ' ))
			MIMEType = ptr + 1;

	} else if( !strcmp( str, "content-length" )) {

		Size = atoi( args );

		if( Size > 0 )
			Flags.Set( HTTPF_ENTITY_BODY );

	} else if( !strcmp( str, "last-modified" )) {

		if( args[0] ) {
			LastModified = args;
			Flags.Set( HTTPF_LAST_MODIFIED );
		}

	} else if( !strcmp( str, "date" )) {

		if( args[0] ) {
			Date = args;
			Flags.Set( HTTPF_DATE );
		}

	} else if( !strcmp( str, "connection" ) ||
			   !strcmp( str, "proxy-connection" )) {

		if( strstr( args, "keep-alive" ))
			Flags.Set( HTTPF_KEEP_ALIVE );
		else if( strstr( args, "close" )) {
			Flags.Clear( HTTPF_KEEP_ALIVE );
			Flags.Set( HTTPF_ENTITY_BODY ); // maybe there's a body, who knows...
		}

	} else if( !strcmp( str, "transfer-encoding" )) {

		if( strstr( args, "chunked" ))
			Flags.Set( HTTPF_CHUNKED );

		Flags.Set( HTTPF_ENTITY_BODY );

	} else if( !strcmp( str, "pragma" )) {

		if( strstr( args, "no-cache" ))
			Flags.Set( HTTPF_VALIDATE );

	} else if( !strcmp( str, "cache-control" )) {

		if(( ptr = strstr( args, "no-cache" )) || strstr( args, "must-revalidate" ))
			Flags.Set( HTTPF_VALIDATE );

		if( ptr ||
			strstr( args, "no-store" ) ||
			strstr( args, "private" ))
			Flags.Set( HTTPF_DONT_CACHE );

		if( ptr = strstr( args, "max-age=" ))
			MaxAge = atoi( args + 8 );

	} else if( !strcmp( str, "expires" )) {

		if( args[0] ) {
			Expires = args;
			Flags.Set( HTTPF_EXPIRES );
		}

	} else if( !strcmp( str, "location" )) {

		if( args[0] ) {
			Location = args;
			Flags.Set( HTTPF_LOCATION );
		}

	} else if( !strcmp( str, "if-modified-since" )) {

		if( args[0] ) {
			IfModifiedSince = args;
			Flags.Set( HTTPF_IFMODIFIED );
		}

	} else if( !strcmp( str, "if-unmodified-since" )) {

		if( args[0] ) {
			IfUnmodifiedSince = args;
			Flags.Set( HTTPF_IFUNMODIFIED );
		}

	} else if( !strcmp( str, "range" )) {

		if( ptr = strstr( args, "bytes=" )) {
			Range = ptr + 6;
			Flags.Set( HTTPF_DONT_CACHE ); // don't cache partial contents
		}

	} else if( !strcmp( str, "authorization" )) {

		if( ptr = strchr( orig.c_str(), ' ' )) {
			Authorization = ptr + 1;
			Flags.Set( HTTPF_AUTHORIZATION );
		}

		Flags.Set( HTTPF_DONT_CACHE ); // don't cache protected contents

	} else if( !strcmp( str, "etag" )) {

		if( ptr = strchr( orig.c_str(), ' ' ))
			ETag = ptr + 1;

	} else if( !strcmp( str, "if-match" )) {

		if( ptr = strchr( orig.c_str(), ' ' )) {
			ETag = ptr + 1;
			Flags.Set( HTTPF_IF_MATCH );
		}

	} else if( !strcmp( str, "if-none-match" )) {

		if( ptr = strchr( orig.c_str(), ' ' )) {
			ETag = ptr + 1;
			Flags.Set( HTTPF_IF_NONE_MATCH );
		}

	} else if( !strcmp( str, "cookie" )) {

		if( ptr = strchr( orig.c_str(), ' ' )) {
			EntityID += " Cookie: ";
			EntityID += ptr + 1;
			Flags.Set( HTTPF_VALIDATE );
		}
	}
#if HAVE_ZLIB_H
	else if( !strcmp( str, "content-encoding" )) {

		if( strstr( args, "deflate" ))
			Flags.Set( HTTPF_DEFLATE );
		else if( strstr( args, "gzip" ) || strstr( args, "x-gzip" ))
			Flags.Set( HTTPF_GZIP );

		if( Flags.IsSet( HTTPF_DEFLATE | HTTPF_GZIP ))
			GZipper = new GZipCodec();

	} else if( !strcmp( str, "accept-encoding" )) {

		if( strstr( args, "deflate" ))
			Flags.Set( HTTPF_DEFLATE );

		if( strstr( args, "gzip" ) || strstr( args, "x-gzip" ))
			Flags.Set( HTTPF_GZIP );
	}
#endif
}
//---------------------------------------------------------------------------
const char *HTTP::GetEntityID( void )
{
	EntityID = MethodURL.Encode();

	for( int i = 0; i < Headers.Count(); i++ ) {
		char        str[ 1024 ], *to = str;
		const char  *from = Headers[ i ];
		int         n = 0;

		while(( n < sizeof( str ) - 1 ) && *from && ( *from != ':' )) {
			*to++ = tolower( *from++ );
			n++;
		}

		*to = '\0';

		if( from[0] && from[1] )
			from += 2;

		if( !strcmp( str, "cookie" ))
			EntityID += " Cookie: " + string( from );
	}

	return( EntityID.c_str() );
}
//---------------------------------------------------------------------------
void HTTP::ErrorMsg( int code, const char *text )
{
	StringList	headers;
	char		buf[ 1024 ];
	int			len;

	if( Method != M_HEAD ) {
		const char	*msg = GetCodeMsg( code );

		snprintf( buf, sizeof( buf ),
				  "<!DOCTYPE HTML PUBLIC \"-//IETF//DTD HTML 2.0//EN\">\n"
				  "<HTML><HEAD>"
				  "<TITLE>%d %s</TITLE>"
				  "</HEAD><BODY>"
				  "<H1>%s</H1>"
				  "%s<P>"
				  "<HR><ADDRESS>prometeo/mod_http/"MOD_VERSION" proxy</ADDRESS>"
				  "</BODY></HTML>\n",
				  code, msg,
				  msg,
				  text );

		len = strlen( buf );

		headers.Add( "Content-Length: %d", len );
	}

	SendHeader( code, "text/html", !KeepAlive(), &headers );

	if( Method != M_HEAD )
		Sock->AsyncSend( buf, len );
}
//---------------------------------------------------------------------------
const char *HTTP::GetCodeMsg( int code ) const
{
	for( int i = 0; i < NUM_CODES; i++ )
		if( Codes[ i ].code == code )
			return( Codes[ i ].title );

	return( "" );
}
//---------------------------------------------------------------------------
void HTTP::Redirect( const char *url )
{
	StringList	headers;
	char		buf[ 4096 ];
	int			len;

	snprintf( buf, sizeof( buf ),
			  "<!DOCTYPE HTML PUBLIC \"-//IETF//DTD HTML 2.0//EN\">\n"
			  "<HTML><HEAD>"
			  "<TITLE>302 Found</TITLE>"
			  "</HEAD><BODY>"
			  "<H1>Found</H1>"
			  "The resource you requested is now available <A HREF=\"%s\">here</A><P>"
			  "<HR><ADDRESS>prometeo/mod_http/"MOD_VERSION" proxy</ADDRESS>"
			  "</BODY></HTML>\n",
			  url );

	len = strlen( buf );

	headers.Add( "Location: %s", url );
	headers.Add( "Content-Length: %d", len );

	SendHeader( HTTP_MOVED_TEMPORARILY, "text/html", !KeepAlive(), &headers );

	Sock->AsyncSend( buf, len );
}
//---------------------------------------------------------------------------
void HTTP::SendHeader( int code, const char *type, bool closecon, StringList *ExtraHeaders )
{
	NetDate	date;

	date.Set();

	Sock->AsyncPrintf( "HTTP/1.1 %d %s\r\n"
					   "Date: %s\r\n"
					   "Via: 1.1 prometeo (prometeo-mod_http/"MOD_VERSION")\r\n"
					   "Connection: %s\r\n",
					   code, GetCodeMsg( code ), date.ToStr(), closecon ? "close" : "Keep-Alive" );

	if( type && type[0] )
		Sock->AsyncPrintf( "Content-Type: %s\r\n", type );

	if( ExtraHeaders )
		for( int i = 0; i < ExtraHeaders->Count(); i++ )
			Sock->AsyncPrintf( "%s\r\n", ExtraHeaders->Get( i ));

#if HAVE_ZLIB_H
	if( GZipper )
		Sock->AsyncPrintf( "Content-Encoding: %s\r\n",
						   Flags.IsSet( HTTPF_DEFLATE ) ? "deflate" : "gzip" );
#endif

	Sock->AsyncSend( "\r\n", 2 );

	Flags.Set( HTTPF_HEADER_SENT );
	Flags.Set( HTTPF_KEEP_ALIVE, !closecon );
}
//---------------------------------------------------------------------------
void HTTP::SendMethod( const char *method, const char *uri, StringList *headers )
{
	Sock->AsyncPrintf( "%s %s HTTP/1.1\r\n", method, uri );

	if( headers )
		for( int i = 0; i < headers->Count(); i++ )
			Sock->AsyncPrintf( "%s\r\n", headers->Get( i ));

#if HAVE_ZLIB_H
	Sock->AsyncPrintf( "Accept-Encoding: gzip, deflate\r\n" );
#endif

	Sock->AsyncPrintf( "Via: 1.1 prometeo (prometeo-mod_http/"MOD_VERSION")\r\n"
					   "\r\n" );
}
//---------------------------------------------------------------------------
const char *HTTP::GetHeader( int index ) const
{
	const char *hdr = NULL;

	if( index < Headers.Count() )
		hdr = Headers[ index ];

	return( hdr );
}
//---------------------------------------------------------------------------
bool HTTP::DecodeBodyData( const char *data, int len )
{
	bool	done;

	if( Flags.IsSet( HTTPF_CHUNKED ))
		done = DecodeChunkedData( data, len, true );
	else if( Size )
		done = DecodeFixedLenData( data, len );
	else
		done = DecodeVarLenData( data, len );

#if HAVE_ZLIB_H
	if( GZipper && Data.GetSize() ) {
		bool	ok;

		if( Flags.IsSet( HTTPF_DEFLATE ))
			ok = GZipper->InflateData( Data.GetData(), Data.GetSize() );
		else
			ok = GZipper->GZipDecodeData( Data.GetData(), Data.GetSize() );

		if( ok ) {
				
			Data.SetContent( GZipper->GetData(), GZipper->GetSize() );

			if( done ) {

				if( Flags.IsSet( HTTPF_DEFLATE ))
					ok = GZipper->InflateData( NULL, 0 );
				else
					ok = GZipper->GZipDecodeData( NULL, 0 );

				if( ok )
					Data.Append( GZipper->GetData(), GZipper->GetSize() );
			}
			
		} else {
			Flags.Set( HTTPF_BODY_ERROR );
			Data.Clear();
			done = true;
		}
	}
#endif

	return( done );
}
//---------------------------------------------------------------------------
char *HTTP::GetDecodedData( int *len )
{
	char *ret = NULL;

	if( Flags.IsSet( HTTPF_BODY_ERROR ))
		*len = -1;
	else {
		ret  = Data.GetData();
		*len = Data.GetSize();
	}

	return( ret );
}
//---------------------------------------------------------------------------
void HTTP::SendBodyData( const char *data, int len )
{
#if HAVE_ZLIB_H
	bool	eof = len == 0;
		
	if( GZipper ) {
		int		origlen = len;
		bool	ok;

		if( Flags.IsSet( HTTPF_DEFLATE ))
			ok = GZipper->DeflateData( data, len );
		else
			ok = GZipper->GZipEncodeData( data, len );

		if( ok ) {
				
			data = GZipper->GetData();
			len  = GZipper->GetSize();

		} else
			Flags.Clear( HTTPF_KEEP_ALIVE );

		// avoid considering an EOF when the compressor
		// doesn't produce any data in the current round
		if( origlen && !len )
			return;
	}
#endif

	if( Flags.IsSet( HTTPF_1_1 )) {

		// chunked encoding
		Sock->AsyncPrintf( "%x\r\n", len );

		if( len )
			Sock->AsyncSend( data, len );

		Sock->AsyncSend( "\r\n", 2 );

#if HAVE_ZLIB_H
		if( GZipper && eof )
			Sock->AsyncSend( "0\r\n\r\n", 5 );
#endif

	} else if( len )
		Sock->AsyncSend( data, len );
}
//---------------------------------------------------------------------------
void HTTP::CompressBody( void )
{
#if HAVE_ZLIB_H
	if( CanCompress() )
		GZipper = new GZipCodec();
#endif
}
//---------------------------------------------------------------------------
bool HTTP::DecodeFixedLenData( const char *data, int len )
{
	bool	done = false;

	Offset += len;
	done    = Offset >= Size;

	if( len > 0 )
		Data.SetContent( data, len );
	else {
		done = true;
		Flags.Set( HTTPF_BODY_ERROR );
		Flags.Clear( HTTPF_KEEP_ALIVE );
	}

	return( done );
}
//---------------------------------------------------------------------------
bool HTTP::DecodeVarLenData( const char *data, int len )
{
	bool	done = len == 0;

	if( done )
		Data.Clear();
	else
		Data.SetContent( data, len );

	return( done );
}
//---------------------------------------------------------------------------
bool HTTP::DecodeChunkedData( const char *data, int len, bool flush )
{
	bool done = false;

	if( len == 0 ) {
		Flags.Set( HTTPF_BODY_ERROR );
		Flags.Clear( HTTPF_KEEP_ALIVE );
		return( true );
	}

	if( flush )
		Data.Clear();

	if( Flags.IsSet( HTTPF_CHUNK_BODY )) {
		int max = len;

		// must be cleared to avoid confusion later on
		TmpBuf.Clear();

		if( Offset + len > Size )
			max = Size - Offset;

		Data.Append( data, max );

		Offset += max;

		if( Offset >= Size ) {

			Offset = 0;
			Size   = 0;

			Flags.Clear( HTTPF_CHUNK_BODY );
			Flags.Set( HTTPF_CHUNK_FOOTER );
		}

		// process the rest of the data received, if any
		if( max < len )
			done = DecodeChunkedData( data + max, len - max, false );

	} else if( Flags.IsSet( HTTPF_CHUNK_FOOTER )) {
		char	*ptr;
		int		size, i = 0;

		TmpBuf.Append( data, len );

		// start examining the buffer where we previously left
		ptr  = TmpBuf.GetData() + Offset;
		size = TmpBuf.GetSize() - Offset;

		// look for an empty line in the received data
		while( i < size ) {
			int num;

			// count the length of the string till newline
			for( num = 0; ( i < size ) && ( ptr[ i ] != '\n' ); num++, i++ );

			// found a new line
			if( i < size ) {

				Offset += i + 1; // the next char we have not examined yet

				// empty line found? end of chunk
				if(( num == 0 ) || (( num == 1 ) && ( ptr[ i - 1 ] == '\r' ))) {

					size   = TmpBuf.GetSize() - Offset;
					Offset = 0;

					Flags.Clear( HTTPF_CHUNK_FOOTER );

					if( Flags.IsSet( HTTPF_CHUNK_LAST ))
						done = true;
					else if( size > 0 ) {
						Buffer	tmp( &ptr[ i + 1 ], size );

						TmpBuf.Clear();

						done = DecodeChunkedData( tmp.GetData(), size, false );
					}

					break;
				}
			}
		}

	} else {
		int num;

		TmpBuf.Append( data, len );

		for( num = 0; num < len; num++ )
			if( data[ num ] == '\n' ) {
				char	*ptr;
				int		size;

				ptr  = TmpBuf.GetData();
				size = TmpBuf.GetSize() - 1;

				ptr[ size-- ] = '\0';

				if(( size >= 0 ) && ( ptr[ size ] == '\r' ))
					ptr[ size ] = '\0';

				Size   = 0;
				Offset = 0;

				sscanf( ptr, "%x", &Size );

				TmpBuf.Clear();

				if( Size == 0 )
					Flags.Set( HTTPF_CHUNK_FOOTER | HTTPF_CHUNK_LAST );
				else
					Flags.Set( HTTPF_CHUNK_BODY );

				if( ++num < len )
					done = DecodeChunkedData( data + num, len - num, false );

				break;
			}
	}

	return( done );
}
//---------------------------------------------------------------------------
bool HTTP::GetRange( int rangeidx, int size, int *start, int *stop ) const
{
	bool	ret = false;

	if( Range.c_str()[0] ) {
		const char	*ptr;

		ptr = Range.c_str();

		while( rangeidx && ptr )
			if( ptr = strchr( ptr, ',' )) {
				ptr++;
				rangeidx--;
			}

		if( ptr ) {
			char   *stopptr;

			if( *ptr == '-' ) {

				*start = size ? ( size - atoi( ++ptr )) : 0;
				*stop  = 0;
				ret    = true;

			} else if( stopptr = strchr( ptr, '-' )) {

				*start = atoi( ptr         );
				*stop  = atoi( stopptr + 1 );
				ret    = true;
			}
		}
	}

	return( ret );
}
//---------------------------------------------------------------------------
