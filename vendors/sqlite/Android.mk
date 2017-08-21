LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)


LOCAL_CFLAGS += $(sqlite_flags)



LOCAL_SRC_FILES:= \
	android_database_SQLiteCommon.cpp \
	android_database_SQLiteConnection.cpp \
	android_database_SQLiteGlobal.cpp \
	android_database_SQLiteDebug.cpp \
	android_database_CursorWindow.cpp \
	CursorWindow.cpp \
	JNIHelp.cpp \
	JNIString.cpp

LOCAL_SRC_FILES += sqlite3.c

LOCAL_C_INCLUDES += $(LOCAL_PATH)

LOCAL_MODULE:= libsqlite3x
LOCAL_LDLIBS += -ldl -llog 

include $(BUILD_SHARED_LIBRARY)

