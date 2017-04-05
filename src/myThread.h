/*
 * myThread.h
 *  线程处理基本函数声明
 *  Created on: 2014-11-21
 *      Author: revolyw
 */

#ifndef MYTHREAD_H_
#define MYTHREAD_H_

typedef pthread_t tThread;
typedef void* (*ThreadFunc) (void*);
typedef pthread_mutex_t tMutex;

//启动线程
int StartThread(tThread* pThread, ThreadFunc pFunc, void *pParam);
//阻塞线程
int JoinThread(tThread hThread);
//释放线程占用资源
int DetachThread(tThread hThread);
//初始化互斥变量
void InitializeMutex(tMutex* pMutex);
//销毁互斥变量
int DestroyMutex(tMutex* pMutex);
//互斥变量加锁
int LockMutex(tMutex* pMutex);
//互斥变量解锁
int UnlockMutex(tMutex* pMutex);

#endif /* MYTHREAD_H_ */
