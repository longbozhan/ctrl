
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <cstdio>
#include <string>
#include <iostream>
#include <fstream>
#include <sys/time.h>
#include <syslog.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>

#include "ctrlproc.h"

using namespace std;

CtrlProc::CtrlProc(int fdRead, int fdWrite)
{

	m_fdRead = fdRead;
	m_fdWrite = fdWrite;
}

CtrlProc::~CtrlProc()
{

}


void CtrlProc::DoIt(void)
{
	int iRet = 0, iLoop = 1;
	S_PROC_CMD stCmd;

	while (iLoop)
	{
		try 
		{
			iRet = ReadCmd(&stCmd);

			if (iRet != 0) continue;

			switch (stCmd.iCmd)
			{
				case CMD_SELECT:
					iRet = Select( stCmd.iID );
					break;
				default:
					iRet = -1;
					break;
			}
		}
		catch(...)
		{
			perror("err1\n");
		}
		
		if (iRet != 0)
		{
			perror("err2\n");
			iRet = RET_ERROR;
		}
		WriteRst(iRet);
	}
}

int CtrlProc::Select(unsigned int iID)
{
	printf("pid(%d) in select, ID:%d\n", getpid(),  iID);
	fflush(stdout);
	sleep(8);
	return 0;
}
	
int CtrlProc::WriteRst(int iCmd)
{
	if(write( m_fdWrite, &iCmd , sizeof(iCmd)) != sizeof(iCmd))
	{
		printf("CtrlProc::WriteRst.write Error: %s", strerror(errno));
		return -1;
	}

	return 0;
}

int CtrlProc::ReadCmd(S_PROC_CMD *pstCmd)
{
	int iRet = read( m_fdRead, pstCmd, sizeof(S_PROC_CMD));
	if (iRet != sizeof(S_PROC_CMD))
	{
		if (iRet == 0)
		{
			printf("CtrlProc::ReadCmd.read exit");
			exit(0);
		}
		printf("CtrlProc::ReadCmd.read Error: %s", strerror(errno));
		return -1;
	} 

	return 0;
}



