/*****************************************************************
|
|   BlueTune - Command-Line Player
|
|   (c) 2002-2006 Gilles Boccon-Gibod
|   Author: Gilles Boccon-Gibod (bok@bok.net)
|
 ****************************************************************/
/** @file 
 * Main code for BtPlay
 */

/*----------------------------------------------------------------------
|    includes
+---------------------------------------------------------------------*/
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <memory.h>
#include <errno.h>
#include <syslog.h>
#include <time.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <pthread.h>

#include "Atomix.h"
#include "BlueTune.h"

#include "eq_api.h"
//#define BLT_CONFIG_MODULES_ENABLE_AudioMixer_OUTPUT
 #if defined(BLT_CONFIG_MODULES_ENABLE_AudioMixer_OUTPUT)
#include <audiomixer.h>
#endif
/**********************************************************************/
/*G+ additions start*/
/**********************************************************************/

/**************************************************************************
 *                              M A C R O S                               *
 **************************************************************************/
#if 0
#define DEBUG0 printf
#else
#define DEBUG0(...)
#endif

#define	BTPLAY_PIDBASE	"/var/run/btplay"
#define	BTPLAY_PIDFILE		BTPLAY_PIDBASE ".pid"
#define	BTPLAY_STATEFILE	BTPLAY_PIDBASE ".state"
#define	BTPLAY_PROPSFILE	BTPLAY_PIDBASE ".properties"	/*property: artist ...etc*/
#define	BTPLAY_INFORFILE	BTPLAY_PIDBASE ".information"	/*stream information: bitrate, sample rate ... etc*/
#define	BTPLAY_CMDIN		"/tmp/.btplay-cmdin"
#define	BTPLAY_CMDOUT		"/tmp/.btplay-cmdout"

#define BTRELEASE_VER "0.0.0.03"
#define VER_STR	BTRELEASE_VER ".01"
#ifdef __ARMEL__
#define EQ_ENABLE
#endif
#define CHTRACE
/**************************************************************************
 *                          D A T A    T Y P E S                          *
 **************************************************************************/
/*btplay running state*/
enum btState_e{
	BT_STATE_STOP=0,
	BT_STATE_PLAY,
	BT_STATE_PAUSE,
	BT_STATE_FF,
	BT_STATE_FB,
};
enum eqMode_e {
	EQ_NONE,
    EQ_DBB,
	EQ_ROCK,
	EQ_JAZZ,
	EQ_POP,
	EQ_LIVE
};
/*A struct for resource porperty*/
typedef struct btProperty_s {
	char name[32]; ///< Property name
	char value[256]; ///< Value as received, truncated if necessary
} btProperty_t;

typedef struct btResource_s{
	char* name;		/*!< full path of resource to be play*/
	char* type;		/*!< resource type, if not specify, auto detect*/
} btResouece_t;
/**************************************************************************
 *                           C O N S T A N T S                            *
 **************************************************************************/
#define MAX_PROPERTIES 32
#define MAX_INFORMAION 10
#define PUMP_CMD_FREQUENCY 8 
 
/**************************************************************************
 *                         G L O B A L    D A T A                         *
 **************************************************************************/
static int g_btState = BT_STATE_STOP;		/*!< btplay state*/
static unsigned long  g_seektime = 0;		/*!< seek time: 0 do nothing*/
static unsigned long  record_FF_time =0;   /*for ff function*/
static int g_playmode = 0 ; /* g_playmode =1 : loop, g_playmode = 0  once*/

char g_cmdIn[256] = BTPLAY_CMDIN;	/*!< command in pipe path*/
char g_cmdOut[256] = BTPLAY_CMDOUT;	/*!< command out pipe pat*/
int gh_CmdIn = 0;					/*!< handle of command in pipe*/
FILE *gp_fCmdOut = NULL;				/*!< file pionter of comming out pipe*/

static int g_debug = 0;					/*!< enable debug or not*/

int g_usingCmdPipe = 0;				/*!< using pipe or not*/
int g_invokedAsDaemon = 0;			 /*!< 1 called by btplayd; 0 called by btplay*/
int g_terminate = 0;					/*!< terminate or not*/

static int g_btcmdSerial = 0;			/*!< command serial number*/
static int g_playingSeconds = 0;		/*!< playing second*/	

btProperty_t g_props[MAX_PROPERTIES] = {{0},{0}};
btProperty_t g_infos[MAX_INFORMAION] = {{0},{0}};

btResouece_t g_btResource = {NULL, NULL};

static char g_playingTrack[256] = {0};	/*store current playing file name only*/
unsigned int total_time = 0;

#ifdef EQ_ENABLE 
static int g_band_gain_level[EQ_BAND_NUM] = {-12, -6, 0, 6, 12};	/* -12 dB ~ 12 dB*/
static int g_eq_mode = EQ_NONE;
#endif
/**************************************************************************
 *                 E X T E R N A L    R E F E R E N C E S                 *
 **************************************************************************/

/**************************************************************************
 *               F U N C T I O N    D E C L A R A T I O N S               *
 **************************************************************************/
#ifdef EQ_ENABLE
static int musicSetEQMode(unsigned char mode)
{
	switch(mode)
	{
		case EQ_DBB:
		g_band_gain_level[0] = 6;
		g_band_gain_level[1] = 4;
		g_band_gain_level[2] = 1;
		g_band_gain_level[3] = -2;
		g_band_gain_level[4] = -5;
		break;

		case EQ_ROCK:
		g_band_gain_level[0] = 9;
		g_band_gain_level[1] = 3;
		g_band_gain_level[2] = -3;
		g_band_gain_level[3] = 5;
		g_band_gain_level[4] = 11;
		break;


		case EQ_JAZZ:
		g_band_gain_level[0] = 6;
		g_band_gain_level[1] = -3;
		g_band_gain_level[2] = 5;
		g_band_gain_level[3] = -3;
		g_band_gain_level[4] = 6;
		break;


		case EQ_POP:
		g_band_gain_level[0] = -5;
		g_band_gain_level[1] = 5;
		g_band_gain_level[2] = -3;
		g_band_gain_level[3] = 5;
		g_band_gain_level[4] = -5;
		break;


		case EQ_LIVE:
		g_band_gain_level[0] = -2;
		g_band_gain_level[1] = -1;
		g_band_gain_level[2] = 5;
		g_band_gain_level[3] = -1;
		g_band_gain_level[4] = -2;
		break;

		default:
		printf("Error:EQ none-implement EQmode!!\n");
		return -1;
		break;
	}
	set_eq_band(g_band_gain_level);
	return 0;		
} 
#endif 
 
static int btGetNextSerial(void);
static int btPostMessage(char *msg);

// Set close on exec flag in file descriptor.
// We do this because we want this file to be closed across any execve() calls we make...
int set_cloexec_flag(int desc, int value) {
        int oldflags = fcntl(desc,F_GETFD,0);
        if (oldflags<0)
                return oldflags;
        if (value!=0)
                oldflags |= FD_CLOEXEC;
        else
                oldflags &= ~FD_CLOEXEC;
        return fcntl(desc,F_SETFD,oldflags);
}

// Dump current time and flush stdout using fmt which must have %s
void DumpTime( const char *fmt )
{
	char buff[256];
	time_t now = time( NULL );
	struct tm *now_tm = localtime( &now );
	strftime( buff, sizeof(buff), "%Y-%m-%d %H:%M:%S", now_tm );
	ATX_ConsoleOutputF( fmt, buff );
	fflush_unlocked( stdout );
}

// Copy src to dest, truncating if necessary to fit dest size
void strcpy_trunc( char *dest, const char *src, size_t n )
{
	strncpy( dest, src, n );
	dest[n-1] = '\0';
}

// Write properties to file unconditionally. Return number of lines written or -1 if error
int WriteProperties()
{
	FILE *f = fopen( BTPLAY_PROPSFILE, "w" );
	int n;
	int ret = 0;
	if (!f)
	{
		ATX_ConsoleOutput( "btPlay ERROR - unable to write to " BTPLAY_PROPSFILE "\n" );
		return -1;
	}
	for (n = 0; n < MAX_PROPERTIES; n++)
	{
		if (!g_props[n].name[0])
		{
			break;
		}
		fprintf( f, "%s=%s\n", g_props[n].name, g_props[n].value );
		ret++;
	}
	fclose( f );
	return ret;
}


// Clear properties
void ClearProperties()
{
	memset( g_props, 0, sizeof(g_props) );
}

// Add property to collection and write properties iff changed
int UpdateProperty( const char *name, const char *s )
{
	// If NULL, use empty string
	if (!s) s = "";
	// Find a slot
	int n;
	int changed = 0;
	for (n = 0; n < MAX_PROPERTIES; n++)
	{
		if (g_props[n].name[0] == '\0')
		{
			// This one is free, use it
			strcpy_trunc( g_props[n].name, name, sizeof(g_props[n].name) );
			strcpy_trunc( g_props[n].value, s, sizeof(g_props[n].value) );
			changed++;
			break;
		}
		// Check for update
		if (strcmp( g_props[n].name, name ))
		{
			continue;
		}
		// Replace value if changed
		if (strcmp( g_props[n].value, s ))
		{
			changed++;
			strcpy_trunc( g_props[n].value, s, sizeof(g_props[n].value) );
		}
		break;
	}

	// Write collection iff changed
	if (changed)
	{
		return WriteProperties();
	}

	return 0;
}


// Write stream infomation  to file unconditionally. Return number of lines written or -1 if error
int WriteInformation()
{
	FILE *f = fopen( BTPLAY_INFORFILE, "w" );
	int n;
	int ret = 0;
	if (!f)
	{
		ATX_ConsoleOutput( "btPlay ERROR - unable to write to " BTPLAY_INFORFILE "\n" );
		return -1;
	}
	for (n = 0; n < MAX_INFORMAION; n++)
	{
		if (!g_infos[n].name[0])
		{
			break;
		}
		fprintf( f, "%s=%s\n", g_infos[n].name, g_infos[n].value );
		ret++;
	}
	fclose( f );
	return ret;
}


// Clear information
void ClearInformation()
{
	memset( g_infos, 0, sizeof(g_infos) );
}

