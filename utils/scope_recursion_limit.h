#ifndef __SCOPE_RECURSION_LIMIT_H__
#define __SCOPE_RECURSION_LIMIT_H__
#include "threadprivate.h"

class ScopeRecursionLimit {
    public:
        ScopeRecursionLimit(ThreadPrivate *_tp): tp_(_tp) {
            tp_->set((void*)((intptr_t)tp_->get() + 1));
        }

        ~ScopeRecursionLimit() {
            tp_->set((void*)((intptr_t)tp_->get() - 1));
        }

        /**
         * return
         *     true-limit
         *     false-pass
         */
        bool CheckLimit(int _maxRecursionNum = 1) {
            return (intptr_t)tp_->get() > _maxRecursionNum;
        }

        intptr_t Get() const {
            return (intptr_t)tp_->get();
        }

    private:
        ScopeRecursionLimit(const ScopeRecursionLimit&);
        ScopeRecursionLimit& operator=(const ScopeRecursionLimit&);

    private:
        ThreadPrivate * tp_;
};

#ifndef __CONCAT__
#define __CONCAT_IMPL(x, y)    x##y
#define __CONCAT__(x, y)    __CONCAT_IMPL(x, y)
#endif

#define    DEFINE_SCOPERECURSIONLIMIT(classname)\
    static ThreadPrivate __CONCAT__(xtpp, __LINE__)(NULL);\
    ScopeRecursionLimit classname(&__CONCAT__(xtpp, __LINE__))


#endif //__SCOPE_RECURSION_LIMIT_H__
