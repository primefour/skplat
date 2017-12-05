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

// #define LOG_NDEBUG 0
#define LOG_TAG "libutils.threads"

#include "Threads.h"
#include "Log.h"

#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <errno.h>
#include <assert.h>
#include <unistd.h>

#if defined(HAVE_PTHREADS)
# include <unistd.h>
# include <pthread.h>
# include <sched.h>
# include <sys/resource.h>
# include <sys/types.h>

#include <unistd.h>
#include <sys/syscall.h>
#define gettid() syscall(SYS_gettid)

#endif

#if defined(HAVE_PRCTL)
#include <sys/prctl.h>
#endif

/*
 * ===========================================================================
 *      Thread wrappers
 * ===========================================================================
 */
// ----------------------------------------------------------------------------
#if defined(HAVE_PTHREADS)
// ----------------------------------------------------------------------------

/*
 * Create and run a new thread.
 *
 * We create it "detached", so it cleans up after itself.
 */

typedef void* (*android_pthread_entry)(void*);
typedef int   (*thread_func_t)(void *);

pid_t getTid()
{
#ifdef HAVE_GETTID
    return gettid();
#else
    return getpid();
#endif
}

pthread_t getThreadId()
{
    return (pthread_t)pthread_self();
}

struct thread_data_t {
    thread_func_t   entryFunction;
    void*           userData;
    int             priority;
    char *          threadName;

    // we use this trampoline when we need to set the priority with
    // nice/setpriority, and name with prctl.
    static int trampoline(const thread_data_t* t) {
        thread_func_t f = t->entryFunction;
        void* u = t->userData;
        int prio = t->priority;
        char * name = t->threadName;
        delete t;
        setpriority(PRIO_PROCESS, 0, prio);
        
        if (name) {
#if defined(HAVE_PRCTL)
            // Mac OS doesn't have this, and we build libutil for the host too
            int hasAt = 0;
            int hasDot = 0;
            char *s = name;
            while (*s) {
                if (*s == '.') hasDot = 1;
                else if (*s == '@') hasAt = 1;
                s++;
            }
            int len = s - name;
            if (len < 15 || hasAt || !hasDot) {
                s = name;
            } else {
                s = name + len - 15;
            }
            prctl(PR_SET_NAME, (unsigned long) s, 0, 0, 0);
#endif
            free(name);
        }
        return f(u);
    }
};

int createRawThreadEtc(thread_func_t entryFunction,
                               void *userData,
                               const char* threadName,
                               int32_t threadPriority,
                               size_t threadStackSize,
                               pthread_t *threadId)
{
    pthread_attr_t attr; 
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

/* valgrind is rejecting RT-priority create reqs */
    if (threadPriority != PRIORITY_DEFAULT || threadName != NULL) {
        // Now that the pthread_t has a method to find the associated
        // android_thread_id_t (pid) from pthread_t, it would be possible to avoid
        // this trampoline in some cases as the parent could set the properties
        // for the child.  However, there would be a race condition because the
        // child becomes ready immediately, and it doesn't work for the name.
        // prctl(PR_SET_NAME) only works for self; prctl(PR_SET_THREAD_NAME) was
        // proposed but not yet accepted.
        thread_data_t* t = new thread_data_t;
        t->priority = threadPriority;
        t->threadName = threadName ? strdup(threadName) : NULL;
        t->entryFunction = entryFunction;
        t->userData = userData;
        entryFunction = (thread_func_t)&thread_data_t::trampoline;
        userData = t;            
    }

    if (threadStackSize) {
        pthread_attr_setstacksize(&attr, threadStackSize);
    }
    
    errno = 0;
    pthread_t thread;
    int result = pthread_create(&thread, &attr,
                    (android_pthread_entry)entryFunction, userData);
    pthread_attr_destroy(&attr);
    if (result != 0) {
        ALOGE("CreateRawThreadEtc failed (entry=%p, res=%d, errno=%d)\n"
             "(android threadPriority=%d)",
            entryFunction, result, errno, threadPriority);
        return 0;
    }

    // Note that *threadID is directly available to the parent only, as it is
    // assigned after the child starts.  Use memory barrier / lock if the child
    // or other threads also need access.
    if (threadId != NULL) {
        *threadId = (pthread_t)thread; // XXX: this is not portable
    }
    return 1;
}


