#include "HttpServer.h"

#include <borealis.hpp>

namespace beiklive::network
{

HttpServer::HttpServer()
    : router_(stopRequested_)
{
}

HttpServer::~HttpServer()
{
    Stop();
}

bool HttpServer::Start(int port)
{
    if (running_.load())
        return true;

    mg_mgr_init(&mgr_);
    std::string url = "http://0.0.0.0:" + std::to_string(port);
    if (!mg_http_listen(&mgr_, url.c_str(), EventHandler, this))
    {
        mg_mgr_free(&mgr_);
        return false;
    }

    port_ = port;
    stopRequested_.store(false);
    running_.store(true);
    thread_ = std::thread(&HttpServer::runLoop, this);
    brls::Logger::info("HTTP server listening on {}", url);
    return true;
}

void HttpServer::Stop()
{
    if (!running_.exchange(false))
        return;

    stopRequested_.store(true);
    if (thread_.joinable())
        thread_.join();
    mg_mgr_free(&mgr_);
    brls::Logger::info("HTTP server stopped");
}

void HttpServer::Update()
{
    if (stopRequested_.load())
        Stop();
}

void HttpServer::runLoop()
{
    while (running_.load() && !stopRequested_.load())
        mg_mgr_poll(&mgr_, 5);

    running_.store(false);
}

void HttpServer::EventHandler(mg_connection* c, int ev, void* evData)
{
    if (ev != MG_EV_HTTP_MSG)
        return;

    auto* self = static_cast<HttpServer*>(c->fn_data);
    self->router_.Handle(c, static_cast<mg_http_message*>(evData));
}

} // namespace beiklive::network
