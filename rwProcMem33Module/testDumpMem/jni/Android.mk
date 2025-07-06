LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_C_INCLUDES += $(LOCAL_PATH)/libzip

LOCAL_CPPFLAGS += -std=c++20
LOCAL_CFLAGS += -fPIE -fvisibility=hidden
LOCAL_LDFLAGS += -fPIE -pie
LOCAL_DISABLE_FATAL_LINKER_WARNINGS := true
LOCAL_MODULE    := testDumpMem.out

FILE_LIST += $(wildcard $(LOCAL_PATH)/libzip/*.c*)

LOCAL_SRC_FILES :=  $(FILE_LIST:$(LOCAL_PATH)/%=%) testDumpMem.cpp

LOCAL_LDLIBS := -lz

include $(BUILD_EXECUTABLE)
