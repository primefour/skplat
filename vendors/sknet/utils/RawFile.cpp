#include"RawFile.h"
#include<stdio.h>
#include<string.h>
#include<unistd.h>
#include<sys/types.h>
#include<sys/stat.h>
#include<fcntl.h>
#include<stdlib.h>
#include<errno.h>
#include <arpa/inet.h>
#include"Log.h"
#include"FileUtils.h"

RawFile::RawFile(const char *filePath){
    ASSERT(filePath != NULL,"invalidate input");
    mfilePath = filePath;
    mFd = -1;
    mEndianType = XBIG_ENDIAN;
}

RawFile::~RawFile(){
    if(mFd != -1){
        close(mFd);
    }
    mFd = -1;
}

RawFile::RawFile(const RawFile &fileObj){
    if(fileObj.mFd != -1){
        mFd = dup(fileObj.mFd);
    }
    mfilePath = fileObj.mfilePath;
    mEndianType = fileObj.mEndianType;
    mOpenFlags = -1;
}


RawFile& RawFile::operator=(const RawFile &fileObj){
    if(fileObj.mFd != -1){
        mFd = dup(fileObj.mFd);
    }
    mfilePath = fileObj.mfilePath;
    mEndianType = fileObj.mEndianType;
    return *this;
}

RawFile* RawFile::open(const char *filePath,int flags,int endian){
    ASSERT(filePath != NULL,"Invalidate input");
    RawFile *fileObj = new RawFile(filePath);
    int ret = access(filePath,F_OK);
    fileObj->mfilePath = filePath;
    fileObj->mEndianType = endian;
    if(ret < 0 ){
        if(flags & O_CREAT ){
            fileObj->mFd = ::open(filePath,flags,0666);
            fileObj->mOpenFlags = flags & ~O_CREAT;
        }else{
            ALOGW("file %s not exist and open should with O_CREAT flags or %s ",filePath,strerror(errno));
        }
    }else{
        flags = flags & ~O_CREAT;
        if(FileUtils::isRegularFile(filePath)){
            fileObj->mFd = ::open(filePath,flags);
            fileObj->mOpenFlags = flags;
        }
    }
    return fileObj;
}

int RawFile::open(int flags,int endian){
    int ret = access(mfilePath.c_str(),F_OK);
    mEndianType = endian;
    if(ret < 0 ){
        if(flags & O_CREAT ){
            mFd = ::open(mfilePath.c_str(),flags,0666);
            mOpenFlags = flags & ~O_CREAT;
            if(mFd < 0){
                ALOGE("file %s open failed error msg: %s ",mfilePath.c_str(),strerror(errno));
                return UNKNOWN_ERROR;
            }
        }else{
            ALOGW("file %s not exist and open should with O_CREAT flags or %s ",mfilePath.c_str(),strerror(errno));
            return BAD_VALUE;
        }
    }else{
        flags = flags & ~O_CREAT;
        if(FileUtils::isRegularFile(mfilePath.c_str())){
            mFd = ::open(mfilePath.c_str(),flags);
            mOpenFlags = flags;
            if(mFd < 0){
                ALOGE("file %s open failed error msg: %s ",mfilePath.c_str(),strerror(errno));
                return UNKNOWN_ERROR;
            }
        }else{
            ALOGW("file %s is not regular file",mfilePath.c_str());
            return BAD_VALUE;
        }
    }
    return OK;
}

/*
 * read count of len data 
 */
int RawFile::read(char *buff,int len){
    if(mFd < 0){
        ALOGW("mfd < 0 ");
        return UNKNOWN_ERROR;
    }
    return ::read(mFd,buff,len);
}

/*
 * read the count of readLength data and append to buffer
 */