// Add stream informaion to collection and write properties iff changed
int UpdateInformation( const char *name, const char *s )
{
	// If NULL, use empty string
	if (!s) s = "";
	// Find a slot
	int n;
	int changed = 0;
	for (n = 0; n < MAX_INFORMAION; n++)
	{
		if (g_infos[n].name[0] == '\0')
		{
			// This one is free, use it
			strcpy_trunc( g_infos[n].name, name, sizeof(g_infos[n].name) );
			strcpy_trunc( g_infos[n].value, s, sizeof(g_infos[n].value) );
			changed++;
			break;
		}
		// Check for update
		if (strcmp( g_infos[n].name, name ))
		{
			continue;
		}
		// Replace value if changed
		if (strcmp( g_infos[n].value, s ))
		{
			changed++;
			strcpy_trunc( g_infos[n].value, s, sizeof(g_infos[n].value) );
		}
		break;
	}

	// Write collection iff changed
	if (changed)
	{
		return WriteInformation();
	}
	
	return 0;
}


// Write current play state. 
//BT_STATE_STOP(0), BT_STAE_PLAY(1), BT_PLAY_PAUSE(2)
void WritePlayState()
{
	FILE *f = fopen( BTPLAY_STATEFILE, "w" );
	if (!f)
	{
		ATX_ConsoleOutput( "btPlay ERROR: cannot write " BTPLAY_STATEFILE "\n" );
		syslog( LOG_ERR, "error: cannot write " BTPLAY_STATEFILE " errno=%d (%s)\n", errno, strerror(errno) );
		return;
	}
	
	int value = g_btState;
	
	if (g_debug)
	{
		ATX_ConsoleOutputF( "WritePlayState  %d\n", value );
	}
	fprintf( f, "%d\n", value );
	fclose( f );
}


// Return pid of already running daemon or 0 if none
int RunningInstancePid()
{
	FILE *f = fopen( BTPLAY_PIDFILE, "r" );
	int alreadyRunning = 0;
	char buff[256];
	int oldPid = 0;
	if (f)
	{
		if (fgets( buff, sizeof(buff), f ))
		{
			oldPid = atoi(buff);
			// Make sure it's not running
			sprintf( buff, "/proc/%u/stat", oldPid );
			alreadyRunning = (access( buff, F_OK ) == 0);
		}
		fclose( f );
	}
	return alreadyRunning ? oldPid : 0;
}

// Write current pid iff there is not an instance already running
int WritePidExclusive()
{
	// Get existing pid
	FILE *f;
	int alreadyRunning = RunningInstancePid();
	if (alreadyRunning)
	{
		ATX_ConsoleOutputF( "btplayd ERROR: instance %u is already running - cannot continue\n", alreadyRunning );
		return 0;
	}
	f = fopen( BTPLAY_PIDFILE, "w" );
	if (!f)
	{
		ATX_ConsoleOutput( "btplayd ERROR: cannot create " BTPLAY_PIDFILE " - cannot continue\n" );
		return 0;
	}
	fprintf( f, "%d\n", getpid() );
	fclose( f );
	return 1;
}


/****************************************************************

Command verbs should be prefixed by a serial number.
Now supporting command is below:

[serial] play file
Play the given file

[serial] stop
Stop the current playing

[serial] pause 
Pause the cureent playing

[serial] resume 
Resume the previous pause

[serial] seek time
Seek to time

*****************************************************************/
// Command tokens, in lexical order. Must correspond with matching enum values.
static char *PIPE_CMD[] = {
	"attr",
	"debug",
	"pause",
	"ping",
	"play",
	"quit",
	"resume",
	"seek",
	"stop",
	"plaf",
	"plab",
	"setmode",
	"seteq",
};
enum _PIPE_CMD {
	PCMD_ATTR = 0,
	PCMD_DEBUG,
	PCMD_PAUSE,
	PCMD_PING,
	PCMD_PLAY,
	PCMD_QUIT,
	PCMD_RESUME,
	PCMD_SEEK,
	PCMD_STOP,
	PCMD_FF,
	PCMD_FB,
	PCMD_SETMODE,
	PCMD_SETEQ,
	_PCMD_COUNT
};

// Perform divide-and-conquer comparison recursively
// Returns index of matched entry or -1 if not found
int DivideAndConquer( const char *s, int start, int count )
{
  #if 0
	int compareIndex = start + count / 2;
	int compareValue = strcasecmp( s, PIPE_CMD[compareIndex] );
	if (compareValue == 0)
	{
		if (g_debug)
		{
			ATX_ConsoleOutputF( "DivideAndConquer(%s,%d,%d) matched %d\n", s, start, count, compareIndex );
		}
		return compareIndex;
	}
	if (g_debug)
	{
		ATX_ConsoleOutputF( "DivideAndConquer(%s,%d,%d) index %d compare %d\n", s, start, count, compareIndex, compareValue );
	}
	// If we've compared the only value and no match, we're done
	if (count == 1)
	{
		return -1;
	}
	if (compareValue < 0)
	{
		// Compare up to compareIndex-1
		return DivideAndConquer( s, start, compareIndex - start );
	}
	// else>0
	return DivideAndConquer( s, compareIndex + 1, start + count - compareIndex - 1 );
  #else
    int i = 0;
	for(i=0;i<_PCMD_COUNT;i++){
		if(strcasecmp( s, PIPE_CMD[i] ) == 0){
			return i ;
		}
		/*fprintf( stderr,"not match [%s][%s]",s,PIPE_CMD[i]);*/
	}
	fprintf( stderr,"Can't find [%s]",s);
	return -1;
  #endif  
}

// Return PCMD_ enum value or -1 if not found
int LookupPipeCmd( const char *cmd )
{
	return DivideAndConquer( cmd, 0, _PCMD_COUNT );
}

// Return 1 if created or already exists.
int AssertNode( const char *path )
{
	// Does it exist already?
	struct stat s;
	if (stat( path, &s ) == 0)
	{
		// Is it a pipe?
		if (S_ISFIFO( s.st_mode ))
		{
			return 1;
		}
		// Not a pipe, remove it
		unlink( path );
	}
	// Create it
	if (mknod( path, S_IFIFO | 0666, 0 ) != 0)
	{
		ATX_ConsoleOutputF( "AssertNode(%s) mknod failed errno = %d\n", path, errno );
		return 0;
	}
	return 1;
}

// Perform initialization for command pipes. Returns true if successful/
int CmdPipeInit()
{
	// If not using command pipes, nothing to do
	if (!g_usingCmdPipe)
	{
		return 1;
	}
	// Assert nodes
	if (!AssertNode( g_cmdIn ))
	{
		ATX_ConsoleOutputF( "CmdPipeInit() - failed to assert pipe node %s\n", g_cmdIn );
		return 0;
	}
	if (!AssertNode( g_cmdOut ))
	{
		ATX_ConsoleOutputF( "CmdPipeInit() - failed to assert pipe node %s\n", g_cmdOut );
		return 0;
	}
	// Open file objects. This will block unless the other end of the input command
	// pipe is already open.
	ATX_ConsoleOutputF( "Attempting to open input command pipe %s - this may block\n", g_cmdIn );
	gh_CmdIn = open( g_cmdIn, O_NONBLOCK );
	if (gh_CmdIn <= 0)
	{
		ATX_ConsoleOutputF( "Failed to open - errno = %d\n", errno );
		return 0;
	}
	// Make sure handles are closed across any exec calls we make
	set_cloexec_flag( gh_CmdIn, 1 );

	ATX_ConsoleOutputF( "Opened successfully. Attempting to open output command pipe %s (may block)\n", g_cmdOut );
	gp_fCmdOut = fopen( g_cmdOut, "w" );
	if (!gp_fCmdOut)
	{
		ATX_ConsoleOutputF( "Failed to open - errno = %d\n", errno );
		close( gp_fCmdOut );
		gp_fCmdOut = 0;
		return 0;
	}
	// Make sure handles are closed across any exec calls we make
	set_cloexec_flag( fileno_unlocked(gp_fCmdOut), 1 );

	ATX_ConsoleOutput( "Opened output successfully.\n" );
	return 1;
}

void CmdPipeCleanup()
{
	// If not using command pipes, nothing to do
	if (!g_usingCmdPipe)
	{
		return;
	}
	if (gh_CmdIn)
	{
		ATX_ConsoleOutputF( "Closing command input %s\n", g_cmdIn );
		close( gh_CmdIn );
		gh_CmdIn = 0;
	}
	if (gp_fCmdOut)
	{
		ATX_ConsoleOutputF( "Closing command output %s\n", g_cmdOut );
		fclose( gp_fCmdOut );
		gp_fCmdOut = NULL;
	}
}

