DEBUG                 = 0
PERF_TEST             = 0
LOAD_FROM_MEMORY      = 1
USE_BLARGG_APU       ?= 0
LAGFIX                = 1
USE_OLD_COLOUR_OPS    = 0

SPACE :=
SPACE := $(SPACE) $(SPACE)
BACKSLASH :=
BACKSLASH := \$(BACKSLASH)
filter_out1 = $(filter-out $(firstword $1),$1)
filter_out2 = $(call filter_out1,$(call filter_out1,$1))
unixpath = $(subst \,/,$1)
unixcygpath = /$(subst :,,$(call unixpath,$1))

ifeq ($(platform),)
	platform = unix
	ifeq ($(shell uname -s),)
		platform = win
	else ifneq ($(findstring MINGW,$(shell uname -s)),)
		platform = win
	else ifneq ($(findstring Darwin,$(shell uname -s)),)
		platform = osx
		arch = intel
		ifeq ($(shell uname -p),arm64)
			arch = arm
		endif
		ifeq ($(shell uname -p),powerpc)
			arch = ppc
		endif
	else ifneq ($(findstring win,$(shell uname -s)),)
		platform = win
	else ifneq ($(findstring SunOS,$(shell uname -s)),)
		platform = sun
	endif
endif

# system platform
system_platform = unix
ifeq ($(shell uname -a),)
	EXE_EXT = .exe
	system_platform = win
else ifneq ($(findstring Darwin,$(shell uname -a)),)
	system_platform = osx
	arch = intel
	ifeq ($(shell uname -p),powerpc)
		arch = ppc
	endif
else ifneq ($(findstring MINGW,$(shell uname -a)),)
	system_platform = win
endif

ifeq ($(USE_BLARGG_APU), 1)
	TARGET_NAME := snes9x2005_plus
else
	TARGET_NAME := snes9x2005
endif
GIT_VERSION := " $(shell git rev-parse --short HEAD || echo unknown)"
ifneq ($(GIT_VERSION)," unknown")
	CFLAGS += -DGIT_VERSION=\"$(GIT_VERSION)\"
endif

DEFS :=
ifneq (,$(findstring msvc,$(platform)))
LIBM :=
else
LIBM := -lm
endif
LIBS :=

ifeq ($(platform), unix)
	TARGET := $(TARGET_NAME)_libretro.so
	fpic := -fPIC
	SHARED := -shared -Wl,--no-undefined -Wl,--version-script=link.T
	CFLAGS += -fno-builtin -fno-exceptions -ffunction-sections
# ARM
else ifneq (,$(findstring armv,$(platform)))
       TARGET := $(TARGET_NAME)_libretro.so
       fpic := -fPIC
       SHARED := -shared -Wl,--no-undefined -Wl,--version-script=link.T
       CC = gcc
       CXX = g++
       PLATFORM_DEFINES += -marm
       ifneq (,$(findstring softfloat,$(platform)))
               PLATFORM_DEFINES += -mfloat-abi=softfp
       else ifneq (,$(findstring hardfloat,$(platform)))
               PLATFORM_DEFINES += -mfloat-abi=hard
       endif
       ifneq (,$(findstring neon,$(platform)))
               FLAGS += -mfpu=neon
               ASFLAGS += -mfpu=neon
               HAVE_NEON = 1
       endif
       PLATFORM_DEFINES += -DARM
else ifeq ($(platform), linux-portable)
	TARGET := $(TARGET_NAME)_libretro.so
	fpic := -fPIC -nostdlib
	SHARED := -shared -Wl,--version-script=link.T
	CFLAGS += -fno-builtin -fno-exceptions -ffunction-sections
	LIBM :=
else ifeq ($(platform),sun)
	TARGET := $(TARGET_NAME)_libretro.so
	fpic := -fPIC
	SHARED := -shared -z defs
	CFLAGS += -fno-builtin -fno-exceptions -ffunction-sections
	CC = gcc
else ifeq ($(platform), osx)
	TARGET := $(TARGET_NAME)_libretro.dylib
	fpic := -fPIC
	SHARED := -dynamiclib

ifeq ($(arch),ppc)
	FLAGS += -DMSB_FIRST
	OLD_GCC = 1
endif
	OSXVER = $(shell sw_vers -productVersion | cut -d. -f 2)
        OSX_LT_MAVERICKS = `(( $(OSXVER) <= 9)) && echo "YES"`
ifeq ($(OSX_LT_MAVERICKS),YES)
	#this breaks compiling on Mac OS Mojave
	fpic += -mmacosx-version-min=10.1
