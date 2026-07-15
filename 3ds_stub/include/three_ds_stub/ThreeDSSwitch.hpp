#pragma once

// Azahar defines u128 as a two-word array while libnx defines it as a native
// integer, and Azahar's Service namespace collides with libnx's Service type.
// Rename only the libnx spellings so both APIs can coexist in a translation unit.
#define u128 libnx_u128
#define Service libnx_Service
#define Result libnx_Result
#include <switch.h>
#undef Result
#undef Service
#undef u128
