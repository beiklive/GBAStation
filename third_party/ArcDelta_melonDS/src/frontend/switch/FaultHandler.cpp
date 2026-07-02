#include <switch.h>
#include <stdio.h>

extern char __start__;
extern char __rodata_start;

void HandleFault(u64 pc, u64 lr, u64 fp, u64 faultAddr, u32 desc)
{
    if (pc >= (u64)&__start__ && pc < (u64)&__rodata_start)
    {
        printf("unintentional fault in .text at %p (type %d) (trying to access %p?)\n", 
            pc - (u64)&__start__, desc, faultAddr);
        
        int frameNum = 0;
        while (true)
        {
            printf("stack frame %d %p\n", frameNum, lr - (u64)&__start__);
            lr = *(u64*)(fp + 8);
            fp = *(u64*)fp;

            frameNum++;
            if (frameNum > 16 || fp == 0 || (fp & 0x7) != 0)
                break;
        }
    }
    else
    {
        printf("unintentional fault somewhere in deep (address) space at %p (type %d)\n", pc, desc);
    }
}