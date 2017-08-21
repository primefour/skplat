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
PROJECT_NETWORK_PATH := $(PROJECT_ROOT_PATH)/network/


LOCAL_C_INCLUDES += $(PROJECT_UTILS_PATH) \
	$(PROJECT_LOG_PATH) \
	$(PROJECT_DEVICES_PATH) \
	$(PROJECT_NETWORK_PATH) \ 

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

#devices source file list
DEVICES_SRC += $(wildcard $(PROJECT_DEVICES_PATH)/*.*)
DEVICES_SRC := $(filter %.cc %.c,$(DEVICES_SRC))

#network source file list
NETWORK_SRC += $(wildcard $(PROJECT_NETWORK_PATH)/*.*)
NETWORK_SRC := $(filter %.cc %.c,$(NETWORK_SRC))



#LOCAL_SHARED_LIBRARIES := -lstdc++ libstlport
LOCAL_LDLIBS    := -lm -llog -lz

$(info NDK_PROJECT_PATH =$(NDK_PROJECT_PATH))
$(info PROJECT_ROOT_PATH =$(PROJECT_ROOT_PATH))  

$(info NDK_PROJECT_PATH =$(UTILS_SRC))

#add source code
LOCAL_SRC_FILES += $(UTILS_SRC)
LOCAL_SRC_FILES += $(LOG_SRC)
LOCAL_SRC_FILES += $(DEVICES_SRC)
LOCAL_SRC_FILES += $(JNI_SRC)
LOCAL_SRC_FILES += $(NETWORK_SRC)

#add local include file 
LOCAL_C_INCLUDES += $(LOCAL_PATH)

###third party source code import####
##sqlite
SQLITE_DIR=$(PROJECT_ROOT_PATH)/vendors/sqlite/
include $(PROJECT_ROOT_PATH)/vendors/sqlite/sqlite.mk
LOCAL_SRC_FILES += $(SQLITE_SRC_FILES)
LOCAL_C_INCLUDES += $(SQLITE_C_INCLUDES)

##remove jni
LOCAL_SRC_FILES:= $(LOCAL_SRC_FILES:$(LOCAL_PATH)/%=%)


LOCAL_CFLAGS := -Os -Wall -Wno-sign-compare -Wno-unused-local-typedefs -D__STDC_FORMAT_MACROS
LOCAL_CFLAGS += -Wno-unused-parameter -Wno-int-to-pointer-cast
LOCAL_CFLAGS += -Wno-uninitialized -Wno-parentheses
LOCAL_CPPFLAGS += -Wno-conversion-null

include $(BUILD_SHARED_LIBRARY)
