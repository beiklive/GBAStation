#pragma once

#include <cstdio>
#include <string>

namespace beiklive::melonds::platform
{

FILE* openFile(const std::string& path, const std::string& mode);
FILE* openLocalFile(const std::string& path, const std::string& mode);
void closeFile(FILE* f);

bool fileExists(const std::string& path);
bool localFileExists(const std::string& path);

std::string getConfigPath();
std::string getBiosPath();

} // namespace beiklive::melonds::platform
