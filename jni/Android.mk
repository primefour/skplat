LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)
LOCAL_MODULE:=skplat
$(info $(LOCAL_PATH))
$(info Inc path $(LOCAL_PATH)) 

#root directory
PROJECT_ROOT_PATH := $(LOCAL_PATH)/../
PROJECT_LOG_PATH := $(PROJECT_ROOT_PATH)/log/
PROJECT_UTILS_PATH := $(PROJECT_ROOT_PATH)/utils/
PROJECT_DEVICES_PATH := $(PROJECT_ROOT_PATH)/devices/

LOCAL_C_INCLUDES += $(PROJECT_UTILS_PATH) \
	$(PROJECT_LOG_PATH) \
	$(PROJECT_DEVICES_PATH) \ 

#local jni interface file
JNI_SRC := $(wildcard $(LOCAL_PATH)/*.*)
$(info JNI_SRC=$(JNI_SRC))
JNI_SRC := $(filter %.cc %.c,$(JNI_SRC))

#utils source file list
UTILS_SRC += $(wildcard $(PROJECT_UTILS_PATH)/*.*)
UTILS_SRC := $(filter %.cc %.c,$(UTILS_SRC))

#log source file list
LOG_SRC := $(wildcard $(PROJECT_LOG_PATH)/*.*)
LOG_SRC := $(filter %.cc %.c,$(LOG_SRC))

#utils source file list
DEVICES_SRC += $(wildcard $(PROJECT_DEVICES_PATH)/*.*)
DEVICES_SRC := $(filter %.cc %.c,$(DEVICES_SRC))


#LOCAL_SHARED_LIBRARIES := -lstdc++ libstlport
LOCAL_LDLIBS    := -lm -llog -lz

$(info NDK_PROJECT_PATH =$(NDK_PROJECT_PATH))
$(info PROJECT_ROOT_PATH =$(PROJECT_ROOT_PATH))  

LOCAL_SRC_FILES += $(UTILS_SRC)
LOCAL_SRC_FILES += $(LOG_SRC)
LOCAL_SRC_FILES += $(DEVICES_SRC)

LOCAL_C_INCLUDES += $(LOCAL_PATH)
LOCAL_SRC_FILES:= $(LOCAL_SRC_FILES:$(LOCAL_PATH)/%=%)

LOCAL_CFLAGS := -Os -Wall -Wno-sign-compare -Wno-unused-local-typedefs -D__STDC_FORMAT_MACROS

include $(BUILD_SHARED_LIBRARY)
