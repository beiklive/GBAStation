//
// Created by cpasjuste on 04/02/2021.
//

#include "cross2d/c2d.h"


using namespace c2d;

PS3Mutex::PS3Mutex() : Mutex() {
    sys_mutex_attr_t mutex_attr;
    if (sysMutexCreate(&mutex, &mutex_attr)) {
        printf("PS3Mutex: could not create mutex\n");
    }

    available = true;
}

bool PS3Mutex::lock() {
    if (!available) {
        printf("PS3Mutex::lock: could not lock mutex (available = false)\n");
        return false;
    }

    if (sysMutexLock(mutex, 0)) {
        printf("PS3Mutex::lock: could not lock mutex (sysMutexLock failed)\n");
        return false;
    }

    return true;
}

bool PS3Mutex::unlock() {
    if (!available) {
        printf("PS3Mutex::unlock: could not unlock mutex (available = false)\n");
        return false;
    }

    if (sysMutexUnlock(mutex)) {
        printf("PS3Mutex::unlock: could not unlock mutex (sysMutexUnlock failed)\n");
        return false;
    }

    return true;
}

PS3Mutex::~PS3Mutex() {
    if (available) {
        sysMutexDestroy(mutex);
    }
}
