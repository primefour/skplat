#ifndef __HTTP_CHUNK_READER_H__
#define __HTTP_CHUNK_READER_H__
#include"RefBase.h"
#include"BufferUtils.h"
#include"Mutex.h"
class HttpChunkFilter:public RefBase{
    public:
        HttpChunkFilter(){
            mBufferCache = new BufferUtils();
            mEof = false;
        }
        int write(const char *buff,int count);
        int read(sp<BufferUtils> &recvBuffer);
        inline bool endOfFile(){ return mEof; }
        long size(){
            return mBufferCache->size();
        }
    private:
        //this Cache will store only one chunk size data and will reset after parser successfully
        sp<BufferUtils> mBufferCache;
        Mutex mMutex;
        bool mEof;
};

#endif //__HTTP_CHUNK_READER_H__
