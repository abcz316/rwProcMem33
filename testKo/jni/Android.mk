LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_CPPFLAGS += -std=c++1y
LOCAL_CFLAGS += -fPIE
LOCAL_LDFLAGS += -fPIE -pie
LOCAL_DISABLE_FATAL_LINKER_WARNINGS := true
LOCAL_MODULE    := testKo.out
LOCAL_SRC_FILES :=  ../main.cpp ../MemoryReaderWriter.c ../cvector.c
include $(BUILD_EXECUTABLE)
