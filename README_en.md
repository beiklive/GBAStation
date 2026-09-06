# GBAStation

![logo](./resources/icon/default.png)

GBAStation is a multi-core emulator frontend for Nintendo Switch. The main application handles the game library, file detection, configuration management, button mapping, resource updates, and chain-loading. It no longer relies on dynamically loaded libretro cores.

The current architecture uses a split model: small cores are built into the main application, while larger cores are shipped as standalone NRO files. FC, SFC, MD, GBA, GB, GBC, and similar cores are integrated directly into the main application. NDS, 3DS, arcade, DC, PSP, and other larger cores run independently as `GBAStation*Stub.nro`, then return to `sdmc:/switch/GBAStation.nro` after the core exits.

This split reduces startup overhead for lightweight platforms while preserving the glslp rendering chain. It also allows large cores such as 3DS, DC, and PSP to keep their own build systems, dependencies, and rendering backends.

## Supported Platforms

| Platform | Runtime |
|----------|---------|
| GB / GBC / GBA | Built-in core |
| FC | Built-in core |
| SFC | Built-in core |
| MD | Built-in core |
| NDS | Standalone `GBAStationNDS.nro` project |
| 3DS | `GBAStation3DSStub.nro` |
| Arcade | `GBAStationFBNeoStub.nro` |
| Dreamcast | `GBAStationFlycastStub.nro` |
| PSP | `GBAStationPPSSPPStub.nro` |

The main application keeps the logic for opening files, detecting platforms, importing games into the library, selecting cores, and preparing launch arguments. Menus, runtime settings, button mappings, and runtime features for standalone cores are implemented by their corresponding Stub applications.

## Key Features

| Feature | Description |
|---------|-------------|
| Game library management | Automatic scanning, recently played games, favorites, search, category filters, pinyin sorting, and batch deletion |
| File detection | Detects games for each platform based on file extensions and platform rules |
| Chain loading | Launches standalone core NRO files from the main application and returns to the main application after the core exits |
| External core configuration | Configure each platform's core path, return path, core options, and button mappings from the settings page |
| GameDB updates | Supports game database updates, cover maintenance, and metadata maintenance |
| Web management | Upload ROMs, import save files, edit covers, and manage the game library over the local network |
| Button mapping | Per-platform configuration with support for single buttons, button combinations, and multiple mapping groups |
| Runtime features | Fast-forward, save states, load states, cheats, video settings, core settings, and more are provided by each core menu |
| glslp shaders | Built-in small cores support glslp rendering chains and shader parameters |
| Release updates | Release packages include both the main application and all core NRO files |

## SD Card Layout

After extracting a release package, keep the following structure:

```text
sdmc:/switch/GBAStation.nro
sdmc:/GBAStation/core/GBAStationNDS.nro
sdmc:/GBAStation/core/GBAStation3DSStub.nro
sdmc:/GBAStation/core/GBAStationFBNeoStub.nro
sdmc:/GBAStation/core/GBAStationFlycastStub.nro
sdmc:/GBAStation/core/GBAStationPPSSPPStub.nro
```

## Build

### Nintendo Switch

The devkitPro / devkitA64 environment and CMake 3.10 or later are required. On macOS, install CMake with `brew install cmake`:

```bash
cd BeikLiveStation
./switchbuild.sh
```

Local builds copy external cores from neighboring project directories by default:

```text
../GBAStation_fbneo/GBAStationFBNeoStub.nro
../GBAStation_flycast/GBAStationFlycastStub.nro
../GBAStation_ppsspp/GBAStationPPSSPPStub.nro
../GBAStation_3DS/GBAStation3DSStub.nro
```

Build the NDS core separately from the neighboring `GBAStation_melonds`
project, then place `build_switch/GBAStationNDS.nro` in `sdmc:/GBAStation/core/`.

Build artifacts are generated at:

```text
build_switch/GBAStation.nro
build_switch/GBAStation/core/GBAStation3DSStub.nro
build_switch/GBAStation/core/GBAStationFBNeoStub.nro
build_switch/GBAStation/core/GBAStationFlycastStub.nro
build_switch/GBAStation/core/GBAStationPPSSPPStub.nro
```

### Windows

The desktop version is used for frontend development and resource debugging. It cannot run external cores yet:

```bat
cd BeikLiveStation
windowsbuild.bat
```

## License

This project is released under the license declared in the [LICENSE](LICENSE) file. The main application, standalone cores, rendering backends, and third-party dependencies are each governed by their respective open-source licenses.

## Support the Author

If this project helps you, feel free to Star the project, submit Issues / PRs, or support development through the QR code below.

![pay](./assets/pay.png)



## Emulator Screenshots

![alt text](./assets/1.png)
![alt text](./assets/2.png)
![alt text](./assets/3.png)
![alt text](./assets/4.png)
