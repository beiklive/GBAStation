//
// Created by cpasjuste on 04/02/2021.
//

#include "cross2d/c2d.h"

using namespace c2d;

DCMutex::DCMutex() : Mutex() {
    mutex_init(&mutex, MUTEX_TYPE_DEFAULT);
}

bool DCMutex::lock() {

    if (mutex_lock(&mutex) == 0) {
        return true;
    }

    return false;
}

bool DCMutex::unlock() {

    if (mutex_unlock(&mutex) == 0) {
        return true;
    }

    return false;
}

DCMutex::~DCMutex() {
    mutex_destroy(&mutex);
}
