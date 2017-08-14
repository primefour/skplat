#include <cstdio>
#include <time.h>
#include <algorithm>
#include <sys/time.h>
#include <string.h>
#include <errno.h>
#include <assert.h>
#include "logencoder.h"
#include "fileutils.h"
#include "platform.h"


LogEncoder::LogEncoder(){
    memset(&cstream_,0, sizeof(cstream_));
}

LogEncoder::~LogEncoder() {
    if (Z_NULL != cstream_.state) {
        deflateEnd(&cstream_);
    }
}

bool LogEncoder::Write(const void* _data,size_t _length,void *_output,size_t &_len) {
    if (NULL == _data || NULL == _output || 0 == _length || _len <= _length){
        return false;
    }
    return WriteWithCompress(_data,_length,_output,_len);
}


bool LogEncoder::WriteWithCompress(const void* _data, size_t _length,void *_output,size_t &_len) {
    cstream_.zalloc = Z_NULL;
    cstream_.zfree = Z_NULL;
    cstream_.opaque = Z_NULL;
    if (Z_OK != deflateInit2(&cstream_, Z_BEST_COMPRESSION,Z_DEFLATED, -MAX_WBITS, MAX_MEM_LEVEL, Z_DEFAULT_STRATEGY)) {
        ConsolePrintf("initialize cstream fail for log compress");
        return false;
    }

    cstream_.avail_in = (uInt)_length;
    cstream_.next_in = (Bytef*)_data;

    uInt avail_out = _len;
    cstream_.next_out = (Bytef*)_output; 
    cstream_.avail_out = _len;

    if (Z_OK != deflate(&cstream_, Z_SYNC_FLUSH)) {
        ConsolePrintf("cstream fail to compress log");
        return false;
    }

    if (Z_NULL != cstream_.state) {
        ConsolePrintf("deflateEnd cstream fail to compress log");
        deflateEnd(&cstream_);
    }
    _len = avail_out - cstream_.avail_out;
    return true;
}

