#ifndef __DOWNLOADER_MANAGER_H__
#define __DOWNLOADER_MANAGER_H__
#include<string>
#include"HttpRequest.h"
#include"HttpTransfer.h"
#include"Mutex.h"
#include"Condition.h"

class RangeDownloader;

class DownloaderManager{
    public:
        static const char *downloaderPartialFolder;
        static const char *downloaderFolder;
        DownloaderManager(sp<HttpRequest> &req,const char *filePath,long contentLength);
        virtual ~DownloaderManager();
        class CompleteObserver{
            public:
                CompleteObserver(DownloaderManager *mg){
                    mMng = mg;
                }

                inline void onComplete(Range &rg){
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

                inline void onFailed(Range &rg){
                    AutoMutex _l(mMng->mMutex);
                    ALOGW("get a failed range %ld - %ld ",rg.begin,rg.end);
                    mMng->mFailedCount ++;

                    if(mMng->mFailedCount + mMng->mCompleteCount >= mMng->mDownloadCount){
                        //download failed and all download thread exit
                        mMng->mCond.signal();
                    }
                }

                inline void onProgress(int idx,int percent){
                    ALOGD("download percent idx %d ==>  %d ",idx,percent);
                }

            private:
                DownloaderManager *mMng;
        };
        int wait4Complete();
        void start();
        void cancel();
    private:
        void recoverFailedTask();
        void getFilePathByRange(Range &rg,char *buff,int count);
        std::string mfilePath;
        void divdeContentLength(long content);
        Range *mRanges;
        sp<RangeDownloader> *mDownloadThreads;
        sp<HttpRequest> mMainRequest;
        int mCompleteCount;
        int mFailedCount;
        int mDownloadCount;
        Mutex mMutex;
        Condition mCond;
        CompleteObserver mObserver;
};

#endif
