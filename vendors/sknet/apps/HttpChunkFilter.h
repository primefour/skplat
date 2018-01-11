#ifndef __HTTP_CHUNK_READER_H__
#define __HTTP_CHUNK_READER_H__
#include"RefBase.h"
#include"BufferUtils.h"
#include"Mutex.h"
#include"ReadFilterNode.h"

class HttpChunkFilter:public ReadFilterNode{
    public:
        HttpChunkFilter(){ }
        virtual ~HttpChunkFilter(){
        }
        //do nothing transcode
        virtual int transcode(sp<BufferUtils> &inputBuffer,sp<BufferUtils> &outputBuffer);
    private:
        //this Cache will store only one chunk size data and will reset after parser successfully
        Mutex mMutex;
};

#endif //__HTTP_CHUNK_READER_H__
