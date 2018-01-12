#ifndef __BUFFER_FILTER_H__
#define __BUFFER_FILTER_H__
#include "RefBase.h"
#include "BufferUtils.h"
class BufferFilter:public RefBase{
    public:
        BufferFilter(){
            mFiltersHeader = NULL;
            mDataSize = 0;
            mContentLength  = -1;
        }

        virtual ~BufferFilter(){
            ALOGD("%s %d ",__func__,__LINE__);
        } 
        //send source buffer to filters
        int write(const char *buff,int count){
            mDataSize += count;
            if(mFiltersHeader == NULL && mRecvBuffer != NULL){
                return mRecvBuffer->append(buff,count);
            }else{
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
            }else{
                if(recvBuffer != mRecvBuffer){
                    int count = mRecvBuffer->size();
                    if(count > 0){
                        recvBuffer->append(mRecvBuffer->data(),count);
                        mRecvBuffer->consume(count);
                    }
                    return count;
                }else{
                    //already in the recvBuffer
                    return 0;
                }
            }
            return ret;
        }

        void setFilterHeader(ReadFilterNode *filter){
            mFiltersHeader = filter;
        }

        inline long size(){ 
            return mDataSize; 
        }

        void setRecvBuffer(sp<BufferUtils> &recvBuffer,long content = -1){
            mRecvBuffer = recvBuffer;
            mContentLength = content;
        }

        inline int error(){ return mFiltersHeader != NULL ? mFiltersHeader->errors():0; }

        inline bool endOfFile(){ 
            if(mFiltersHeader != NULL){
                return mFiltersHeader->endOfFile();
            }else if(mContentLength != -1){
                return mDataSize >= mContentLength;
            }else{
                return false;
            }
        }

        sp<ReadFilterNode> mFiltersHeader;
        sp<BufferUtils> mRecvBuffer;
        long mDataSize;
        long mContentLength;
};

#endif
