#ifndef LOCK_H_
#define LOCK_H_

#include <unistd.h>
#include "__assert.h"
#include "mutex.h"

template <typename MutexType>
class BaseScopedLock {
  public:
    explicit BaseScopedLock(MutexType& mutex, bool initiallyLocked = true)
        : mutex_(mutex) , islocked_(false) {
        if (!initiallyLocked) {
            return;
        }
        lock();
    }

    explicit BaseScopedLock(MutexType& mutex, long _millisecond) : mutex_(mutex) , islocked_(false) {
        timedlock(_millisecond);
    }

    ~BaseScopedLock() {
        if (islocked_) {
            unlock();
        }
    }

    bool islocked() const {
        return islocked_;
    }

    void lock() {
        ASSERT(!islocked_);

        if (!islocked_ && mutex_.lock()) {
            islocked_ = true;
        }

        ASSERT(islocked_);
    }

    void unlock() {
        ASSERT(islocked_);

        if (islocked_) {
            mutex_.unlock();
            islocked_ = false;
        }
    }

    bool trylock() {
        if (islocked_) return false;

        islocked_ = mutex_.trylock();
        return islocked_;
    }

    bool timedlock(long _millisecond) {
        ASSERT(!islocked_);

        if (islocked_) {
            return true;
        }
        islocked_ = mutex_.timedlock(_millisecond);
        return islocked_;
    }

    MutexType& internal() {
        return mutex_;
    }

  private:
    MutexType& mutex_;
    bool islocked_;
};

typedef BaseScopedLock<Mutex> ScopedLock;

#endif /* LOCK_H_ */
