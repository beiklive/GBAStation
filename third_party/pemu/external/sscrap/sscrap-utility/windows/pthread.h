//
// Created by cpasjuste on 10/12/2019.
//

#ifndef SSCRAP_PTHREAD_H
#define SSCRAP_PTHREAD_H

#include <windows.h>

#define pthread_t HANDLE
#define pthread_attr_t HANDLE
#define pthread_mutex_t HANDLE
#define pthread_mutexattr_t HANDLE
#define pthread_mutex_lock(x) WaitForSingleObject(x, INFINITE)
#define pthread_mutex_unlock(x) ReleaseMutex(x)

int pthread_create(pthread_t* thread, pthread_attr_t* attr, void* (*start_routine)(void*), void* arg);

int pthread_join(pthread_t th, void** thread_return);

int pthread_mutex_init(pthread_mutex_t* mutex, const pthread_mutexattr_t* mutexattr);

int pthread_mutex_destroy(pthread_mutex_t* mutex);

#endif //SSCRAP_PTHREAD_H
