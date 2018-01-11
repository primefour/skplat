#ifndef __BUFFER_FILTER_H__
#define __BUFFER_FILTER_H__
#include "RefBase.h"
#include "BufferUtils.h"
class BufferFilter:public RefBase{
    public:
        BufferFilter(){}
        virtual ~BufferFilter(){ } 
        //send source buffer to filters
        int write(const char *buff,int count){
            if(mFiltersHeader == NULL){
                return 0;
            }else{
                mDataSize += count;
                return mFiltersHeader->write(buff,count);
            }
        }

        virtual int read(sp<BufferUtils> &recvBuffer){
            int ret = 0;
            //check whether is end of file
            if(endOfFile()){
                return 0;
            }
            //check whether there is error?
            if(error() < 0){
                return error();
            }

            if(mFiltersHeader != NULL){
                ret = mFiltersHeader->read(recvBuffer);
            }

            int count = recvBuffer->size();
            if(count > 0){
                return count;
            }else{
                return ret;
            }
        }

        void setFilterHeader(sp<ReadFilterNode> &filter){
            mFiltersHeader = filter;
        }

        inline long size(){ return mDataSize; }
        inline int error(){ return mFiltersHeader->errors(); }
        inline void setEof(bool eof){ mFiltersHeader->setEof(eof); }
        inline bool endOfFile(){ return mFiltersHeader->endOfFile(); }
        sp<ReadFilterNode> mFiltersHeader;
        long mDataSize;
};

#endif
