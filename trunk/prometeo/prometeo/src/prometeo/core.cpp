/***************************************************************************
                                  core.cpp
                             -------------------
    revision             : $Id: core.cpp,v 1.7 2003-02-16 20:31:15 tellini Exp $
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

#include "main.h"

#include <stdio.h>
#include <signal.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <errno.h>
#include <fstream>

#include "registry.h"
#include "acl.h"
#include "ctrlipc.h"
#include "loader.h"
#include "dnscache.h"
#include "iodispatcher.h"
#include "file.h"
#include "tcpsocket.h"
#include "sslsocket.h"

#define PROM_PID_FILE	"/tmp/prometeo.pid"
#define SPOOL_DIR		"/var/spool/prometeo"

//--------------------------------------------------------------------------
static void StdErrCB( File *file, Prom_FC_Reason reason, int data, void *userdata );
static void SigChildExit( int signum );
static void SigReloadCfg( int signum );
static void SigQuit( int signum );
static void SigDie( int signum );
static void SigNoAction( int signum );

static volatile int LastSig = -1;

//--------------------------------------------------------------------------
Core::Core()
{
	// bring in these classes from libprom
	// otherwise the modules won't find them
	delete new TcpSocket( -1 );
#if USE_SSL
	delete new SSLSocket( NULL, -1 );
	delete new SSLCtx( SSLCtx::DUMMY );
#endif

	Cfg     = new Registry( this );
	Log     = new Logger();
	IO      = new IODispatcher();
	IPC     = NULL;
	Mods    = NULL;
	DNS     = NULL;
	ACL     = NULL;
	ErrFile = NULL;
}
//--------------------------------------------------------------------------
Core::~Core()
{
	// be careful with the order of the deletions
	delete IPC;
	delete Mods;
	delete DNS;
	delete ACL;
	delete Cfg;
	delete Log;
	delete ErrFile;
	delete IO;
}
//--------------------------------------------------------------------------
void Core::Run( void )
{
	if( Daemonize() ) {

		CfgReload();

		Log->Log( LOG_INFO, PACKAGE" "VERSION" starting" );

		ACL  = new Acl( Cfg, "root/ACL" );
		IPC  = new CtrlIPC();
		DNS  = new DNSCache( IO );
		Mods = new Loader();

		Mods->LoadModules();

		Prom_set_ps_display( "ready" );

		MainLoop();

		Log->Log( LOG_INFO, PACKAGE" "VERSION" quitting" );

		IPC->Cleanup();

		unlink( PROM_PID_FILE );
	}
}
//--------------------------------------------------------------------------
void Core::MainLoop( void )
{
	time_t	CheckTime;

	Running = true;

	time( &CheckTime );

	while( Running ) {
		time_t	now;

		IO->WaitEvents( 1 );

		// stuff to do at most once per second
		if( time( &now ) > CheckTime ) {

			DNS->Flush( now );
			IO->CheckTimeouts( now );
			Mods->OnTimer( now );

			CheckTime = now;
		}
	}
}
//--------------------------------------------------------------------------
bool Core::Daemonize( void )
{
	bool	child = false;
#ifndef DEBUG
	pid_t	pid = fork();

	if( pid == -1 )
		Log->Log( LOG_CRIT, "fork() failed - %s", strerror( errno ));

	else if( pid == 0 ) { // child

		if( CheckPid() ) {
			int	fds[2];
#endif
			child = true;

			Prom_init_ps_display( "master" );

#ifndef DEBUG
			// we don't need the standard file descriptors, but
			// closing them is a *bad* idea: some libraries
			// we use may think it could be nice to use them,
			// causing havoc
			if( !freopen( "/dev/null", "r", stdin  ))
				Log->Log( LOG_ERR, "core: couldn't replace stdin" );
			
			if( !freopen( "/dev/null", "w", stdout ))
				Log->Log( LOG_ERR, "core: couldn't replace stdout" );
			
			// create a pipe to catch errors
			if( !pipe( fds )) {

				ErrFile = new File( fds[0] );

				ErrFile->SetAsyncCallback( StdErrCB, this );

				IO->AddFD( ErrFile, PROM_IOF_READ );
			
				dup2( fds[1], STDERR_FILENO );
			}

			setsid();
#endif
			umask( 022 );

			mkdir( SPOOL_DIR, 0700 );
			chdir( SPOOL_DIR );

			signal( SIGCHLD, SigChildExit );
			signal( SIGHUP,  SigReloadCfg );
			signal( SIGQUIT, SigQuit      );
			signal( SIGINT,  SigQuit      );
			signal( SIGTERM, SigQuit      );
			signal( SIGSEGV, SigDie       );
#ifdef SIGBUS
			signal( SIGBUS,  SigDie       );
#endif
			signal( SIGABRT, SigDie       );
			signal( SIGFPE,  SigDie       );
			signal( SIGUSR1, SigNoAction  );
			signal( SIGPIPE, SigNoAction  );
#ifndef DEBUG
		}
	}
#endif

	return( child );
}
//--------------------------------------------------------------------------
pid_t Core::Fork( char *ident )
{
	pid_t	pid;

	// ignore SIGHUP because:
	//   a. the child should ignore it
	//   b. the parent should stay blocked until the child is ready
	//
	// all other signals won't be ignored, since they cause the application
	// to quit.
	signal( SIGHUP, SIG_IGN );

	InitWait();

	pid = fork();

	if( pid == 0 ) {			// child

		// avoid that the child inherits some listening sockets and stuff
		if( Mods )
			Mods->OnFork();

		if( DNS )
			DNS->OnFork();

		delete IPC;
		delete ErrFile;

		// at this point, we can unblock the parent
		WakeUpParent();

		IPC     = NULL;
		ErrFile = NULL;

		Prom_init_ps_display( ident );

	} else if( pid > 0 ) {		// parent

		// let's wait until the child is done with the initial setup
		Wait();
	}

	if( pid != 0 )
		signal( SIGHUP, SigReloadCfg );

	return( pid );
}
//--------------------------------------------------------------------------
bool Core::InitWait( void )
{
	LastSig = -1;

	sigemptyset( &ZeroMask );
	sigemptyset( &NewMask );

	sigaddset( &NewMask, SIGUSR1 );

	return( sigprocmask( SIG_BLOCK, &NewMask, &OldMask ) >= 0 );
}
//--------------------------------------------------------------------------
void Core::Wait( void )
{
	while( LastSig == -1 )
		sigsuspend( &ZeroMask );

	sigprocmask( SIG_SETMASK, &OldMask, NULL );
}
//--------------------------------------------------------------------------
void Core::WakeUpParent( void )
{
	kill( getppid(), SIGUSR1 );

	sigprocmask( SIG_SETMASK, &OldMask, NULL );
}
//--------------------------------------------------------------------------
bool Core::CheckPid( void )
{
	bool		ret = true;
	ifstream	ifh( PROM_PID_FILE );

	if( ifh ) {

		ifh.close();

		ret = false;

		printf( "prometeo is already running\n" );

	} else {
		ofstream	ofh( PROM_PID_FILE );

		if( ofh ) {

			ofh << getpid();

			ofh.close();
		}
	}

	return( ret );
}
//---------------------------------------------------------------------------
void Core::CfgReload( void )
{
	Cfg->Load();
	Log->Setup( Cfg );

	if( DNS )
		DNS->ReloadCfg();

	if( Mods )
		Mods->CfgChanged();

	Log->Log( LOG_INFO, "configuration loaded" );
}
//--------------------------------------------------------------------------
static void StdErrCB( File *file, Prom_FC_Reason reason, int data, void *userdata )
{
	if(( reason == PROM_FILE_READ ) && ( data == -1 )) {
		char	buf[ 1024 ];
		int		len;
	
		if(( len = file->Read( buf, sizeof( buf ) - 1 )) > 0 ) {
			char	*ptr = buf, *start = buf, *end;
			
			end  = &buf[ len ];
			*end = '\0';
			
			while(( ptr < end ) && ( ptr = strchr( ptr, '\n' ))) {

				*ptr++ = '\0';

				App->Log->Log( LOG_ERR, "stderr: %s", start );

				start = ptr;
			}
		
			if( start < end )
				App->Log->Log( LOG_ERR, "stderr: %s", start );
		}
	}
}
//--------------------------------------------------------------------------
static void SigChildExit( int signum )
{
    int status;

	// XXX check pid and re-spawn subprocesses if necessary
    while( waitpid( -1, &status, WNOHANG ) > 0 );

    signal( SIGCHLD, SigChildExit );
}
//--------------------------------------------------------------------------
static void SigReloadCfg( int signum )
{
	App->CfgReload();
	signal( SIGHUP, SigReloadCfg );
}
//--------------------------------------------------------------------------
static void SigQuit( int signum )
{
	App->Quit();
}
//--------------------------------------------------------------------------
static void SigDie( int signum )
{
	switch( signum ) {
			
		case SIGSEGV:
			throw "segmentation violation";

#ifdef SIGBUS
		case SIGBUS:
			throw "bus fault";
#endif

		case SIGABRT:
			throw "abnormal termination";

		case SIGFPE:
			throw "FPU exception";
	}
}
//--------------------------------------------------------------------------
static void SigNoAction( int signum )
{
	LastSig = signum;
	
	signal( signum, SigNoAction );
}
//--------------------------------------------------------------------------

