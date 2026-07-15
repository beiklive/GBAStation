#include <cctype>
#include <cstring>
#include <string>

#include <switch.h>

#include "three_ds_stub/ThreeDSLog.hpp"
#include "three_ds_stub/ThreeDSRuntime.hpp"

namespace {

bool EndsWithNoCase(const std::string& value, const char* suffix) {
    const std::size_t suffix_length = std::strlen(suffix);
    if (value.size() < suffix_length) {
        return false;
    }
    const std::size_t offset = value.size() - suffix_length;
    for (std::size_t i = 0; i < suffix_length; ++i) {
        if (std::tolower(static_cast<unsigned char>(value[offset + i])) !=
            std::tolower(static_cast<unsigned char>(suffix[i]))) {
            return false;
        }
    }
    return true;
}

void SetReturnNro(const std::string& path) {
    if (path.empty() || !envHasNextLoad()) {
        return;
    }
    std::string args = "\"";
    for (const char character : path) {
        if (character == '\\' || character == '"') {
            args.push_back('\\');
        }
        args.push_back(character);
    }
    args.push_back('"');
    const Result rc = envSetNextLoad(path.c_str(), args.c_str());
    beiklive::three_ds_stub::appendLog(
        "GBAStation3DSStub: envSetNextLoad rc=%#x path=%s", rc, path.c_str());
}

} // namespace

int main(int argc, char* argv[]) {
    using namespace beiklive::three_ds_stub;

    initializeLog();
    svcSetThreadCoreMask(CUR_THREAD_HANDLE, 1, 1ULL << 1);

    std::string rom_path;
    std::string return_nro = "sdmc:/switch/GBAStation.nro";
    bool return_to_main = true;

    for (int i = 1; i < argc; ++i) {
        if (!argv[i]) {
            continue;
        }
        if (std::strcmp(argv[i], "--return") == 0 && i + 1 < argc) {
            return_nro = argv[++i];
        } else if (std::strcmp(argv[i], "--exit-to-home") == 0) {
            return_to_main = false;
        } else if (rom_path.empty() && !EndsWithNoCase(argv[i], ".nro")) {
            rom_path = argv[i];
        }
    }

    appendLog("GBAStation3DSStub: start rom=%s return=%s", rom_path.c_str(),
              return_nro.c_str());

    ThreeDSRuntime runtime;
    int result = 0;
    if (!runtime.Init() || !runtime.LoadGame(rom_path)) {
        appendLog("GBAStation3DSStub: initialization failed detail=%s",
                  runtime.LastError().c_str());
        result = 1;
    } else {
        while (appletMainLoop() && runtime.RunFrame()) {
        }
    }

    runtime.Shutdown();
    if (return_to_main) {
        SetReturnNro(return_nro);
    }
    appendLog("GBAStation3DSStub: exit result=%d", result);
    return result;
}