endif
   ifeq ($(CROSS_COMPILE),1)
		TARGET_RULE   = -target $(LIBRETRO_APPLE_PLATFORM) -isysroot $(LIBRETRO_APPLE_ISYSROOT)
		CFLAGS   += $(TARGET_RULE)
		CPPFLAGS += $(TARGET_RULE)
		CXXFLAGS += $(TARGET_RULE)
		LDFLAGS  += $(TARGET_RULE)
   endif
ifndef ($(NOUNIVERSAL))
	FLAGS += $(ARCHFLAGS)
	LDFLAGS += $(ARCHFLAGS)
endif

# iOS
else ifneq (,$(findstring ios,$(platform)))
	TARGET := $(TARGET_NAME)_libretro_ios.dylib
	fpic := -fPIC
	SHARED := -dynamiclib
        MINVERSION :=

ifeq ($(IOSSDK),)
	IOSSDK := $(shell xcodebuild -version -sdk iphoneos Path)
endif

ifeq ($(platform),ios-arm64)
	CC = cc -arch arm64 -isysroot $(IOSSDK)
	CXX = c++ -arch arm64 -isysroot $(IOSSDK)   
else
	CC = cc -arch armv7 -isysroot $(IOSSDK)
	CXX = c++ -arch armv7 -isysroot $(IOSSDK)
endif

ifeq ($(platform),$(filter $(platform),ios9 ios-arm64))
	MINVERSION = -miphoneos-version-min=8.0
else
	MINVERSION = -miphoneos-version-min=5.0
endif
        SHARED += $(MINVERSION)
	CC += $(MINVERSION)
	CXX += $(MINVERSION)
# tvOS
else ifeq ($(platform), tvos-arm64)
	TARGET := $(TARGET_NAME)_libretro_tvos.dylib
	fpic := -fPIC
	SHARED := -dynamiclib

ifeq ($(IOSSDK),)
	IOSSDK := $(shell xcodebuild -version -sdk appletvos Path)
endif
        CFLAGS += -DIOS
        CC = cc -arch arm64 -isysroot $(IOSSDK)
        MINVERSION = -mappletvos-version-min=11.0
        SHARED += $(MINVERSION)
        CC += $(MINVERSION)
        CXX += $(MINVERSION)

# Theos iOS
else ifeq ($(platform), theos_ios)
	DEPLOYMENT_IOSVERSION = 5.0
	TARGET = iphone:latest:$(DEPLOYMENT_IOSVERSION)
	ARCHS = armv7 armv7s
	TARGET_IPHONEOS_DEPLOYMENT_VERSION=$(DEPLOYMENT_IOSVERSION)
	THEOS_BUILD_DIR := objs
	include $(THEOS)/makefiles/common.mk
	LIBRARY_NAME = $(TARGET_NAME)_libretro_ios
else ifeq ($(platform), qnx)
	TARGET := $(TARGET_NAME)_libretro_$(platform).so
	fpic := -fPIC
	SHARED := -shared -Wl,--no-undefined -Wl,--version-script=link.T
	CC = qcc -Vgcc_ntoarmv7le
	CXX = QCC -Vgcc_ntoarmv7le_cpp
# Lightweight PS3 Homebrew SDK
else ifneq (,$(filter $(platform), ps3 psl1ght))
	TARGET := $(TARGET_NAME)_libretro_$(platform).a
	CC = $(PS3DEV)/ppu/bin/ppu-$(COMMONLV)gcc$(EXE_EXT)
	CXX = $(PS3DEV)/ppu/bin/ppu-$(COMMONLV)g++$(EXE_EXT)
	AR = $(PS3DEV)/ppu/bin/ppu-$(COMMONLV)ar$(EXE_EXT)
	STATIC_LINKING = 1
	FLAGS += -DMSB_FIRST -D__PS3__
	OLD_GCC = 1
	ifeq ($(platform), psl1ght)
		FLAGS += -D__PSL1GHT__
	endif	
# PSP1
else ifeq ($(platform), psp1)
	TARGET := $(TARGET_NAME)_libretro_$(platform).a
	CC = psp-gcc$(EXE_EXT)
	CXX = psp-g++$(EXE_EXT)
	AR = psp-ar$(EXE_EXT)
	STATIC_LINKING = 1
	LOAD_FROM_MEMORY = 0
	FLAGS += -G0
	CFLAGS += \
		-march=allegrex -mno-abicalls -fno-pic \
		-fno-builtin -fno-exceptions -ffunction-sections
	DEFS +=  -DPSP -D_PSP_FW_VERSION=371
	STATIC_LINKING := 1
	USE_OLD_COLOUR_OPS = 1
