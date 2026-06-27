/*
    Copyright 2019-2025 Hydr8gon

    This file is part of NooDS.

    NooDS is free software: you can redistribute it and/or modify it
    under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    NooDS is distributed in the hope that it will be useful, but
    WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
    General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with NooDS. If not, see <https://www.gnu.org/licenses/>.
*/

#include "../core.h"

uint32_t framebuffer[256 * 192 * 8];
uint32_t *fbCur = framebuffer;
uint32_t *fbEnd = framebuffer + 256 * 192 * 2;

class CoreWrap {
public:
    CoreWrap(const char *path);
    int getFps() const { return core->fps; }

private:
    Core *core;
    std::thread *thread;
};

void runCore(Core *core) {
    // Run the emulator and update the framebuffer
    while (core) {
        core->runCore();
        core->gpu.getFrame(framebuffer, false);
    }
}

CoreWrap::CoreWrap(const char *path) {
    // Initialize the core with a NDS or GBA ROM based on file extension
    std::string path2 = path;
    if (path2.find(".nds", path2.length() - 4) != std::string::npos)
        core = new Core(path2, "");
    else if (path2.find(".gba", path2.length() - 4) != std::string::npos)
        core = new Core("", path2);
    else
        core = new Core();
    thread = new std::thread(&runCore, core);
}

bool loadSettings(const char *path) {
    // Load settings and update path prefixes in case the app UUID changed
    std::string path2 = path;
    if (!Settings::load(path2)) return false;
    strncpy(&Settings::bios9Path[0], path, path2.size() - 1);
    strncpy(&Settings::bios7Path[0], path, path2.size() - 1);
    strncpy(&Settings::firmwarePath[0], path, path2.size() - 1);
    strncpy(&Settings::gbaBiosPath[0], path, path2.size() - 1);
    strncpy(&Settings::sdImagePath[0], path, path2.size() - 1);
    return true;
}

int bytesBridge(void *info, void *buffer, int count) {
    // Copy framebuffer bytes to an image and move the pointer
    int left = uintptr_t(fbEnd) - uintptr_t(fbCur);
    count = std::max(0, std::min(left, count));
    memcpy(buffer, fbCur, count);
    fbCur = (uint32_t*)((uint8_t*)fbCur + count);
    return count;
}

int forwardBridge(void *info, int count) {
    // Move the framebuffer pointer forward
    int left = uintptr_t(fbEnd) - uintptr_t(fbCur);
    count = std::max(0, std::min(left, count));
    fbCur = (uint32_t*)((uint8_t*)fbCur + count);
    return count;
}

void rewindBridge(void *info) {
    // Reset the framebuffer pointer
    fbCur = framebuffer;
}
