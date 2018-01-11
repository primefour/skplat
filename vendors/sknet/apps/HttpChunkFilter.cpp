/*******************************************************************************
 **      Filename: apps/HttpChunkParser.cpp
 **        Author: crazyhorse                  
 **   Description: ---
 **        Create: 2018-01-10 15:18:16
 ** Last Modified: 2018-01-10 15:18:16
 ******************************************************************************/
#include"HttpChunkFilter.h"
#include"AppUtils.h"

int HttpChunkFilter::transcode(sp<BufferUtils> &inputBuffer,sp<BufferUtils> &outputBuffer){
    const char *srcData = inputBuffer->data();
    int srcSize = inputBuffer->size();
    long count = 0;
    int i = 0;
    //remove '\r\n'
    while(*(srcData +i) == '\r' && *(srcData +i +1) == '\n' && i < srcSize){
        i+=2;
    }

    if(srcSize < i){
        return 0;
    }

    //skip "\r\n" after count
    const char *actualData = strstr(srcData +i,"\r\n");
    if(actualData == NULL){
        return 0;
    }
    const char *actualCount = srcData +i;
    count = parseHex(actualCount,count);
    ALOGD("count is = %ld ",count);
    //check eof 
    if(count <= 0){
        mEof = true;
        return 0;
    }
    actualData += 2;
    if(count <= srcSize - (actualData - srcData)){
        outputBuffer->append(srcData,srcSize);
        inputBuffer->consume(actualData + count - srcData);
        return count;
    }else{
        return 0;
    }
    return count;
}

/*
int HttpChunkFilter::read(sp<BufferUtils> &recvBuffer){
    AutoMutex _l(mMutex);
    const char *srcData = mBufferCache->data();
    int srcSize = mBufferCache->size();
    long count = 0;
    int i = 0;
    //remove '\r\n'
    while(*(srcData +i) == '\r' && *(srcData +i +1) == '\n' && i < srcSize){
        i+=2;
    }

    if(srcSize < i){
        return 0;
    }

    //skip "\r\n" after count
    const char *actualData = strstr(srcData +i,"\r\n");
    if(actualData == NULL){
        return 0;
    }
    const char *actualCount = srcData +i;
    count = parseHex(actualCount,count);
    ALOGD("count is = %ld ",count);
    //check eof 
    if(count <= 0){
        mEof = true;
        return 0;
    }
    actualData += 2;
    
   // ALOGD("srcSize %d actualData - srcData = %ld xj %ld ",srcSize,
    //        actualData - srcData,srcSize - (actualData - srcData));
    //ALOGD("++ x:%ld  count:%ld+++++++ %ld ",actualData - srcData,count,actualData + count - srcData);
    if(count <= srcSize - (actualData - srcData)){
        recvBuffer->append(srcData,srcSize);
        mBufferCache->consume(actualData + count - srcData);
        //ALOGD("xj raw string is %s  ",mBufferCache->data());
        return count;
    }else{
        return 0;
    }
    return count;
}

*/


