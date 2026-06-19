#pragma once

#include <cstddef>

namespace beiklive::network
{

class NetworkManager
{
public:
    bool Initialize();
    void Shutdown();

private:
    void* socketBuffer_ = nullptr;
    bool initialized_ = false;
};

} // namespace beiklive::network