# Vita
else ifeq ($(platform), vita)
	TARGET := $(TARGET_NAME)_libretro_$(platform).a
	CC = arm-vita-eabi-gcc$(EXE_EXT)
	CXX = arm-vita-eabi-g++$(EXE_EXT)
	AR = arm-vita-eabi-ar$(EXE_EXT)
	STATIC_LINKING = 1
	LOAD_FROM_MEMORY = 0
	DEFS +=  -DVITA
	STATIC_LINKING := 1
# CTR (3DS)
else ifeq ($(platform), ctr)
	TARGET := $(TARGET_NAME)_libretro_$(platform).a
	CC = $(DEVKITARM)/bin/arm-none-eabi-gcc$(EXE_EXT)
	CXX = $(DEVKITARM)/bin/arm-none-eabi-g++$(EXE_EXT)
	AR = $(DEVKITARM)/bin/arm-none-eabi-ar$(EXE_EXT)
	CFLAGS += -DARM11 -D_3DS
	CFLAGS += -march=armv6k -mtune=mpcore -mfloat-abi=hard
	CFLAGS += -Wall -mword-relocations
	CFLAGS += -fomit-frame-pointer -ffast-math
	CFLAGS += -D_3DS
	PLATFORM_DEFINES := -D_3DS
	STATIC_LINKING = 1
# Nintendo Game Cube
else ifeq ($(platform), ngc)
	TARGET := $(TARGET_NAME)_libretro_$(platform).a
	CC = $(DEVKITPPC)/bin/powerpc-eabi-gcc$(EXE_EXT)
	AR = $(DEVKITPPC)/bin/powerpc-eabi-ar$(EXE_EXT)
	CFLAGS += -DGEKKO -DHW_DOL -mrvl -mcpu=750 -meabi -mhard-float -D__ppc__ -DMSB_FIRST
	CFLAGS += -U__INT32_TYPE__ -U __UINT32_TYPE__ -D__INT32_TYPE__=int
	STATIC_LINKING = 1
# Nintendo Wii
else ifeq ($(platform), wii)
	TARGET := $(TARGET_NAME)_libretro_$(platform).a
	CC = $(DEVKITPPC)/bin/powerpc-eabi-gcc$(EXE_EXT)
	AR = $(DEVKITPPC)/bin/powerpc-eabi-ar$(EXE_EXT)
	CFLAGS += -DGEKKO -DHW_RVL -mrvl -mcpu=750 -meabi -mhard-float -D__ppc__ -DMSB_FIRST
	CFLAGS += -U__INT32_TYPE__ -U __UINT32_TYPE__ -D__INT32_TYPE__=int
	STATIC_LINKING = 1
# Nintendo WiiU
else ifeq ($(platform), wiiu)
	TARGET := $(TARGET_NAME)_libretro_$(platform).a
	CC = $(DEVKITPPC)/bin/powerpc-eabi-gcc$(EXE_EXT)
	AR = $(DEVKITPPC)/bin/powerpc-eabi-ar$(EXE_EXT)
	CFLAGS += -DGEKKO -DWIIU -DHW_RVL -mcpu=750 -meabi -mhard-float -D__ppc__ -DMSB_FIRST
	CFLAGS += -ffunction-sections -fdata-sections -D__wiiu__ -D__wut__
	STATIC_LINKING = 1
else ifeq ($(platform), emscripten)
	TARGET := $(TARGET_NAME)_libretro_$(platform).bc
	STATIC_LINKING = 1

# GCW0
else ifeq ($(platform), gcw0)
	TARGET := $(TARGET_NAME)_libretro.so
	CC = /opt/gcw0-toolchain/usr/bin/mipsel-linux-gcc
	CXX = /opt/gcw0-toolchain/usr/bin/mipsel-linux-g++
	AR = /opt/gcw0-toolchain/usr/bin/mipsel-linux-ar
	fpic := -fPIC -nostdlib
	SHARED := -shared -Wl,--version-script=link.T
	LIBM :=
	FLAGS += -fomit-frame-pointer -ffast-math -march=mips32 -mtune=mips32r2 -mhard-float
	FLAGS += -DFAST_LSB_WORD_ACCESS

