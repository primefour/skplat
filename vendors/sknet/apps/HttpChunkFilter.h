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
        int write(char *buff,int count);
        /*
         * input should xxx\r\njljjkl 
         * or \r\nxxx\r\njslfjafj
         */
        bool checkChunk();
        int read(sp<BufferUtils> &recvBuffer);
        inline bool endOfFile(){ return mEof; }
    private:
        //this Cache will store only one chunk size data and will reset after parser successfully
        sp<BufferUtils> mBufferCache;
        Mutex mMutex;
        bool mEof;
};

#endif //__HTTP_CHUNK_READER_H__
