#include <jni.h>
#include <android/log.h>
/*standard library*/
#include <time.h>
#include <math.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <pthread.h>
#include <unistd.h>
#include <assert.h>
#include <vector>
#include "var_cache.h"
#include "scope_jenv.h"
#include "JNI_OnLoad.h"

extern "C" {
    /*
extern int RegisterSklog(JNIEnv* env);
extern int RegisterKXAlarm(JNIEnv* env);
extern int RegisterABEvent(JNIEnv* env);
extern int RegisterND(JNIEnv* env);
*/

pthread_key_t g_env_key;
pthread_once_t once_control=PTHREAD_ONCE_INIT;

static void __DetachCurrentThread(void* a) {
    if (NULL != VarCache::Singleton()->GetJvm()) {
        VarCache::Singleton()->GetJvm()->DetachCurrentThread();
    }
}

void once_run(void) {
    if (0 != pthread_key_create(&g_env_key, __DetachCurrentThread)) {
        __android_log_print(ANDROID_LOG_ERROR, "skplat", "create g_env_key fail");
    }
}  

JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM *jvm, void *reserved) {
	JNIEnv* env = NULL;
	if (jvm->GetEnv((void**) &env, JNI_VERSION_1_6) != JNI_OK) {
		__android_log_print(ANDROID_LOG_ERROR,"skplat","ERROR: GetEnv failed\n");
        return JNI_VERSION_1_6;
	}
    pthread_once(&once_control,once_run);
    ScopeJEnv jenv(jvm);
    VarCache::Singleton()->SetJvm(jvm);

    /*
    RegisterSklog(env);
    RegisterKXAlarm(env);
    RegisterABEvent(env);
    RegisterND(env);
    */

    LoadClass(jenv.GetEnv());
    LoadStaticMethod(jenv.GetEnv());
    LoadMethod(jenv.GetEnv());


    std::vector<JniOnload_t>& ref = BootRegister_Container<JniOnload_t>() ;
    for (std::vector<JniOnload_t>::const_iterator it= ref.begin(); it!=ref.end(); ++it){
        it->func(jvm, reserved);
    }
    return JNI_VERSION_1_6;
}

JNIEXPORT void JNICALL JNI_OnUnload(JavaVM* vm, void* reserved) {
    VarCache::Release();
}
}


