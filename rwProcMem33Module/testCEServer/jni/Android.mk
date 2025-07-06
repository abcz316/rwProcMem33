LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_CPPFLAGS += -std=c++20
LOCAL_CFLAGS += -fPIE
LOCAL_CFLAGS += -fvisibility=hidden
LOCAL_LDFLAGS += -fPIE -pie
LOCAL_DISABLE_FATAL_LINKER_WARNINGS := true
LOCAL_MODULE    := testCEServer
LOCAL_SRC_FILES :=  testCEServer.cpp api.cpp porthelp.cpp native-api.cpp symbols.cpp
LOCAL_LDLIBS := -lz -llog
include $(BUILD_EXECUTABLE)
