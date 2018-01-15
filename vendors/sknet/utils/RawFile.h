#ifndef __RAW_FILE_H__
#define __RAW_FILE_H__
#include<stdio.h>
#include<string>
#include<unistd.h>
#include<fcntl.h>
#include<sys/types.h>
#include"BufferUtils.h"


/*
 * this is raw file implement,raw file binary is big endian default
 * if the binary file read is little endian,should use LITTLE_ENDIAN
 * when open file 
 */
class RawFile{
    public:
        enum {
            XBIG_ENDIAN,
            XLITTLE_ENDIAN,
        };
        RawFile(const char *filePath);
        RawFile(const RawFile &fileObj);
        virtual ~RawFile();
        static RawFile* open(const char *filePath,int flags = O_RDWR,int endian = XBIG_ENDIAN);
        RawFile& operator=(const RawFile &fileObj);
        int open(int flags = O_RDWR,int endian = XBIG_ENDIAN);
        int read(char *buff,int len);
        int read(BufferUtils &buffer,int readLength);
        int readInt32(int32_t &data);
        int readLine(char *buff,int len);
        int readLine(BufferUtils& buffer);
        int write(const char *buff,int len);
        int append(char *buff,int len);
        int append(BufferUtils& buffer,int writeLength = -1);
        int writeInt32(int32_t data);
        int write(BufferUtils&buffer,int writeLength = -1);
        size_t lseek(int offset,int flags);
        size_t loffset();
        size_t size();
        int flush();
    private:
        std::string mfilePath;
        int mEndianType;
        int mFd;
        int mOpenFlags;
};
#endif
