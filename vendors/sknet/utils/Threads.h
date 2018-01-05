/*
 * Copyright (C) 2007 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef _LIBS_UTILS_THREAD_H
#define _LIBS_UTILS_THREAD_H

#include <stdint.h>
#include <sys/types.h>
#include <time.h>

#if defined(HAVE_PTHREADS)
# include <pthread.h>
#endif

#include "Condition.h"
#include "Mutex.h"
#include "Timers.h"
#include <stdint.h>
#include <sys/types.h>
#include "RefBase.h"

// ---------------------------------------------------------------------------
// C API

#ifdef __cplusplus
extern "C" {
#endif

enum {
    /*
     * ***********************************************
     * ** Keep in sync with android.os.Process.java **
     * ***********************************************
     * 
     * This maps directly to the "nice" priorities we use in Android.
     * A thread priority should be chosen inverse-proportionally to
     * the amount of work the thread is expected to do. The more work
     * a thread will do, the less favorable priority it should get so that 
     * it doesn't starve the system. Threads not behaving properly might
     * be "punished" by the kernel.
     * Use the levels below when appropriate. Intermediate values are
     * acceptable, preferably use the {MORE|LESS}_FAVORABLE constants below.
     */
    ANDROID_PRIORITY_LOWEST         =  19,

    /* use for background tasks */
    ANDROID_PRIORITY_BACKGROUND     =  10,
    
    /* most threads run at normal priority */
    ANDROID_PRIORITY_NORMAL         =   0,
    
    /* threads currently running a UI that the user is interacting with */
    ANDROID_PRIORITY_FOREGROUND     =  -2,

    /* the main UI thread has a slightly more favorable priority */
    ANDROID_PRIORITY_DISPLAY        =  -4,
    
    /* ui service treads might want to run at a urgent display (uncommon) */
    ANDROID_PRIORITY_URGENT_DISPLAY =  -13,
    
    /* all normal audio threads */
    ANDROID_PRIORITY_AUDIO          = -16,
    
    /* service audio threads (uncommon) */
    ANDROID_PRIORITY_URGENT_AUDIO   = -19,

    /* should never be used in practice. regular process might not 
     * be allowed to use this level */
    ANDROID_PRIORITY_HIGHEST        = -20,

    ANDROID_PRIORITY_DEFAULT        = ANDROID_PRIORITY_NORMAL,
    ANDROID_PRIORITY_MORE_FAVORABLE = -1,
    ANDROID_PRIORITY_LESS_FAVORABLE = +1,
};

#ifdef __cplusplus
} // extern "C"
#endif

// ---------------------------------------------------------------------------
// C++ API
#ifdef __cplusplus
// ---------------------------------------------------------------------------
enum {
    PRIORITY_LOWEST         = ANDROID_PRIORITY_LOWEST,
    PRIORITY_BACKGROUND     = ANDROID_PRIORITY_BACKGROUND,
    PRIORITY_NORMAL         = ANDROID_PRIORITY_NORMAL,
    PRIORITY_FOREGROUND     = ANDROID_PRIORITY_FOREGROUND,
    PRIORITY_DISPLAY        = ANDROID_PRIORITY_DISPLAY,
    PRIORITY_URGENT_DISPLAY = ANDROID_PRIORITY_URGENT_DISPLAY,
    PRIORITY_AUDIO          = ANDROID_PRIORITY_AUDIO,
    PRIORITY_URGENT_AUDIO   = ANDROID_PRIORITY_URGENT_AUDIO,
    PRIORITY_HIGHEST        = ANDROID_PRIORITY_HIGHEST,
    PRIORITY_DEFAULT        = ANDROID_PRIORITY_DEFAULT,
    PRIORITY_MORE_FAVORABLE = ANDROID_PRIORITY_MORE_FAVORABLE,
    PRIORITY_LESS_FAVORABLE = ANDROID_PRIORITY_LESS_FAVORABLE,
};
#endif  // __cplusplus
// ---------------------------------------------------------------------------

class Thread : virtual public RefBase
{
public:
    // Create a Thread object, but doesn't create or start the associated
    // thread. See the run() method.
                        Thread(bool canCallJava = true);
    virtual             ~Thread();

    // Start the thread in threadLoop() which needs to be implemented.
    virtual status_t    run(    const char* name = 0,
                                int32_t priority = PRIORITY_DEFAULT,
                                size_t stack = 0);
    
    // Ask this object's thread to exit. This function is asynchronous, when the
    // function returns the thread might still be running. Of course, this
    // function can be called from a different thread.
    virtual void        requestExit();

    // Good place to do one-time initializations
    virtual status_t    readyToRun();
    
    // Call requestExit() and wait until this object's thread exits.
    // BE VERY CAREFUL of deadlocks. In particular, it would be silly to call
    // this function from this object's thread. Will return WOULD_BLOCK in
    // that case.
            status_t    requestExitAndWait();

    // Wait until this object's thread exits. Returns immediately if not yet running.
    // Do not call from this object's thread; will return WOULD_BLOCK in that case.
            status_t    join();

#ifdef HAVE_ANDROID_OS
    // Return the thread's kernel ID, same as the thread itself calling gettid() or
    // androidGetTid(), or -1 if the thread is not running.
            pid_t       getTid() const;
#endif

protected:
    // exitPending() returns true if requestExit() has been called.
            bool        exitPending() const;
    
private:
    // Derived class must implement threadLoop(). The thread starts its life
    // here. There are two ways of using the Thread object:
    // 1) loop: if threadLoop() returns true, it will be called again if
    //          requestExit() wasn't called.
    // 2) once: if threadLoop() returns false, the thread will exit upon return.
    virtual bool        threadLoop() = 0;

private:
    Thread& operator=(const Thread&);
    static  int             _threadLoop(void* user);
    const   bool            mCanCallJava;
    // always hold mLock when reading or writing
            pthread_t       mThread;
    mutable Mutex           mLock;
            Condition       mThreadExitedCondition;
            status_t        mStatus;
    // note that all accesses of mExitPending and mRunning need to hold mLock
    volatile bool           mExitPending;
    volatile bool           mRunning;
            sp<Thread>      mHoldSelf;
    // legacy for debugging, not used by getTid() as it is set by the child thread
    // and so is not initialized until the child reaches that point
            pid_t           mTid;
};


// ---------------------------------------------------------------------------
#endif // _LIBS_UTILS_THREAD_H
// ---------------------------------------------------------------------------
