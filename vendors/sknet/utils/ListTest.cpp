#include"List.h"
#include"Log.h"
#include"RefBase.h"

class TestClass{
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

int main(){
    List<TestClass> testList ;
    TestClass a;
    testList.push_back(a);
    ALOGD("sizeof list is %zd ",testList.size());
    testList.push_back(a);
    ALOGD("sizeof list is %zd ",testList.size());

    List<TestClass>::iterator it = testList.begin();
    List<TestClass>::iterator end = testList.end();

    while(it != end){
        it->TestFunc();
        it ++;
    }

    return 0;
}