// Process command pipe input.
//Return: 0 nothing to do; 1 change state; 2 change play file; 3 stop
int CmdPump()
{
	if (!g_usingCmdPipe)
	{
		return 0;
	}
	
	int returnVal = 0;
	if (g_debug > 1)
	{
		DumpTime( "%s CmdPump(enter)\n" );
	}
	
	// Process maximum of 3 commands per pump invocation
	int cmdPass;
	for (cmdPass = 0; !g_terminate && cmdPass < 3; cmdPass++)
	{
		struct timeval timeout;
		fd_set readfds;
		char buff[1024];
		int n;
		int maxHandle = gh_CmdIn;

		FD_ZERO( &readfds );
		FD_SET( gh_CmdIn, &readfds );

		// If nothing selected, exit immediately
		timeout.tv_sec = 0;
		timeout.tv_usec = 0;
		CHTRACE
		if (select(maxHandle+1, &readfds, NULL, NULL, &timeout) <= 0) {
			break;
		}

		if (!FD_ISSET( maxHandle, &readfds ))
		{
			if (g_debug > 1)
			{
				ATX_ConsoleOutputF( "CmdPump() select ready but FD_ISSET(%d,&readfds) false\n", maxHandle );
			}
			break;
		}

		if (g_debug > 1)
		{
			ATX_ConsoleOutput( "p" );
			fflush_unlocked( stdout );
		}
		// We can't use fgets with select
		if (g_debug > 1)
		{
			DumpTime( "%s CmdPump(pre-fread)\n" );
		}
		ssize_t bytesRead = read( gh_CmdIn, buff, sizeof(buff)-1 );
		CHTRACE
		if (bytesRead <= 0)
		{
			if (g_debug > 1)
			{
				DumpTime( "%s CmdPump(post-fread null)\n" );
			}
			// We'll get 0 on feof() or ferror()
			if (g_debug > 2)
			{
				ATX_ConsoleOutputF( "CmdPump(pass %d) select ready but nothing read!\n", cmdPass );
				fflush_unlocked( stdout );
			}
			break;
		}
		buff[bytesRead-1] = '\0';

		DEBUG0("btplay: cmdstr = %s \n", buff);
		
		char *cmdLine;
		int bufferedLineCount, nLine;
		char *bufferedLines[256];
		bufferedLineCount = 0;
		for (cmdLine = strtok( buff, "\r\n" ); cmdLine != NULL && bufferedLineCount < sizeof(bufferedLines)/sizeof(bufferedLines[0]);
				cmdLine = strtok( NULL, "\r\n" ))
		{
			bufferedLines[bufferedLineCount] = cmdLine;
			bufferedLineCount++;
		}

		if (g_debug > 1)
		{
			DumpTime( "%s CmdPump(post-fread)\n" );
			ATX_ConsoleOutputF( "CmdPump() got %d bytes [%s] and parsed %d lines\n", bytesRead, buff, bufferedLineCount );
			fflush_unlocked( stdout );
		}

		for (nLine = 0; nLine < bufferedLineCount; nLine++)
		{
			int cmdSerial = 0;
			cmdLine = bufferedLines[nLine];
			// Get command token and args
			cmdLine += strspn( cmdLine, " \t" );
			// Check for optional serial number
			if (cmdLine[0] == '[')
			{
				cmdSerial = atoi( &cmdLine[1] );
				cmdLine += strspn( cmdLine, "[0123456789] \t" );
				if (g_debug > 0)
				{
					ATX_ConsoleOutputF( "CmdPump() got cmd[%d] line = \"%s\"\n", cmdSerial, cmdLine );
				}
			}
			int cmdLength = strcspn( cmdLine, " \t" );
			int startArgs = cmdLength + strspn( &cmdLine[cmdLength], " \t" );

			// All play commands take a type (or * for unspecified) followed by space and uri.
			// Get offset to second arg and length of first
			int firstArgLength = strcspn( &cmdLine[startArgs], " \t" );
			int secondArgStart = startArgs + firstArgLength + strspn( &cmdLine[startArgs+firstArgLength], " \t" );

			char firstArg[64] = {0};
			if (firstArgLength > 0 && firstArgLength < sizeof(firstArg)-2 && secondArgStart > startArgs)
			{
				strncpy( firstArg, &cmdLine[startArgs], firstArgLength );
				firstArg[firstArgLength] = '\0';
				if (firstArg[0] == '*')
				{
					firstArg[0] = '\0';
				}
			}

			char cmd[256];
			strncpy( cmd, cmdLine, cmdLength );
			cmd[cmdLength] = '\0';

			if (g_debug)
			{
				ATX_ConsoleOutputF( "Debug: cmd[%d]=%s args=%s\n", cmdSerial, cmd, &cmdLine[startArgs] );
			}
			printf("cmd :%s",cmd);
			int cmdHash = LookupPipeCmd( cmd );
			if (g_debug)
			{
				ATX_ConsoleOutputF( "Debug[%d]: hash=%d\n", cmdPass, cmdHash );
			}
			DEBUG0("btplay: CMD1 = %d \n", cmdHash);
                        //DEBUG0("parse the cmd filename=%s,mode=%s",strtok(&cmdLine[startArgs],":"),strtok(NULL,":"));
            
			switch (cmdHash)
			{
				case PCMD_DEBUG:
					CHTRACE
					// get or set debug level
					if (firstArgLength > 0)
					{
						g_debug = atoi( &cmdLine[startArgs] );
					}
					fprintf( gp_fCmdOut, "OK %d debug=%d\n", cmdSerial, g_debug );
					break;
				case PCMD_PAUSE:
					if((g_btState == BT_STATE_PLAY)||( g_btState == BT_STATE_FF)||(g_btState == BT_STATE_FB))
					{
						g_btState = BT_STATE_PAUSE;
						WritePlayState();
						returnVal = 1;	/*state change*/
					}
					fprintf( gp_fCmdOut, "OK %d pause\n", cmdSerial );
					break;
				case PCMD_PING:
					fprintf( gp_fCmdOut, "OK %d ping\n", cmdSerial );
					break;
				case PCMD_PLAY:
				    if(( g_btState == BT_STATE_FF)||(g_btState == BT_STATE_FB))
					  {
					    g_btState = BT_STATE_PLAY;
						WritePlayState();
						record_FF_time = 0;
						fprintf( gp_fCmdOut, "OK %d play %s \n", cmdSerial, g_btResource.name);
					  }
					else if (cmdLine[startArgs])
					{
					        if (g_btResource.name != NULL) free(g_btResource.name );
						total_time = atoi(&cmdLine[startArgs]);
						g_btResource.name = strdup(&cmdLine[secondArgStart]);
						printf("g_btResource.name:%s\n\r",g_btResource.name);
						//total_time = atoi(&cmdLine[secondArgStart]);
						printf("total_time %d \n\r",total_time);
						g_btState = BT_STATE_PLAY;
						WritePlayState();
						returnVal = 2;
						fprintf( gp_fCmdOut, "OK %d play %s \n", cmdSerial, g_btResource.name);
					}
					else
					{
						fprintf( gp_fCmdOut, "OK %d play\n", cmdSerial );			
					}
					break;	
				case PCMD_QUIT:
					g_terminate = 1;
					total_time = 0;
					fprintf( gp_fCmdOut, "OK %d quit\n", cmdSerial );	
					break;
				case PCMD_RESUME:
					if ( g_btState == BT_STATE_PAUSE )
					{
						g_btState = BT_STATE_PLAY;
						WritePlayState();
						returnVal = 1;	/*state change*/
					}
					fprintf( gp_fCmdOut, "OK %d resume\n", cmdSerial );
					break;
				case PCMD_SEEK:					
					if ( g_btState == BT_STATE_PLAY ||g_btState==BT_STATE_PAUSE)
					{
						if ( cmdLine[startArgs] )
						{
							g_seektime = atoi(&cmdLine[startArgs]);	
							DEBUG0("btplay:seek = %d \n", g_seektime);
							if( g_seektime != 0 )
							{
								returnVal = 1; /*state change*/
							}
						}
					}

					fprintf( gp_fCmdOut, "OK %d seek %s\n", cmdSerial, &cmdLine[startArgs]);
					break;

				case PCMD_STOP:
					if( g_btState != BT_STATE_STOP )
					{
						g_btState = BT_STATE_STOP;
						WritePlayState();
						returnVal = 3;
						total_time = 0;
					}
					fprintf( gp_fCmdOut, "OK %d stop\n", cmdSerial);
					break;
				case PCMD_FF:
					if( g_btState != BT_STATE_STOP )
					{
					  g_btState = BT_STATE_FF;
					  WritePlayState();
					  record_FF_time =0;
					}
					fprintf( gp_fCmdOut, "OK %d FF\n", cmdSerial);	
					break;
				case PCMD_FB:
					if( g_btState != BT_STATE_STOP )	
					{
						g_btState = BT_STATE_FB;
						WritePlayState();
						record_FF_time = 0;
					}
					fprintf( gp_fCmdOut, "OK %d FB\n", cmdSerial); 
					break;
				case PCMD_SETMODE: 
			    if(strcmp(&cmdLine[startArgs],"once")==0)
				  {
						DEBUG0("set once play mode \n\r");
						g_playmode = 0;
					}
					else if(strcmp(&cmdLine[startArgs],"loop")==0)
					{
						DEBUG0("set loop play mode \n\r");
						g_playmode = 1;
					}	
					else
						DEBUG0("the mode is can not macth\n\r");		
				  fprintf( gp_fCmdOut, "OK %d setmode\n", cmdSerial); 
					break;  
				case PCMD_SETEQ:
					{
#ifdef EQ_ENABLE
					char *mode = &cmdLine[startArgs];
					if (mode) 
					{
						int support = 1;
						if (strcmp(mode, "None") == 0 )
						{
							g_eq_mode = EQ_NONE;
						}
						else if (strcmp(mode, "Dbb") == 0 ) 
						{
							g_eq_mode = EQ_DBB;
							musicSetEQMode(EQ_DBB);
						} 
						else if (strcmp(mode, "Rock") == 0) 
						{
							g_eq_mode = EQ_ROCK;
							musicSetEQMode(EQ_ROCK);
						}
						else if (strcmp(mode, "Jazz") == 0) 
						{
							g_eq_mode = EQ_JAZZ;
							musicSetEQMode(EQ_JAZZ);
						}
						else if (strcmp(mode, "Pop") == 0) 
						{
							g_eq_mode = EQ_POP;
							musicSetEQMode(EQ_POP);
						}
						else if (strcmp(mode, "Live") == 0) 
						{
							g_eq_mode = EQ_LIVE;
							musicSetEQMode(EQ_LIVE);
						}
						else
						{
							support = 0;		
						}
						if ( support ) 
						{
							DEBUG0("the %s is support \n\r",mode);
						}
						else
						{
							printf("%s is not support!\n\r",mode);
						}
					}
					else 
					{
						printf("%s get empty mode !\n\r");
					}
					fprintf( gp_fCmdOut, "OK %d seteq\n", cmdSerial);
#endif
					break;
				}				
				default:
					CHTRACE
					ATX_ConsoleOutputF( "Unexpected command %s - ignored\n", cmd );
					syslog( LOG_ERR, "Unexpected command %s ignored\n", cmd );
					fprintf( gp_fCmdOut, "ERR %d invalid command %s\n", cmdSerial, cmdLine );
					break;
			} // switch
			fflush_unlocked( gp_fCmdOut );
		} // for all lines in input
	} // for (cmdPass in 0..3

	if (g_debug > 1)
	{
		DumpTime( "%s CmdPump(exit)\n" );
	}
	return returnVal;
}

static int btGetNextSerial(void)
{
	return g_btcmdSerial++;	
}

