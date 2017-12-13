#ifndef __BUFFER_UTILS_H__
#define __BUFFER_UTILS_H__
#include"RefBase.h"
class BufferUtils :public RefBase{
    public:
        int write(char *buff,size_t size);
        size_t offset(int offset,int seekWhere);
        int read(char *buff,size_t size);
        int append(char *data,size_t size);
        int append(const BufferUtils& buffer);
    private:
        SharedBuffer mBuffer;
        //where begin to write or read 
        size_t mOffset;
        //data size
        size_t mSize;
        //buffer size
        size_t mCapacity;
};

#endif
