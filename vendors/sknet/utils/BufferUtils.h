#ifndef __BUFFER_UTILS_H__
#define __BUFFER_UTILS_H__
#include"RefBase.h"
const int BUFFER_UTILS_DEFAULT_SIZE = 1024;
class BufferUtils :public RefBase{
    public:
        BufferUtils(int capacity = BUFFER_UTILS_DEFAULT_SIZE);
        virtual ~BufferUtils();
        ssize_t setCapacity(size_t new_capacity) ;
        int write(const char *buff,size_t size);
        size_t offset(int offset = 0,int seekWhere = SEEK_CUR);
        size_t curPostion()const;
        size_t read(char *buff,size_t size);
        size_t append(const char *data,size_t size);
        size_t append(BufferUtils& buffer);
        void setTo(const BufferUtils& buffer);
        void setTo(char *data,size_t size);
        const char* data() const;
        char* data() ;
        const char *dataWithOffset()const;
        char* dataWithOffset();
        size_t size() const;
        size_t capacity() const;
        void release();
        BufferUtils(const BufferUtils &a);
        void operator=(const  BufferUtils&a );
        size_t consume(long size);
    private:
        //storage to save data
        void* mBuffer; //this is sharedBuffer buffer
        //where begin to write or read 
        size_t mOffset;
        //data size
        size_t mSize;
        //buffer size
        size_t mCapacity;
};

#endif