static int BLTP_PostMessage(char *msg)
{
	fprintf( gp_fCmdOut, "MSG %d %s\n", btGetNextSerial(), msg );
	fflush_unlocked( gp_fCmdOut );
}

/**
*@brief Convet pcm by given equalizer setting
*@param src source buffer of pcm
*@param length length of buffer, in byte
*@return the first address of converted pcm buffer, 
*        NULL means no convertion
*/
static 
void *BLTP_EqualizerConvert(void *src, BLT_UInt32 length)
{
	void *dst = NULL;
#ifdef EQ_ENABLE	
	unsigned int i, len;
	short *pcm_in, *pcm_out;
	unsigned int total;

	if (g_eq_mode == EQ_NONE) {
		return NULL;
	}
	if ( length%32 != 0 ) {
		/*limited of libeq*/
		printf("Equalizer convert length(%d) is not support", length);
		return NULL;
	}
	dst = ATX_AllocateZeroMemory(length);
	if ( dst ) {
		total = length/4;	/*length in byte*/
		pcm_in = (short*)src;
		pcm_out = (short*)dst;
		for (i = 0; i < total; i += EQ_BLOCK_SIZE ) {
			len = (total-i) > EQ_BLOCK_SIZE ? EQ_BLOCK_SIZE : (total-i);
			len = eq_run(&pcm_out[i*2], &pcm_in[i*2], len);				
		}
	}
#endif

	return dst;
}

static int BLTP_UpdateTime(BLT_Decoder* decoder)
{
	BLT_Result   result;
	BLT_DecoderStatus status;

	result = BLT_Decoder_GetStatus(decoder, &status);
	if( BLT_FAILED(result) ) {
		return BLT_FAILURE;
	}

	/*check update*/
	if ( g_playingSeconds != status.time_stamp.seconds ) {
		char msg[50];
		sprintf(msg, "time %d", status.time_stamp.seconds);
		BLTP_PostMessage(msg);
		g_playingSeconds = status.time_stamp.seconds;
		if((total_time != 0)&&(total_time<g_playingSeconds))
		  	g_btState =BT_STATE_STOP;	
	}
	if(g_btState == BT_STATE_FF)
		{
		  if ((record_FF_time==0) ||(record_FF_time+2 <g_playingSeconds))
		       {
						g_seektime  =  (g_playingSeconds +1)*1000;
						record_FF_time =g_playingSeconds;
           }				
		}
	
   if(g_btState == BT_STATE_FB)
   	{
		  DEBUG0("g_playingSeconds[%d],record_FF_time[%d]\n\r",g_playingSeconds,record_FF_time);
		  if(g_playingSeconds ==1)
				   {
						DEBUG0("stop the music\n\r");
						g_btState =BT_STATE_STOP;
				   }
	     else if((record_FF_time==0)||(record_FF_time<g_playingSeconds+2))
		      {
			     g_seektime  =  (g_playingSeconds -2)*1000;			 			 
			     record_FF_time = g_playingSeconds;
			  }
	 }   
	return BLT_SUCCESS;
}
/**********************************************************************/
/*G+ additions end*/
/**********************************************************************/
/*----------------------------------------------------------------------
|    types
+---------------------------------------------------------------------*/
typedef struct {
    ATX_String    name;
    unsigned char value[16];
} BLTP_Key;

typedef struct {
    BLT_CString     output_name;
    BLT_CString     output_type;
    float           output_volume;
    unsigned int    duration;
    unsigned int    verbosity;
    BLT_Boolean     list_modules;
    ATX_List*       plugin_directories;
    ATX_List*       plugin_files;
    ATX_List*       extra_nodes;
    ATX_Properties* core_properties;
    ATX_Properties* stream_properties;
    ATX_List*       keys;
} BLTP_Options;

typedef struct  {
    /* interfaces */
    ATX_IMPLEMENTS(BLT_EventListener);
    ATX_IMPLEMENTS(ATX_PropertyListener);
    ATX_IMPLEMENTS(BLT_KeyManager);
} BLTP;

/*----------------------------------------------------------------------
|    globals
+---------------------------------------------------------------------*/
BLTP_Options Options;

/*----------------------------------------------------------------------
|    flags
+---------------------------------------------------------------------*/
#define BLTP_VERBOSITY_STREAM_TOPOLOGY      1
#define BLTP_VERBOSITY_STREAM_INFO          2
#define BLTP_VERBOSITY_MODULE_INFO          4
#define BLTP_VERBOSITY_MISC                 8

/*----------------------------------------------------------------------
|    macros
+---------------------------------------------------------------------*/
#define BLTP_CHECK(result)                                      \
do {                                                            \
    if (BLT_FAILED(result)) {                                   \
        fprintf(stderr, "runtime error on line %d\n", __LINE__);\
        exit(1);                                                \
    }                                                           \
} while(0)

/*----------------------------------------------------------------------
|    BLTP_PrintUsageAndExit
+---------------------------------------------------------------------*/
static void
BLTP_PrintUsageAndExit(int exit_code)
{
    ATX_ConsoleOutput(
        "--- BlueTune command line player\n"
        "--- BlueTune version " BLT_BLUETUNE_SDK_VERSION_STRING " build " BLT_SVN_VERSION_STRING "\n\n"
        "usage: btplay [options] <input-spec> [<input-spec>..]\n"
        "  each <input-spec> is either an input name (file or URL), or an input type\n"
        "  (--input-type=<type>) followed by an input name\n"
        "\n"
        );
    ATX_ConsoleOutput(
        "options:\n"
        "  -h\n"
        "  --help\n" 
        "  --output=<name>\n"
        "  --output-type=<type>\n"
        "  --output-volume=<volume> (between 0.0 and 1.0)\n"
        "  --duration=<n> (seconds)\n"
        "  --list-modules\n"
        "  --load-plugins=<directory>[,<file-extension>]\n"
        "  --load-plugin=<plugin-filename>\n"
        );
    ATX_ConsoleOutput(
        "  --add-node=<node-name>\n"
        "  --property=<scope>:<type>:<name>:<value>\n"
        "    where <scope> is C (Core) or S (Stream),\n"
        "    <type> is I (Integer), S (String) or B (Boolean),\n"
        "    and <value> is an integer, string or boolean ('true' or 'false')\n"
        "    as appropriate\n"
        );
    ATX_ConsoleOutput(
        "  --verbose=<name> : print messages related to <name>, where name is\n"
        "                     'stream-topology', 'stream-info', 'module-info' or 'all'\n"
        "                     (multiple --verbose= options can be specified)\n"
        "  --key=<name>:<value> : content decryption key for content ID <name>.\n"
        "                         The key value is in hexadecimal\n"
        );
    exit(exit_code);
}

/*----------------------------------------------------------------------
|    BLTP_ParseHexNibble
+---------------------------------------------------------------------*/
static int
BLTP_ParseHexNibble(char nibble)
{
    switch (nibble) {
        case '0': return 0;
        case '1': return 1;
        case '2': return 2;
        case '3': return 3;
        case '4': return 4;
        case '5': return 5;
        case '6': return 6;
        case '7': return 7;
        case '8': return 8;
        case '9': return 9;
        case 'a':
        case 'A': return 0x0A;
        case 'b':
        case 'B': return 0x0B;
        case 'c':
        case 'C': return 0x0C;
        case 'd':
        case 'D': return 0x0D;
        case 'e':
        case 'E': return 0x0E;
        case 'f':
        case 'F': return 0x0F;
        default: return -1;
    }
}

/*----------------------------------------------------------------------
|    BLTP_ParseKey
+---------------------------------------------------------------------*/
static void
BLTP_ParseKey(const char* name_and_value)
{
    BLTP_Key*    key;
    unsigned int length = ATX_StringLength(name_and_value);
    
    /* we need at least a ':' followed by 32 hex chars */
    if (length < 33) {
        fprintf(stderr, "ERROR: invalid syntax for --key argument\n");
        return;
    }
    
    key = (BLTP_Key*)ATX_AllocateZeroMemory(sizeof(BLTP_Key));
    ATX_String_AssignN(&key->name, name_and_value, length-33);
    {
        unsigned int i;
        unsigned int x = 0;
        for (i=length-32; i<length; i+=2) {
            int nib1 = BLTP_ParseHexNibble(name_and_value[i  ]);
            int nib2 = BLTP_ParseHexNibble(name_and_value[i+1]);
            if (nib1 < 0 || nib2 < 0) {
                fprintf(stderr, "ERROR: invalid syntax for --key argument\n");
            }
            key->value[x++] = (nib1<<4) | nib2;
        }
    }
    
    ATX_List_AddData(Options.keys, key);
}

