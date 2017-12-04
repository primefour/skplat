#include"SharedBuffer.h"
#include"Log.h"

int main(){
    int size = 1024;
    SharedBuffer* sb = SharedBuffer::alloc(size);
    ALOGD("SB REF %d ",sb->onlyOwner());
    char *data =(char *)sb->data();

    const SharedBuffer*sb2 =SharedBuffer::sharedBuffer((const void *)data);
    sb2->acquire();
    ALOGD("SB REF %d ",sb->onlyOwner());
}