# MIYOO
else ifeq ($(platform), miyoo)
	TARGET := $(TARGET_NAME)_libretro.so
	CC = /opt/miyoo/usr/bin/arm-linux-gcc
   	CXX = /opt/miyoo/usr/bin/arm-linux-g++
   	AR = /opt/miyoo/usr/bin/arm-linux-ar
	fpic := -fPIC -nostdlib
	SHARED := -shared -Wl,--version-script=link.T
	LIBM :=
	FLAGS += -fomit-frame-pointer -ffast-math -march=armv5te -mtune=arm926ej-s
	FLAGS += -DFAST_ALIGNED_LSB_WORD_ACCESS -fno-unroll-loops

# (armv7 a7, hard point, neon based) ### 
# NESC, SNESC, C64 mini 
else ifeq ($(platform), classic_armv7_a7)
	TARGET := $(TARGET_NAME)_libretro.so
	fpic := -fPIC
	SHARED := -shared -Wl,--version-script=link.T -Wl,--no-undefined
	CFLAGS += -Ofast \
	-flto=4 -fwhole-program -fuse-linker-plugin \
	-fdata-sections -ffunction-sections -Wl,--gc-sections \
	-fno-stack-protector -fno-ident -fomit-frame-pointer \
	-falign-functions=1 -falign-jumps=1 -falign-loops=1 \
	-fno-unwind-tables -fno-asynchronous-unwind-tables -fno-unroll-loops \
	-fmerge-all-constants -fno-math-errno \
	-marm -mtune=cortex-a7 -mfpu=neon-vfpv4 -mfloat-abi=hard
	CXXFLAGS += $(CFLAGS)
	CPPFLAGS += $(CFLAGS)
	ASFLAGS += $(CFLAGS)
	HAVE_NEON = 1
	ARCH = arm
	BUILTIN_GPU = neon
	USE_DYNAREC = 1
	ifeq ($(shell echo `$(CC) -dumpversion` "< 4.9" | bc -l), 1)
	  CFLAGS += -march=armv7-a
	else
	  CFLAGS += -march=armv7ve
	  # If gcc is 5.0 or later
	  ifeq ($(shell echo `$(CC) -dumpversion` ">= 5" | bc -l), 1)
	    LDFLAGS += -static-libgcc -static-libstdc++
	  endif
	endif
#######################################

# Windows MSVC 2010 x64
else ifeq ($(platform), windows_msvc2010_x64)
	CC  = cl.exe
	CXX = cl.exe

PATH := $(shell IFS=$$'\n'; cygpath "$(VS100COMNTOOLS)../../VC/bin/amd64"):$(PATH)
PATH := $(PATH):$(shell IFS=$$'\n'; cygpath "$(VS100COMNTOOLS)../IDE")
INCLUDE := $(shell IFS=$$'\n'; cygpath "$(VS100COMNTOOLS)../../VC/include")
LIB := $(shell IFS=$$'\n'; cygpath -w "$(VS100COMNTOOLS)../../VC/lib/amd64")
BIN := $(shell IFS=$$'\n'; cygpath "$(VS100COMNTOOLS)../../VC/bin/amd64")

WindowsSdkDir := $(shell reg query "HKLM\SOFTWARE\Microsoft\Microsoft SDKs\Windows\v7.1A" -v "InstallationFolder" | grep -o '[A-Z]:\\.*')
WindowsSdkDir ?= $(shell reg query "HKLM\SOFTWARE\Microsoft\Microsoft SDKs\Windows\v7.0A" -v "InstallationFolder" | grep -o '[A-Z]:\\.*')

WindowsSDKIncludeDir := $(shell cygpath -w "$(WindowsSdkDir)\Include")
WindowsSDKGlIncludeDir := $(shell cygpath -w "$(WindowsSdkDir)\Include\gl")
WindowsSDKLibDir := $(shell cygpath -w "$(WindowsSdkDir)\Lib\x64")

INCFLAGS_PLATFORM = -I"$(WindowsSDKIncludeDir)"
export INCLUDE := $(INCLUDE);$(WindowsSDKIncludeDir);$(WindowsSDKGlIncludeDir)
export LIB := $(LIB);$(WindowsSDKLibDir)
TARGET := $(TARGET_NAME)_libretro.dll
PSS_STYLE :=2
LDFLAGS += -DLL
CFLAGS += -D_CRT_SECURE_NO_DEPRECATE
NO_GCC = 1

# Windows MSVC 2010 x86
else ifeq ($(platform), windows_msvc2010_x86)
	CC  = cl.exe
	CXX = cl.exe

