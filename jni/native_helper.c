/*******************************************************************************
 **      Filename: utils/jni/native_help.cc
 **        Author: crazyhorse                  
 **   Description: ---
 **        Create: 2017-03-30 11:32:10
 ** Last Modified: 2017-03-30 11:32:10
 ******************************************************************************/
#include <jni.h>
#include <stdio.h>
#include <android/log.h>
#include <time.h>
#include <math.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <unistd.h>
#include <assert.h>
#include <sys/syscall.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <android/log.h>
#include <pthread.h>
#include "native_helper.h"

#define LOG_TAG "native_helper"

// InterruptedException
static const char *interrupted_exception_class_path_name = "java/lang/InterruptedException";

// RuntimeException
static const char *runtime_exception_class_path_name = "java/lang/RuntimeException";


jfieldID java_get_field(JNIEnv *env, char* class_name, JavaField field) {
    jclass clazz = (*env)->FindClass(env, class_name);
    jfieldID jField = (*env)->GetFieldID(env, clazz, field.name, field.signature);
    (*env)->DeleteLocalRef(env, clazz);
    return jField;
}

jmethodID java_get_method(JNIEnv *env, jclass java_class, JavaMethod method) {
    return (*env)->GetMethodID(env, java_class, method.name, method.signature);
}


int register_native_methods(JNIEnv* env, const char* class_name, 
                                    JNINativeMethod* methods, int num_methods) {
	jclass clazz;
    __android_log_print(ANDROID_LOG_DEBUG,LOG_TAG,"Native registration find class %s", class_name);
	clazz = (*env)->FindClass(env, class_name);
	if (clazz == NULL) {
        __android_log_print(ANDROID_LOG_ERROR,LOG_TAG,"Native registration unable to find class %s\n", class_name);
		return JNI_FALSE;
	}

	if ((*env)->RegisterNatives(env, clazz, methods, num_methods) < 0) {
        __android_log_print(ANDROID_LOG_ERROR,LOG_TAG,"RegisterNatives failed for %s\n", class_name);
		return JNI_FALSE;
	}

	return JNI_TRUE;
}

void throw_exception(JNIEnv *env, const char * exception_class_path_name,
		const char *msg) {
	jclass newExcCls = (*env)->FindClass(env, exception_class_path_name);
	if (newExcCls == NULL) {
		assert(FALSE);
	}
	(*env)->ThrowNew(env, newExcCls, msg);
	(*env)->DeleteLocalRef(env, newExcCls);
}

void throw_interrupted_exception(JNIEnv *env, const char * msg) {
	throw_exception(env, interrupted_exception_class_path_name, msg);
}

void throw_runtime_exception(JNIEnv *env, const char * msg) {
	throw_exception(env, runtime_exception_class_path_name, msg);
}
