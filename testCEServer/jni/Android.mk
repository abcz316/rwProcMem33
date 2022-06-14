LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_CPPFLAGS += -std=c++1y
LOCAL_CFLAGS += -fPIE
LOCAL_CFLAGS += -fvisibility=hidden
LOCAL_LDFLAGS += -fPIE -pie
LOCAL_DISABLE_FATAL_LINKER_WARNINGS := true
LOCAL_MODULE    := testCEServer.out
LOCAL_SRC_FILES :=  ../main.cpp ../api.cpp ../porthelp.cpp ../native-api.cpp ../symbols.cpp
LOCAL_LDLIBS := -lz -llog
include $(BUILD_EXECUTABLE)
