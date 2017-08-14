/*******************************************************************************
 **      Filename: utils/jni/native_help.h
 **        Author: crazyhorse                  
 **   Description: ---
 **        Create: 2017-03-30 11:31:02
 ** Last Modified: 2017-03-30 11:31:02
 ******************************************************************************/
#ifndef _NATIVE_HELPERS_H_
#define _NATIVE_HELPERS_H_

#include <jni.h>
#ifndef NELEM
#define NELEM(x) ((int)(sizeof(x) / sizeof((x)[0])))
#endif

typedef struct {
    const char* name;
    const char* signature;
} JavaMethod;

typedef struct {
    char* name;
    char* signature;
} JavaField;

jfieldID java_get_field(JNIEnv *env, char * class_name, JavaField field);
jmethodID java_get_method(JNIEnv *env, jclass java_class, JavaMethod method);
int register_native_methods(JNIEnv* env, const char* class_name, 
                                JNINativeMethod* methods, int num_methods); 
void throw_exception(JNIEnv *env, const char * exception_class_path_name,const char *msg);
void throw_interrupted_exception(JNIEnv *env, const char * msg);
void throw_runtime_exception(JNIEnv *env, const char * msg); 
void detach_current_thread();
JNIEnv *get_valid_env();


#endif /* HELPERS_H_ */

