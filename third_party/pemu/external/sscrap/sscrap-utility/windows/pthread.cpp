//
// Created by cpasjuste on 10/12/2019.
//

#include <process.h>
#include "pthread.h"

int pthread_create(pthread_t* thread, pthread_attr_t* attr, void* (*start_routine)(void*), void* arg) {
	
	*thread = (HANDLE) _beginthreadex(NULL, 0, (_beginthreadex_proc_type) start_routine, arg, 0, NULL);

	return 0;
}

int pthread_join(pthread_t th, void** thread_return) {

	WaitForSingleObject(th, INFINITE);

	return 0;
}

int pthread_mutex_init(pthread_mutex_t* mutex, const pthread_mutexattr_t* mutexattr) {

	*mutex = CreateMutex(NULL, FALSE, NULL);

	return 0;
}

int pthread_mutex_destroy(pthread_mutex_t* mutex) {

	CloseHandle(*mutex);

	return 0;
}
