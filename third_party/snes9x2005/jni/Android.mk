LOCAL_PATH := $(call my-dir)

ROOT_DIR     := $(LOCAL_PATH)/..
CORE_DIR     := $(ROOT_DIR)/source
LIBRETRO_DIR := $(ROOT_DIR)

LOAD_FROM_MEMORY := 1
FLAGS :=

include $(ROOT_DIR)/Makefile.common

COREFLAGS := -ffast-math $(FLAGS)

GIT_VERSION := " $(shell git rev-parse --short HEAD || echo unknown)"
ifneq ($(GIT_VERSION)," unknown")
  COREFLAGS += -DGIT_VERSION=\"$(GIT_VERSION)\"
endif

include $(CLEAR_VARS)
LOCAL_MODULE    := retro
LOCAL_SRC_FILES := $(SOURCES_C)
LOCAL_CFLAGS    := $(COREFLAGS)
LOCAL_LDFLAGS   := -Wl,-version-script=$(LIBRETRO_DIR)/link.T
include $(BUILD_SHARED_LIBRARY)
