#pragma once

#include <cstdint>
#include <cstddef>
#include <string>
#include <vector>

namespace beiklive::melonds {

struct PlatformUserData {
    std::string savePath;
    std::string firmwarePath;
};

bool LoadBinaryFile(const std::string& path, void* data, size_t size);
bool LoadBinaryVector(const std::string& path, std::vector<uint8_t>& out);
bool WriteBinaryFile(const std::string& path, const void* data, size_t size);
void AsyncWriteBinaryFile(std::string path, std::vector<uint8_t> data);
void FlushAsyncBinaryWrites();
uint64_t GetTimeUs();
void SleepNs(uint64_t ns);

} // namespace beiklive::melonds