PATH := $(shell IFS=$$'\n'; cygpath "$(VS100COMNTOOLS)../../VC/bin"):$(PATH)
PATH := $(PATH):$(shell IFS=$$'\n'; cygpath "$(VS100COMNTOOLS)../IDE")
INCLUDE := $(shell IFS=$$'\n'; cygpath "$(VS100COMNTOOLS)../../VC/include")
LIB := $(shell IFS=$$'\n'; cygpath -w "$(VS100COMNTOOLS)../../VC/lib")
BIN := $(shell IFS=$$'\n'; cygpath "$(VS100COMNTOOLS)../../VC/bin")

WindowsSdkDir := $(shell reg query "HKLM\SOFTWARE\Microsoft\Microsoft SDKs\Windows\v7.1A" -v "InstallationFolder" | grep -o '[A-Z]:\\.*')
WindowsSdkDir ?= $(shell reg query "HKLM\SOFTWARE\Microsoft\Microsoft SDKs\Windows\v7.0A" -v "InstallationFolder" | grep -o '[A-Z]:\\.*')

WindowsSDKIncludeDir := $(shell cygpath -w "$(WindowsSdkDir)\Include")
WindowsSDKGlIncludeDir := $(shell cygpath -w "$(WindowsSdkDir)\Include\gl")
WindowsSDKLibDir := $(shell cygpath -w "$(WindowsSdkDir)\Lib")

INCFLAGS_PLATFORM = -I"$(WindowsSDKIncludeDir)"
export INCLUDE := $(INCLUDE);$(WindowsSDKIncludeDir);$(WindowsSDKGlIncludeDir)
export LIB := $(LIB);$(WindowsSDKLibDir)
TARGET := $(TARGET_NAME)_libretro.dll
PSS_STYLE :=2
LDFLAGS += -DLL
CFLAGS += -D_CRT_SECURE_NO_DEPRECATE
NO_GCC = 1

# Windows MSVC 2008 x86
else ifeq ($(platform), windows_msvc2008_x86)
	CC  = cl.exe
	CXX = cl.exe

PATH := $(shell IFS=$$'\n'; cygpath "$(VS90COMNTOOLS)../../VC/bin"):$(PATH)
PATH := $(PATH):$(shell IFS=$$'\n'; cygpath "$(VS90COMNTOOLS)../IDE")
INCLUDE := $(shell IFS=$$'\n'; cygpath "$(VS90COMNTOOLS)../../VC/include")
LIB := $(shell IFS=$$'\n'; cygpath -w "$(VS90COMNTOOLS)../../VC/lib")
BIN := $(shell IFS=$$'\n'; cygpath "$(VS90COMNTOOLS)../../VC/bin")

WindowsSdkDir := $(INETSDK)

export INCLUDE := $(INCLUDE);$(INETSDK)/Include;libretro-common/include/compat/msvc
export LIB := $(LIB);$(WindowsSdkDir);$(INETSDK)/Lib
TARGET := $(TARGET_NAME)_libretro.dll
PSS_STYLE :=2
LDFLAGS += -DLL
CFLAGS += -D_CRT_SECURE_NO_DEPRECATE
NO_GCC = 1

# Windows MSVC 2005 x86
else ifeq ($(platform), windows_msvc2005_x86)
	CC  = cl.exe
	CXX = cl.exe

PATH := $(shell IFS=$$'\n'; cygpath "$(VS80COMNTOOLS)../../VC/bin"):$(PATH)
PATH := $(PATH):$(shell IFS=$$'\n'; cygpath "$(VS80COMNTOOLS)../IDE")
INCLUDE := $(shell IFS=$$'\n'; cygpath "$(VS80COMNTOOLS)../../VC/include")
LIB := $(shell IFS=$$'\n'; cygpath -w "$(VS80COMNTOOLS)../../VC/lib")
BIN := $(shell IFS=$$'\n'; cygpath "$(VS80COMNTOOLS)../../VC/bin")

WindowsSdkDir := $(shell reg query "HKLM\SOFTWARE\Microsoft\MicrosoftSDK\InstalledSDKs\8F9E5EF3-A9A5-491B-A889-C58EFFECE8B3" -v "Install Dir" | grep -o '[A-Z]:\\.*')

WindowsSDKIncludeDir := $(shell cygpath -w "$(WindowsSdkDir)\Include")
WindowsSDKAtlIncludeDir := $(shell cygpath -w "$(WindowsSdkDir)\Include\atl")
WindowsSDKCrtIncludeDir := $(shell cygpath -w "$(WindowsSdkDir)\Include\crt")
WindowsSDKGlIncludeDir := $(shell cygpath -w "$(WindowsSdkDir)\Include\gl")
WindowsSDKMfcIncludeDir := $(shell cygpath -w "$(WindowsSdkDir)\Include\mfc")
WindowsSDKLibDir := $(shell cygpath -w "$(WindowsSdkDir)\Lib")

