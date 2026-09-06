#pragma once

#include <string>
#include <string_view>

namespace beiklive::ps1_meta
{

struct DiscInfo
{
    std::string serial;
};

/// Extracts the normalized PlayStation serial from SYSTEM.CNF/BOOT.
/// Returns an empty string when the image format or metadata is unsupported.
std::string ExtractSerial(const std::string& path);

/// Normalizes a DuckStation-style BOOT path such as cdrom:\\SCES_123.45;1.
std::string NormalizeSerialFromBootPath(std::string_view bootPath);

} // namespace beiklive::ps1_meta