/*----------------------------------------------------------------------
|    BLTP_ParseCommandLine
+---------------------------------------------------------------------*/
static char**
BLTP_ParseCommandLine(char** args)
{
    char* arg;

    /* setup default values for options */
    Options.output_name   = BLT_DECODER_DEFAULT_OUTPUT_NAME;
    Options.output_type   = NULL;
    Options.output_volume = -1.0f;
    Options.duration      = 0;
    Options.verbosity     = 0;
    Options.list_modules  = BLT_FALSE;
    ATX_List_Create(&Options.keys);
    ATX_List_Create(&Options.plugin_directories);
    ATX_List_Create(&Options.plugin_files);
    ATX_List_Create(&Options.extra_nodes);
    ATX_Properties_Create(&Options.core_properties);
    ATX_Properties_Create(&Options.stream_properties);
    
    while ((arg = *args)) {
        if (ATX_StringsEqual(arg, "-h") ||
            ATX_StringsEqual(arg, "--help")) {
            BLTP_PrintUsageAndExit(0);
        } else if (ATX_StringsEqualN(arg, "--output=", 9)) {
            Options.output_name = arg+9;
        } else if (ATX_StringsEqualN(arg, "--output-type=", 14)) {
            Options.output_type = arg+14;
        } else if (ATX_StringsEqualN(arg, "--output-volume=", 16)) {
            float volume;
            if (ATX_SUCCEEDED(ATX_ParseFloat(arg+16, &volume, ATX_TRUE))) {
                if (volume >= 0.0f && volume <= 1.0f) {
                    Options.output_volume = volume;
                } else {
                    fprintf(stderr, "ERROR: output volume value out of range\n");
                    return NULL;
                }
            } else {
                fprintf(stderr, "ERROR: invalid output volume value\n");
                return NULL;
            }
        } else if (ATX_StringsEqualN(arg, "--duration=", 11)) {
            int duration = 0;
            ATX_ParseInteger(arg+11, &duration, ATX_FALSE);
            Options.duration = duration;
        } else if (ATX_StringsEqual(arg, "--list-modules")) {
            Options.list_modules = BLT_TRUE;
        } else if (ATX_StringsEqualN(arg, "--load-plugins=", 15)) {
            ATX_String* directory = (ATX_String*)ATX_AllocateMemory(sizeof(ATX_String));
            *directory = ATX_String_Create(arg+15);
            ATX_List_AddData(Options.plugin_directories, directory);
        } else if (ATX_StringsEqualN(arg, "--load-plugin=", 14)) {
            ATX_String* plugin = (ATX_String*)ATX_AllocateMemory(sizeof(ATX_String));
            *plugin = ATX_String_Create(arg+14);
            ATX_List_AddData(Options.plugin_files, plugin);
        } else if (ATX_StringsEqualN(arg, "--add-node=", 11)) {
            ATX_String* node = (ATX_String*)ATX_AllocateMemory(sizeof(ATX_String));
            *node = ATX_String_Create(arg+11);
            ATX_List_AddData(Options.extra_nodes, node);
        } else if (ATX_StringsEqualN(arg, "--property=", 11)) {
            char*             property = arg+11;
            ATX_Properties*   properties = NULL;
            char*             name = property+4;
            char*             value_string;
            ATX_PropertyValue value;
            if (ATX_StringLength(property) < 7 || property[1] != ':' || property[3] != ':') {
                fprintf(stderr, "ERROR: invalid property syntax\n");
                return NULL;
            }
            switch (property[0]) {
                case 'C': properties = Options.core_properties; break;
                case 'S': properties = Options.stream_properties; break;
                default: fprintf(stderr, "ERROR: invalid property scope\n"); return NULL;
            }
            value_string = &property[4];
            while (*value_string != '\0' && *value_string != ':') {
                value_string++;
            }
            if (*value_string != ':') {
                fprintf(stderr, "ERROR: invalid property syntax\n");
                return NULL;
            } 
            *value_string++ = '\0';
            switch (property[2]) {
                case 'I': 
                    value.type = ATX_PROPERTY_VALUE_TYPE_INTEGER;
                    if (ATX_FAILED(ATX_ParseInteger(value_string, &value.data.integer, ATX_FALSE))) {
                        fprintf(stderr, "ERROR: invalid integer property syntax\n");
                        return NULL;
                    }
                    break;
                    
                case 'S':
                    value.type = ATX_PROPERTY_VALUE_TYPE_STRING;
                    value.data.string = value_string;
                    break;
                    
                case 'B':
                    value.type = ATX_PROPERTY_VALUE_TYPE_INTEGER;
                    if (ATX_StringsEqual(value_string, "true")) {
                        value.data.boolean = ATX_TRUE;
                    } else if (ATX_StringsEqual(value_string, "false")) {
                        value.data.boolean = ATX_FALSE;
                    } else {
                        fprintf(stderr, "ERROR: invalid boolean property syntax\n");
                        return NULL;
                    }
                    break;
                    
                default:
                    fprintf(stderr, "ERROR: invalid property type\n");
                    return NULL;
            }
            ATX_Properties_SetProperty(properties, name, &value);
        } else if (ATX_StringsEqualN(arg, "--verbose=", 10)) {
            if (ATX_StringsEqual(arg+10, "stream-topology")) {
                Options.verbosity |= BLTP_VERBOSITY_STREAM_TOPOLOGY;
            } else if (ATX_StringsEqual(arg+10, "stream-info")) {
                Options.verbosity |= BLTP_VERBOSITY_STREAM_INFO;
            } else if (ATX_StringsEqual(arg+10, "module-info")) {
                Options.verbosity |= BLTP_VERBOSITY_MODULE_INFO;
            } else if (ATX_StringsEqual(arg+10, "all")) {
                Options.verbosity = 0xFFFFFFFF;
            }
        } else if (ATX_StringsEqualN(arg, "--key=", 6)) {
            BLTP_ParseKey(arg+6);
        } else {
            return args;
        }
        ++args;
    }

    return args;
}

/*----------------------------------------------------------------------
|    BLTP_PrintPropertyValue
+---------------------------------------------------------------------*/
static void
BLTP_PrintPropertyValue(const ATX_PropertyValue* value)
{
    switch (value->type) {
      case ATX_PROPERTY_VALUE_TYPE_STRING:
        ATX_ConsoleOutputF("%s", value->data.string);
        break;

      case ATX_PROPERTY_VALUE_TYPE_INTEGER:
        ATX_ConsoleOutputF("%d", value->data.integer);
        break;

      default:
        break;
    }
}

/*----------------------------------------------------------------------
|    BLTP_ListModules
+---------------------------------------------------------------------*/
static void
BLTP_ListModules(BLT_Decoder* decoder)
{
    ATX_List*     modules = NULL;
    ATX_ListItem* item = NULL;
    int           i = 0;
    BLT_Result    result;
    
    /* get the list of modules from the decoder */
    result = BLT_Decoder_EnumerateModules(decoder, &modules);
    if (BLT_FAILED(result)) return;
    
    /* print info about each module */
    for (item = ATX_List_GetFirstItem(modules);
         item;
         item = ATX_ListItem_GetNext(item)) {
        BLT_Module*    module = (BLT_Module*)ATX_ListItem_GetData(item);
        BLT_ModuleInfo info;
        if (BLT_SUCCEEDED(BLT_Module_GetInfo(module, &info))) {
            unsigned int j;
            ATX_ConsoleOutputF("Module %02d: %s\n", i, info.name?info.name:"");
            if (Options.verbosity & BLTP_VERBOSITY_STREAM_INFO) {
                if (info.uid) {
                    ATX_ConsoleOutputF("  uid = %s\n", info.uid);
                }
                for (j=0; j<info.property_count; j++) {
                    ATX_ConsoleOutputF("  %s = ", info.properties[j].name);
                    BLTP_PrintPropertyValue(&info.properties[j].value);
                    ATX_ConsoleOutput("\n");
                }
            }
            i++;
        }
    }
    
    /* cleanup */
    ATX_List_Destroy(modules);
}

/*----------------------------------------------------------------------
|    BLTP_SetupKeyManager
+---------------------------------------------------------------------*/
static void
BLTP_SetupKeyManager(BLTP* player, BLT_Decoder* decoder)
{
    ATX_Properties* properties;
    BLT_Decoder_GetProperties(decoder, &properties);

    {
        ATX_PropertyValue value;
        value.type         = ATX_PROPERTY_VALUE_TYPE_POINTER;
        value.data.pointer = &ATX_BASE(player, BLT_KeyManager);
        ATX_Properties_SetProperty(properties, BLT_KEY_MANAGER_PROPERTY, &value);
    }
}

/*----------------------------------------------------------------------
|    BLTP_OnStreamPropertyChanged
+---------------------------------------------------------------------*/
BLT_VOID_METHOD
BLTP_OnStreamPropertyChanged(ATX_PropertyListener*    self,
                             ATX_CString              name,
                             const ATX_PropertyValue* value)    
{
	BLT_COMPILER_UNUSED(self);

	if (!(Options.verbosity & BLTP_VERBOSITY_STREAM_INFO)) return;

	if (name == NULL) {
		if ( g_debug > 0 )
			ATX_ConsoleOutput("BLTP::OnStreamPropertyChanged - All Properties Cleared\n");
		ClearProperties();
		 UpdateProperty( "Tags/Title", g_playingTrack );
	} else {
		if (value == NULL) {
			if ( g_debug > 0 )
				ATX_ConsoleOutputF("BLTP::OnStreamPropertyChanged - Property %s cleared\n", name);
			UpdateProperty(name, value );
		} else {
			//ATX_ConsoleOutputF("BLTP::OnStreamPropretyChanged - %s = ", name);
			//BLTP_PrintPropertyValue(value);
			//ATX_ConsoleOutput("\n");
			switch (value->type) {
			case ATX_PROPERTY_VALUE_TYPE_STRING:
				//ATX_ConsoleOutputF("%s\n", value->string);
				UpdateProperty( name, value->data.string );
				break;

				case ATX_PROPERTY_VALUE_TYPE_INTEGER:
				{
					char buff[64];
					sprintf( buff, "%d", value->data.integer );
					UpdateProperty( name, buff );
					//ATX_ConsoleOutputF("%d\n", value->integer);
				}
				break;

				default:
				ATX_ConsoleOutputF("Unhandled property type %d for %s\n", value->type, name);
			}			
		}
	}
}

/*----------------------------------------------------------------------
|    BLTP_GetKeyByName
+---------------------------------------------------------------------*/
BLT_METHOD
BLTP_GetKeyByName(BLT_KeyManager* self,
                  const char*     name,
                  unsigned char*  key, 
                  unsigned int*   key_size)
{
    ATX_ListItem* item = ATX_List_GetFirstItem(Options.keys);
    BLT_COMPILER_UNUSED(self);
    
    /* check the key size */
    if (*key_size < 16) {
        *key_size = 16;
        return BLT_ERROR_BUFFER_TOO_SMALL;
    }
    
    for (; item; item = ATX_ListItem_GetNext(item)) {
        BLTP_Key* key_info = (BLTP_Key*)ATX_ListItem_GetData(item);
        if (ATX_String_Equals(&key_info->name, name, ATX_FALSE)) {
            /* found a match */
            ATX_CopyMemory(key, key_info->value, 16);
            *key_size = 16;
            return BLT_SUCCESS;
        }
    }
    
    return ATX_ERROR_NO_SUCH_ITEM;
}

