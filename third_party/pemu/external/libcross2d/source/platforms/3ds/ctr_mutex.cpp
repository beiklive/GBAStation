//
// Created by cpasjuste on 04/02/2021.
//

#include "cross2d/c2d.h"

using namespace c2d;

CTRMutex::CTRMutex() : Mutex() {
    svcCreateMutex(&handle, false);
}

bool CTRMutex::lock() {
    Result rc = svcWaitSynchronization(handle, U64_MAX);
    return R_SUCCEEDED(rc);
}

bool CTRMutex::unlock() {
    Result rc = svcReleaseMutex(handle);
    return R_SUCCEEDED(rc);
}

CTRMutex::~CTRMutex() {
    svcCloseHandle(handle);
}
