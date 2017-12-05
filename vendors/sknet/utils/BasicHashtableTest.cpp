#include"BasicHashtable.h"
#include"Log.h"


class IntEntry{
    public:
        int value;
        int getKey() const{
            return value;
        }
        IntEntry(int v):value(v){
            ALOGD("IntEntry constructor value %d ",value);
        }
        IntEntry(const IntEntry &e){
            value = e.value;
            ALOGD("IntEntry copy constructor value %d ",e.value);
        }
        ~IntEntry(){
            ALOGD("IntEntry decontructor value %d ",value);
        }
};

int main(){
    BasicHashtable<int,IntEntry> intHash(100,0.5) ;
    ALOGD("CREATE INT ENTYR ");
    IntEntry a(1);
    ALOGD("ADD INT ENTYR ");
    int idxa = intHash.add(1,a);
    ALOGD("ADD INT ENTYR idxa = %d ",idxa);
    ALOGD("CREATE INT ENTYR ");
    IntEntry b(101);
    ALOGD("ADD INT ENTYR ");
    int idxb = intHash.add(301,b);
    ALOGD("ADD INT ENTYR idxb = %d ",idxb );
    const IntEntry &x = intHash.entryAt(idxb);
    ALOGD("GET VALUE OF X : %d ",x.getKey());
    return 0;
}
