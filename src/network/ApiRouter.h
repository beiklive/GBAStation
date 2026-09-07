#pragma once

#include "mongoose.h"

#include <atomic>
#include <cstdint>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

namespace beiklive::network
{

class ApiRouter
{
public:
    explicit ApiRouter(std::atomic<bool>& stopRequested);

    void Handle(mg_connection* c, mg_http_message* hm);

private:
    // 一次“批量上传”（拖放/选目录）内按 (目标目录, 落盘后 stem 小写) 分组。
    // 组内同名文件（cue/bin/多轨）只导入一个代表到游戏库，其余仅保存。
    struct BatchGroupState
    {
        std::string key;            // targetDir + '\n' + 落盘 stem 小写
        std::string finalStem;      // 组最终 stem（含中文拼音清洗/截断 hash/冲突后缀，整组一致）
        std::string winnerName;     // 组代表（按扩展名优先级表选出）的原始文件名
        bool decided = false;
    };
    struct BatchState
    {
        std::string id;
        int remaining = 0;          // 尚未 finish/cancel 的成员数，归零即清理
        std::vector<std::string> names; // 全批原始文件名（清单）
        std::unordered_map<std::string, BatchGroupState> groups;
    };

    struct UploadSession
    {
        std::string token;
        std::string kind;
        std::string gameId;
        std::string originalName;
        std::string title;
        std::string originalStem;
        std::string targetPath;
        std::string finalPath;
        int platform = 0;
        std::uint64_t totalSize = 0;
        bool importNameMapping = false;
        bool renamedFromChinese = false;
        bool platformExplicit = false; // 客户端显式指定机种时跳过 detect 二次判定
        bool importToDb = true;        // false = 仅保存文件不入游戏库（组内非代表成员）
        std::string batchId;           // 批量上传批次（为空 = 单文件）
        std::string batchGroupKey;     // 本会话所属组 key
    };

    std::atomic<bool>& stopRequested_;
    std::mutex uploadMutex_;
    std::unordered_map<std::string, UploadSession> uploads_;
    std::unordered_map<std::string, BatchState> batches_;
    std::uint64_t nextToken_ = 1;

    void handleApi(mg_connection* c, mg_http_message* hm, const std::string& method, const std::string& uri);
    void handleGames(mg_connection* c);
    void handleGameById(mg_connection* c, mg_http_message* hm, const std::string& method, const std::string& uri);

    // 批量上传组成员规划（须已持有 uploadMutex_）：决定组代表与整组一致的最终 stem。
    // 返回该成员最终使用的落盘 stem；importToDb=false 表示组内非代表（仅保存文件）。
    std::string planBatchMemberLocked(const std::string& batchId,
                                      int platform,
                                      const std::string& originalName,
                                      const std::string& policyStem,
                                      const std::string& targetDir,
                                      const std::vector<std::string>& batchFiles,
                                      bool& importToDb,
                                      std::string& groupKey);

    void releaseBatchMemberLocked(const std::string& batchId); // 归零后清理批状态

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
    void handleAlbum(mg_connection* c, mg_http_message* hm, const std::string& method, const std::string& uri);
    void handleLogoFile(mg_connection* c, mg_http_message* hm);
    void handleCoverFile(mg_connection* c, mg_http_message* hm, const std::string& gameId);
    void handleSystem(mg_connection* c, mg_http_message* hm, const std::string& method, const std::string& uri);
    void handleFiles(mg_connection* c, mg_http_message* hm, const std::string& method, const std::string& uri);
    void serveStatic(mg_connection* c, mg_http_message* hm);

    std::string makeToken();
    bool writeChunk(const UploadSession& session, mg_http_message* hm, std::uint64_t offset, std::string& error);
};

} // namespace beiklive::network
