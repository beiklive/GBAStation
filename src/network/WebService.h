#pragma once

#include <string>

namespace beiklive::network::WebService
{

bool Start(int port = 8080);
void Stop();
void Update();
bool IsRunning();
int Port();
std::string Url();
std::string LastError();
std::string KeepAwakeMessage();

} // namespace beiklive::network::WebService
