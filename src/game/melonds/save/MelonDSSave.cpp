#include "save/MelonDSSave.hpp"
#include "MelonDSInstance.hpp"

#include <fstream>
#include <borealis.hpp>

namespace beiklive::melonds
{

MelonDSSave::MelonDSSave(MelonDSInstance& instance)
    : m_instance(instance)
{
}

void MelonDSSave::setSavePath(const std::string& path, const std::string& romName)
{
    (void)romName;
    m_savePath = path;
}

void MelonDSSave::setFirmwarePath(const std::string& path)
{
    m_firmwarePath = path;
}

bool MelonDSSave::saveState(const std::string& statePath)
{
    if (!m_instance.IsInitialized())
        return false;

    return m_instance.DoSavestate(statePath, true);
}

bool MelonDSSave::loadState(const std::string& statePath)
{
    if (!m_instance.IsInitialized())
        return false;

    return m_instance.DoSavestate(statePath, false);
}

bool MelonDSSave::readSaveData(std::vector<uint8_t>& outData)
{
    if (m_savePath.empty())
        return false;

    std::ifstream f(m_savePath, std::ios::binary | std::ios::ate);
    if (!f)
        return false;

    size_t size = static_cast<size_t>(f.tellg());
    f.seekg(0, std::ios::beg);

    outData.resize(size);
    f.read(reinterpret_cast<char*>(outData.data()), size);
    return true;
}

bool MelonDSSave::writeSaveData(const std::vector<uint8_t>& data)
{
    if (m_savePath.empty() || data.empty())
        return false;

    std::ofstream f(m_savePath, std::ios::binary);
    if (!f)
        return false;

    f.write(reinterpret_cast<const char*>(data.data()), data.size());
    return true;
}

} // namespace beiklive::melonds
