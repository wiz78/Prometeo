/***************************************************************************
                                 mod_http.cpp
                             -------------------
    revision             : $Id: mod_http.cpp,v 1.22 2003-06-02 17:20:02 tellini Exp $
    copyright            : (C) 2002-2003 by Simone Tellini
    email                : tellini@users.sourceforge.net

    description          : caching HTTP proxy
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
#include "registry.h"
#include "dnscache.h"

#include "mod_http.h"
#include "cache.h"
#include "filter.h"
#include "popupblocker.h"
#include "resizeblocker.h"

#undef DBG
#define DBG(x)

#define TIMEOUT_CONNECT		45
#define TIMEOUT_REQUEST		15
#define TIMEOUT_DATA		90

#define INTERVAL_PRUNE		60		// prune the cache every X seconds
#define INTERVAL_INDEX		120		// write down the cache index every X seconds

static const char *GetManifest( const char *key, const char *name );
static HANDLE SetupModule( const char *key );
static BOOL CleanupModule( HANDLE mod );
static void CfgChanged( HANDLE mod );
static void OnFork( HANDLE mod );
static void OnTimer( HANDLE mod, time_t now );
static void SocketCB( SOCKREF sock, Prom_SC_Reason reason, int data, void *userdata );
static void DNSCallback( int addrlen, void *userdata );
static void CacheObjListener( void *userdata, char *data, int len );

//---------------------------------------------------------------------------
PROM_MODULE =
{
	1,						// API version supported by this module
	"mod_http",
	PROM_MT_CUSTOM,

	GetManifest,

	SetupModule,
	CleanupModule,

	CfgChanged,
	OnFork,
	OnTimer,
};
//---------------------------------------------------------------------------
static const char *GetManifest( const char *key, const char *name )
{
	static string		Manifest;
	string				basekey = string( key ) + "/", mod = name;

	Manifest =	"<Page name=\"" + mod + "\">"
				"	<Label>" + mod + " options</Label>"

				"	<Option type=\"integer\" name=\"port\" default=\"8080\">"
				"		<Label>Port</Label>"
				"		<Descr>The port the proxy will listen on</Descr>"
				"		<Key name=\"" + basekey + "port\"/>"
				"	</Option>"

				"	<Option type=\"integer\" name=\"maxcachesize\" default=\"500\">"
				"		<Label>Max cache size</Label>"
				"		<Descr>The maximum size of the cache, in MB</Descr>"
				"		<Key name=\"" + basekey + "maxcachesize\"/>"
				"	</Option>"

				"	<Option type=\"integer\" name=\"maxobjectsize\" default=\"600000\">"
				"		<Label>Max object size</Label>"
				"		<Descr>Only object smaller than this size will be stored "
				"in cache. In bytes.</Descr>"
				"		<Key name=\"" + basekey + "maxobjectsize\"/>"
				"	</Option>"

				"	<Option type=\"bool\" name=\"logrequests\" default=\"1\">"
				"		<Label>Log requests</Label>"
				"		<Descr>Set this if you want all the requests to be logged.</Descr>"
				"		<Key name=\"" + basekey + "logrequests\"/>"
				"	</Option>"

#if HAVE_ZLIB_H
				"	<Option type=\"bool\" name=\"gzipencoding\" default=\"1\">"
				"		<Label>Enable gzip compression</Label>"
				"		<Descr>If set, client which support gzip or deflate content "
				"encodings will be served compressed pages. You can disable it if "
				"you want to save some CPU power on the proxy machine.</Descr>"
				"		<Key name=\"" + basekey + "gzipencoding\"/>"
				"	</Option>"
#endif
				"	<Option type=\"bool\" name=\"blockpopups\" default=\"0\">"
				"		<Label>Block popups</Label>"
				"		<Descr>Set this if you want mod_http to try and block the annoying popup windows.</Descr>"
				"		<Key name=\"" + basekey + "blockpopups\"/>"
				"	</Option>"

				"	<Option type=\"bool\" name=\"blockresize\" default=\"0\">"
				"		<Label>Block resizeTo()</Label>"
				"		<Descr>Set this if you want mod_http to prevent Javascript code to resize your browser window.</Descr>"
				"		<Key name=\"" + basekey + "blockresize\"/>"
				"	</Option>"

				"	<Option type=\"bool\" name=\"allowconnect\" default=\"1\">"
				"		<Label>Allow the CONNECT method</Label>"
				"		<Descr>Set to enable the CONNECT method, used to proxy https:// requests.</Descr>"
				"		<Key name=\"" + basekey + "allowconnect\"/>"
				"	</Option>"

				"	<Option type=\"text\" name=\"connectports\" default=\"443\">"
				"		<Label>Restrict CONNECT to these ports</Label>"
				"		<Descr>The CONNECT method can be used to open undesired connections unless it's limited to"
				" the needed ports. Enter the ports you want to enable it for, one per line. Enter 0 if you really wish"
				" not to restrict it.</Descr>"
				"		<Key name=\"" + basekey + "connectports\"/>"
				"	</Option>"

				"	<Option type=\"list\" name=\"hostmap\">"
				"		<Label>Host map</Label>"
				"		<Descr>Hosts mapping table.</Descr>"
				"		<Key name=\"" + basekey + "HostMap\"/>"
				"		<ListKey name=\"host\">"
				"			<Label>Hostname</Label>"
				"			<Descr>The hostname to map.</Descr>"
				"		</ListKey>"
				"		<Fields>"
				"			<Option type=\"string\" name=\"target\" default=\"\" show=\"yes\">"
				"				<Label>Target</Label>"
				"				<Descr>The name of the host where requests for Host should be redirected to.</Descr>"
				"				<Key name=\"target\"/>"
				"			</Option>"
				"			<Option type=\"integer\" name=\"port\" default=\"80\" show=\"yes\">"
				"				<Label>Port</Label>"
				"				<Descr>The port to redirected to.</Descr>"
				"				<Key name=\"port\"/>"
				"			</Option>"
				"		</Fields>"
				"	</Option>"

				"	<Option type=\"list\" name=\"filters\">"
				"		<Label>Filters</Label>"
				"		<Descr>URL filtering table.</Descr>"
				"		<Key name=\"" + basekey + "Filters\"/>"
				"		<ListKey name=\"label\">"
				"			<Label>Label</Label>"
				"			<Descr>A label to identify the filter.</Descr>"
				"		</ListKey>"
				"		<Fields>"
				"			<Option type=\"text\" name=\"url\" default=\"\" show=\"no\">"
				"				<Label>URLs</Label>"
				"				<Descr>A list of URL's which this filter should be applied to. You can use regular expressions; don't forget to start them with \"http://\", if needed.</Descr>"
				"				<Key name=\"url\"/>"
				"			</Option>"
				"			<Option type=\"select\" name=\"action\" default=\"noaction\" show=\"yes\">"
				"				<Items>"
				"					<Item value=\"noaction\">no action</Item>"
				"					<Item value=\"nocache\">don't cache</Item>"
				"					<Item value=\"forbid\">forbid</Item>"
				"					<Item value=\"redirect\">redirect</Item>"
				"				</Items>"
				"				<Label>Action</Label>"
				"				<Descr>What to do when the filter is triggered.</Descr>"
				"				<Key name=\"action\"/>"
				"			</Option>"
				"			<Option type=\"string\" name=\"redirect\" default=\"\" show=\"no\">"
				"				<Label>Redirect to</Label>"
				"				<Descr>The URL to redirect to. It's ignored if action is not \"redirect\".</Descr>"
				"				<Key name=\"redirect\"/>"
				"			</Option>"
				"		</Fields>"
				"	</Option>"

				"</Page>";

	return( Manifest.c_str() );
}
//---------------------------------------------------------------------------
static HANDLE SetupModule( const char *key )
{
	return( new HTTPProxy( key ));
}
//---------------------------------------------------------------------------
static BOOL CleanupModule( HANDLE mod )
{
	BOOL ret = TRUE;

	if( mod ) {
		HTTPProxy *proxy = (HTTPProxy *)mod;

		ret = proxy->Cleanup();

		if( ret )
			delete proxy;
	}

	return( ret );
}
//---------------------------------------------------------------------------
static void CfgChanged( HANDLE mod )
{
	if( mod )
		((HTTPProxy *)mod )->ReloadCfg();
}
//---------------------------------------------------------------------------
static void OnFork( HANDLE mod )
{
	if( mod )
		((HTTPProxy *)mod )->OnFork();
}
//---------------------------------------------------------------------------
static void OnTimer( HANDLE mod, time_t now )
{
	if( mod )
		((HTTPProxy *)mod )->OnTimer( now );
}
//---------------------------------------------------------------------------
HTTPProxy::HTTPProxy( const char *key )
{
	Key             = key;
	Port            = 8080;
	ListeningSocket = NULL;
	MaxObjectSize   = 600 * 1024;
	HostMap         = new HostMapper( Key + "/HostMap" );
	Filters         = new FilterMgr( Key + "/Filters" );
	CONNECTPorts	= NULL;
	CONNECTNumPorts = 0;

	time( &LastPruneTime );

	LastIndexTime = LastPruneTime;

	Flags.Set( MODF_COMPRESS | MODF_LOG_REQUESTS | MODF_ALLOW_CONNECT );

	ReloadCfg();
}
//---------------------------------------------------------------------------
HTTPProxy::~HTTPProxy()
{
	delete ListeningSocket;
	delete HostMap;
	delete[] CONNECTPorts;

	CacheMgr.WriteIndex();
}
//---------------------------------------------------------------------------
void HTTPProxy::ReloadCfg( void )
{
	unsigned long maxcache = CacheMgr.GetMaxSize();

	if( App->Cfg->OpenKey( Key.c_str(), false )) {
		string				tmp;
		string::size_type	pos;

		Port          = App->Cfg->GetInteger( "port", Port );
		maxcache      = App->Cfg->GetInteger( "maxcachesize", maxcache );
		MaxObjectSize = App->Cfg->GetInteger( "maxobjectsize", MaxObjectSize );

		Flags.Set( MODF_COMPRESS,      App->Cfg->GetInteger( "gzipencoding", Flags.IsSet( MODF_COMPRESS      )));
		Flags.Set( MODF_LOG_REQUESTS,  App->Cfg->GetInteger( "logrequests",  Flags.IsSet( MODF_LOG_REQUESTS  )));
		Flags.Set( MODF_BLOCK_POPUPS,  App->Cfg->GetInteger( "blockpopups",  Flags.IsSet( MODF_BLOCK_POPUPS  )));
		Flags.Set( MODF_BLOCK_RESIZE,  App->Cfg->GetInteger( "blockresize",  Flags.IsSet( MODF_BLOCK_RESIZE  )));
		Flags.Set( MODF_ALLOW_CONNECT, App->Cfg->GetInteger( "allowconnect", Flags.IsSet( MODF_ALLOW_CONNECT )));

		delete[] CONNECTPorts;

		tmp = App->Cfg->GetString( "connectports", "443" );

		if(( pos = tmp.find_first_not_of( " " )) != string::npos )
			tmp.erase( 0, pos );

		if( tmp == "0" )
			CONNECTPorts = NULL;
		else {
			int	num = 1;

			pos = tmp.find( "\n" );

			while( pos != string::npos ) {
				num++;
				pos = tmp.find( "\n", pos + 1 );
			}
			
			CONNECTPorts      = new int[ num ];
			CONNECTNumPorts   = num;
			CONNECTPorts[ 0 ] = atoi( tmp.c_str() );
			pos               = tmp.find( "\n" );
			num               = 1;

			while( pos != string::npos ) {

				tmp.erase( 0, pos + 1 );
				
				CONNECTPorts[ num++ ] = atoi( tmp.c_str() );
				
				pos = tmp.find( "\n" );
			}
		}

		App->Cfg->CloseKey();
	}

	Setup();
	CacheMgr.SetMaxSize( maxcache );
	HostMap->ReloadCfg();
	Filters->Load();
}
//---------------------------------------------------------------------------
void HTTPProxy::Setup( void )
{
	if( !ListeningSocket || ( ListeningSocket->GetLocalPort() != Port )) {

		delete ListeningSocket;

		ListeningSocket = new TcpSocket();

		ListeningSocket->SetAsyncCallback( SocketCB, this );
		ListeningSocket->UseDispatcher( App->IO );

		if( ListeningSocket->Bind( Port )) {

			if( !ListeningSocket->Listen())
				App->Log->Log( LOG_ERR,
							   "mod_http: cannot listen on port %d - %s",
							   Port, strerror( errno ));

		} else
			App->Log->Log( LOG_ERR,
						   "mod_http: cannot bind on port %d - %s",
						   Port, strerror( errno ));
	}
}
//---------------------------------------------------------------------------
bool HTTPProxy::Cleanup( void )
{
	while( !Sockets.IsEmpty() ) {
		HTTPData *data = (HTTPData *)Sockets.RemHead();

		// we must make sure that we don't leave potential
		// callbacks around
		if( data->State == S_RESOLVING )
			App->DNS->CancelRequest( data->Client.GetURL().GetHost(), DNSCallback, data );

		if( data->Cached )
			data->Cached->Release();

		if( data->ServerSock ) {
			Connections.RemoveConnection( data->Client.GetURL().GetHostPort(), data->ServerSock );
			delete data->ServerSock;
		}

		delete data->ClientSock;
		delete data;
	}

	Connections.Clear();
	CacheMgr.WriteIndex();

	return( true );
}
//---------------------------------------------------------------------------
void HTTPProxy::OnFork( void )
{
	delete ListeningSocket;

	ListeningSocket = NULL;

	Cleanup();
}
//---------------------------------------------------------------------------
void HTTPProxy::OnTimer( time_t now )
{
	CacheMgr.Flush();

	if( now > LastPruneTime + INTERVAL_PRUNE ) {
		LastPruneTime = now;
		CacheMgr.Prune();
	}

	if( now > LastIndexTime + INTERVAL_INDEX ) {
		LastIndexTime = now;
		CacheMgr.WriteIndex();
	}
}
//---------------------------------------------------------------------------
void HTTPProxy::SocketEvent( Socket *sock, Prom_SC_Reason reason, int data )
{
	switch( reason ) {

		case PROM_SOCK_ACCEPT:
			Accept((TcpSocket *)data );
			break;

		case PROM_SOCK_ERROR:
		case PROM_SOCK_TIMEOUT:
			Error(( HTTPData * )sock->GetUserData(), data, (TcpSocket *)sock );
			break;

		case PROM_SOCK_READ:
			GotData(( HTTPData * )sock->GetUserData(), data, sock );
			break;

		case PROM_SOCK_WRITTEN:
			Written(( HTTPData * )sock->GetUserData(), data, sock );
			break;

		case PROM_SOCK_CONNECTED:
			if( data )
				Error(( HTTPData * )sock->GetUserData(), data, (TcpSocket *)sock );
			else
				Connected(( HTTPData * )sock->GetUserData() );
			break;
	}
}
//---------------------------------------------------------------------------
static void SocketCB( SOCKREF sock, Prom_SC_Reason reason, int data, void *userdata )
{
	((HTTPProxy *)userdata )->SocketEvent( sock, reason, data );
}
//---------------------------------------------------------------------------
void HTTPProxy::Accept( TcpSocket *sock )
{
	if( sock->IsValid() ) {
		HTTPData *data = new HTTPData();

		Sockets.AddTail( data );

		DBG( App->Log->Log( LOG_ERR, "HTTPProxy::Accept( %d ) - data = %08x", sock->GetFD(), data ));

		data->Proxy      = this;
		data->ClientSock = sock;
		data->ServerSock = NULL;
		data->Cached     = NULL;
		data->State      = S_REQUEST;

		sock->SetUserData( data, NULL );
		sock->UseDispatcher( App->IO );
		sock->SetAsyncCallback( SocketCB, this );

		data->Client.SetSocket( data->ClientSock );

		data->ClientSock->AsyncRecv( data->ClientBuf, sizeof( data->ClientBuf ), 0, TIMEOUT_REQUEST );

	} else
		delete sock;
}
//---------------------------------------------------------------------------
void HTTPProxy::Error( HTTPData *data, int err, TcpSocket *sock )
{
	DBG( App->Log->Log( LOG_ERR, "HTTPProxy::Error( %08x, %d, %d ) - state = %d, sstate = %d",
						data, err, sock->GetFD(), data->State, data->ServerState  ));

	bool	closeit = sock == data->ClientSock;

	if( !closeit ) {
		int oldstate = data->ServerState;

		data->ServerState = S_CLOSING;

		switch( oldstate ) {

			case S_CONNECTING:
				SendError( data, HTTP_GATEWAY_TIMEOUT, "Cannot connect to the requested host." );
				break;

			case S_SERVER_RESPONSE:
				SendError( data, HTTP_BAD_GATEWAY, "An error occurred while receiving the server response." );
				break;

			case S_BODY:
				StoreObj *obj;

				if( data->Cached && ( obj = data->Cached->GetStoreObj( false )))
					obj->WriteComplete( false ); // this will also trigger a call to CloseServerSocket()
				else
					closeit = !CloseServerSocket( data );
				break;

			default:
				closeit = !CloseServerSocket( data );
				break;
		}

	} else if(( data->ServerState != S_BODY ) || !IsCacheable( data ))
		closeit = !CloseServerSocket( data );

	if( closeit )
		CloseClientSocket( data );
}
//---------------------------------------------------------------------------
void HTTPProxy::GotData( HTTPData *data, int len, Socket *sock )
{
	DBG( App->Log->Log( LOG_ERR, "HTTPProxy::GotData( %08x, %d, %d )", data, len, sock->GetFD() ));

	if( len == 0 ) {

		SocketClosed( data, sock );

	} else if( sock == data->ClientSock ) {
		bool	more = false;

		switch( data->State ) {

			case S_REQUEST:
				if( data->Client.AddHeaderData( data->ClientBuf, len ))
					HandleRequest( data );
				else
					more = true;
				break;

			case S_TUNNELING:
				data->ServerSock->AsyncSend( data->ClientBuf, len );
				more = true;
				break;
		}

		if( more && data->ClientSock )
			data->ClientSock->AsyncRecv( data->ClientBuf, sizeof( data->ClientBuf ), 0, TIMEOUT_DATA );

	} else {
		bool	more = false;

		switch( data->ServerState ) {

			case S_SERVER_RESPONSE:
				if( data->Server.AddHeaderData( data->ServerBuf, len ))
					HandleResponse( data );
				else
					more = true;
				break;

			case S_TUNNELING:
				data->ClientSock->AsyncSend( data->ServerBuf, len );
				more = true;
				break;

			case S_BODY:
				HandleBody( data, len );
				break;
		}

		if( more )
			data->ServerSock->AsyncRecv( data->ServerBuf, sizeof( data->ServerBuf ), 0, TIMEOUT_DATA );
	}
}
//---------------------------------------------------------------------------
void HTTPProxy::SocketClosed( HTTPData *data, Socket *sock )
{
	DBG( App->Log->Log( LOG_ERR, "HTTPProxy::SocketClosed( %08x ) - fd: %d", data, sock->GetFD() ));

	if( sock == data->ServerSock ) {

		switch( data->ServerState ) {

			case S_BODY:
				HandleBody( data, 0 );
				break;

			case S_TUNNELING:
				if( !CloseServerSocket( data ))
					CloseClientSocket( data );
				break;

			case S_SERVER_RESPONSE:
				data->ServerState = S_CLOSING;
				
				if( data->Cached )
					SendFromCache( data );
				else
					SendError( data, HTTP_BAD_GATEWAY, "The server closed the connection unexpectedly." );
				break;

			default:
				data->ServerState = S_CLOSING;

				SendError( data, HTTP_BAD_GATEWAY, "The server closed the connection unexpectedly." );
				break;
		}

	} else {

		if(( data->ServerState != S_BODY ) || !IsCacheable( data ))
			CloseServerSocket( data );

		CloseClientSocket( data );
	}
}
//---------------------------------------------------------------------------
void HTTPProxy::HandleRequest( HTTPData *data )
{
	if( Flags.IsSet( MODF_LOG_REQUESTS ))
		App->Log->Log( LOG_INFO, "mod_http: %s \"%s %s %s\"",
					   data->ClientSock->GetPeerName(),
					   data->Client.GetMethodStr(),
					   data->Client.GetURL().Encode(),
					   data->Client.GetProtocol() );

	if( !FilterRequest( data ))
		switch( data->Client.GetMethod() ) {

			case HTTP::M_PUT:
			case HTTP::M_POST:
				ConnectToServer( data );
				break;

			case HTTP::M_CONNECT:
				Handle_CONNECT( data );
				break;

			case HTTP::M_GET:
			case HTTP::M_HEAD:
				Handle_GET_HEAD( data );
				break;

			default:
				SendError( data, HTTP_NOT_IMPLEMENTED,
						"Your browser used a method I don't understand." );
				break;
		}
}
//---------------------------------------------------------------------------
bool HTTPProxy::FilterRequest( HTTPData *data )
{
	Filter *filter = Filters->Match( data->Client.GetURL().Encode() );
	bool	ret = false;;

	if( filter )
		switch( filter->GetAction() ) {

			case Filter::F_REDIRECT:
				data->Client.Redirect( filter->GetTarget().c_str() );
				ResetConnection( data );

				ret = true;
				break;

			case Filter::F_FORBID:
				SendError( data, HTTP_FORBIDDEN,
						   "Sorry, you're not authorized to see this page." );
				ret = true;
				break;
		}

	return( ret );
}
//---------------------------------------------------------------------------
void HTTPProxy::Handle_CONNECT( HTTPData *data )
{
	bool	ok = false;
		
	if( Flags.IsSet( MODF_ALLOW_CONNECT )) {

		if( !CONNECTPorts )
			ok = true;
		else {
				
			for( int i = 0; !ok && ( i < CONNECTNumPorts ); i++ )
				if( data->Client.GetURL().GetPort() == CONNECTPorts[ i ] )
					ok = true;
		}
	}
	
	if( ok )
		ConnectToServer( data );
	else
		SendError( data, HTTP_FORBIDDEN, "Sorry, you can't perform this operation." );
}
//---------------------------------------------------------------------------
void HTTPProxy::SendError( HTTPData *data, int code, const char *text )
{
	bool reset = true;

	DBG( App->Log->Log( LOG_ERR, "HTTPProxy::SendError( %08x, %d, %s )", data, code, text ));

	if( data->ClientSock ) {

		if( data->ServerState == S_CLOSING )
			CloseServerSocket( data );
			
		if( !data->Client.HeaderSent() )
			data->Client.ErrorMsg( code, text );
		else
			reset = !CloseClientSocket( data );
	}

	if( reset )
		ResetConnection( data );
}
//---------------------------------------------------------------------------
void HTTPProxy::ResetConnection( HTTPData *data )
{
	DBG( App->Log->Log( LOG_ERR, "HTTPProxy::ResetConnection( %08x )", data ));

	if( !ResetServerConnection( data )) {

		if( data->ClientSock && ( data->State != S_CLOSING ) && data->Client.KeepAlive() ) {
			Buffer	tmp;
			char	*ptr;
			int		len = 0;

			if( data->Cached ) {
				data->Cached->Release();
				data->Cached = NULL;
			}

			data->State = S_REQUEST;

			// the client might pipeline requests, in which
			// case we might have part of the next request in the buffer
			ptr = data->Client.GetDecodedData( &len );

			if( len > 0 )
				tmp.SetContent( ptr, len );

			data->Client.Reset();
			data->Server.Reset();

			if(( len > 0 ) && data->Client.AddHeaderData( tmp.GetData(), len ))
				HandleRequest( data );
			else
				data->ClientSock->AsyncRecv( data->ClientBuf, sizeof( data->ClientBuf ), 0, TIMEOUT_REQUEST );

		} else
			CloseClientSocket( data );
	}
}
//---------------------------------------------------------------------------
bool HTTPProxy::ResetServerConnection( HTTPData *data )
{
	bool	deleted = false, closing = data->ServerState == S_CLOSING;

	DBG( App->Log->Log( LOG_ERR, "HTTPProxy::ResetServerConnection( %08x )", data ));
	
	data->ServerState = S_REQUEST;

	if( data->ServerSock && data->Server.KeepAlive() && !closing ) {

		Connections.ReleaseConnection( data->Client.GetURL().GetHostPort(), data->ServerSock );

		data->ServerSock = NULL;

	} else
		deleted = CloseServerSocket( data );

	return( deleted );
}
//---------------------------------------------------------------------------
bool HTTPProxy::CloseClientSocket( HTTPData *data )
{
	DBG( App->Log->Log( LOG_ERR, "HTTPProxy::CloseClientSocket( %08x )", data ));
		
	if( data->State == S_RESOLVING )
		App->DNS->CancelRequest( data->Client.GetURL().GetHost(), DNSCallback, data );

	data->State = S_CLOSING;

	if( data->ClientSock && !data->ClientSock->WritePending() ) {

		delete data->ClientSock;

		data->ClientSock = NULL;
	}

	return( FreeHTTPData( data ));
}
//---------------------------------------------------------------------------
void HTTPProxy::Written( HTTPData *data, int len, Socket *sock )
{
	// all data has been sent, we can close the client socket if needed
	if( data && ( sock == data->ClientSock ) && ( data->State == S_CLOSING ))
		CloseClientSocket( data );
}
//---------------------------------------------------------------------------
bool HTTPProxy::CloseServerSocket( HTTPData *data )
{
	DBG( App->Log->Log( LOG_ERR, "HTTPProxy::CloseServerSocket( %08x )", data ));

	data->ServerState = S_CLOSING;
		
	if( data->ServerSock ) {

		Connections.RemoveConnection( data->Client.GetURL().GetHostPort(), data->ServerSock );

		delete data->ServerSock;

		data->ServerSock = NULL;
	}

	return( FreeHTTPData( data ));
}
//---------------------------------------------------------------------------
bool HTTPProxy::FreeHTTPData( HTTPData *data )
{
	bool deleted = false;

	DBG( App->Log->Log( LOG_ERR, "HTTPProxy::FreeHTTPData( %08x )", data ));
	
	// if any socket is still open we can't delete the object now,
	// it can be still be used!
	if( data && !data->ClientSock && !data->ServerSock ) {

		DBG( App->Log->Log( LOG_ERR, "HTTPProxy::FreeHTTPData( %08x ) - data->Cached = %08x",
							data, data->Cached ));

		if( data->Cached ) {

			if( StoreObj *obj = data->Cached->GetStoreObj( false ))
				obj->RemListener( ::CacheObjListener, data );
				
			data->Cached->Release();
		}

		delete data;

		deleted = true;
	}

	return( deleted );
}
//---------------------------------------------------------------------------
void HTTPProxy::ConnectToServer( HTTPData *data )
{
	URL&	url = data->Client.GetURL();

	if( !url.GetHost()[0] ) {
		short	port;

		if( data->ClientSock->GetOriginalDest( &data->Addr, &port )) {

			url.SetPort( port );

			Resolved( data, sizeof( data->Addr ));

		} else
			SendError( data, HTTP_FORBIDDEN, "This is just a proxy, you can't request local resources!" );

	} else if(( data->Client.GetMethod() != HTTP::M_CONNECT ) && 
				strcmp( url.GetScheme(), "http" )) {

		SendError( data, HTTP_NOT_IMPLEMENTED, "Unsupported scheme." );

	} else {
		const char	*host;
		short		port = url.GetPort();

		host = HostMap->Map( url.GetHost(), &port );

		url.SetHost( host );
		url.SetPort( port );

		if( data->ServerSock )
			CloseServerSocket( data );

		data->ServerSock = Connections.FindConnection( url.GetHostPort() );

		if( data->ServerSock )
			Connected( data );
		else {

			data->State       = S_RESOLVING;
			data->ServerState = S_RESOLVING;

			App->DNS->AsyncResolve( url.GetHost(), &data->Addr, DNSCallback, data );
		}
	}
}
//---------------------------------------------------------------------------
static void DNSCallback( int addrlen, void *userdata )
{
	HTTPData *data = (HTTPData *)userdata;

	data->Proxy->Resolved( data, addrlen );
}
//---------------------------------------------------------------------------
void HTTPProxy::Resolved( HTTPData *data, int addrlen )
{
	if( data->ClientSock && addrlen ) {

		if( data->ServerSock )
			CloseServerSocket( data );

		data->ServerSock = new TcpSocket();

	    if( data->ServerSock->IsValid() ) {

			data->State       = S_CONNECTING;
			data->ServerState = S_CONNECTING;

			data->ServerSock->UseDispatcher( App->IO );
			data->ServerSock->SetAsyncCallback( SocketCB, data->Proxy );
			data->ServerSock->SetUserData( data, NULL );

			data->ServerSock->AsyncConnect( &data->Addr, data->Client.GetURL().GetPort(), TIMEOUT_CONNECT );

		} else {

			data->ServerState = S_CLOSING;

			SendError( data, HTTP_INTERNAL_ERROR, "Cannot create a new socket." );
		}

	} else
		SendError( data, HTTP_GATEWAY_TIMEOUT, "Cannot resolve the requested host." );
}
//---------------------------------------------------------------------------
void HTTPProxy::Connected( HTTPData *data )
{
	DBG( App->Log->Log( LOG_ERR, "HTTPProxy::Connected( %08x ) - fd: %d", data, data->ServerSock->GetFD() ));
	
	data->ServerSock->SetUserData( data, NULL );
	data->ServerSock->SetAsyncCallback( SocketCB, data->Proxy );

	data->Server.SetSocket( data->ServerSock );

	switch( data->Client.GetMethod() ) {

		case HTTP::M_GET:
			SendRequest( "GET", data );
			break;

		case HTTP::M_HEAD:
			SendRequest( "HEAD", data );
			break;

		case HTTP::M_CONNECT:
			data->Client.SendHeader( HTTP_OK, NULL, true, NULL );
			InitTunnel( data );
			break;

		default:
			StartTunneling( data );
			break;
	}
}
//---------------------------------------------------------------------------
void HTTPProxy::StartTunneling( HTTPData *data )
{
	DBG( App->Log->Log( LOG_ERR, "HTTPProxy::StartTunneling( %08x )", data ));

	if( data->ClientSock && data->ServerSock ) {
		int i = 0;

		data->ServerSock->AsyncPrintf( "%s %s %s\r\n",
									   data->Client.GetMethodStr(),
									   data->Client.GetURL().GetRest(),
									   data->Client.GetProtocol() );

		while( const char *hdr = data->Client.GetHeader( i++ )) {
			char		buffer[ 512 ], *to = buffer;
			const char	*from = hdr;
			int			n = 0;

			while(( n < sizeof( buffer ) - 1 ) && *from && ( *from != ':' )) {
				*to++ = tolower( *from++ );
				n++;
			}

			*to = '\0';

			if( strcmp( buffer, "connection" ) &&
				strcmp( buffer, "proxy-connection" ) &&
				strcmp( buffer, "keep-alive" ))
				data->ServerSock->AsyncPrintf( "%s\r\n", hdr );
		}
		
		data->ServerSock->AsyncPrintf( "\r\n" );

		InitTunnel( data );

	} else {
		// the client must have dropped the connection while we were
		// waiting to connect to the origin server, so just clean up

		if( !CloseServerSocket( data )) 
			CloseClientSocket( data );
	}
}
//---------------------------------------------------------------------------
void HTTPProxy::InitTunnel( HTTPData *data )
{	
	int			i;
	const char	*ptr;
	
	data->State       = S_TUNNELING;
	data->ServerState = S_TUNNELING;

	ptr = data->Client.GetDecodedData( &i );

	if( i > 0 )
		data->ServerSock->AsyncSend( ptr, i );

	// an error in the first AsyncRecv() would trigger a destruction of ServerSock
	// and possibly of data as well - see HTTPProxy::Error()
	if( data->ClientSock->AsyncRecv( data->ClientBuf, sizeof( data->ClientBuf )))
		data->ServerSock->AsyncRecv( data->ServerBuf, sizeof( data->ServerBuf ));
}
//---------------------------------------------------------------------------
void HTTPProxy::Handle_GET_HEAD( HTTPData *data )
{
	data->Cached = CacheMgr.Find( data->Client.GetEntityID() );

	if( !data->Client.Validate() && data->Cached && data->Cached->IsFresh() ) {
		bool		notmod = false, pre = false;
		const char	*etag = data->Cached->GetETag();

		// check If-None-Match/If-Match conditions
		if( etag[ 0 ] ) {
			const char *ptr = data->Client.GetETag();

			if( data->Client.IsNoneMatchValid() ) {

				if( strchr( ptr, '*' ) || strstr( ptr, etag ))
					notmod = true;

			} else if( data->Client.IsMatchValid() ) {

				if( !strchr( ptr, '*' ) && !strstr( ptr, etag ))
					pre = true;
			}
		}

		// check If-Modified-Since/If-Unmodified-Since conditions
		if( !notmod && ( data->Client.IsModifiedSinceValid() ))
			notmod = data->Cached->GetLastModified() <= data->Client.GetModifiedSince();
		else if( !pre && ( data->Client.IsUnmodifiedSinceValid() ))
			pre = data->Cached->GetLastModified() > data->Client.GetUnmodifiedSince();

		if( notmod )
			data->Client.SendHeader( HTTP_NOT_MODIFIED, NULL, !data->Client.KeepAlive(), NULL );
		else if( pre ) {
			StringList	headers;

			headers.Add( "Content-Length: 0" );
			data->Client.SendHeader( HTTP_PRECONDITION_FAILED, NULL, !data->Client.KeepAlive(), &headers );

		} else
			SendFromCache( data );

		if( notmod || pre )
			ResetConnection( data );

	} else
		ConnectToServer( data );
}
//---------------------------------------------------------------------------
void HTTPProxy::SendRequest( const char *method, HTTPData *data )
{
	StringList	headers;
	const char	*ptr;
	int			i = 0, port = data->Client.GetURL().GetPort();
	bool		cond = true;

	// some broken servers seem to think that host:80 and host are
	// different...
	if( port == 80 )
		headers.Add( "Host: %s", data->Client.GetURL().GetHost() );
	else
		headers.Add( "Host: %s:%d", data->Client.GetURL().GetHost(), port );

	headers.Add( "Connection: Keep-Alive" );

	if( data->Client.IsAuthValid() )
		headers.Add( "Authorization: %s", data->Client.GetAuthorization() );

	while( ptr = data->Client.GetHeader( i++ )) {
		char		str[ 1024 ], *to = str;
		const char	*from = ptr;
		int			n = 0;

		while(( n < sizeof( str ) - 1 ) && *from ) {
			*to++ = tolower( *from++ );
			n++;
		}

		*to = '\0';

		if( !strncmp( str, "referer:", 8 ) ||
		    !strncmp( str, "accept-charset:", 15 ) ||
		    !strncmp( str, "accept-language:", 16 ) ||
		    !strncmp( str, "cookie:", 7 ) ||
		    !strncmp( str, "user-agent:", 11 ))
			headers.Add( "%s", ptr );

		if( !strncmp( str, "range:", 6 )) {
			headers.Add( "%s", ptr );
			cond = false;
		}
    }

	if( cond && data->Cached && !data->Client.NotFromCache() ) {
		const char *etag = data->Cached->GetETag();

		if( etag[0] )
			headers.Add( "If-None-Match: %s", etag );

		headers.Add( "If-Modified-Since: %s", data->Cached->GetLastModified().ToStr() );
	}

	data->Server.SendMethod( method, data->Client.GetURL().GetRest(), &headers );

	data->ServerState = S_SERVER_RESPONSE;

	data->ServerSock->AsyncRecv( data->ServerBuf, sizeof( data->ServerBuf ), 0, TIMEOUT_DATA );
}
//---------------------------------------------------------------------------
void HTTPProxy::HandleResponse( HTTPData *data )
{
	switch( data->Server.GetReturnCode() ) {

		case HTTP_OK:
			if( data->Client.GetMethod() == HTTP::M_HEAD )
				ForwardResponse( data );		
			else
				DownloadAndCache( data );
			break;

		case HTTP_NOT_MODIFIED:
			SendFromCache( data );
			break;

		default:
			ForwardResponse( data );
			break;
	}
}
//---------------------------------------------------------------------------
void HTTPProxy::ForwardResponse( HTTPData *data )
{
	// don't cache the object, only forward it to the client
	if( data->Cached ) {
		data->Cached->Release();
		data->Cached = NULL;
	}

	if( data->ClientSock ) {
		StringList	headers;

		data->State       = S_BODY;
		data->ServerState = S_BODY;

		CopyServerHeaders( data, headers );
		data->Client.SendHeader( data->Server.GetReturnCode(), NULL, !data->Client.KeepAlive(), &headers );

		if(( data->Client.GetMethod() == HTTP::M_HEAD ) || !data->Server.HasEntityBody() )
			ResetConnection( data );	
		else
			ProcessFirstData( data );

	} else
		CloseServerSocket( data );
}
//---------------------------------------------------------------------------
// processes the first data received right after the response header, if any
void HTTPProxy::ProcessFirstData( HTTPData *data )
{
	char	*ptr;
	int		len;

	ptr = data->Server.GetDecodedData( &len );

	if( len > 0 ) {

		memcpy( data->ServerBuf, ptr, len );

		HandleBody( data, len );

	} else if( data->ServerSock )
		data->ServerSock->AsyncRecv( data->ServerBuf, sizeof( data->ServerBuf ), 0, TIMEOUT_DATA );
	else
		CloseClientSocket( data );
}
//---------------------------------------------------------------------------
void HTTPProxy::DownloadAndCache( HTTPData *data )
{
	StringList	headers;

	DBG( App->Log->Log( LOG_ERR, "HTTPProxy::DownloadAndCache( %08x )", data ));

	if( data->Cached ) {
		CacheMgr.Delete( data->Cached );
		data->Cached->Release();
		data->Cached = NULL;
	}

	if( IsCacheable( data ))
		data->Cached = CacheMgr.Create( data->Client.GetEntityID() );

	if( data->Cached ) {

		data->Cached->SetURI( data->Client.GetURL().Encode() );
		data->Cached->SetETag( data->Server.GetETag() );
		data->Cached->SetMIMEType( data->Server.GetMIMEType() );

		if( data->Server.IsDateValid() )
			data->Cached->SetCreated( data->Server.GetDate().ToEpoch() );

		if( data->Server.IsLastModifiedValid() )
			data->Cached->SetLastModified( data->Server.GetLastModified() );

		if( data->Server.IsExpiresValid() )
			data->Cached->SetExpires( data->Server.GetExpires() );
		else if( int max = data->Server.GetMaxAge() )
			data->Cached->SetMaxAge( max );

		PrepareHeaders( data, headers );

		if( data->Client.Is11() )
			headers.Add( "Transfer-Encoding: chunked" );

		headers.Add( "Content-Type: %s", data->Cached->GetMIMEType() );

		data->Cached->GetStoreObj()->AddListener( ::CacheObjListener, data );

	} else
		CopyServerHeaders( data, headers );

	data->State       = S_BODY;
	data->ServerState = S_BODY;

	if( data->ClientSock )
		data->Client.SendHeader( HTTP_OK, NULL, !data->Client.KeepAlive(), &headers );

	ProcessFirstData( data );
}
//---------------------------------------------------------------------------
static void CacheObjListener( void *userdata, char *data, int len )
{
	((HTTPData *)userdata )->Proxy->CacheObjListener( (HTTPData *)userdata, data, len );
}
//---------------------------------------------------------------------------
void HTTPProxy::CacheObjListener( HTTPData *data, char *buf, int len )
{
	DBG( App->Log->Log( LOG_ERR, "HTTPProxy::CacheObjListener( %08x, -, %d ) - data->Cached = %08x",
						data, len, data->Cached ));

	if( len >= 0 ) {

		if( data->ClientSock )
			data->Client.SendBodyData( buf, len );

		if( len == 0 ) { // end of object

			DBG( App->Log->Log( LOG_ERR, "HTTPProxy::CacheObjListener( %08x, -, - ) EOF", data ));

			if( StoreObj *obj = data->Cached->GetStoreObj() )
				obj->RemListener( ::CacheObjListener, data );

			ResetConnection( data );

		} else if( data->Cached->GetSize() > MaxObjectSize ) {
			DBG( App->Log->Log( LOG_ERR, "HTTPProxy::CacheObjListener( %08x, -, - ) DoR", data ));
			CacheMgr.Delete( data->Cached );
		}

	} else { // write error - this cache obj must be deleted

		DBG( App->Log->Log( LOG_ERR, "HTTPProxy::CacheObjListener( %08x, -, - ) ERROR", data ));

		if( StoreObj *obj = data->Cached->GetStoreObj() )
			obj->RemListener( ::CacheObjListener, data );

		CacheMgr.Delete( data->Cached );

		data->ServerState = S_CLOSING;

		SendError( data, HTTP_INTERNAL_ERROR, "Error reading/writing the cache object." );
	}
}
//---------------------------------------------------------------------------
void HTTPProxy::SendFromCache( HTTPData *data )
{
	if( !ResetServerConnection( data )) {
		StoreObj *obj =  data->Cached->GetStoreObj();

		if( obj ) {
			StringList	headers;
			bool		done;

			data->State = S_BODY;

			PrepareHeaders( data, headers );

			if( data->Client.Is11() )
				headers.Add( "Transfer-Encoding: chunked" );

			// XXX check Range:
			data->Client.SendHeader( HTTP_OK, data->Cached->GetMIMEType(), !data->Client.KeepAlive(), &headers );

			done = data->Client.GetMethod() == HTTP::M_HEAD;

			if( !done ) {

				if( unsigned int size = obj->GetSize() )
					data->Client.SendBodyData( obj->GetData(), size );

				if( obj->IsReading() )
					obj->AddListener( ::CacheObjListener, data );
				else {
					data->Client.SendBodyData( NULL, 0 );
					done = true;
				}
			}

			if( done )
				ResetConnection( data );

		} else
			SendError( data, HTTP_INTERNAL_ERROR, "Cannot open the cache object!" );
	}
}
//---------------------------------------------------------------------------
void HTTPProxy::HandleBody( HTTPData *data, int len )
{
	bool	more, closeserver;
	char	*decoded;

	DBG( App->Log->Log( LOG_ERR, "HTTPProxy::HandleBody( %08x, %d )", data, len ));

	if( len == 0 )
		data->ServerState = S_CLOSING;

	more    = !data->Server.DecodeBodyData( data->ServerBuf, len );
	decoded = data->Server.GetDecodedData( &len );


	DBG( App->Log->Log( LOG_ERR, "HTTPProxy::HandleBody( %08x, - ) - more = %d, len = %d, data->Cached = %08x",
					 	data, more, len, data->Cached ));
	
	if( len > 0 ) {

		if( data->Cached ) {

			// this will also send the data to the client
			// since we're listening on this object
			data->Cached->Write( decoded, len );

		} else if( data->ClientSock )
			data->Client.SendBodyData( decoded, len );

	} else if( len < 0 ) {

		if( data->Cached )
			data->Cached->GetStoreObj()->WriteComplete( false );
		else {

			data->ServerState = S_CLOSING;

			SendError( data, HTTP_BAD_GATEWAY, "Received corrupt data from the origin server." );
		}
	}

	if( more )
		data->ServerSock->AsyncRecv( data->ServerBuf, sizeof( data->ServerBuf ), 0, TIMEOUT_DATA );

	else if( len >= 0 ) {

		if( data->Cached ) // this will also trigger a ResetConnection()
			data->Cached->GetStoreObj()->WriteComplete();
		else {

			// signal the end of the object
			if( data->ClientSock )
				data->Client.SendBodyData( NULL, 0 );

			ResetConnection( data );
		}
	}
}
//---------------------------------------------------------------------------
void HTTPProxy::CopyServerHeaders( HTTPData *data, StringList& headers )
{
	int i = 0;

	while( const char *hdr = data->Server.GetHeader( i++ )) {
		char        str[ 1024 ], *to = str;
		const char  *from = hdr;
		int         n = 0;

		while(( n < sizeof( str ) - 1 ) && *from && ( *from != ':' )) {
			*to++ = tolower( *from++ );
			n++;
		}

		*to = '\0';

		// skip the hop-to-hop headers
		if( strcmp( str, "transfer-encoding" ) &&
			strcmp( str, "proxy-connection" ) &&
			strcmp( str, "keep-alive" ) &&
			strcmp( str, "date" ) &&
			strcmp( str, "te" ) &&
			strcmp( str, "upgrade" ) &&
			strcmp( str, "content-length" ) &&
			strcmp( str, "content-encoding" ) &&
			strcmp( str, "connection" ))
			headers.Add( "%s", hdr );

		if( !strcmp( str, "content-type" )) {

			if( Flags.IsSet( MODF_BLOCK_POPUPS | MODF_BLOCK_RESIZE ) &&
				( strstr( hdr, "text/html" ) || strstr( hdr, "text/javascript" ))) {

				if( Flags.IsSet( MODF_BLOCK_POPUPS ))
					data->Client.AddFilter( new PopupBlocker() );

				if( Flags.IsSet( MODF_BLOCK_RESIZE ))
					data->Client.AddFilter( new ResizeBlocker() );
			}
		
#if HAVE_ZLIB_H
			// enable gzip encoding if the object is text
			if( Flags.IsSet( MODF_COMPRESS ) && strstr( hdr, "text/" ))
				data->Client.CompressBody();
#endif
		}
	}

	if( data->Server.HasEntityBody() && data->Client.Is11() )
		headers.Add( "Transfer-Encoding: chunked" );
}
//---------------------------------------------------------------------------
void HTTPProxy::PrepareHeaders( HTTPData *data, StringList& headers )
{
	const char *str;

	str = data->Cached->GetETag();

	if( str[0] )
		headers.Add( "ETag: %s", str );

	if( data->Cached->IsExpiresValid() )
		headers.Add( "Expires: %s", data->Cached->GetExpires().ToStr() );

	headers.Add( "Last-Modified: %s", data->Cached->GetLastModified().ToStr() );
	headers.Add( "Age: %d", data->Cached->GetAge() );

	if( data->Cached->GetAge() > ( 24 * 3600 ))
		headers.Add( "Warning: 113 prometeo-mod_http \"Heuristic expiration\"" );

	if( !strncmp( data->Cached->GetMIMEType(), "text/html", 9 ) ||
		!strncmp( data->Cached->GetMIMEType(), "text/javasript", 14 )) {

		if( Flags.IsSet( MODF_BLOCK_POPUPS ))
			data->Client.AddFilter( new PopupBlocker() );

		if( Flags.IsSet( MODF_BLOCK_RESIZE ))
			data->Client.AddFilter( new ResizeBlocker() );
	}
	
#if HAVE_ZLIB_H
	// enable gzip encoding if the object is text
	if( Flags.IsSet( MODF_COMPRESS ) &&
		!strncmp( data->Cached->GetMIMEType(), "text/", 5 ))
		data->Client.CompressBody();
#endif
}
//---------------------------------------------------------------------------
bool HTTPProxy::IsCacheable( HTTPData *data )
{
	bool	ret;

	ret = data->Client.IsCacheable() &&
		  data->Server.IsCacheable() &&
		 ( data->Server.GetSize() <= MaxObjectSize );

	if( ret ) {
		Filter *filter = Filters->Match( data->Client.GetURL().Encode() );

		if( filter && ( filter->GetAction() == Filter::F_DONT_CACHE ))
			ret = false;
	}

	return( ret );
}
//---------------------------------------------------------------------------
