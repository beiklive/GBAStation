#pragma once

#include <string>
#include <vector>
#include <cstdint>

namespace beiklive::melonds
{

class MelonDSInstance;

class MelonDSSave
{
public:
    explicit MelonDSSave(MelonDSInstance& instance);

    void setSavePath(const std::string& path, const std::string& romName);
    void setFirmwarePath(const std::string& path);

    bool saveState(const std::string& statePath);
    bool loadState(const std::string& statePath);

    bool readSaveData(std::vector<uint8_t>& outData);
    bool writeSaveData(const std::vector<uint8_t>& data);

private:
    MelonDSInstance& m_instance;
    std::string m_savePath;
    std::string m_firmwarePath;
};

} // namespace beiklive::melonds
