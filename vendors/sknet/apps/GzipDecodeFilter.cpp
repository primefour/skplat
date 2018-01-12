/*******************************************************************************
 **      Filename: apps/HttpChunkParser.cpp
 **        Author: crazyhorse                  
 **   Description: ---
 **        Create: 2018-01-10 15:18:16
 ** Last Modified: 2018-01-10 15:18:16
 ******************************************************************************/
#include"GzipDecodeFilter.h"
#include"AppUtils.h"
#include<zlib.h>

void GzipDecodeFilter::reset(){
    mEof = false;
    if(mGzipStreamer != NULL) {
        inflateEnd(mGzipStreamer);
        delete mGzipStreamer;
        mGzipStreamer = NULL;
    }
    mGzipStreamer = new z_stream();
    mGzipStreamer->zalloc = NULL;
    mGzipStreamer->zfree = NULL;
    mGzipStreamer->opaque = NULL;
    mGzipStreamer->avail_in = 0;
    mGzipStreamer->next_in = NULL;
    mEof = false;
    mErrors = 0;
    // initialize z_stream with gzip/zlib format auto detection enabled.
    if (Z_OK != inflateInit2(mGzipStreamer, 47)) {
        mErrors = UNKNOWN_ERROR;
    }
    mBufferCache->reset();
}

int GzipDecodeFilter::transcode(sp<BufferUtils> &inputBuffer,sp<BufferUtils> &outputBuffer){
    const char *srcData = inputBuffer->data();
    int srcSize = inputBuffer->size();
    if(srcSize <= 0 || mEof || mErrors){
        ALOGD("srcSize %d  eof %d merrors %d ",srcSize,mEof,mErrors);
        return 0;
    }
    mGzipStreamer->avail_in = srcSize;
    mGzipStreamer->next_in = (unsigned char*) const_cast<char*>(srcData);
    unsigned char outbuf[1024] ={0};
    int buffSz = sizeof(outbuf) -1;
    int ret = 0;
    while (1) {
        mGzipStreamer->avail_out = buffSz;
        mGzipStreamer->next_out = outbuf;
        int ret = ::inflate(mGzipStreamer, Z_NO_FLUSH);
        if (ret == Z_STREAM_END) {
            mEof= true;
            ret = 0;
        } else if (ret != Z_OK && ret != Z_BUF_ERROR) {
            ALOGE("libz::inflate() failed. cause:%s", mGzipStreamer->msg);
            mErrors = UNKNOWN_ERROR;
            ret = mErrors;
        }
        size_t availData = buffSz - mGzipStreamer->avail_out;
        if(availData > 0){
            outputBuffer->append((const char *)outbuf,availData);
            //ALOGD("==> output Buffer = %s ",outbuf);
        }

        if(mGzipStreamer->avail_out > 0) {
            ret = 0;
            break;
        }
    }
    inputBuffer->consume(srcSize - mGzipStreamer->avail_in);
    return ret;
}

/*
int GzipDecodeFilter::read(sp<BufferUtils> &recvBuffer){
    AutoMutex _l(mMutex);
    const char *srcData = mBufferCache->data();
    int srcSize = mBufferCache->size();
    if(srcSize <= 0 || mEof || mErrors){
        ALOGD("srcSize %d  eof %d merrors %d ",srcSize,mEof,mErrors);
        return 0;
    }
    mGzipStreamer->avail_in = srcSize;
    mGzipStreamer->next_in = (unsigned char*) const_cast<char*>(srcData);
    unsigned char outbuf[1024];
    int ret = 0;
    while (1) {
        mGzipStreamer->avail_out = sizeof(outbuf);
        mGzipStreamer->next_out = outbuf;
        int ret = ::inflate(mGzipStreamer, Z_NO_FLUSH);
        if (ret == Z_STREAM_END) {
            mEof= true;
            break;
        } else if (ret != Z_OK && ret != Z_BUF_ERROR) {
            ALOGE("libz::inflate() failed. cause:%s", mGzipStreamer->msg);
            mErrors = UNKNOWN_ERROR;
            ret = mErrors;
            break;
        }
        size_t availData = sizeof(outbuf)- mGzipStreamer->avail_out;
        recvBuffer->append((const char *)outbuf,availData);
        if(mGzipStreamer->avail_out > 0) {
            break;
            ret = 0;
        }
    }
    mBufferCache->consume(srcSize - mGzipStreamer->avail_in);
    return ret;
}
*/
