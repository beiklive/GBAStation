#pragma once

#include "mongoose.h"

#include <atomic>
#include <cstdint>
#include <mutex>
#include <string>
#include <unordered_map>

namespace beiklive::network
{

class ApiRouter
{
public:
    explicit ApiRouter(std::atomic<bool>& stopRequested);

    void Handle(mg_connection* c, mg_http_message* hm);

private:
    struct UploadSession
    {
        std::string token;
        std::string kind;
        std::string gameId;
        std::string originalName;
        std::string title;
        std::string targetPath;
        std::string finalPath;
        int platform = 0;
        std::uint64_t totalSize = 0;
    };

    std::atomic<bool>& stopRequested_;
    std::mutex uploadMutex_;
    std::unordered_map<std::string, UploadSession> uploads_;
    std::uint64_t nextToken_ = 1;

    void handleApi(mg_connection* c, mg_http_message* hm, const std::string& method, const std::string& uri);
    void handleGames(mg_connection* c);
    void handleGameById(mg_connection* c, mg_http_message* hm, const std::string& method, const std::string& uri);
    void handleUploadStart(mg_connection* c, mg_http_message* hm);
    void handleUploadChunk(mg_connection* c, mg_http_message* hm);
    void handleUploadFinish(mg_connection* c, mg_http_message* hm);
    void handleUploadCancel(mg_connection* c, mg_http_message* hm);
    void handleSaveStart(mg_connection* c, mg_http_message* hm, const std::string& gameId);
    void handleSaveList(mg_connection* c, mg_http_message* hm, const std::string& gameId);
    void handleSaveDelete(mg_connection* c, mg_http_message* hm, const std::string& gameId);
    void handleSaveExport(mg_connection* c, mg_http_message* hm, const std::string& gameId);
    void handleCoverStart(mg_connection* c, mg_http_message* hm, const std::string& gameId);
    void handleCoverSelect(mg_connection* c, mg_http_message* hm, const std::string& gameId);
    void handleImages(mg_connection* c, mg_http_message* hm);
    void handleImageFile(mg_connection* c, mg_http_message* hm);
    void handleLogoFile(mg_connection* c, mg_http_message* hm);
    void handleCoverFile(mg_connection* c, mg_http_message* hm, const std::string& gameId);
    void handleSystem(mg_connection* c, mg_http_message* hm, const std::string& method, const std::string& uri);
    void handleFiles(mg_connection* c, mg_http_message* hm, const std::string& method, const std::string& uri);
    void serveStatic(mg_connection* c, mg_http_message* hm);

    std::string makeToken();
    bool writeChunk(const UploadSession& session, mg_http_message* hm, std::uint64_t offset, std::string& error);
};

} // namespace beiklive::network
