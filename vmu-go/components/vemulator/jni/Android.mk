LOCAL_PATH := $(call my-dir)

CORE_DIR     := $(LOCAL_PATH)/..
ROOT_DIR     := $(LOCAL_PATH)/..

include $(ROOT_DIR)/Makefile.common

include $(CLEAR_VARS)
LOCAL_MODULE    := retro
LOCAL_SRC_FILES := $(SOURCES_C) $(SOURCES_CXX)
LOCAL_CFLAGS    := -O3 -ffast-math -funroll-loops -D__LIBRETRO__  $(INCFLAGS)
LOCAL_LDFLAGS   := -Wl,-version-script=$(CORE_DIR)/link.T

ifeq ($(TARGET_ARCH),arm)
LOCAL_CFLAGS += -DANDROID_ARM
endif

ifeq ($(TARGET_ARCH),x86)
LOCAL_CFLAGS +=  -DANDROID_X86
endif

ifeq ($(TARGET_ARCH),mips)
LOCAL_CFLAGS += -DANDROID_MIPS -D__mips__ -D__MIPSEL__
endif

include $(BUILD_SHARED_LIBRARY)
