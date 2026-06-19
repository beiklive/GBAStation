#include "NetworkManager.h"

#ifdef __SWITCH__
#include <malloc.h>
#include <switch.h>
#endif

namespace beiklive::network
{

namespace
{
#ifdef __SWITCH__
constexpr std::size_t SOCKET_BUFFER_SIZE = 0x100000;
#endif
}

bool NetworkManager::Initialize()
{
#ifdef __SWITCH__
    if (initialized_)
        return true;

    socketBuffer_ = memalign(0x1000, SOCKET_BUFFER_SIZE);
    if (!socketBuffer_)
        return false;

    Result rc = socketInitializeDefault();
    if (R_FAILED(rc))
    {
        // Some platforms/libraries may have already initialized sockets.
        // In that case keep the buffer unused and let listen() report a real failure if sockets are unavailable.
        free(socketBuffer_);
        socketBuffer_ = nullptr;
        return true;
    }
    initialized_ = true;
#endif
    return true;
}

void NetworkManager::Shutdown()
{
#ifdef __SWITCH__
    if (initialized_)
    {
        socketExit();
        initialized_ = false;
    }

    if (socketBuffer_)
    {
        free(socketBuffer_);
        socketBuffer_ = nullptr;
    }
#endif
}

} // namespace beiklive::network