export INCLUDE := $(INCLUDE);$(WindowsSDKIncludeDir);$(WindowsSDKAtlIncludeDir);$(WindowsSDKCrtIncludeDir);$(WindowsSDKGlIncludeDir);$(WindowsSDKMfcIncludeDir);libretro-common/include/compat/msvc
export LIB := $(LIB);$(WindowsSDKLibDir)
TARGET := $(TARGET_NAME)_libretro.dll
PSS_STYLE :=2
LDFLAGS += -DLL
CFLAGS += -D_CRT_SECURE_NO_DEPRECATE
NO_GCC = 1

# Windows MSVC 2003 x86
else ifeq ($(platform), windows_msvc2003_x86)
	CC  = cl.exe
	CXX = cl.exe

PATH := $(shell IFS=$$'\n'; cygpath "$(VS71COMNTOOLS)../../Vc7/bin"):$(PATH)
PATH := $(PATH):$(shell IFS=$$'\n'; cygpath "$(VS71COMNTOOLS)../IDE")
INCLUDE := $(shell IFS=$$'\n'; cygpath "$(VS71COMNTOOLS)../../Vc7/include")
LIB := $(shell IFS=$$'\n'; cygpath -w "$(VS71COMNTOOLS)../../Vc7/lib")
BIN := $(shell IFS=$$'\n'; cygpath "$(VS71COMNTOOLS)../../Vc7/bin")

WindowsSdkDir := $(INETSDK)

export INCLUDE := $(INCLUDE);$(INETSDK)/Include;libretro-common/include/compat/msvc
export LIB := $(LIB);$(WindowsSdkDir);$(INETSDK)/Lib
TARGET := $(TARGET_NAME)_libretro.dll
PSS_STYLE :=2
LDFLAGS += -DLL
CFLAGS += -D_CRT_SECURE_NO_DEPRECATE
NO_GCC = 1

