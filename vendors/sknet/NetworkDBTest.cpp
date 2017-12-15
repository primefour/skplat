#include "NetworkDatabase.h"
#include "Log.h"

int main(){
    ALOGD("Hello world");
    //sp<NetworkDatabase> netDb = NULL;
    NetworkDatabase::getInstance();
    //ASSERT(netDb == NULL,"net database create fail");
    //netDb->incStrong(NULL);
    return 0;
}
