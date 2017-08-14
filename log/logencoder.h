#ifndef __LOG_ENCODER_H__
#define __LOG_ENCODER_H__

#include <zlib.h>
#include <string>
#include <stdint.h>

//provider compress and crypt function if need
class LogEncoder{
    public :
        LogEncoder();
        ~LogEncoder(); 
        bool Write(const void* _data, size_t _length,void *_output,size_t &_len) ;
    private:
        bool WriteWithCompress(const void* _data, size_t _length,void *_output,size_t &_len); 
        z_stream cstream_;
};

#endif
