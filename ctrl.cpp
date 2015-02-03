#include <iostream>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <syslog.h>
#include <signal.h>
#include <stdio.h>
#include <unistd.h>
#include <map>
#include <vector>
#include <fstream>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <signal.h>
#include <unistd.h>

#include "ctrl.h"

using namespace std;

#define MS_VARTYPE_STRING		1
#define MS_VARTYPE_INT			2
#define MS_VARTYPE_UINT			3
#define MAX_PATH_NAME			128
#define SRV_ERR_CONF		 	-1	
#define MB_TO_BYTE			1024*1024	
#define MAX_REMINDDAY			1200

struct _configinfo_t
{
	char sVarName[64];
	int iVarType;
	void* pVarAddr;
	int iVarSize;
};


static CCtrl stCtrl;

static int g_iMaxProcs = 20;
static int g_iMinProcs = 10;
static char g_sFileLockPath[MAX_PATH_NAME]={0};

static vector<struct tm> vRemindDay;

static int g_iRemindHour = 0;
static int g_iRemindDay = 5;
static int g_iCheckInterval = 600;

static void InitAsDaemon();
static void ShowHelp(char* sCommand);
static void OnAlarm(int iSigNo);
static void GetServerConfig( const char* sConfigFile );

static void GetRemindDayList();
static bool IsTimeOut(const struct tm * pTm);

static int LoadConfigVarValue( const char* sBuffer, struct _configinfo_t* ptConfigInfo );

//Config Var Structure
struct _configinfo_t g_stConfigInfoArray[] = {
    { "ServerMaxProcs", MS_VARTYPE_INT, &g_iMaxProcs, 1000 },
    { "ServerMinProcs", MS_VARTYPE_INT, &g_iMinProcs, 500 },
    { "FileLockPath", MS_VARTYPE_STRING, g_sFileLockPath, MAX_PATH_NAME },
    { "RemindHour", MS_VARTYPE_INT, &g_iRemindHour, 1000 },
    { "RemindDay", MS_VARTYPE_INT, &g_iRemindDay, 1000 },
    { "CheckInterval", MS_VARTYPE_INT, &g_iCheckInterval, 1000 },
};

CCtrl::CCtrl()
{
}

CCtrl::~CCtrl()
{
}

void CCtrl::ForkChildProcs(int childnum)
{
	pid_t cld_pid;
	for (int i=0; i<childnum; i++)
	{
		// init
		ChildProcsInfo procs;
		memset( &procs, 0, sizeof(procs) );

		// open pipe error
		if (pipe( procs.fd1 ) != 0)
		{
			printf("ForkChildProcs::pipe(fd1) Error");
			continue;
		}	
		if (pipe( procs.fd2 ) != 0)
		{
			printf("ForkChildProcs::pipe(fd2) Error");
			close(procs.fd1[0]);
			close(procs.fd1[1]);
			continue;
		}	

		procs.procsstate = procsstate_idle;
		
		// fork
		if ((cld_pid = fork()) < 0)
		{
			// error
			printf("ForkChildProcs::fork() Error");
			continue;
		}
		else if (cld_pid == 0)
		{
			// child
			close( procs.fd1[1] );
			close( procs.fd2[0] );
			CtrlProc stPro( procs.fd1[0], procs.fd2[1] );
			stPro.DoIt();
			exit(0);
		}
		else
		{
			// parent 
			close( procs.fd1[0] );
			close( procs.fd2[1] );
			m_mapProcsList.insert( pair<pid_t, ChildProcsInfo>(cld_pid, procs) );
		}
	}	
}

void CCtrl::Select( int iCmd )
{
	srand(time(NULL));
	int iID = rand() % 1000;
	printf("pid:%d dispatch::Select Begin ID:%d\n", getpid(), iID );
	
	fflush(stdout);
	DoIt( iCmd, iID );
}

