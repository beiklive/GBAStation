#pragma once

#include <memory>
#include <string>

namespace beiklive::three_ds_stub {

class ThreeDSRuntime {
public:
    ThreeDSRuntime();
    ~ThreeDSRuntime();

    bool Init();
    bool LoadGame(const std::string& path);
    bool RunFrame();
    void Reset();
    void Shutdown();

    [[nodiscard]] bool ExitRequested() const;
    [[nodiscard]] const std::string& LastError() const;

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace beiklive::three_ds_stub

