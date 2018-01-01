#include"BufferUtils.h"
#include"SharedBuffer.h"
#include"Log.h"


void BufferUtils::release(){
    if (mBuffer) {
        const SharedBuffer* sb = SharedBuffer::sharedBuffer(mBuffer);
        if (sb->release(SharedBuffer::eKeepStorage) == 1) {
            SharedBuffer::dealloc(sb);
        } 
        mBuffer = NULL;
    }
}

ssize_t BufferUtils::setCapacity(size_t new_capacity) {
    size_t  current_capacity = capacity();
    ssize_t amount = new_capacity - size();
    if (amount <= 0) {
        // we can't reduce the capacity
        return current_capacity;
    } 
    SharedBuffer* sb = SharedBuffer::alloc(new_capacity);
    if (sb) {
        void* array = sb->data();
        memcpy(array,mBuffer,size());
        release();
        mBuffer = const_cast<void*>(array);
    } else {
        return NO_MEMORY;
    }
    return new_capacity;
}


BufferUtils::BufferUtils(int capacity){
    SharedBuffer* sb = SharedBuffer::alloc(capacity);
    if (sb) {
        mBuffer = sb->data();
    }else{
        ASSERT(false,"no memory for buffer utils");
    }
    mOffset = 0;
    mSize = 0;
    mCapacity = capacity;
}

BufferUtils::~BufferUtils(){
    release();
}

int BufferUtils::write(const char *buff,size_t size){
    ASSERT(buff != NULL || size > 0,"buff is NULL");

    if(mOffset + size > mCapacity){
        //enlarge the size of buffer
        size_t resize = mCapacity + mCapacity/2;
        if(resize <= mOffset + size ){
            resize = mCapacity + mCapacity/2 + size ;
        }
        int ret = setCapacity(resize);
        if(ret > 0){
            mCapacity = ret;
        }else{
            ALOGW("No memory for BufferUtils");
            return 0;
        }
    }
    //copy data
    memcpy((char*)mBuffer+mOffset,buff,size);
    if(mSize < mOffset + size){
        mSize = mOffset + size;
        mOffset = mSize;
    }else{
        mOffset += size;
    }
    return size;
}

size_t BufferUtils::offset(int offset,int seekWhere){
    if(seekWhere == SEEK_SET){
        mOffset = offset;
    }else if (seekWhere == SEEK_CUR){
        mOffset += offset;
    }else if(seekWhere == SEEK_END){
        mOffset = mSize - offset;
    }

    if(mOffset < 0){
        mOffset = 0;
    }else if (mOffset > mSize){
        mOffset = mSize;
    }
    return mOffset;
}

size_t BufferUtils::read(char *buff,size_t size){
    ASSERT(buff != NULL || size > 0,"invalidate parameters");
    int cpySize = mSize - mOffset > size ? size:mSize - mOffset;
    memcpy(buff,(char *)mBuffer + mOffset,cpySize);
    mOffset += cpySize;
    return cpySize;
}

size_t BufferUtils::append(const char *data,size_t size){
    ASSERT(data != NULL || size > 0,"data is NULL");
    if(mSize + size > mCapacity){
        //enlarge the size of buffer
        size_t resize = mCapacity + mCapacity/2;
        if(resize <= mSize + size ){
            resize = mCapacity + mCapacity/2 + size ;
        }
        int ret = setCapacity(resize);
        if(ret > 0){
            mCapacity = ret;
        }else{
            ALOGW("No memory for BufferUtils");
            return 0;
        }
    }
    //copy data
    memcpy((char *)mBuffer+mSize,data,size);
    mSize += size;
    return size;
}

size_t BufferUtils::append(BufferUtils& buffer){
    return append(buffer.data(),buffer.size());
}

const char *BufferUtils::dataWithOffset()const{
    return (char *)mBuffer + mOffset;
}

char* BufferUtils::dataWithOffset(){
    return (char *)mBuffer + mOffset;
}

const char* BufferUtils::data()const{
    return (char *)mBuffer;
}

char* BufferUtils::data(){
    return (char *)mBuffer;
}

size_t BufferUtils::size() const{
    return mSize;
}

size_t BufferUtils::capacity()const{
    return mCapacity;
}

BufferUtils::BufferUtils(const BufferUtils &a){
    setTo(a);
}

void BufferUtils::operator=(const BufferUtils &a){
    setTo(a);
}

size_t BufferUtils::curPostion()const{
    return mOffset;
}

void BufferUtils::setTo(const BufferUtils& buffer){
    release();
    mSize = buffer.size();
    mCapacity = buffer.capacity();
    mOffset = buffer.curPostion();
    SharedBuffer* sb = SharedBuffer::alloc(mCapacity);
    if (sb) {
        void* array = sb->data();
        memcpy(array,buffer.data(),mSize);
        mBuffer = const_cast<void*>(array);
    }else{
        ALOGE("No memory for set to ");
    }
}

void BufferUtils::setTo(char *data,size_t size){
    release();
    int capacity = size + size /2;
    SharedBuffer* sb = SharedBuffer::alloc(capacity);
    if (sb) {
        void* array = sb->data();
        memcpy(array,data,size);
        mBuffer = const_cast<void*>(array);
    }else{
        ALOGE("No memory for set to ");
    }
    mOffset = 0 ;
    mSize = size;
    mCapacity = capacity;
}
