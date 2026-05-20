#pragma once

#include <string>
#include <functional>
#include <cstdint>

#include "types.h"

namespace beiklive::melonds::platform
{

void setNDSSavePath(const std::string& path);
void setGBASavePath(const std::string& path);
void setFirmwarePath(const std::string& path);
void setStopCallback(std::function<void()> cb);

void writeNDSSave(const uint8_t* data, uint32_t len, uint32_t offset, uint32_t writelen);
void writeGBASave(const uint8_t* data, uint32_t len, uint32_t offset, uint32_t writelen);

void writeDateTime(int year, int month, int day, int hour, int minute, int second);

void signalStop(int reason);

} // namespace beiklive::melonds::platform
