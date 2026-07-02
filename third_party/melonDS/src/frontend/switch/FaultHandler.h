#ifndef FAULTHANDLER_H
#define FAULTHANDLER_H

#include "../../types.h"

void HandleFault(u64 pc, u64 lr, u64 fp, u64 faultAddr, u32 desc);

#endif