void CCtrl::Run()
{
	int  iCmd;
	bool bDatelyStat = false;
	bool bWeeklyStat = false;
	time_t tBegin, tLeft;
	struct tm * stTimeT;
	map<pid_t, ChildProcsInfo>::iterator it;

	// fork child process first
	ForkChildProcs(g_iMinProcs);

	// get from mem
	while (true)
	{
		// the every 10 sec;
		tBegin = time(NULL);
		stTimeT = localtime(&tBegin);


		if (g_iCheckInterval == 0)
		{
			printf("PROCRUNSTAT:Running do nothing, because CheckInterval=%d \n",
					m_mapProcsList.size(), stTimeT->tm_mday, stTimeT->tm_hour, bDatelyStat?1:0, g_iCheckInterval );
			// do nothing;
			sleep(3);
			continue;
		}

		Select(CMD_SELECT);

		sleep(1);
	}
	//end of while
}


void CCtrl::SelectIdleProc()
{
	int iRet = 0;
	int iChildState = 0, iMaxFd, retval;
	map<pid_t, ChildProcsInfo>::iterator it;

	// for select 
	fd_set rfds;
	struct timeval tv;

	// init
	tv.tv_sec = 0;
	tv.tv_usec = 0;
	FD_ZERO(&rfds);
	iMaxFd = 0;

	// set first
	for (it=m_mapProcsList.begin(); it!=m_mapProcsList.end(); ++it)
	{
		FD_SET( it->second.fd2[0], &rfds );
		if(iMaxFd < it->second.fd2[0]) iMaxFd = it->second.fd2[0];
	}

	// select 
	retval = select(iMaxFd + 1, &rfds, NULL, NULL, &tv);

	if (retval == -1)
	{
		printf("SelectIdleProc::select retval=-1 Error\n");
	}
	else if (retval)
	{
		for (it=m_mapProcsList.begin(); it!=m_mapProcsList.end(); ++it)
		{
			if (FD_ISSET( it->second.fd2[0], &rfds ))
			{
				retval = read( it->second.fd2[0], &iChildState, sizeof(iChildState) );
				if(retval == -1 && errno==EINTR)
				{
					printf("SelectIdleProc::select read Error break\n");
					break;
				}
				if(retval == -1)
				{
					printf("SelectIdleProc::select read Error continue\n");
					continue;
				}

				// to do
				if (iChildState == RET_OK)
				{
					it->second.procsstate = procsstate_idle;
					printf("SelectIdleProc::OK [%u]\n", it->second.iID );
				}
				else if (iChildState == RET_ERROR)
				{
					it->second.procsstate = procsstate_idle;
					printf("SelectIdleProc::select read Error\n");
				}
				else
				{
					it->second.procsstate = procsstate_idle;
					printf("SelectIdleProc::select read Other\n");
				}
			}
			// end of if, end set the retval
		}
		// end of for, end reading the selected
	}
	else
	{
		printf("DoIt::select retval=0\n");
	}
}

void CCtrl::DoIt( int iCmd, unsigned int iID )
{
	bool bHasIdle = false;
	int idx, iFileSize;
	S_PROC_CMD stProcCmd;
	map<pid_t, ChildProcsInfo>::iterator it;

	stProcCmd.iCmd = iCmd;
	stProcCmd.iID = iID;
	
	while (true)
	{
		// select 
		SelectIdleProc();

		// finding the idle child process
		for (it=m_mapProcsList.begin(); it!=m_mapProcsList.end(); ++it)
		{
			if (it->second.procsstate == procsstate_idle)
			{
				// yes I got it 
				bHasIdle = true;
				break;
			}
		}
		
		if (bHasIdle)
		{
			printf( "DoIt::write CMD[%u]\n", iID);
			write(it->second.fd1[1], &stProcCmd, sizeof(stProcCmd));

			it->second.iID = iID;
			it->second.procsstate = procsstate_busy;

			break;
		}
		else if (m_mapProcsList.size() < g_iMaxProcs)
		{
			// fork another 
			ForkChildProcs(1);
		}
		else
		{
			printf( "DoIt::No more idle childprocess, waiting, allcount[%d]\n", m_mapProcsList.size() );
			sleep(1);
		}
	}
	// end of while
}

