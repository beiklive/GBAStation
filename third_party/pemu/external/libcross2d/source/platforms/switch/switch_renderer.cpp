//
// Created by cpasjuste on 26/11/18.
//

#include "cross2d/c2d.h"

using namespace c2d;

SWITCHRenderer::SWITCHRenderer(const Vector2f &size) : SDL2Renderer(size) {
    printf("SWITCHRenderer(%ix%i)\n", (int) size.x, (int) size.y);
#ifdef NDEBUG
    if (hosversionBefore(8, 0, 0)) {
        if (R_SUCCEEDED(pcvInitialize())) {
            SwitchSys::stock_cpu_clock = SwitchSys::getClock(SwitchSys::Module::Cpu);
            SwitchSys::stock_gpu_clock = SwitchSys::getClock(SwitchSys::Module::Gpu);
            SwitchSys::stock_emc_clock = SwitchSys::getClock(SwitchSys::Module::Emc);
        }
    } else {
        if (R_SUCCEEDED(clkrstInitialize())) {
            SwitchSys::stock_cpu_clock = SwitchSys::getClock(SwitchSys::Module::Cpu);
            SwitchSys::stock_gpu_clock = SwitchSys::getClock(SwitchSys::Module::Gpu);
            SwitchSys::stock_emc_clock = SwitchSys::getClock(SwitchSys::Module::Emc);
        }
    }

    printf("SWITCHRenderer(): clocks: cpu=%i, gpu=%i, emc=%i\n",
           SwitchSys::stock_cpu_clock, SwitchSys::stock_gpu_clock, SwitchSys::stock_emc_clock);
#endif
}

SWITCHRenderer::~SWITCHRenderer() {
    printf("~SWITCHRenderer()\n");
#ifdef NDEBUG
    printf("SWITCHRenderer(): previous clocks: cpu=%i, gpu=%i, emc=%i\n",
           SwitchSys::getClock(SwitchSys::Module::Cpu),
           SwitchSys::getClock(SwitchSys::Module::Gpu),
           SwitchSys::getClock(SwitchSys::Module::Emc));

    if (SwitchSys::getClock(SwitchSys::Module::Cpu) != SwitchSys::stock_cpu_clock) {
        if (SwitchSys::stock_cpu_clock > 0) {
            SwitchSys::setClock(SwitchSys::Module::Cpu, SwitchSys::stock_cpu_clock);
        }
    }

    if (SwitchSys::getClock(SwitchSys::Module::Gpu) != SwitchSys::stock_gpu_clock) {
        if (SwitchSys::stock_gpu_clock > 0) {
            SwitchSys::setClock(SwitchSys::Module::Gpu, SwitchSys::stock_gpu_clock);
        }
    }

    if (SwitchSys::getClock(SwitchSys::Module::Emc) != SwitchSys::stock_emc_clock) {
        if (SwitchSys::stock_emc_clock > 0) {
            SwitchSys::setClock(SwitchSys::Module::Emc, SwitchSys::stock_emc_clock);
        }
    }

    printf("SWITCHRenderer(): restored clocks: cpu=%i, gpu=%i, emc=%i\n",
           SwitchSys::getClock(SwitchSys::Module::Cpu),
           SwitchSys::getClock(SwitchSys::Module::Gpu),
           SwitchSys::getClock(SwitchSys::Module::Emc));

    if (hosversionBefore(8, 0, 0)) {
        pcvExit();
    } else {
        clkrstExit();
    }
#endif
}
