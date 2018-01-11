#ifndef __GZIP_DECODE_FILTER_H__
#define __GZIP_DECODE_FILTER_H__
#include"RefBase.h"
#include"BufferUtils.h"
#include"Mutex.h"
#include"ReadFilterNode.h"
#include <zlib.h>
class GzipDecodeFilter :public ReadFilterNode{
    public:
        GzipDecodeFilter(){
            mGzipStreamer = NULL;
        }

        virtual ~GzipDecodeFilter(){
            reset();
        }
        //do nothing transcode
        virtual int transcode(sp<BufferUtils> &inputBuffer,sp<BufferUtils> &outputBuffer);
        void reset();
    private:
        z_stream* mGzipStreamer;
};

#endif
