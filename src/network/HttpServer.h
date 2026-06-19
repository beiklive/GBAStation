#pragma once

#include "ApiRouter.h"
#include "mongoose.h"

#include <atomic>
#include <string>
#include <thread>

namespace beiklive::network
{

class HttpServer
{
public:
    HttpServer();
    ~HttpServer();

    bool Start(int port = 8080);
    void Stop();
    void Update();

    bool IsRunning() const { return running_.load(); }
    int Port() const { return port_; }

private:
    static void EventHandler(mg_connection* c, int ev, void* evData);
    void runLoop();

    mg_mgr mgr_{};
    std::atomic<bool> running_{false};
    std::atomic<bool> stopRequested_{false};
    std::thread thread_;
    ApiRouter router_;
    int port_ = 0;
};

} // namespace beiklive::network
