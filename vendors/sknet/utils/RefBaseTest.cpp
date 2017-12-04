#include"Log.h"
#include"RefBase.h"
#include"StrongPointer.h"


class TestClass:public RefBase{
    public:
        TestClass() {
            ALOGD("TestClass construction %p ",this);
        }

        virtual ~TestClass(){
            ALOGD("TestClass desconstruction %p ",this);
        }
        void TestFunc(){
            ALOGD("test function %p %d ",this,__LINE__);
        }
};


class TestClass2:public RefBase{
    public:
        TestClass2(){
            ALOGD("TestClass2 construction %p ",this);
            extendObjectLifetime(OBJECT_LIFETIME_WEAK); 
        }

        virtual ~TestClass2(){
            ALOGD("TestClass2 desconstruction %p ",this);
        }
        void TestFunc(){
            ALOGD("test function %p %d ",this,__LINE__);
        }
};

int main(){
    ALOGD("#######################################################");
    {
        //test sp
        sp<TestClass> tc = new TestClass();
        tc->TestFunc();
        TestClass *tc2 = tc.get();
        tc2->TestFunc();
    }


    ALOGD("#######################################################");
    {
        //test wp
        wp<TestClass> tb = new TestClass();
        TestClass *tc3 = tb.promote().get();
        if(tc3  != NULL){
            tc3->TestFunc();
        }
    }

    ALOGD("#######################################################");
    {
        //test sp
        sp<TestClass2> tc = new TestClass2();
        tc->TestFunc();
        TestClass2 *tc2 = tc.get();
        tc2->TestFunc();
    }

    ALOGD("#######################################################");

    {
        //test wp
        wp<TestClass2> tb = new TestClass2();
        TestClass2 *tc3 = tb.promote().get();
        if(tc3  != NULL){
            tc3->TestFunc();
        }
    }

    ALOGD("xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx");
    return 0;

}
