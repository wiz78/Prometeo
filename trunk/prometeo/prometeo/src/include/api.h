/***************************************************************************
                                    api.h
                             -------------------
    copyright            : (C) 2002 by Simone Tellini
    email                : tellini@users.sourceforge.net
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef PROMETEO_API_H
#define PROMETEO_API_H

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>

#ifdef __cplusplus
#define CDECL "C"
#else
#define CDECL
#endif

#ifndef API_EXPORT
#define API_EXPORT(type)    extern CDECL type
#endif

#ifndef BOOL
#define BOOL	int
#endif
#ifndef TRUE
#define TRUE	1
#endif
#ifndef FALSE
#define FALSE	0
#endif

typedef void *HANDLE; /* Generic handle */

/* The current version of this API */
#define PROM_API_VERSION	1

/*
 * Modules types
 */
#define PROM_MT_LOGGER		1	/* the module adds functions to log events */
#define PROM_MT_CUSTOM		2	/* everything which doesn't match the previous
								   descriptions
								*/

/*
 * Use the following macro to define the module structure
 * in your code
 */
#define PROM_MODULE			Prom_Module PrometeoModule
#define PROM_MODINFO_NAME	"PrometeoModule"

/* Functions to be provided by PROM_MT_LOGGER modules */
typedef struct prometeo_loggerfuncs
{
	void			( *Log )( HANDLE mod, int level, char *fmt, ... );
} Prom_LoggerFuncs;

/*
 * The current definition of the module structure.
 */

typedef struct prometeo_module
{
	unsigned int	Version;		// API version
	const char 		*Name;			// Module's name
	unsigned short	Type;			// module type

	// returns the XML manifest of the module
	// 
	// key is the registry key containing the module configuration
	// 
	// name is the name assigned by the user to the module (it's possible
	// to load the same module several times with different names and
	// settings)
	// 
	// REQUIRED
	const char		*( *GetManifest )( const char *key, const char *name );

	// Setup can be NULL if not needed
	//
	// it should return NULL in case of error, anything
	// else if succesful. The return value will be passed
	// to all the other calls.
	//
	// key is the name of the registry key containing the module
	// configuration
	HANDLE			( *Setup )( const char *key );
	// Cleanup must be provided: it should return TRUE if
	// the module can be unloaded, FALSE if there are
	// requests pending. It shouldn't accept new jobs after
	// the first Cleanup() call anyway.
	BOOL			( *Cleanup )( HANDLE mod );

	// config change notification
	// REQUIRED
	void			( *CfgChanged )( HANDLE mod );

	// perform all the necessary operations to release unneeded
	// resources when someone forks (close listening sockets, for
	// instance)
	// REQUIRED
	void			( *OnFork )( HANDLE mod );

	// called once every second. Now is current time.
	// can be NULL
	void			( *OnTimer )( HANDLE mod, time_t now );

	union {
		Prom_LoggerFuncs	*Logger;
	}				Funcs;

} Prom_Module;


/*
 * Write a log line.
 * level is one of the LOG_* constant defined in syslog.h
 */
API_EXPORT( void ) Prom_Log( int level, char *fmt, ... );

/*
 * If you need to fork, you need this function.
 *
 * Do NOT use fork() directly, as it can lead to problems. For instance,
 * listening sockets are inherited by the child, causing the parent to fail
 * if it ever needs to close them and reopen them before the child exits
 * (it'll get an EADDRINUSE error).
 *
 * The function cares to destroy some internal objects which shouldn't
 * be accessed by children (such as the controller ipc object) and free
 * some resources.
 *
 * ident is an identifier of the process
 * 
 */
API_EXPORT( pid_t ) Prom_Fork( char *ident );

 
/*
 * Functions to set the process title (what appears in the output of ps)
 *
 * Call this once during subprocess startup to set the identification
 * values. E.g. Prom_init_ps_display( "DNS cache" );
 */
API_EXPORT( void ) Prom_init_ps_display( const char *ident );
/* Updates the process title */
API_EXPORT( void ) Prom_set_ps_display( const char *fmt, ... );

/*
 * addrlen is the length of the resolved address structure, or 0
 * in case of errors
 */
typedef void ( *Prom_DNS_Callback )( int addrlen, void *userdata );

/* Hostname resolution */
#if HAVE_IPV6
typedef struct in6_addr	Prom_Addr;
#else
typedef struct in_addr	Prom_Addr;
#endif
API_EXPORT( BOOL ) Prom_Resolve( char *hostname, Prom_Addr *addr );
API_EXPORT( void ) Prom_AsyncResolve( char *hostname, Prom_Addr *addr,
									  Prom_DNS_Callback callback, void *userdata );

/* Asynchronous callback for socket I/O */
typedef enum {
				PROM_SOCK_TIMEOUT	= 0,	// data is 0
				PROM_SOCK_ERROR 	= 1,	// data is errno
				PROM_SOCK_READ		= 2,	// data is the number of bytes read
				PROM_SOCK_WRITTEN	= 3,	// data is 0
				PROM_SOCK_EXCEPT	= 4,	// data is 0
				PROM_SOCK_ACCEPT	= 5,	// data is the new Socket *
				PROM_SOCK_CONNECTED	= 6,	// data is 0 if successful, errno otherwise
			 } Prom_SC_Reason;
#ifdef __cplusplus
class Socket;
typedef Socket *SOCKREF;
#else
typedef void *SOCKREF;
#endif
typedef void ( *Prom_SockCallback )( SOCKREF socket, Prom_SC_Reason reason, int data, void *userdata );

/* Asynchronous callback for file I/O */
typedef enum {
				PROM_FILE_ERROR 	= 1,	// data is errno
				PROM_FILE_READ		= 2,	// data is the amount of data read or 0 if EOF
			 } Prom_FC_Reason;
#ifdef __cplusplus
class File;
typedef File *FILEREF;
#else
typedef void *FILEREF;
#endif
typedef void ( *Prom_FileCallback )( FILEREF file, Prom_FC_Reason reason, int data, void *userdata );

/* generic destructor prototype */
typedef void ( *Prom_Destructor )( void *userdata );


/* the name of the unix socket used by prometeoctl */
#define PROM_CTRL_SOCKNAME	"/tmp/.prometeo"

#endif /* PROMETEO_API_H */
