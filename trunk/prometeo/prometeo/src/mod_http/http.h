/***************************************************************************
                                   http.h
                             -------------------
    revision             : $Id: http.h,v 1.6 2003-04-06 10:57:37 tellini Exp $
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

#ifndef HTTP_H
#define HTTP_H

#include <string>

#include "buffer.h"
#include "bitfield.h"
#include "tcpsocket.h"

#include "url.h"
#include "netdate.h"
#include "stringlist.h"
#include "list.h"

// Flags - internal use only
#define HTTPF_DONT_CACHE			(1 << 0)
#define HTTPF_CHUNKED				(1 << 1)
#define HTTPF_EXPIRES				(1 << 2)
#define HTTPF_IFMODIFIED			(1 << 3)
#define HTTPF_LOCATION				(1 << 4)
#define HTTPF_IFUNMODIFIED			(1 << 5)
#define HTTPF_VALIDATE				(1 << 6)
#define HTTPF_AUTHORIZATION			(1 << 7)
#define HTTPF_1_1					(1 << 8)
#define HTTPF_KEEP_ALIVE			(1 << 9)
#define HTTPF_HEADER_SENT			(1 << 10)
#define HTTPF_CHUNK_BODY			(1 << 11) // receiving the chunk body
#define HTTPF_CHUNK_FOOTER			(1 << 12)
#define HTTPF_CHUNK_LAST			(1 << 13)
#define HTTPF_LAST_MODIFIED			(1 << 14)
#define HTTPF_IF_MATCH				(1 << 15)
#define HTTPF_IF_NONE_MATCH			(1 << 16)
#define HTTPF_DATE					(1 << 17)
#define HTTPF_ENTITY_BODY			(1 << 18)
#define HTTPF_BODY_ERROR			(1 << 19)
#define HTTPF_DEFLATE				(1 << 20)
#define HTTPF_GZIP					(1 << 21)
#define HTTPF_NO_CACHE				(1 << 22)

class GZipCodec;
class StreamFilter;

class HTTP
{
public:
	typedef enum { M_NONE = 0, M_UNKNOWN, M_RESPONSE, M_GET, M_HEAD, M_POST, M_PUT } HTTPMethod;

						HTTP();
						~HTTP();

	void				Reset( void );
	void				SetSocket( TcpSocket *sock ) { Sock = sock; }

	bool				AddHeaderData( const char *data, int len );

	bool				DecodeBodyData( const char *data, int len );
	char				*GetDecodedData( int *len );
	void				SendBodyData( const char *data, int len );
	void				CompressBody( void );

	void				ErrorMsg( int code, const char *text );
	const char			*GetCodeMsg( int code ) const;

	void				Redirect( const char *url );

	void				SendHeader( int code, const char *type, bool closecon, StringList *ExtraHeaders );
	void				SendMethod( const char *method, const char *uri, StringList *headers );

	void				AddFilter( StreamFilter *fil );
	
	HTTPMethod			GetMethod( void ) const         { return( Method ); }
	const char			*GetMethodStr( void ) const     { return( MethodStr.c_str() ); }
	URL&				GetURL( void )                  { return( MethodURL ); }
	const char			*GetProtocol( void ) const      { return( Protocol.c_str() ); }
	const char			*GetAuthorization( void ) const { return( Authorization.c_str() ); }
	const char			*GetMIMEType( void ) const      { return( MIMEType.c_str() ); }
	const char			*GetETag( void ) const          { return( ETag.c_str() ); }
	const char			*GetEntityID( void );
	NetDate&			GetLastModified( void )         { return( LastModified ); }
	NetDate&			GetModifiedSince( void )        { return( IfModifiedSince ); }
	NetDate&			GetUnmodifiedSince( void )      { return( IfUnmodifiedSince ); }
	NetDate&			GetExpires( void )              { return( Expires ); }
	NetDate&			GetDate( void )                 { return( Date ); }
	unsigned int		GetSize( void ) const           { return( Size ); }
	const char			*GetHeader( int index ) const;
	int					GetReturnCode( void ) const     { return( ReturnCode ); }
	int					GetMaxAge( void ) const         { return( MaxAge ); }
	bool				GetRange( int rangeidx, int size, int *start, int *stop ) const;

	bool				KeepAlive( void ) const              { return( Flags.IsSet( HTTPF_KEEP_ALIVE )); }
	bool				HeaderSent( void ) const             { return( Flags.IsSet( HTTPF_HEADER_SENT )); }
	bool				Is11( void ) const                   { return( Flags.IsSet( HTTPF_1_1 )); }
	bool				HasEntityBody( void ) const          { return( Flags.IsSet( HTTPF_ENTITY_BODY )); }
	bool				IsDateValid( void ) const            { return( Flags.IsSet( HTTPF_DATE )); }
	bool				IsAuthValid( void ) const            { return( Flags.IsSet( HTTPF_AUTHORIZATION )); }
	bool				IsLastModifiedValid( void ) const    { return( Flags.IsSet( HTTPF_LAST_MODIFIED )); }
	bool				IsModifiedSinceValid( void ) const   { return( Flags.IsSet( HTTPF_IFMODIFIED )); }
	bool				IsUnmodifiedSinceValid( void ) const { return( Flags.IsSet( HTTPF_IFUNMODIFIED )); }
	bool				IsMatchValid( void ) const           { return( Flags.IsSet( HTTPF_IF_MATCH )); }
	bool				IsNoneMatchValid( void ) const       { return( Flags.IsSet( HTTPF_IF_NONE_MATCH )); }
	bool				IsExpiresValid( void ) const         { return( Flags.IsSet( HTTPF_EXPIRES )); }
	bool				IsCacheable( void ) const            { return( !Flags.IsSet( HTTPF_DONT_CACHE )); }
	bool				Validate( void ) const               { return( Flags.IsSet( HTTPF_VALIDATE )); }
	bool				NotFromCache( void ) const			 { return( Flags.IsSet( HTTPF_NO_CACHE )); }
	bool				CanCompress( void ) const            { return( Flags.IsSet( HTTPF_DEFLATE | HTTPF_GZIP )); }

private:
	TcpSocket			*Sock;
	Buffer				Data;
	Buffer				TmpBuf;
	BitField			Flags;
	HTTPMethod			Method;
	string				MethodStr;
	URL					MethodURL;
	string				Protocol;
	string				MIMEType;
	string				Authorization;
	string				Location;
	string				Range;
	string				ETag;
	string				EntityID;
	int					MaxAge;
	unsigned int		Size;
	unsigned int		Offset;
	int					ReturnCode;
	NetDate				Date;
	NetDate				LastModified;
	NetDate				IfModifiedSince;
	NetDate				IfUnmodifiedSince;
	NetDate				Expires;
	StringList			Headers;
	GZipCodec			*GZipper;
	List				Filters;

	void				ParseMethod( char *str );
	void				ParseHeader( char *str );
	bool				DecodeFixedLenData( const char *data, int len );
	bool				DecodeVarLenData( const char *data, int len );
	bool				DecodeChunkedData( const char *data, int len, bool flush );
};

#define HTTP_OK                     200
#define HTTP_CREATED                201
#define HTTP_ACCEPTED               202
#define HTTP_NON_AUTH_INFO          203
#define HTTP_NO_CONTENT             204
#define HTTP_RESET_CONTENT          205
#define HTTP_PARTIAL_CONTENT        206
#define HTTP_MOVED_TEMPORARILY      302
#define HTTP_NOT_MODIFIED           304
#define HTTP_BAD_REQUEST            400
#define HTTP_UNAUTHORIZED           401
#define HTTP_FORBIDDEN              403
#define HTTP_NOT_FOUND              404
#define HTTP_TIMEOUT                408
#define HTTP_PRECONDITION_FAILED    412
#define HTTP_INTERNAL_ERROR         500
#define HTTP_NOT_IMPLEMENTED        501
#define HTTP_BAD_GATEWAY            502
#define HTTP_SERVICE_UNAVAILABLE    503
#define HTTP_GATEWAY_TIMEOUT        504

#endif

