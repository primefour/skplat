#ifndef __FILTER_NODE_H__
#define __FILTER_NODE_H__
#include"RefBase.h"
#include"BufferUtils.h"
#include"Mutex.h"
class ReadFilterNode:public RefBase{
    public:
        ReadFilterNode(){
            mEof = false;
            mErrors = 0;
            mChild = NULL;
            mBufferCache = new BufferUtils();
        }

        virtual void reset(){
            mEof = false;
            mErrors = 0;
            mBufferCache->reset();
        }

        virtual int write(const char *buff,int count){
            return mBufferCache->append(buff,count);
        }

        int errors(){ return mErrors; }
        //do nothing transcode
        virtual int transcode(sp<BufferUtils> &inputBuffer,sp<BufferUtils> &outputBuffer){
            outputBuffer->append(inputBuffer->data(),inputBuffer->size());
            inputBuffer->consume(inputBuffer->size());
        }

        int read(sp<BufferUtils> &recvBuffer){
            int ret = 0;
            //check whether is end of file
            if(mEof){
                return 0;
            }
            //check whether there is error?
            if(mErrors < 0){
                return mErrors;
            }
            int count = mBufferCache->size();
            if(count > 0){
                if(mChild != NULL){
                    ret = transcode(mBufferCache,mChild->mBufferCache);
                    if(ret  < 0){
                        ALOGE("transcode failed ");
                        mErrors = UNKNOWN_ERROR;
                    }
                    if(mChild->mBufferCache->size() > 0){
                        ret = mChild->read(recvBuffer);
                        if(ret < 0){
                            ALOGE("transcode failed ");
                            mErrors = UNKNOWN_ERROR;
                        }
                    }
                }else{
                    if(mBufferCache->size() > 0){
                        //copy mBufferCache to recvbuffer;
                        recvBuffer->append(mBufferCache->data(),mBufferCache->size());
                        recvBuffer->consume(mBufferCache->size());
                    }
                }
            }
            //send out data and the eof or error while detecte by the next time
            count = recvBuffer->size();
            if(count > 0){
                return count;
            }else {
                return ret;
            }
        }

        inline void setChild(ReadFilterNode *filter){
            mChild = filter;
        }
        inline void setEof(bool eof){
            mEof = eof;
        }
        inline bool endOfFile(){ return mEof; }
        //mEof will set at transcode or by the parents
        sp<ReadFilterNode> mChild;
        sp<BufferUtils> mBufferCache;
        bool mEof;
        int mErrors;
};

#endif
