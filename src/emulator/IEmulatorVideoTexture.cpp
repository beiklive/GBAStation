#include "emulator/IEmulatorVideoTexture.hpp"

namespace beiklive {

std::recursive_mutex& EmulatorGLMutex()
{
    static std::recursive_mutex mutex;
    return mutex;
}

} // namespace beiklive
