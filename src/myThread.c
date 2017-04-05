/*
 * myThread.c
 *  线程处理基本函数定义
 *  Created on: 2014-11-21
 *      Author: revolyw
 */

#include <stdio.h>
#include<pthread.h>
#include "myThread.h"

int StartThread(tThread* pThread, ThreadFunc pFunc, void *pParam) {
	if (pthread_create(pThread, NULL, (void*) pFunc, pParam) == 0)
		return 0;
	else
		return -1;
}
int JoinThread(tThread hThread) {
	if (pthread_join(hThread, NULL) != 0)
		return -1;
	else
		return 0;
}
int DetachThread(tThread hThread) {
	if (pthread_detach(hThread) != 0)
		return -1;
	else
		return 0;
}
void InitializeMutex(tMutex* pMutex) {
	if (pMutex) {
		pthread_mutex_init(pMutex, NULL);
	}
}
int DestroyMutex(tMutex* pMutex) {
	pthread_mutex_destroy(pMutex);
	return 0;
}
int LockMutex(tMutex* pMutex) {
	if (pthread_mutex_lock(pMutex) != 0)
		return -1;
	else
		return 0;
}
int UnlockMutex(tMutex* pMutex) {
	if (pthread_mutex_lock(pMutex) != 0)
		return -1;
	else
		return 0;
}