int RawFile::read(BufferUtils &buffer,int readLength){
    if(mFd < 0){
        ALOGW("mfd < 0 ");
        return UNKNOWN_ERROR;
    }
    char buff[1024] ={0};
    int readCount = 0;
    int n = 0;
    while(0 < readLength){
        int readSize = sizeof(buff) > readLength ?readLength:sizeof(buff);
        n = ::read(mFd,buff,readSize);
        if(n >0){
            readCount += n;
            readLength -= n;
            buffer.append(buff,n);
        }else if(n == 0 ){
            ALOGD("get end of file read count is %d ",readCount);
            break;
        }else{
            ALOGD("read error %s ",strerror(errno));
            break;
        }
    }
    return readCount > 0? readCount:UNKNOWN_ERROR;
}

/*
 * read 4 bytes and change to int32_t
 */
int RawFile::readInt32(int32_t &data){
    if(mFd < 0){
        ALOGW("mfd < 0 ");
        return UNKNOWN_ERROR;
    }
    int32_t tmp;
    int ret = ::read(mFd,&tmp,sizeof(tmp));
    if(ret < sizeof(int32_t)){
        return UNKNOWN_ERROR;
    }
    if(mEndianType == XBIG_ENDIAN){
        data = tmp;
        return OK;
    }else{
        data = ntohl(tmp);
        return OK;
    }
}

int RawFile::readLine(char *buff,int len){
    if(mFd < 0){
        ALOGW("mfd < 0 ");
        return UNKNOWN_ERROR;
    }
    int ret = -1;
    char *ptr = buff;
    int i = 0;
    while((ret = ::read(mFd,ptr,1) > 0)
            && (*ptr != '\n') 
            && ++i < len){ 
        ptr++;
    }
    //set string end
    buff[len -1] ='\0';
    return i > 0 ? i:ret;
}

int RawFile::readLine(BufferUtils& buffer){
    if(mFd < 0){
        ALOGW("mfd < 0 ");
        return UNKNOWN_ERROR;
    }
    int ret = -1;
    int i = 0;
    char ptr;
    while((ret = ::read(mFd,&ptr,1) > 0)
            && ptr != '\n'){ 
        buffer.append(&ptr,1);
        i++;
    }
    return i > 0 ? i:ret;
}

int RawFile::writeInt32(int32_t data){
    if(mFd < 0){
        ALOGW("mfd < 0 ");
        return UNKNOWN_ERROR;
    }
    if(mOpenFlags & O_WRONLY != O_WRONLY){
        ALOGE("this is read only file object");
        return BAD_VALUE;
    }
    int32_t tmp = htonl(tmp);
    int ret = ::write(mFd,&tmp,sizeof(tmp));
    if(ret < 0){
        ALOGE("write int32 file %s failed",mfilePath.c_str());
        return UNKNOWN_ERROR;
    }
    return OK;
}

int RawFile::write(char *buff,int len){
    if(mFd < 0){
        ALOGW("mfd < 0 ");
        return UNKNOWN_ERROR;
    }
    if(mOpenFlags & O_WRONLY != O_WRONLY){
        ALOGE("this is read only file object");
        return BAD_VALUE;
    }
    int ret = ::write(mFd,buff,len);
    return ret;

}
 
int RawFile::write(BufferUtils&buffer,int writeLength){
    if(mFd < 0){
        ALOGW("mfd < 0 ");
        return UNKNOWN_ERROR;
    }
    if(mOpenFlags & O_WRONLY != O_WRONLY){
        ALOGE("this is read only file object");
        return BAD_VALUE;
    }
    int ret;
    if(writeLength == -1 && buffer.size() > 0){
        ret = ::write(mFd,buffer.data(),buffer.size());
    }else{
        ret = ::write(mFd,buffer.data(),writeLength);
    }
    return ret;
}

size_t RawFile::lseek(int offset,int flags){

    if(mFd >= 0){
        return ::lseek(mFd,offset,flags);
    }else{
        ALOGW("mfd < 0");
        return BAD_VALUE;
    }
}
size_t RawFile::loffset(){
    if(mFd < 0){
        ALOGW("mFd < 0");
        return BAD_VALUE;
    }
    return ::lseek(mFd,0,SEEK_CUR);
}

size_t RawFile::size(){
    size_t pos = loffset();
    size_t size = lseek(0,SEEK_END);
    lseek(pos,SEEK_SET);
    return size;
}