/*----------------------------------------------------------------------
|    BLTP_ShowStreamTopology
+---------------------------------------------------------------------*/
static void
BLTP_ShowStreamTopology(ATX_Object* source)
{
    BLT_Stream*        stream;
    BLT_MediaNode*     node;
    BLT_StreamNodeInfo s_info;
    BLT_Result         result;

    /* cast the source object to a stream object */
    stream = ATX_CAST(source, BLT_Stream);
    if (stream == NULL) return;

    result = BLT_Stream_GetFirstNode(stream, &node);
    if (BLT_FAILED(result)) return;
    while (node) {
        const char* name;
        BLT_MediaNodeInfo n_info;
        result = BLT_Stream_GetStreamNodeInfo(stream, node, &s_info);
        if (BLT_FAILED(result)) break;
        result = BLT_MediaNode_GetInfo(node, &n_info);
        if (BLT_SUCCEEDED(result)) {
            name = n_info.name ? n_info.name : "?";
        } else {
            name = "UNKNOWN";
        }

        if (s_info.input.connected) {
            ATX_ConsoleOutput("-");
        } else {
            ATX_ConsoleOutput(".");
        }

        switch (s_info.input.protocol) {
          case BLT_MEDIA_PORT_PROTOCOL_NONE:
            ATX_ConsoleOutput("!"); break;
          case BLT_MEDIA_PORT_PROTOCOL_PACKET:
            ATX_ConsoleOutput("#"); break;
          case BLT_MEDIA_PORT_PROTOCOL_STREAM_PUSH:
            ATX_ConsoleOutput(">"); break;
          case BLT_MEDIA_PORT_PROTOCOL_STREAM_PULL:
            ATX_ConsoleOutput("<"); break;
          default:
            ATX_ConsoleOutput("@"); break;
        }

        ATX_ConsoleOutputF("[%s]", name);

        switch (s_info.output.protocol) {
          case BLT_MEDIA_PORT_PROTOCOL_NONE:
            ATX_ConsoleOutput("!"); break;
          case BLT_MEDIA_PORT_PROTOCOL_PACKET:
            ATX_ConsoleOutput("#"); break;
          case BLT_MEDIA_PORT_PROTOCOL_STREAM_PUSH:
            ATX_ConsoleOutput(">"); break;
          case BLT_MEDIA_PORT_PROTOCOL_STREAM_PULL:
            ATX_ConsoleOutput("<"); break;
          default:
            ATX_ConsoleOutput("@"); break;
        }

        if (s_info.output.connected) {
            ATX_ConsoleOutput("-");
        } else {
            ATX_ConsoleOutput(".");
        }

        result = BLT_Stream_GetNextNode(stream, node, &node);
        if (BLT_FAILED(result)) break;
    }
    ATX_ConsoleOutput("\n");
}

/*----------------------------------------------------------------------
|    BLTP_OnEvent
+---------------------------------------------------------------------*/
BLT_VOID_METHOD 
BLTP_OnEvent(BLT_EventListener* self,
             ATX_Object*        source,
             BLT_EventType      type,
             const BLT_Event*   event)
{
	BLT_COMPILER_UNUSED(self);
	if (type == BLT_EVENT_TYPE_STREAM_INFO && 
	Options.verbosity & BLTP_VERBOSITY_STREAM_INFO) {
		char value[32];
		const BLT_StreamInfoEvent* e = (BLT_StreamInfoEvent*)event;
		
		if ( g_debug > 0 )
			ATX_ConsoleOutputF("BLTP::OnEvent - info update=%x\n", e->update_mask);

		/*Clear the previous information first*/
		ClearInformation();
		UpdateInformation("file", g_playingTrack);	/*write file name*/
			
		if (e->update_mask & BLT_STREAM_INFO_MASK_NOMINAL_BITRATE) {
			if ( g_debug > 0 )
				ATX_ConsoleOutputF("  nominal_bitrate = %ld\n", e->info.nominal_bitrate);
			sprintf(value, "%d", e->info.nominal_bitrate);
			UpdateInformation("nominal_bitrate", value);
		}
		
		if (e->update_mask & BLT_STREAM_INFO_MASK_AVERAGE_BITRATE) {
			if ( g_debug > 0 )
				ATX_ConsoleOutputF("  average_bitrate = %ld\n", e->info.average_bitrate);
			sprintf(value, "%d", e->info.average_bitrate);
			UpdateInformation("average_bitrate", value);
		} 
		
		if (e->update_mask & BLT_STREAM_INFO_MASK_INSTANT_BITRATE) {
			if ( g_debug > 0 )
				ATX_ConsoleOutputF("  instant_bitrate = %ld\n", e->info.instant_bitrate);
			sprintf(value, "%d", e->info.instant_bitrate);
			UpdateInformation("instant_bitrate", value);
		}
		
		if (e->update_mask & BLT_STREAM_INFO_MASK_SIZE) {
			if ( g_debug > 0 )
				ATX_ConsoleOutputF("  size            = %" ATX_INT64_PRINTF_FORMAT "d\n", e->info.size);
		}
		if (e->update_mask & BLT_STREAM_INFO_MASK_DURATION) {
			if ( g_debug > 0 )
				ATX_ConsoleOutputF("  duration        = %" ATX_INT64_PRINTF_FORMAT "d\n", e->info.duration);
			sprintf(value, "%d", e->info.duration);
			UpdateInformation("duration", value);
		} 
		
		if (e->update_mask & BLT_STREAM_INFO_MASK_SAMPLE_RATE) {
			if ( g_debug > 0 )
				ATX_ConsoleOutputF("  sample_rate     = %ld\n", e->info.sample_rate);
			sprintf(value, "%d", e->info.sample_rate);
			UpdateInformation("sample_rate", value);
		}
		
		if (e->update_mask & BLT_STREAM_INFO_MASK_CHANNEL_COUNT) {
			if ( g_debug > 0 )
				ATX_ConsoleOutputF("  channel_count   = %ld\n", e->info.channel_count);
			sprintf(value, "%d", e->info.channel_count);
			UpdateInformation("channel_count", value);
		}
		
		if (e->update_mask & BLT_STREAM_INFO_MASK_FLAGS) {
			if ( g_debug > 0 )
				ATX_ConsoleOutputF("  flags           = %x", e->info.flags);
			if (e->info.flags & BLT_STREAM_INFO_FLAG_VBR) {
				if ( g_debug > 0 )
					ATX_ConsoleOutputF(" (VBR)\n");
				sprintf(value, "%d", e->info.flags);
				UpdateInformation("flags", value);
			} else {
				if ( g_debug > 0 )
					ATX_ConsoleOutput("\n");
			}
		}
		if (e->update_mask & BLT_STREAM_INFO_MASK_DATA_TYPE) {
			if ( g_debug > 0 )
				ATX_ConsoleOutputF("  data_type       = %s\n", 
						e->info.data_type ? e->info.data_type : "");
		}
	} else if (type == BLT_EVENT_TYPE_STREAM_TOPOLOGY &&
	   Options.verbosity & BLTP_VERBOSITY_STREAM_TOPOLOGY) {
		const BLT_StreamTopologyEvent* e = (BLT_StreamTopologyEvent*)event;
		switch (e->type) {
			case BLT_STREAM_TOPOLOGY_NODE_ADDED:
				ATX_ConsoleOutput("STREAM TOPOLOGY: node added\n");  
			break;
			case BLT_STREAM_TOPOLOGY_NODE_REMOVED:
				ATX_ConsoleOutput("STREAM TOPOLOGY: node removed\n");
			break;
			case BLT_STREAM_TOPOLOGY_NODE_CONNECTED:
				ATX_ConsoleOutput("STREAM TOPOLOGY: node connected\n");
			break;
			default:
			break;
		}
		BLTP_ShowStreamTopology(source);
	}
}

/*----------------------------------------------------------------------
|   standard GetInterface implementation
+---------------------------------------------------------------------*/
ATX_BEGIN_GET_INTERFACE_IMPLEMENTATION(BLTP)
    ATX_GET_INTERFACE_ACCEPT(BLTP, BLT_EventListener)
    ATX_GET_INTERFACE_ACCEPT(BLTP, ATX_PropertyListener)
ATX_END_GET_INTERFACE_IMPLEMENTATION

/*----------------------------------------------------------------------
|    ATX_PropertyListener interface
+---------------------------------------------------------------------*/
ATX_BEGIN_INTERFACE_MAP(BLTP, ATX_PropertyListener)
    BLTP_OnStreamPropertyChanged
ATX_END_INTERFACE_MAP

/*----------------------------------------------------------------------
|    BLT_EventListener interface
+---------------------------------------------------------------------*/
ATX_BEGIN_INTERFACE_MAP(BLTP, BLT_EventListener)
    BLTP_OnEvent
ATX_END_INTERFACE_MAP

/*----------------------------------------------------------------------
|    BLT_KeyManager interface
+---------------------------------------------------------------------*/
ATX_BEGIN_INTERFACE_MAP(BLTP, BLT_KeyManager)
    BLTP_GetKeyByName
ATX_END_INTERFACE_MAP

/*----------------------------------------------------------------------
|    BLTP_CheckElapsedTime
+---------------------------------------------------------------------*/
static BLT_Result
BLTP_CheckElapsedTime(BLT_Decoder* decoder, unsigned int duration)
{
    BLT_DecoderStatus status;
    
    if (duration == 0) return BLT_SUCCESS;
    BLT_Decoder_GetStatus(decoder, &status);
    if (status.time_stamp.seconds > (int)duration) {
        ATX_ConsoleOutput("END of specified duration\n");
        return BLT_FAILURE;
    }
    return BLT_SUCCESS;
}
/*********************************************
operation the audiomixer directly
********************************************/
#if defined(BLT_CONFIG_MODULES_ENABLE_AudioMixer_OUTPUT)
typedef enum {
    BLT_OSS_OUTPUT_STATE_CLOSED,
    BLT_OSS_OUTPUT_STATE_OPEN,
    BLT_OSS_OUTPUT_STATE_CONFIGURED,
	BLT_OSS_OUTPUT_STATE_FORCEDSTOP
} AudioMixerState;

