#include <jni.h>
#include <vector>
#include <string>
#include "sklog.h"
#include "logbase.h"
#include "scoped_jstring.h"
#include "var_cache.h"
#include <android/log.h>

/*for android logs*/
#define LOG_TAG "android_log"
#define ALOGI(...) {__android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__);}
#define ALOGE(...) {__android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__);}

#define LONGTHREADID2INT(a) ((a >> 32)^((a & 0xFFFF)))

DEFINE_FIND_CLASS(KXlog, "com/skplat/LogImpl")

extern "C" {
#include "native_helper.h"

JNIEXPORT void JNICALL jni_sklog_init(JNIEnv *env, jclass, jint level, jint mode, 
                            jstring _cache_dir, jstring _log_dir, jstring _nameprefix) {
	if (NULL == _log_dir || NULL == _nameprefix) {
		return;
	}
	std::string cache_dir;
	if (NULL != _cache_dir) {
		ScopedJstring cache_dir_jstr(env, _cache_dir);
		cache_dir = cache_dir_jstr.GetChar();
	}

	ScopedJstring log_dir_jstr(env, _log_dir);
	ScopedJstring nameprefix_jstr(env, _nameprefix);
	log_file_init(log_dir_jstr.GetChar(), nameprefix_jstr.GetChar());
	log_set_level((LogLevel)level);

}

JNIEXPORT void JNICALL jni_sklog_close(JNIEnv *env, jobject) {
	log_file_close();
}

JNIEXPORT void JNICALL jni_sklog_flush(JNIEnv *env, jobject) {
    log_flush();
}

JNIEXPORT void JNICALL jni_log_write(JNIEnv *env, jclass, int _level, jstring _tag, jstring _filename,
		  jstring _funcname, jint _line, jint _pid, jlong _tid, jlong _maintid, jstring _log) {

	if (!log_for((LogLevel)_level)) {
		return;
	}

	LogInfo log_info;
	gettimeofday(&log_info.timeval, NULL);
	log_info.level = (LogLevel)_level;
	log_info.line = (int)_line;
	log_info.pid = (int)_pid;
	log_info.tid = LONGTHREADID2INT(_tid);
	log_info.maintid = LONGTHREADID2INT(_maintid);

	const char* tag_cstr = NULL;
	const char* filename_cstr = NULL;
	const char* funcname_cstr = NULL;
	const char* log_cstr = NULL;

	if (NULL != _tag) {
		tag_cstr = env->GetStringUTFChars(_tag, NULL);
	}

	if (NULL != _filename) {
		filename_cstr = env->GetStringUTFChars(_filename, NULL);
	}

	if (NULL != _funcname) {
		funcname_cstr = env->GetStringUTFChars(_funcname, NULL);
	}

	if (NULL != _log) {
		log_cstr = env->GetStringUTFChars(_log, NULL);
	}

	log_info.tag = NULL == tag_cstr ? "" : tag_cstr;
	log_info.filename = NULL == filename_cstr ? "" : filename_cstr;
	log_info.func_name = NULL == funcname_cstr ? "" : funcname_cstr;
	log_write(&log_info, NULL == log_cstr ? "NULL == log" : log_cstr);

	if (NULL != _tag) {
		env->ReleaseStringUTFChars(_tag, tag_cstr);
	}

	if (NULL != _filename) {
		env->ReleaseStringUTFChars(_filename, filename_cstr);
	}

	if (NULL != _funcname) {
		env->ReleaseStringUTFChars(_funcname, funcname_cstr);
	}

	if (NULL != _log) {
		env->ReleaseStringUTFChars(_log, log_cstr);
	}
}

JNIEXPORT jint JNICALL jni_get_log_level(JNIEnv *, jobject) {
	return log_get_level();
}

JNIEXPORT void jni_set_log_level(JNIEnv *, jclass, jint _log_level) {
    ALOGI("log_level : %d ",_log_level);
	log_set_level((LogLevel)_log_level);
}

JNIEXPORT void JNICALL jni_set_log_mode(JNIEnv *, jclass, jint _mode) {

}

JNIEXPORT void JNICALL jni_set_console_log_open(JNIEnv *env, jclass, jboolean _is_open) {
    ALOGI("console log:%d ",_is_open);
    log_set_console_enable((bool)_is_open);
}

static JNINativeMethod sklog_methods[] ={
    {"logInit", "(IILjava/lang/String;Ljava/lang/String;Ljava/lang/String;)V",(void*)jni_sklog_init},
    {"logClose","()V",(void*)jni_sklog_close},
    {"logFlush","()V",(void*)jni_sklog_flush},
    {"logWrite","(ILjava/lang/String;Ljava/lang/String;Ljava/lang/String;IIJJLjava/lang/String;)V",(void*)jni_log_write},
    {"getLogLevel","()I", (void*)jni_get_log_level},
    {"setLogLevel","(I)V", (void*)jni_set_log_level},
    {"setLogMode","(I)V", (void*)jni_set_log_mode},
    {"setConsoleLogOpen","(Z)V",(void*)jni_set_console_log_open},
};


int RegisterSklog(JNIEnv* env){
	if (register_native_methods(env,
			KXlog,
			sklog_methods,
            NELEM(sklog_methods)) < 0) {
        ALOGE("ERROR: Exif native registration failed\n");
        throw_exception(env, "java/lang/RuntimeException", "register sklog fail");
        return -1;
    }
    return 0;
}

}
