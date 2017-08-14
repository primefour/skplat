#ifndef __THREAD_PRIVATE_H__ 
#define __THREAD_PRIVATE_H__ 
#include <pthread.h>
typedef void (*cleanup_route)(void*);
class ThreadPrivate {
    public:
        explicit ThreadPrivate(cleanup_route cleanup) {
            pthread_key_create(&_key, cleanup);
        }

        ~ThreadPrivate() {
            pthread_key_delete(_key);
        }

        void* get() const {
            return pthread_getspecific(_key);
        }

        void set(void* value) {
            pthread_setspecific(_key, value);
        }

    private:
        ThreadPrivate(const Tss&);
        ThreadPrivate& operator =(const ThreadPrivate&);
    private:
        pthread_key_t _key;
};

#endif //__THREAD_PRIVATE_H__ 