typedef struct {
    /* base class */
    ATX_EXTENDS   (BLT_BaseMediaNode);

    /* interfaces */
    ATX_IMPLEMENTS(BLT_PacketConsumer);
    ATX_IMPLEMENTS(BLT_OutputNode);
    ATX_IMPLEMENTS(BLT_MediaPort);

    /* members */
    AudioMixerState    state;
    ATX_String        device_name;
  
	audiomixer_handle_t* device_handle;

    BLT_Flags         device_flags;
    BLT_PcmMediaType  media_type;
    BLT_PcmMediaType  expected_media_type;
    BLT_Cardinal      bytes_before_trigger;
    EqualizerConvert  equalizer;
} AudioMixer; 
/****************************
get audiomixer
******************************/
void get_audiomixer(BLT_Decoder *decoder,AudioMixer **self)
{
 	BLT_MediaNode * node;
	printf("entry the change the status of audiomixer \n\r");  
	BLT_Decoder_GetOutputNode(decoder,&node);
	*self =ATX_SELF_EX_O(node,AudioMixer, BLT_BaseMediaNode, BLT_MediaNode);
	
}
/****************************
breif : when recevie stop message,then did not sync the audio.
***************************/
void close_oss(BLT_Decoder *decoder)
{
 AudioMixer *self;
 get_audiomixer(decoder,&self); 
 self->state = BLT_OSS_OUTPUT_STATE_FORCEDSTOP;
}
/*************************************************
pause the audiomixer
***************************************************/
void pause_audiomixer(BLT_Decoder *decoder)
{
  AudioMixer *self;
  get_audiomixer(decoder,&self); 
  audiomixer_clear_input_buffer(self->device_handle);
  audiomixer_pause(self->device_handle);
}
/*************************************************
resume the audiomixer
****************************************************/
void resume_audiomixer(BLT_Decoder *decoder)
{
  AudioMixer *self;
  get_audiomixer(decoder,&self); 
  audiomixer_resume(self->device_handle);
}
#else
void close_oss(BLT_Decoder *decoder)
{
typedef enum {
    BLT_OSS_OUTPUT_STATE_CLOSED,
    BLT_OSS_OUTPUT_STATE_OPEN,
    BLT_OSS_OUTPUT_STATE_CONFIGURED
} OssOutputState;

typedef struct {
    /* base class */
    ATX_EXTENDS   (BLT_BaseMediaNode);

    /* interfaces */
    ATX_IMPLEMENTS(BLT_PacketConsumer);
    ATX_IMPLEMENTS(BLT_OutputNode);
    ATX_IMPLEMENTS(BLT_MediaPort);

    /* members */
    OssOutputState    state;
    ATX_String        device_name;
    int               device_handle;
    BLT_Flags         device_flags;
    BLT_PcmMediaType  media_type;
    BLT_PcmMediaType  expected_media_type;
    BLT_Cardinal      bytes_before_trigger;
    EqualizerConvert  equalizer;
} OssOutput;
  BLT_MediaNode * node;
  OssOutput* self ;
  printf("entry the close_oss");  
 
  BLT_Decoder_GetOutputNode(decoder,&node);
  printf("node adress 0x%x",node);
  self =ATX_SELF_EX_O(node,OssOutput, BLT_BaseMediaNode, BLT_MediaNode);
  printf("address of OssOutput 0x%x\n\r",self);

  self->state = BLT_OSS_OUTPUT_STATE_OPEN ;
}
#endif

