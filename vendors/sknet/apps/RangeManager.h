#ifndef __DOWNLOADER_MANAGER_H__
#define __DOWNLOADER_MANAGER_H__
#include<string>
#include"HttpRequest.h"
#include"HttpTransfer.h"
#include"Mutex.h"
#include"Condition.h"
#include "TaskInfo.h"

class RangeDownloader;

class RangeObserver:public RefBase{
    public:
        virtual void onComplete(Range &rg){
            ALOGD("download complete range is %ld - %ld ",rg.begin,rg.end);
        }

        virtual void onFailed(Range &rg){
            ALOGE("download fail range is %ld - %ld ",rg.begin,rg.end);
        }

        virtual void onProgress(int idx,int percent){
            ALOGD("download percent idx %d ==>  %d ",idx,percent);
        }
};

class RangeManager{
    public:
        static const char *downloaderPartialFolder;
        static const char *downloaderFolder;
        RangeManager(sp<HttpRequest> &req,const char *filePath,
                long contentLength,sp<TaskInfo> &task);
        virtual ~RangeManager();
        class RangeDLObserver:public RangeObserver{
            public:
                RangeDLObserver(RangeManager*mg){
                    mMng = mg;
                }

                void onComplete(Range &rg){
                    AutoMutex _l(mMng->mMutex);
                    mMng->mCompleteCount ++;
                    if(mMng->mFailedCount > 0){
                        //scan ranges and resume download
                        mMng->recoverFailedTask();
                        mMng->mFailedCount = 0;
                    }

                    if(mMng->mCompleteCount >= mMng->mDownloadCount){
                        //download complete and compose all files
                        mMng->mCond.signal();
                        return ;
                    }

                    if(mMng->mFailedCount + mMng->mCompleteCount >= mMng->mDownloadCount){
                        //download failed and all download thread exit
                        mMng->mCond.signal();
                        return;
                    }
                }

                void onFailed(Range &rg){
                    AutoMutex _l(mMng->mMutex);
                    ALOGW("get a failed range %ld - %ld ",rg.begin,rg.end);
                    mMng->mFailedCount ++;

                    if(mMng->mFailedCount + mMng->mCompleteCount >= mMng->mDownloadCount){
                        //download failed and all download thread exit
                        mMng->mCond.signal();
                    }
                }

                void onProgress(int idx,int size){
                    mMng->updateProgress(idx,size);
                }

            private:
                RangeManager *mMng;
        };
        int wait4Complete();
        void start();
        void cancel();
    private:
        void recoverFailedTask();
        void updateProgress(int idx,int size);
        void getFilePathByRange(Range &rg,char *buff,int count);
        std::string mfilePath;
        void divdeContentLength(long content);
        Range *mRanges;
        sp<RangeDownloader> *mDownloadThreads;
        sp<HttpRequest> mMainRequest;
        sp<TaskInfo> mTask;
        int mCompleteCount;
        int mFailedCount;
        int mDownloadCount;
        long mContentLength;
        Mutex mMutex;
        Condition mCond;
};

#endif