# Windows MSVC 2017 all architectures
else ifneq (,$(findstring windows_msvc2017,$(platform)))

    NO_GCC := 1
    CFLAGS += -DNOMINMAX
    CXXFLAGS += -DNOMINMAX
    WINDOWS_VERSION = 1

	PlatformSuffix = $(subst windows_msvc2017_,,$(platform))
	ifneq (,$(findstring desktop,$(PlatformSuffix)))
		WinPartition = desktop
		MSVC2017CompileFlags = -DWINAPI_FAMILY=WINAPI_FAMILY_DESKTOP_APP -FS
		LDFLAGS += -MANIFEST -LTCG:incremental -NXCOMPAT -DYNAMICBASE -DEBUG -OPT:REF -INCREMENTAL:NO -SUBSYSTEM:WINDOWS -MANIFESTUAC:"level='asInvoker' uiAccess='false'" -OPT:ICF -ERRORREPORT:PROMPT -NOLOGO -TLBID:1
		LIBS += kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib
	else ifneq (,$(findstring uwp,$(PlatformSuffix)))
		WinPartition = uwp
		MSVC2017CompileFlags = -DWINAPI_FAMILY=WINAPI_FAMILY_APP -D_WINDLL -D_UNICODE -DUNICODE -D__WRL_NO_DEFAULT_LIB__ -EHsc -FS
		LDFLAGS += -APPCONTAINER -NXCOMPAT -DYNAMICBASE -MANIFEST:NO -LTCG -OPT:REF -SUBSYSTEM:CONSOLE -MANIFESTUAC:NO -OPT:ICF -ERRORREPORT:PROMPT -NOLOGO -TLBID:1 -DEBUG:FULL -WINMD:NO
		LIBS += WindowsApp.lib
	endif

	CFLAGS += $(MSVC2017CompileFlags)
	CXXFLAGS += $(MSVC2017CompileFlags)

	TargetArchMoniker = $(subst $(WinPartition)_,,$(PlatformSuffix))

	CC  = cl.exe
	CXX = cl.exe
	LD = link.exe

	reg_query = $(call filter_out2,$(subst $2,,$(shell reg query "$2" -v "$1" 2>nul)))
	fix_path = $(subst $(SPACE),\ ,$(subst \,/,$1))

	ProgramFiles86w := $(shell cmd //c "echo %PROGRAMFILES(x86)%")
	ProgramFiles86 := $(shell cygpath "$(ProgramFiles86w)")

	WindowsSdkDir ?= $(call reg_query,InstallationFolder,HKEY_LOCAL_MACHINE\SOFTWARE\Wow6432Node\Microsoft\Microsoft SDKs\Windows\v10.0)
	WindowsSdkDir ?= $(call reg_query,InstallationFolder,HKEY_CURRENT_USER\SOFTWARE\Wow6432Node\Microsoft\Microsoft SDKs\Windows\v10.0)
	WindowsSdkDir ?= $(call reg_query,InstallationFolder,HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\Microsoft SDKs\Windows\v10.0)
	WindowsSdkDir ?= $(call reg_query,InstallationFolder,HKEY_CURRENT_USER\SOFTWARE\Microsoft\Microsoft SDKs\Windows\v10.0)
	WindowsSdkDir := $(WindowsSdkDir)

	WindowsSDKVersion ?= $(firstword $(foreach folder,$(subst $(subst \,/,$(WindowsSdkDir)Include/),,$(wildcard $(call fix_path,$(WindowsSdkDir)Include\*))),$(if $(wildcard $(call fix_path,$(WindowsSdkDir)Include/$(folder)/um/Windows.h)),$(folder),)))$(BACKSLASH)
	WindowsSDKVersion := $(WindowsSDKVersion)

	VsInstallBuildTools = $(ProgramFiles86)/Microsoft Visual Studio/2017/BuildTools
	VsInstallEnterprise = $(ProgramFiles86)/Microsoft Visual Studio/2017/Enterprise
	VsInstallProfessional = $(ProgramFiles86)/Microsoft Visual Studio/2017/Professional
	VsInstallCommunity = $(ProgramFiles86)/Microsoft Visual Studio/2017/Community

	VsInstallRoot ?= $(shell if [ -d "$(VsInstallBuildTools)" ]; then echo "$(VsInstallBuildTools)"; fi)
	ifeq ($(VsInstallRoot), )
		VsInstallRoot = $(shell if [ -d "$(VsInstallEnterprise)" ]; then echo "$(VsInstallEnterprise)"; fi)
	endif
	ifeq ($(VsInstallRoot), )
		VsInstallRoot = $(shell if [ -d "$(VsInstallProfessional)" ]; then echo "$(VsInstallProfessional)"; fi)
	endif
	ifeq ($(VsInstallRoot), )
		VsInstallRoot = $(shell if [ -d "$(VsInstallCommunity)" ]; then echo "$(VsInstallCommunity)"; fi)
	endif
	VsInstallRoot := $(VsInstallRoot)

	VcCompilerToolsVer := $(shell cat "$(VsInstallRoot)/VC/Auxiliary/Build/Microsoft.VCToolsVersion.default.txt" | grep -o '[0-9\.]*')
	VcCompilerToolsDir := $(VsInstallRoot)/VC/Tools/MSVC/$(VcCompilerToolsVer)

	WindowsSDKSharedIncludeDir := $(shell cygpath -w "$(WindowsSdkDir)\Include\$(WindowsSDKVersion)\shared")
	WindowsSDKUCRTIncludeDir := $(shell cygpath -w "$(WindowsSdkDir)\Include\$(WindowsSDKVersion)\ucrt")
	WindowsSDKUMIncludeDir := $(shell cygpath -w "$(WindowsSdkDir)\Include\$(WindowsSDKVersion)\um")
	WindowsSDKUCRTLibDir := $(shell cygpath -w "$(WindowsSdkDir)\Lib\$(WindowsSDKVersion)\ucrt\$(TargetArchMoniker)")
	WindowsSDKUMLibDir := $(shell cygpath -w "$(WindowsSdkDir)\Lib\$(WindowsSDKVersion)\um\$(TargetArchMoniker)")

	# For some reason the HostX86 compiler doesn't like compiling for x64
	# ("no such file" opening a shared library), and vice-versa.
	# Work around it for now by using the strictly x86 compiler for x86, and x64 for x64.
	# NOTE: What about ARM?
	ifneq (,$(findstring x64,$(TargetArchMoniker)))
		VCCompilerToolsBinDir := $(VcCompilerToolsDir)\bin\HostX64
	else
		VCCompilerToolsBinDir := $(VcCompilerToolsDir)\bin\HostX86
	endif

	PATH := $(shell IFS=$$'\n'; cygpath "$(VCCompilerToolsBinDir)/$(TargetArchMoniker)"):$(PATH)
	PATH := $(PATH):$(shell IFS=$$'\n'; cygpath "$(VsInstallRoot)/Common7/IDE")
	INCLUDE := $(shell IFS=$$'\n'; cygpath -w "$(VcCompilerToolsDir)/include")
	LIB := $(shell IFS=$$'\n'; cygpath -w "$(VcCompilerToolsDir)/lib/$(TargetArchMoniker)")
	ifneq (,$(findstring uwp,$(PlatformSuffix)))
		LIB := $(LIB);$(shell IFS=$$'\n'; cygpath -w "$(LIB)/store")
	endif
    
	export INCLUDE := $(INCLUDE);$(WindowsSDKSharedIncludeDir);$(WindowsSDKUCRTIncludeDir);$(WindowsSDKUMIncludeDir)
	export LIB := $(LIB);$(WindowsSDKUCRTLibDir);$(WindowsSDKUMLibDir)
	TARGET := $(TARGET_NAME)_libretro.dll
	PSS_STYLE :=2
	LDFLAGS += -DLL

else
	TARGET := $(TARGET_NAME)_libretro.dll
	CC ?= gcc
	CXX ?= g++
	SHARED := -shared -Wl,--no-undefined -Wl,--version-script=link.T
	LDFLAGS += -static-libgcc -static-libstdc++ -lwinmm
endif

LDFLAGS += $(LIBM)

CORE_DIR := ./source
LIBRETRO_DIR := .

include Makefile.common

ifeq ($(OLD_GCC), 1)
	WARNINGS := -Wall
else ifeq ($(NO_GCC), 1)
	WARNINGS :=
else
	WARNINGS := \
		-Wall \
		-Wextra \
		-pedantic \
		-Wno-implicit-function-declaration \
		-Wno-implicit-fallthrough \
		-Wno-sign-compare \
		-Wno-unused-variable \
		-Wno-unused-function \
		-Wno-unused-parameter \
		-Wno-uninitialized \
		-Wno-strict-aliasing \
		-Wno-overflow \
		-fno-strict-overflow
endif

ifeq ($(DEBUG),1)
	FLAGS += -O0 -g
else
	FLAGS += -O2 -DNDEBUG
endif

ifneq (,$(findstring msvc,$(platform)))
ifeq ($(DEBUG),1)
   FLAGS += -MTd
else
   FLAGS += -MT
endif
endif

ifeq ($(PERF_TEST),1)
	FLAGS += -DPERF_TEST
endif

ifeq ($(LAGFIX),1)
	FLAGS += -DLAGFIX
endif

FLAGS += $(INCFLAGS_PLATFORM)

ifeq ($(platform), psp1)
	INCFLAGS += -I$(shell psp-config --pspsdk-path)/include
endif

OBJECTS := $(SOURCES_C:.c=.o)

CXXFLAGS += $(FLAGS)
CFLAGS += $(FLAGS)

OBJOUT   = -o
LINKOUT  = -o 

ifneq (,$(findstring msvc,$(platform)))
	OBJOUT = -Fo
	LINKOUT = -out:
ifeq ($(STATIC_LINKING),1)
	LD ?= lib.exe
	STATIC_LINKING=0
else
	LD = link.exe
endif
else ifneq ($(platform),genode)
	LD = $(CC)
else
	OBJOUT   = -o
	LINKOUT  = -o 
	LD = $(CC)
endif

%.o: %.cpp
	$(CXX) $(fpic) $(CXXFLAGS) -c $(OBJOUT)$@ $<

%.o: %.c
	$(CC) $(fpic) $(CFLAGS) -c $(OBJOUT)$@ $<

ifeq ($(platform), theos_ios)
	COMMON_FLAGS := -DIOS $(COMMON_DEFINES) $(INCFLAGS) -I$(THEOS_INCLUDE_PATH) -Wno-error
	$(LIBRARY_NAME)_CFLAGS += $(COMMON_FLAGS) $(CFLAGS)
	${LIBRARY_NAME}_FILES = $(SOURCES_C)
	include $(THEOS_MAKE_PATH)/library.mk
else
all: $(TARGET)
$(TARGET): $(OBJECTS)
ifeq ($(STATIC_LINKING), 1)
	$(AR) rcs $@ $(OBJECTS)
else
	$(LD) $(fpic) $(SHARED) $(LDFLAGS) $(LINKOUT)$@ $(OBJECTS) $(LIBS)
endif

clean:
	rm -f $(TARGET) $(OBJECTS)

.PHONY: clean
endif
