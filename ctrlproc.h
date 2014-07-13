#ifndef _H_CTRL_PROC_H_
#define _H_CTRL_PROC_H_

#define RET_OK             0
#define RET_ERROR          -1

#include "ctrldefine.h"

struct S_PROC_CMD
{
	int iCmd;
	unsigned int iID;
};

class CtrlProc
{
public:
	CtrlProc(int fdRead, int fdWrite);
	~CtrlProc();

	void DoIt();

private:
	int WriteRst(int iCmd);
	int ReadCmd(S_PROC_CMD *pstCmd);

	int Select(unsigned int iID);

private:

	int m_fdRead;
	int m_fdWrite;
	
};

#endif
