LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_CPPFLAGS += -std=c++20
LOCAL_CFLAGS += -fPIE
LOCAL_CFLAGS += -fvisibility=hidden
LOCAL_LDFLAGS += -fPIE -pie
LOCAL_DISABLE_FATAL_LINKER_WARNINGS := true
LOCAL_MODULE    := testHwBpServer
LOCAL_SRC_FILES :=  testHwBpServer.cpp api.cpp Global.cpp  porthelp.cpp
LOCAL_LDLIBS := -lz -llog
include $(BUILD_EXECUTABLE)