#else
#error "Threads not supported"
#endif

// ----------------------------------------------------------------------------
//
int createThreadEtc(thread_func_t entryFunction,
                            void *userData,
                            const char* threadName,
                            int32_t threadPriority,
                            size_t threadStackSize,
                            pthread_t *threadId)
{
    return createRawThreadEtc(entryFunction, userData, threadName,
        threadPriority, threadStackSize, threadId);
}

int createThreadGetID(thread_func_t fn, void *arg, pthread_t*id)
{
    return createThreadEtc(fn, arg, "unnamed_thread",
                           PRIORITY_DEFAULT, 0, id);
}


int createThread(thread_func_t fn, void* arg)
{
    return createThreadGetID(fn, arg,NULL);
}

int setThreadPriority(pid_t tid, int pri)
{
    int rc = 0;
#if defined(HAVE_PTHREADS)
    int lasterr = 0;

    if (rc) {
        lasterr = errno;
    }

    if (setpriority(PRIO_PROCESS, tid, pri) < 0) {
        rc = INVALID_OPERATION;
    } else {
        errno = lasterr;
    }
#endif
    
    return rc;
}

int getThreadPriority(pid_t tid) {
#if defined(HAVE_PTHREADS)
    return getpriority(PRIO_PROCESS, tid);
#else
    return ANDROID_PRIORITY_NORMAL;
#endif
}


// ----------------------------------------------------------------------------

/*
 * This is our thread object!
 */

Thread::Thread(bool canCallJava)
    :   mCanCallJava(canCallJava),
        mThread(pthread_t(-1)),
        mLock("Thread::mLock"),
        mStatus(NO_ERROR),
        mExitPending(false), mRunning(false) ,mTid(-1)
{
}

Thread::~Thread()
{
}

status_t Thread::readyToRun()
{
    return NO_ERROR;
}

status_t Thread::run(const char* name, int32_t priority, size_t stack)
{
    Mutex::Autolock _l(mLock);

    if (mRunning) {
        // thread already started
        return INVALID_OPERATION;
    }

    // reset status and exitPending to their default value, so we can
    // try again after an error happened (either below, or in readyToRun())
    mStatus = NO_ERROR;
    mExitPending = false;
    mThread = pthread_t(-1);
    
    // hold a strong reference on ourself
    mHoldSelf = this;

    mRunning = true;

    bool res;
    if (mCanCallJava) {
        res = createThreadEtc(_threadLoop,
                this, name, priority, stack, &mThread);
    } else {
        res = createRawThreadEtc(_threadLoop,
                this, name, priority, stack, &mThread);
    }
    
    if (res == false) {
        mStatus = UNKNOWN_ERROR;   // something happened!
        mRunning = false;
        mThread = pthread_t(-1);
        mHoldSelf.clear();  // "this" may have gone away after this.

        return UNKNOWN_ERROR;
    }
    
    // Do not refer to mStatus here: The thread is already running (may, in fact
    // already have exited with a valid mStatus result). The NO_ERROR indication
    // here merely indicates successfully starting the thread and does not
    // imply successful termination/execution.
    return NO_ERROR;

    // Exiting scope of mLock is a memory barrier and allows new thread to run
}

