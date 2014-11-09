ctrl
====

定时执行的后台服务

==================

ctrldefine.h: 基本变量，类型的定义

ctrl.cpp: 主进程， 初始化为后台进程，fork处理进程并保存各个进程的无名管道描述符，通过select读进程状态，让空闲进程执行任务，主要函数有 forkchildprocs, Run, selectIdleProc, initasdeamo, Doit

ctrlproc.cpp: 执行任务的进程，死循环 {readcmd，执行，writerst }

