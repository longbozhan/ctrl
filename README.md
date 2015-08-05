ctrl
====

定时执行任务的后台程序框架

功能:
1. 一个控制进程，多个工作进程，可以同时执行不同的任务
2. 系统性能可控，进程数量可伸缩
3. 选择空闲进程执行任务，保证负载均衡

用法:
1. 在CCtrl::Run中添加需要执行的任务的时间和任务ID，本例子中是Select(CMD_SELECT)
2. 在CtrlProc::DoIt中添加具体的任务,本例子用的是Select( stCmd.iID )

代码说明:
ctrldefine.h: 基本变量，类型的定义

ctrl.cpp: 主进程， 初始化为后台进程，fork处理进程并保存各个进程的无名管道描述符，通过select读进程状态，让空闲进程执行任务，主要函数有 forkchildprocs, Run, selectIdleProc, initasdeamo, Doit

ctrlproc.cpp: 执行任务的进程，死循环 {readcmd，执行，writerst }

=====

a task scheduler framework written by C++

System function:
1. a master controller, many worker, run diffrent task at the same time
2. the number of progress is in a given scope, when the number of progress is not enough, it forks auto
3. select idle progress to run job to ensure loading balance.

usage:
1. add when and what will do the job in CCtrl::Run, the case is Select(CMD_SELECT)
2. add specific job in CtrlProc::DoIt, the case is Select( stCmd.iID )

something infomation about the code:
ctrldefine.h: basic variable, type declared
ctrl.cpp: master controller, dispatch new job to child worker through pipe
ctrlproc.cpp: child worker, loop...