int Thread::_threadLoop(void* user)
{
    Thread* const self = static_cast<Thread*>(user);

    sp<Thread> strong(self->mHoldSelf);
    wp<Thread> weak(strong);
    self->mHoldSelf.clear();

    // this is very useful for debugging with gdb
    self->mTid = ::getTid();

    bool first = true;

    do {
        bool result;
        if (first) {
            first = false;
            self->mStatus = self->readyToRun();
            result = (self->mStatus == NO_ERROR);

            if (result && !self->exitPending()) {
                // Binder threads (and maybe others) rely on threadLoop
                // running at least once after a successful ::readyToRun()
                // (unless, of course, the thread has already been asked to exit
                // at that point).
                // This is because threads are essentially used like this:
                //   (new ThreadSubclass())->run();
                // The caller therefore does not retain a strong reference to
                // the thread and the thread would simply disappear after the
                // successful ::readyToRun() call instead of entering the
                // threadLoop at least once.
                result = self->threadLoop();
            }
        } else {
            result = self->threadLoop();
        }
        ALOGD("result = %d ",result);

        // establish a scope for mLock
        {
            Mutex::Autolock _l(self->mLock);
            if (result == false || self->mExitPending) {
                self->mExitPending = true;
                self->mRunning = false;
                // clear thread ID so that requestExitAndWait() does not exit if
                // called by a new thread using the same thread ID as this one.
                self->mThread = pthread_t(-1);
                // note that interested observers blocked in requestExitAndWait are
                // awoken by broadcast, but blocked on mLock until break exits scope
                self->mThreadExitedCondition.broadcast();
                break;
            }
        }
        
        // Release our strong reference, to let a chance to the thread
        // to die a peaceful death.
        ALOGD("strong point begin clear"); 
        strong.clear();
        ALOGD("strong point end clear"); 
        // And immediately, re-acquire a strong reference for the next loop
        strong = weak.promote();
        if(strong  == NULL){
            ALOGW("weak promote fail");
            Mutex::Autolock _l(self->mLock);
            self->mExitPending = true;
            self->mRunning = false;
            // clear thread ID so that requestExitAndWait() does not exit if
            // called by a new thread using the same thread ID as this one.
            self->mThread = pthread_t(-1);
            // note that interested observers blocked in requestExitAndWait are
            // awoken by broadcast, but blocked on mLock until break exits scope
            self->mThreadExitedCondition.broadcast();
        }
    } while(strong != 0); 
    return 0;
}

void Thread::requestExit()
{
    Mutex::Autolock _l(mLock);
    mExitPending = true;
}

status_t Thread::requestExitAndWait()
{
    Mutex::Autolock _l(mLock);
    if (mThread == getThreadId()) {
        ALOGW(
        "Thread (this=%p): don't call waitForExit() from this "
        "Thread object's thread. It's a guaranteed deadlock!",
        this);

        return WOULD_BLOCK;
    }
    
    mExitPending = true;

    while (mRunning == true) {
        mThreadExitedCondition.wait(mLock);
    }
    // This next line is probably not needed any more, but is being left for
    // historical reference. Note that each interested party will clear flag.
    mExitPending = false;

    return mStatus;
}

status_t Thread::join()
{
    Mutex::Autolock _l(mLock);
    if (mThread == getThreadId()) {
        ALOGW(
        "Thread (this=%p): don't call join() from this "
        "Thread object's thread. It's a guaranteed deadlock!",
        this);

        return WOULD_BLOCK;
    }

    while (mRunning == true) {
        mThreadExitedCondition.wait(mLock);
    }

    return mStatus;
}

#ifdef HAVE_ANDROID_OS
pid_t Thread::getTid() const
{
    // mTid is not defined until the child initializes it, and the caller may need it earlier
    Mutex::Autolock _l(mLock);
    pid_t tid;
    if (mRunning) {
        pthread_t pthread = android_thread_id_t_to_pthread(mThread);
        tid = __pthread_gettid(pthread);
    } else {
        ALOGW("Thread (this=%p): getTid() is undefined before run()", this);
        tid = -1;
    }
    return tid;
}
#endif

bool Thread::exitPending() const
{
    Mutex::Autolock _l(mLock);
    return mExitPending;
}

