#include <stdint.h>
#include <stdlib.h>
#include <memmap.h>

#ifndef PROT_READ
#define PROT_READ         0x1
#endif

#ifndef PROT_WRITE
#define PROT_WRITE        0x2
#endif

#ifndef PROT_READWRITE
#define PROT_READWRITE    0x3
#endif

#ifndef PROT_EXEC
#define PROT_EXEC         0x4
#endif

#ifndef MAP_FAILED
#define MAP_FAILED        ((void *) -1)
#endif

#ifndef MAP_ANONYMOUS
#define MAP_ANONYMOUS     0x20
#endif

#ifndef MAP_PRIVATE
#define MAP_PRIVATE       0x02
#endif

#ifdef _WIN32
#include <windows.h>

void* mmap(void *addr, size_t len, int prot, int flags,
      int fildes, size_t offset)
{
   void *map = NULL;

   if (fildes == -1 || (flags & MAP_ANONYMOUS)) {
      DWORD flProtect = PAGE_NOACCESS;
      if (prot & PROT_READWRITE) flProtect = PAGE_READWRITE;
      else if (prot & PROT_READ) {
         if (prot & PROT_WRITE) flProtect = PAGE_READWRITE;
         else if (prot & PROT_EXEC) flProtect = PAGE_EXECUTE_READ;
         else flProtect = PAGE_READONLY;
      } else if (prot & PROT_WRITE) flProtect = PAGE_READWRITE;
      if (prot & PROT_EXEC) flProtect = PAGE_EXECUTE_READWRITE;

      map = VirtualAlloc(addr, len, MEM_RESERVE | MEM_COMMIT, flProtect);
      if (!map) return MAP_FAILED;
   } else {
      HANDLE handle = CreateFileMapping(
         (HANDLE)_get_osfhandle(fildes), NULL,
         (prot & PROT_WRITE) ? PAGE_READWRITE : PAGE_READONLY,
         0, (DWORD)(len + offset), NULL);
      if (!handle) return MAP_FAILED;

      DWORD access = (prot & PROT_WRITE) ? FILE_MAP_ALL_ACCESS : FILE_MAP_READ;
      map = MapViewOfFile(handle, access, 0, (DWORD)offset, len);
      CloseHandle(handle);
      if (!map) return MAP_FAILED;
   }
   return map;
}

int munmap(void *addr, size_t length)
{
   (void)length;
   if (!VirtualFree(addr, 0, MEM_RELEASE)) {
      if (!UnmapViewOfFile(addr))
         return -1;
   }
   return 0;
}

int mprotect(void *addr, size_t len, int prot)
{
   DWORD flNewProtect = PAGE_READWRITE;
   if (prot & PROT_EXEC) flNewProtect = PAGE_EXECUTE_READWRITE;
   return VirtualProtect(addr, len, flNewProtect, NULL) ? 0 : -1;
}

#elif !defined(HAVE_MMAN)
void* mmap(void *addr, size_t len, int prot, int flags,
      int fildes, size_t offset)
{
   return malloc(len);
}

int munmap(void *addr, size_t len)
{
   free(addr);
   return 0;
}

int mprotect(void *addr, size_t len, int prot)
{
   return 0;
}

#endif

#if defined(__MACH__) && defined(__arm__)
#include <libkern/OSCacheControl.h>
#endif

int memsync(void *start, void *end)
{
   size_t len = (char*)end - (char*)start;
#if defined(__MACH__) && defined(__arm__)
   sys_dcache_flush(start ,len);
   sys_icache_invalidate(start, len);
   return 0;
#elif defined(__arm__) && !defined(__QNX__)
   (void)len;
   __clear_cache(start, end);
   return 0;
#elif defined(HAVE_MMAN)
   return msync(start, len, MS_SYNC | MS_INVALIDATE
#ifdef __QNX__
         MS_CACHE_ONLY
#endif
         );
#else
   (void)len;
   return 0;
#endif
}

int memprotect(void *addr, size_t len)
{
   return mprotect(addr, len, PROT_READ | PROT_WRITE | PROT_EXEC);
}
