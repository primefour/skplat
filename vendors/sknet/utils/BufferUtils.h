#ifndef __BUFFER_UTILS_H__
#define __BUFFER_UTILS_H__
#include"RefBase.h"
const int BUFFER_UTILS_DEFAULT_SIZE = 1024;
class BufferUtils :public RefBase{
    public:
        enum{
            SEEK_SET,
            SEEK_CUR,
            SEEK_END,
        }
        BufferUtils(int capacity = BUFFER_UTILS_DEFAULT_SIZE);
        virtual ~BufferUtils();

        ssize_t setCapacity(size_t new_capacity) ;
        int write(char *buff,size_t size);
        size_t offset(int offset,int seekWhere);
        size_t read(char *buff,size_t size);
        size_t append(char *data,size_t size);
        size_t append(const BufferUtils& buffer);
        void setTo(SharedBuffer& buffer);
        void setTo(char *data,size_t size);
        const char* BufferUtils::data();
        size_t size();
        size_t capacity();
        void release();

    private:
        //storage to save data
        void*  mBuffer; //this is sharedBuffer buffer
        //where begin to write or read 
        size_t mOffset;
        //data size
        size_t mSize;
        //buffer size
        size_t mCapacity;
};

#endif
