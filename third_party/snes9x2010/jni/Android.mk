LOCAL_PATH := $(call my-dir)

CORE_DIR := $(LOCAL_PATH)/..

include $(CORE_DIR)/build/Makefile.common

COREFLAGS := -ffast-math -funroll-loops -DINLINE=inline -D__LIBRETRO__ -DFRONTEND_SUPPORTS_RGB565 $(INCFLAGS)

GIT_VERSION := " $(shell git rev-parse --short HEAD || echo unknown)"
ifneq ($(GIT_VERSION)," unknown")
  COREFLAGS += -DGIT_VERSION=\"$(GIT_VERSION)\"
endif

include $(CLEAR_VARS)
LOCAL_MODULE    := retro
LOCAL_SRC_FILES := $(SOURCES_C)
LOCAL_CFLAGS    := $(COREFLAGS)
LOCAL_LDFLAGS   := -Wl,-version-script=$(CORE_DIR)/libretro/link.T

ifeq ($(TARGET_ARCH_ABI),armeabi-v7a)
  LOCAL_ARM_NEON := true
endif

include $(BUILD_SHARED_LIBRARY)
