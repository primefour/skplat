/*******************************************************************************
 **      Filename: apps/HttpChunkParser.cpp
 **        Author: crazyhorse                  
 **   Description: ---
 **        Create: 2018-01-10 15:18:16
 ** Last Modified: 2018-01-10 15:18:16
 ******************************************************************************/
#include"HttpChunkFilter.h"
#include"AppUtils.h"

int HttpChunkFilter::write(char *buff,int count){
    AutoMutex _l(mMutex);
    return mBufferCache->append(buff,count);
}
/*
 * input should xxx\r\njljjkl 
 * or \r\nxxx\r\njslfjafj
 */
bool HttpChunkFilter::checkChunk(){
    AutoMutex _l(mMutex);
    const char *srcData = mBufferCache->data();
    int srcSize = mBufferCache->size();
    if(srcSize  < 2){
        return false;
    }
    long count = 0;
    int i = 0;
    while(*(srcData +i) == '\r' && *(srcData +i +1) == '\n'){
        i+=2;
    }
    const char *actualCount = srcData +i;
    count = parseHex(actualCount,count);

    //check eof 
    if(count == 0){
        mEof = true;
        return true;
    }

    //skip "\r\n" after count
    const char *actualData = strstr(actualCount,"\r\n");
    actualData += 2;
    if(count <= srcSize - (actualData - srcData)){
        return true;
    }else{
        return false;
    }
}

int HttpChunkFilter::read(sp<BufferUtils> &recvBuffer){
    AutoMutex _l(mMutex);
    const char *srcData = mBufferCache->data();
    int srcSize = mBufferCache->size();
    long count = 0;
    int i = 0;
    while(*(srcData +i) == '\r' && *(srcData +i +1) == '\n'){
        i+=2;
    }
    const char *actualCount = srcData +i;
    count = parseHex(actualCount,count);
    //check eof 
    if(count <= 0){
        mEof = true;
        return 0;
    }
    //skip "\r\n" after count
    const char *actualData = strstr(actualCount,"\r\n");
    actualData += 2;
    if(count <= srcSize - (actualData - srcData) ){
        recvBuffer->append(actualData,count);
        recvBuffer->consume(actualData + count - srcData);
    }else{
        return 0;
    }
    return count;
}


