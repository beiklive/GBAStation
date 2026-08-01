//
// Created by cpasjuste on 19/09/23.
//

#include "main.h"

#include <cctype>
#include <cstring>
#include <string>

#ifdef __SWITCH__
#include <switch.h>
#endif

using namespace c2d;
using namespace pemu;

PEMUUiMain *pemu_ui;

#ifdef __SWITCH__
namespace {

std::string g_returnNroPath = "sdmc:/switch/GBAStation.nro";

bool endsWithNoCase(const std::string &value, const std::string &suffix) {
    if (suffix.size() > value.size()) {
        return false;
    }
    for (size_t i = 0; i < suffix.size(); ++i) {
        char a = static_cast<char>(std::tolower(static_cast<unsigned char>(
            value[value.size() - suffix.size() + i])));
        char b = static_cast<char>(std::tolower(static_cast<unsigned char>(suffix[i])));
        if (a != b) {
            return false;
        }
    }
    return true;
}

std::string quoteArg(const std::string &value) {
    std::string out;
    out.reserve(value.size() + 2);
    out.push_back('"');
    for (char c : value) {
        if (c == '"' || c == '\\') {
            out.push_back('\\');
        }
        out.push_back(c);
    }
    out.push_back('"');
    return out;
}

void returnToGbastation() {
    if (g_returnNroPath.empty() || !envHasNextLoad()) {
        return;
    }
    const std::string args = quoteArg(g_returnNroPath);
    const Result rc = envSetNextLoad(g_returnNroPath.c_str(), args.c_str());
    printf("pFBN: return to GBAStation rc=0x%x path=%s\n", rc, g_returnNroPath.c_str());
}

} // namespace
#endif

int main(int argc, char **argv) {
    // command line game info
    Game game;

    // custom io
    const auto io = new PEMUIo();

    // create main ui/renderer
    pemu_ui = new PEMUUiMain(Vector2f{1280, 720});
    pemu_ui->setIo(io);

    // load configuration
    constexpr int version = (__PEMU_VERSION_MAJOR__ * 100) + __PEMU_VERSION_MINOR__;
    const auto cfg = new PEMUConfig(pemu_ui, version);
    pemu_ui->setConfig(cfg);

    // load skin configuration
    const auto skin = new PEMUSkin(pemu_ui);
    pemu_ui->setSkin(skin);

    // parse command line
    std::string romArg;
    for (int i = 1; i < argc; ++i) {
        if (!argv[i]) {
            continue;
        }
#ifdef __SWITCH__
        if (std::strcmp(argv[i], "--return") == 0 && i + 1 < argc && argv[i + 1]) {
            g_returnNroPath = argv[i + 1];
            ++i;
            continue;
        }
        if (endsWithNoCase(argv[i], ".nro")) {
            continue;
        }
#endif
        if (romArg.empty() && argv[i][0] != '-') {
            romArg = argv[i];
        }
    }

    if (!romArg.empty()) {
        if (io->exist(romArg)) {
            game.path = Utility::baseName(romArg);
            game.name = Utility::removeExt(game.path);
            game.romsPath = Utility::remove(romArg, game.path);
        } else {
            printf("main: file provided as console argument does not exist (%s)\n", romArg.c_str());
            delete (skin);
            delete (cfg);
            delete (pemu_ui);
#ifdef __SWITCH__
            returnToGbastation();
#endif
            return 1;
        }
    }

    // ui
    const auto romList = new PEMURomList(pemu_ui, cfg->getCoreVersion(), cfg->getCoreSupportedExt());
    if (game.path.empty()) {
        romList->build();
        romList->initFav();
    } else {
        delete (romList->rect);
    }
    const auto uiRomList = new PEMUUiRomList(pemu_ui, romList, pemu_ui->getSize());
    const auto uiMenu = new PEMUUiMenu(pemu_ui);
    const auto uiEmu = new PEMUUiEmu(pemu_ui);
    const auto uiState = new PEMUUiMenuState(pemu_ui);
    pemu_ui->init(uiRomList, uiMenu, uiEmu, uiState);

    // load specified game from command line if requested
    if (!game.path.empty()) {
        uiRomList->setVisibility(Visibility::Hidden);
        uiRomList->setGames({game});
        cfg->loadGame(game);
        uiEmu->setExitOnStop(true);
        uiEmu->load(game);
    }

    while (!pemu_ui->done) {
        pemu_ui->flip();
    }

    delete (skin);
    delete (cfg);
    delete (pemu_ui);

#ifdef __SWITCH__
    returnToGbastation();
#endif

#ifdef  __PS4__
    sceSystemServiceLoadExec((char *) "exit", nullptr);
    while (true) {}
#endif

    return 0;
}
