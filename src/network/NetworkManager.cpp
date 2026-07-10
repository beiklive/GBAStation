#include "NetworkManager.h"

#ifdef __SWITCH__
#include <malloc.h>
#include <switch.h>
#endif

#include <mutex>

namespace beiklive::network
{

namespace
{
#ifdef __SWITCH__
constexpr std::size_t SOCKET_BUFFER_SIZE = 0x100000;
std::mutex g_socketMutex;
void* g_socketBuffer = nullptr;
int g_socketRefCount = 0;
bool g_socketOwned = false;
#endif
}

bool NetworkManager::Initialize()
{
#ifdef __SWITCH__
    if (initialized_)
        return true;

    std::lock_guard<std::mutex> lock(g_socketMutex);
    if (g_socketRefCount == 0)
    {
        g_socketBuffer = memalign(0x1000, SOCKET_BUFFER_SIZE);
        if (!g_socketBuffer)
            return false;

        Result rc = socketInitializeDefault();
        if (R_FAILED(rc))
        {
            // Some platforms/libraries may have already initialized sockets.
            // In that case keep our reference alive without owning socketExit().
            free(g_socketBuffer);
            g_socketBuffer = nullptr;
            g_socketOwned = false;
        }
        else
        {
            g_socketOwned = true;
        }
    }

    ++g_socketRefCount;
    initialized_ = true;
#endif
    return true;
}

void NetworkManager::Shutdown()
{
#ifdef __SWITCH__
    if (!initialized_)
        return;

    std::lock_guard<std::mutex> lock(g_socketMutex);
    initialized_ = false;
    if (g_socketRefCount > 0)
        --g_socketRefCount;

    if (g_socketRefCount == 0)
    {
        if (g_socketOwned)
            socketExit();
        g_socketOwned = false;

        if (g_socketBuffer)
        {
            free(g_socketBuffer);
            g_socketBuffer = nullptr;
        }
    }
#endif
}

} // namespace beiklive::network