/*----------------------------------------------------------------------
|    main
+---------------------------------------------------------------------*/
int
main(int argc, char** argv)
{
	BLT_Decoder* decoder = NULL;
	BLT_CString  input_name;
	BLT_CString  input_type = NULL;
	BLTP         player;
	BLT_Result   result;
	
	/*mtrace();*/
	BLT_COMPILER_UNUSED(argc);

	/*Check start daemon or not*/
	g_invokedAsDaemon = (strstr( argv[0], "btplayd" ) != NULL) ? 1 : 0;
	if (g_invokedAsDaemon) {
		g_usingCmdPipe = 1;
	}

	/* print version string */
	ATX_ConsoleOutputF( "btplay%s v" VER_STR "\n", g_invokedAsDaemon ? "d" : " client" );
		
	/*check number of parameter*/
	if (argc < 2 && !g_invokedAsDaemon) BLTP_PrintUsageAndExit(0);

	/* parse command line */
	argv = BLTP_ParseCommandLine(argv+1);
	
	if (argv == NULL && !g_invokedAsDaemon) goto end;

	if (g_invokedAsDaemon) {
		g_btcmdSerial = 0;
		g_playingSeconds = 0;
		#if 0
		// Set up signal handlers
		signal( SIGINT, btSignalHandler );
		signal( SIGTERM, btSignalHandler );
		signal( SIGPIPE, btSignalHandler );
		signal( SIGUSR1, btSignalHandler );
		signal( SIGUSR2, btSignalHandler );
		#endif
		
		// Write pid - flashplayer needs to see this before it will try to open the command pipe
		if (!WritePidExclusive())
		{
			goto end;
		}

		DEBUG0("init pipe!\n");

		/* Open command interface if specified (may block until input opened by another process) */
		if (!CmdPipeInit())
		{
			unlink( BTPLAY_PIDFILE );
			goto end;
		}

		// Write play state (initially not paused)
		WritePlayState();
	}
#ifdef EQ_ENABLE
	eq_init();

#endif
wait_for_start:
    // If decoder exists, kill it
	if (decoder) {
		if (g_debug) ATX_ConsoleOutputF( "Destroying decoder instance %08lx\n", (unsigned long)decoder );
		/* drain any buffered audio */
		BLT_Decoder_Drain(decoder);	
		/*Remove Equalizer convert function*/
		BLT_Decoder_SetEqualizerFunction(decoder, NULL);
		/* destroy the decoder */
		BLT_Decoder_Destroy(decoder);
		decoder = NULL;
	} // destroy decoder and mark it as nonexistent
	if ( g_invokedAsDaemon ) {
		while( (!g_terminate) && (g_btState == BT_STATE_STOP) ) {
			int pumpRes = CmdPump();
			if( pumpRes == 2){
				break;
			}

			if (g_debug > 1) ATX_ConsoleOutput( "w" );
			
			if (g_terminate)
			{
				ATX_ConsoleOutput( "Terminate requested, exiting wait_for_queue\n" );
				break;
			}
			
			usleep( 250000 );
		}
	}
	if(decoder == NULL ) {
		/* create a decoder */
		result = BLT_Decoder_Create(&decoder);
		BLTP_CHECK(result);

		/* setup our interfaces */
		ATX_SET_INTERFACE(&player, BLTP, BLT_EventListener);
		ATX_SET_INTERFACE(&player, BLTP, ATX_PropertyListener);
		ATX_SET_INTERFACE(&player, BLTP, BLT_KeyManager);

		/* listen to stream events */
		BLT_Decoder_SetEventListener(decoder, &ATX_BASE(&player, BLT_EventListener));
		         
		/* listen to stream properties events */
		{
		    ATX_Properties* properties;
		    BLT_Decoder_GetStreamProperties(decoder, &properties);
		    ATX_Properties_AddListener(properties, NULL, &ATX_BASE(&player, ATX_PropertyListener), NULL);
		}

		/* setup a key manager for encrypted files */
		BLTP_SetupKeyManager(&player, decoder);

		/* register builtin modules */
		result = BLT_Decoder_RegisterBuiltins(decoder);
		BLTP_CHECK(result);

		/* load and register loadable plugins */
		{
		    ATX_ListItem* item;
		    for (item = ATX_List_GetFirstItem(Options.plugin_files); item; item = ATX_ListItem_GetNext(item)) {
		        ATX_String* plugin = (ATX_String*)ATX_ListItem_GetData(item);
		        BLT_Decoder_LoadPlugin(decoder, ATX_String_GetChars(plugin), BLT_PLUGIN_LOADER_FLAGS_SEARCH_ALL);
		    }
		    for (item = ATX_List_GetFirstItem(Options.plugin_directories); item; item = ATX_ListItem_GetNext(item)) {
		        ATX_String* spec      = (ATX_String*)ATX_ListItem_GetData(item);
		        const char* directory = ATX_String_GetChars(spec);
		        const char* extension = ".plugin";
		        int         separator = ATX_String_ReverseFindChar(spec, ',');
		        if (separator > 0) {
		            ATX_String_UseChars(spec)[separator] = '\0';
		            extension = directory+separator+1;
		        }
		        BLT_Decoder_LoadPlugins(decoder, directory, extension);
		    }
		}

		/* list the modules if required */
		if (Options.list_modules) BLTP_ListModules(decoder);

		/* set the output */
		result = BLT_Decoder_SetOutput(decoder,
		                               Options.output_name, 
		                               Options.output_type);
		if (BLT_FAILED(result)) {
		    fprintf(stderr, "SetOutput failed: %d (%s)\n", result, BLT_ResultText(result));
		    exit(1);
		}
        /*set output equalizer convert function*/
		result = BLT_Decoder_SetEqualizerFunction(decoder, BLTP_EqualizerConvert);
		/* set the output volume */
		if (Options.output_volume >= 0.0f) {
		    result = BLT_Decoder_SetVolume(decoder, Options.output_volume);
		    if (BLT_FAILED(result)) {
		        fprintf(stderr, "SetVolume failed: %d (%s)\n", result, BLT_ResultText(result));
		    }
		}

		/* add optional extra nodes */
		{
		    ATX_ListItem* item;
		    for (item = ATX_List_GetFirstItem(Options.extra_nodes); item; item = ATX_ListItem_GetNext(item)) {
		        ATX_String* node = (ATX_String*)ATX_ListItem_GetData(item);
		        BLT_Decoder_AddNodeByName(decoder, NULL, ATX_String_GetChars(node));
		    }
		}

		/* set core properties */
		{
		    ATX_Properties* properties;
		    ATX_Iterator*   it;
		    void*           next;
		    BLT_Decoder_GetProperties(decoder, &properties);
		    ATX_Properties_GetIterator(Options.core_properties, &it);
		    while (ATX_SUCCEEDED(ATX_Iterator_GetNext(it, &next))) {
		        ATX_Property* property = (ATX_Property*)next;
		        ATX_Properties_SetProperty(properties, property->name, &property->value); 
		    }
		    ATX_DESTROY_OBJECT(it);
		}
	}

wait_for_change:
	
	if ( !g_invokedAsDaemon ) {	
		/* process each input in turn */
		while ((input_name = *argv++)) {
		    if (ATX_StringsEqualN(input_name, "--input-type=", 13)) {
		        input_type = input_name+13;
		        continue;
		    }

		    /* set the input name */
		    result = BLT_Decoder_SetInput(decoder, input_name, input_type);
		    if (BLT_FAILED(result)) {
		        ATX_ConsoleOutputF("SetInput failed: %d (%s)\n", result, BLT_ResultText(result));
		        input_type = NULL;
		        continue;
		    }

		    /* set stream properties */
		    {
		        ATX_Properties* properties;
		        ATX_Iterator*   it;
		        void*           next;
		        BLT_Decoder_GetStreamProperties(decoder, &properties);
		        ATX_Properties_GetIterator(Options.stream_properties, &it);
		        while (ATX_SUCCEEDED(ATX_Iterator_GetNext(it, &next))) {
		            ATX_Property* property = (ATX_Property*)next;
		            ATX_Properties_SetProperty(properties, property->name, &property->value); 
		        }
		        ATX_DESTROY_OBJECT(it);
		    }

		    /* pump the packets */
		    do {
			/*usleep( 10000 );*/
		        /* process one packet */
		        result = BLT_Decoder_PumpPacket(decoder);
		        
		        /* if a duration is specified, check if we have exceeded it */
		        if (BLT_SUCCEEDED(result)) result = BLTP_CheckElapsedTime(decoder, Options.duration);
		    } while (BLT_SUCCEEDED(result));
		    if (Options.verbosity & BLTP_VERBOSITY_MISC) {
		        ATX_ConsoleOutputF("final result = %d (%s)\n", result, BLT_ResultText(result));
		    }

		    /* reset input type */
		    input_type = NULL;
		}
	}	
	else if( !g_terminate ) {
			/* set the input name */
			if( g_btResource.name ){
				/*record playing file name*/
				char *lastSlash = strrchr(g_btResource.name, '/' );
				if (!lastSlash) lastSlash = g_btResource.name;
				else lastSlash++;
				strncpy( g_playingTrack, lastSlash, sizeof(g_playingTrack)-1 );
				g_playingTrack[sizeof(g_playingTrack)-1] = '\0';

				/*clear playing time first*/
				g_playingSeconds = 0;
				printf("[%s,%d]g_btResource.name=%s",__FUNCTION__,__LINE__,g_btResource.name);
				result = BLT_Decoder_SetInput(decoder, g_btResource.name, g_btResource.type);
				if (BLT_FAILED(result)) {
					ATX_ConsoleOutputF("BtPlay:: SetInput failed (%d)\n", result);
					g_btState = BT_STATE_STOP;
					WritePlayState();
					BLTP_PostMessage("status FAIL");
					goto wait_for_start;
				} 
				else {
					BLTP_PostMessage("status OK");
				}

				int pumpPass = 0;
				/* pump the packets */
				do {
					int pumpRes;
					
					// Terminate requested?
					if (g_terminate)
					{
						ATX_ConsoleOutput( "BtPlay::Exiting main loop, terminate requested\n" );
						break;
					}

					if( g_btState == BT_STATE_PAUSE) {
						if (g_debug > 1) ATX_ConsoleOutput( "#" );
						pumpRes = CmdPump();
						if (g_terminate)
						{
							ATX_ConsoleOutput( "Terminate requested, exiting paused state\n" );
							break;
						}
						switch ( pumpRes ) {
							case 2:			/*play file change*/
								//goto wait_for_change;
							    //break;
							case 3: 			/*stop*/
								if ( decoder != NULL ) {
								    /*unable sync the auido driver */
								    close_oss(decoder);
									/* drain any buffered audio */
									BLT_Decoder_Drain(decoder);
									/*Remove Equalizer convert function*/
									BLT_Decoder_SetEqualizerFunction(decoder, NULL);
									/* destroy the decoder */
									BLT_Decoder_Destroy(decoder);
									decoder = NULL;
									BLTP_PostMessage("status END");
								}
								goto wait_for_start;
								break;
							case 1:			/*state change*/
							     #if defined(BLT_CONFIG_MODULES_ENABLE_AudioMixer_OUTPUT)
							     if(g_btState == BT_STATE_PLAY)
									resume_audiomixer(decoder);
								#endif	
							default:
								break;
						}
						usleep( 250000 );
						continue;
					}

					// Check command pump but not on every packet
					if (++pumpPass % PUMP_CMD_FREQUENCY == 0 )	{
						CHTRACE
						pumpPass = 0;
						pumpRes = CmdPump();
						if (g_terminate) {
							ATX_ConsoleOutput( "Terminate requested, exiting packet pump outer loop\n" );
							break;
						}
						switch (pumpRes) {
							case 1:		/*state change*/
							    #if defined(BLT_CONFIG_MODULES_ENABLE_AudioMixer_OUTPUT)
							    if(g_btState == BT_STATE_PAUSE)
									pause_audiomixer(decoder);
								#endif	
								continue;
								break;
							case 2:		/*play file change*/
								//goto wait_for_change;
								//break;
							case 3:
								// If decoder exists, kill it
								if (decoder) {
									if (g_debug) ATX_ConsoleOutputF( "Destroying decoder instance %08lx on CmdPump() = 3\n", (unsigned long)decoder );
									/*unable sync the auido driver */
									close_oss(decoder);
									/* drain any buffered audio */
									BLT_Decoder_Drain(decoder);
									/*Remove Equalizer convert function*/
									BLT_Decoder_SetEqualizerFunction(decoder, NULL);
									/* destroy the decoder */
									BLT_Decoder_Destroy(decoder);
									decoder = NULL;
									BLTP_PostMessage("status END");
								} // destroy decoder and mark it as nonexistent
								goto wait_for_start;
						}
					}

					/*Seek is specify*/
					if ( g_seektime != 0 ) {
						result = BLT_Decoder_SeekToTime(decoder, (BLT_UInt64) g_seektime);
						g_seektime = 0;
						usleep(10000);
						if(BLT_SUCCEEDED(result))
							continue;
						else {
							if (g_debug > 1) ATX_ConsoleOutput( "btplay:seek fail\n" );
							if (decoder) {
								/* drain any buffered audio */
								BLT_Decoder_Drain(decoder);
								/*Remove Equalizer convert function*/
								BLT_Decoder_SetEqualizerFunction(decoder, NULL);
								/* destroy the decoder */
								BLT_Decoder_Destroy(decoder);
								decoder = NULL;
							} // destroy decoder and mark it as nonexistent
							BLTP_PostMessage("status END");
							goto wait_for_start;	
						}
					}
					
					/* process one packet */
					if (g_debug > 1) ATX_ConsoleOutput( "@" );
					
					CHTRACE
					/*usleep( 10000 );*/
					result = BLT_Decoder_PumpPacket(decoder);

					CHTRACE
					/* if a duration is specified, check if we have exceeded it 
					  if (BLT_SUCCEEDED(result)) result = BLTP_CheckElapsedTime(decoder, Options.duration);		
					*/
					BLTP_UpdateTime(decoder);
					if(g_btState ==BT_STATE_STOP)
					{
					    printf("stop the decoder\n\r");
						if (decoder) {
										if (g_debug) ATX_ConsoleOutputF( "Destroying decoder instance %08lx on CmdPump() = 3\n", (unsigned long)decoder );
										/* drain any buffered audio */
										BLT_Decoder_Drain(decoder);
										/*Remove Equalizer convert function*/
									    BLT_Decoder_SetEqualizerFunction(decoder, NULL);
										/* destroy the decoder */
										BLT_Decoder_Destroy(decoder);
										decoder = NULL;
										WritePlayState();
										BLTP_PostMessage("status END");
									} // destroy decoder and mark it as nonexistent
						goto wait_for_start;
					}				
				} while(!g_terminate && BLT_SUCCEEDED(result));
				if(g_playmode==1)
				      goto wait_for_change;
			}
	}
        

	// If running from command input, wait for something to be added to the queue
	if (g_invokedAsDaemon && !g_terminate) {
		CHTRACE
		g_btState = BT_STATE_STOP;
		WritePlayState();
		BLTP_PostMessage("status END");
		goto wait_for_start;
	}

	if (g_debug)
	{
		ATX_ConsoleOutput( "btplayd exiting\n" );
		fflush_unlocked( stdout );
	}

	if ( g_invokedAsDaemon ) {
		// Delete pid file, state file and properties
		unlink( BTPLAY_PIDFILE );
		unlink( BTPLAY_PROPSFILE );
		unlink( BTPLAY_STATEFILE );
		unlink( BTPLAY_INFORFILE );
		
		if (g_debug)
		{
			ATX_ConsoleOutput( "starting CmdPipeCleanup()\n" );
			fflush_unlocked( stdout );
		}
		// Clean up command pipe handles but leave nodes in place
		CmdPipeCleanup();

		if(g_btResource.name)
			free(g_btResource.name);
		if(g_btResource.type)
			free(g_btResource.type);
	}
	
	/* drain any buffered audio */
	BLT_Decoder_Drain(decoder);

	/* destroy the decoder */
	BLT_Decoder_Destroy(decoder);

	end:
	/* cleanup */
	{
	    ATX_ListItem* item;
	    for (item = ATX_List_GetFirstItem(Options.plugin_files); item; item = ATX_ListItem_GetNext(item)) {
	        ATX_String* s = (ATX_String*)ATX_ListItem_GetData(item);
	        ATX_String_Destruct(s);
	        ATX_FreeMemory(s);
	    }
	    ATX_List_Destroy(Options.plugin_files);
	    for (item = ATX_List_GetFirstItem(Options.plugin_directories); item; item = ATX_ListItem_GetNext(item)) {
	        ATX_String* s = (ATX_String*)ATX_ListItem_GetData(item);
	        ATX_String_Destruct(s);
	        ATX_FreeMemory(s);
	    }
	    ATX_List_Destroy(Options.plugin_directories);
	    for (item = ATX_List_GetFirstItem(Options.extra_nodes); item; item = ATX_ListItem_GetNext(item)) {
	        ATX_String* s = (ATX_String*)ATX_ListItem_GetData(item);
	        ATX_String_Destruct(s);
	        ATX_FreeMemory(s);
	    }
	    ATX_List_Destroy(Options.extra_nodes);
	    
	    ATX_DESTROY_OBJECT(Options.core_properties);
	    ATX_DESTROY_OBJECT(Options.stream_properties);
	}

	return 0;
}