void sig_chld(int signo)
{ 
	pid_t pid; 
	int stat; 
	
	while ( (pid = waitpid(-1, &stat, WNOHANG)) > 0 )
	{ 
		printf( "sig_chld::child %d terminated\n", pid); 
		if(!WIFEXITED(stat))
		{
			printf( "sig_chld::error id[%u]\n", stCtrl.m_mapProcsList[pid].iID ); 
		}
		close( stCtrl.m_mapProcsList[pid].fd1[1] );
		close( stCtrl.m_mapProcsList[pid].fd2[0] );

		stCtrl.m_mapProcsList.erase(pid); 
	}
}

int main(int argc, char* argv[]){
	if (argc != 2)
	{
		ShowHelp(argv[0]);
		exit(1);
	}
	
	try
	{
		GetServerConfig( argv[1] );
		
		InitAsDaemon();
		
		signal( SIGCHLD, sig_chld );
		printf("ctrl start\n");
		
		stCtrl.Run();
	}
	catch (...) 
	{
		exit(2);
	}
	
	return 0;
}

static void InitAsDaemon()
{
    if (fork() > 0)
        exit(0);
	
    setsid();
	
    signal(SIGINT, SIG_IGN);
    signal(SIGHUP, SIG_IGN);
    signal(SIGQUIT, SIG_IGN);
    signal(SIGPIPE, SIG_IGN);
    signal(SIGTTOU, SIG_IGN);
    signal(SIGTTIN, SIG_IGN);
	
    if (fork() > 0)
        exit(0);
	
    umask(0);
}



static void ShowHelp(char* sCommand)
{
    cout << endl;
    cout << "Usage: " << sCommand << " ctrl.conf" << endl;
    cout << endl;
}


void GetServerConfig( const char* sConfigFile ){
    struct stat statbuf;
    memset( &statbuf, 0, sizeof(struct stat) );
    if ( stat( sConfigFile, &statbuf ) != 0 ) {
        cout<< "Config File \"" << sConfigFile << "\" Not Exist! Server Stop!" <<endl;
        exit(2);
    }
    FILE* pf = fopen( sConfigFile, "r" );
    char* sBuffer = new char[statbuf.st_size];
    fread( sBuffer, 1, statbuf.st_size, pf );
    fclose( pf );
    
    for( int i = 0; i < sizeof(g_stConfigInfoArray) / sizeof(g_stConfigInfoArray[0]); i++ ) {
        if (LoadConfigVarValue( sBuffer, &g_stConfigInfoArray[i] ) != 0) {
            cout<< "Load Config \"" << g_stConfigInfoArray[i].sVarName << "\" Error! Server Stop!" <<endl;
            delete []sBuffer;
            exit(2);
        }
    }
    
    delete []sBuffer;
   
    return;
}

int LoadConfigVarValue( const char* sBuffer, struct _configinfo_t* ptConfigInfo ){
    const char *pStart = NULL;  //开始点
    int i = 0;
    int iTemp = 0;
    char pVarValue[256];
    
    if ( ( pStart = strstr( sBuffer, ptConfigInfo->sVarName ) ) == NULL )
        return SRV_ERR_CONF;
    pStart += strlen( ptConfigInfo->sVarName ) + 1;  //开始点为pstart+pvarname长度+‘=’
    while ( pStart[i] != '\r' &&  pStart[i]!= '\n' && pStart[i] != EOF ) {
        pVarValue[i] = pStart[i];
        i++;
        if ( i >= 255 )
            return SRV_ERR_CONF;
    }
    pVarValue[i] = '\0';
    
    switch ( ptConfigInfo->iVarType ){
    case MS_VARTYPE_STRING:
        memset( ptConfigInfo->pVarAddr, 0, ptConfigInfo->iVarSize );
        memcpy( ptConfigInfo->pVarAddr, pVarValue, ptConfigInfo->iVarSize-1 );
        break;
    case MS_VARTYPE_INT:
        i = atoi( pVarValue );
        if ( i > ptConfigInfo->iVarSize )
            return SRV_ERR_CONF;
        memcpy( ptConfigInfo->pVarAddr, &i, sizeof(int) );
        break;
    case MS_VARTYPE_UINT:
        iTemp = (unsigned int)atoi( pVarValue );
        if ( iTemp > ptConfigInfo->iVarSize )
            return SRV_ERR_CONF;
        memcpy( ptConfigInfo->pVarAddr, &iTemp, sizeof(unsigned int) );
        break;
    }
    
    return 0;
}
