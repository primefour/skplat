#include"Atomic.h"
#include"Log.h"


void TestInc(){
    volatile int count = 0;
    int ret = android_atomic_inc(&count);
    ASSERT((ret+1) == count,"android_atomic_inc result fail");
}

void TestDec(){
    volatile int count = 0;
    int ret = android_atomic_dec(&count);
    ASSERT((ret-1) == count,"android_atomic_dec result fail");
}

void Testadd(){
    volatile int count = 0;
    int ret = android_atomic_add(3,&count);
    ASSERT((ret+3) == count,"android_atomic_add result fail");
}


void TestOr(){
    volatile int flags = 0;
    int flagsMask = 0x01;
    android_atomic_or(flagsMask,&flags);
    ASSERT((flags&flagsMask) == flagsMask,"android_atomic_or result fail");
}

void TestCmpxchg(){
    int oldvalue  = 1;
    int op2 = 1;
    int ret = android_atomic_cmpxchg(oldvalue,oldvalue + 1,&op2);
    ASSERT(ret,"android_atomic_cmpxchg test fail");
    ASSERT(oldvalue +1 == op2,"android_atomic_cmpxchg test fail");
    ret = android_atomic_cmpxchg(oldvalue,oldvalue + 1,&op2);
    ASSERT(!ret,"android_atomic_cmpxchg test fail");
    ASSERT((oldvalue +1 == op2),"android_atomic_cmpxchg test fail");
}


int main(){
    TestInc();
    TestDec();
    TestOr();
    TestCmpxchg();
    return 0;
}
