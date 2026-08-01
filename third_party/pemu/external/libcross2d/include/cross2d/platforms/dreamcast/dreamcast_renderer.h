//
// Created by cpasjuste on 21/11/16.
//

#ifndef DREAMCAST_RENDERER_H
#define DREAMCAST_RENDERER_H

#include "cross2d/platforms/gl1/gl_renderer.h"

namespace c2d {

    class DCRenderer : public GLRenderer {

    public:

        class Device {
        public:
            enum class Type {
                Sd = 0,
                Hdd = 1
            };

            enum class Format {
                Ext2,
                Fat
            };

            enum class Flag {
                ReadOnly,
                ReadWrite
            };
            std::string path;
            Type type;
            Format format;
            Flag flag;
            bool inited = false;
            bool mounted = false;
        };

        explicit DCRenderer(const Vector2f &size = Vector2f(0, 0));

        ~DCRenderer() override;

        void flip(bool draw = true, bool inputs = true) override;

        void delay(unsigned int ms) override;

        bool mount(Device::Type type, Device::Flag flag);

        void unmount(Device::Type device);

        Device devices[2];
    };
}

#endif // DREAMCAST_RENDERER_H
