//
// Created by cpasjuste on 21/11/16.
//

#include <ext/concurrence.h>
#include "cross2d/c2d.h"

#if 0
#include "ds.h"
#include <kos/blockdev.h>
#include <ext2/fs_ext2.h>
#include <fat/fs_fat.h>
#endif

using namespace c2d;

#ifdef NDEBUG
KOS_INIT_FLAGS(INIT_DEFAULT | INIT_QUIET | INIT_NO_DCLOAD);
#else
KOS_INIT_FLAGS(INIT_DEFAULT);
#endif

#if __GNUC__ < 6
// OLD KOS (https://github.com/Cpasjuste/kos/tree/gcc-5.2.0)
extern "C" void pvr_set_zclip(float zc);
#endif

extern uint8 romdisk[];
KOS_INIT_ROMDISK(romdisk);

DCRenderer::DCRenderer(const Vector2f &s) : GLRenderer({640, 480}) {
    printf("DCRenderer\n");

#if 0
    fs_ext2_init();
    fs_fat_init();
    devices[0].inited = sd_init() == 0;
    mount(Device::Type::Sd, Device::Flag::ReadOnly);
    //devices[1].inited = g1_ata_init() == 0;
    //mount(Device::Type::Hdd, Device::Flag::ReadOnly);
#endif

#if __GNUC__ < 6
    // OLD KOS (https://github.com/Cpasjuste/kos/tree/gcc-5.2.0)
        pvr_set_zclip(0);
#endif

    glKosInit();
    GLRenderer::initGL();

    available = true;

    printf("DCRenderer(GL)(%p): %ix%i\n", this, (int) Renderer::getSize().x, (int) Renderer::getSize().y);
}

void DCRenderer::flip(bool draw, bool inputs) {
    // call base class (draw childs)
    GLRenderer::flip(draw, inputs);

    if (draw) {
        glKosSwapBuffers();
    }
}

void DCRenderer::delay(unsigned int ms) {
    thd_sleep((int) ms);
}

bool DCRenderer::mount(Device::Type type, Device::Flag flag) {
#if 0
    kos_blockdev_t deviceBlock;
    uint8 deviceType;
    Device *device = type == Device::Type::Sd ? &devices[0] : &devices[1];

    if (!device->inited) {
        return false;
    }

    if (type == Device::Type::Sd) {
        if (sd_blockdev_for_partition(0, &deviceBlock, &deviceType) != 0) {
            return false;
        }
        device->path = "/sd";
        device->type = Device::Type::Sd;
        device->flag = flag;
    } else {
        if (g1_ata_blockdev_for_partition(0, 1, &deviceBlock, &deviceType) != 0) {
            if (g1_ata_blockdev_for_partition(0, 0, &deviceBlock, &deviceType) != 0) {
                return false;
            }
        }
        device->path = "/hdd";
        device->type = Device::Type::Hdd;
        device->flag = flag;
    }

    if (deviceType == 0x83) {
        if (fs_ext2_mount(device->path.c_str(), &deviceBlock,
                          flag == Device::Flag::ReadOnly ? FS_EXT2_MOUNT_READONLY : FS_EXT2_MOUNT_READWRITE) != 0) {
            return false;
        }
        device->format = Device::Format::Ext2;
    } else if (deviceType == 0x04 || deviceType == 0x06 || deviceType == 0x0B || deviceType == 0x0C) {
        if (fs_fat_mount(device->path.c_str(), &deviceBlock,
                         flag == Device::Flag::ReadOnly ? FS_FAT_MOUNT_READONLY : FS_FAT_MOUNT_READWRITE) != 0) {
            return false;
        }
        device->format = Device::Format::Fat;
    }

    device->mounted = true;
#endif

    return true;
}

void DCRenderer::unmount(Device::Type type) {
#if 0
    Device *device = type == Device::Type::Sd ? &devices[0] : &devices[1];
    if (device->inited && device->mounted) {
        if (device->format == Device::Format::Ext2) {
            fs_ext2_sync(device->path.c_str());
            fs_ext2_unmount(device->path.c_str());
        } else {
            fs_fat_sync(device->path.c_str());
            fs_fat_unmount(device->path.c_str());
        }
        device->mounted = false;
    }
#endif
}

DCRenderer::~DCRenderer() {
    printf("~DCRenderer()\n");

#if 0
    if (devices[1].inited) {
        unmount(Device::Type::Hdd);
        g1_ata_shutdown();
    }

    if (devices[0].inited) {
        unmount(Device::Type::Sd);
        sd_shutdown();
    }

    fs_fat_shutdown();
    fs_ext2_shutdown();
#endif
}

#if __GNUC__ < 6
// OLD KOS (https://github.com/Cpasjuste/kos/tree/gcc-5.2.0)
/// crap
namespace {
    __gnu_cxx::__mutex atomic_mutex;
} // anonymous namespace

_Atomic_word __attribute__ ((__unused__))
__gnu_cxx::__exchange_and_add(volatile _Atomic_word *mem, int val) throw() {
    //printf("__exchange_and_add\n");
    __gnu_cxx::__scoped_lock sentry(atomic_mutex);
    _Atomic_word __result;
    __result = *mem;
    *mem += val;
    return __result;
}

void __attribute__ ((__unused__))
__gnu_cxx::__atomic_add(volatile _Atomic_word *mem, int val) throw() {
    __exchange_and_add(mem, val);
}

#endif
