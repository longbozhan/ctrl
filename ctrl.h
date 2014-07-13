#ifndef _H__CTRL_H_
#define _H__CTRL_H_

#include <map>
#include <sys/types.h>
#include <unistd.h>

#include "ctrlproc.h"

using namespace std;

enum procs_state {procsstate_idle = 0, procsstate_busy};

struct ChildProcsInfo
{
	int fd1[2];
	int fd2[2];
	unsigned int iID;
	procs_state procsstate;
};

typedef map<pid_t, ChildProcsInfo> ProcsMap;

class CCtrl
{ 
	public:
		ProcsMap m_mapProcsList;

	public:
		CCtrl();
		~CCtrl();
		void Run();
		void DoIt(int iCmd, unsigned int iID);
		void SelectIdleProc();

		void Select( int iCmd );

	private:
		void ForkChildProcs(int childnum);
	//	MngCtrl      * m_poMngCtrl;
};
#endif
