#ifndef __DOWNLOADER_MANAGER_H__
#define __DOWNLOADER_MANAGER_H__
#include<string>
#include"HttpRequest.h"
#include"HttpTransfer.h"
#include"Vector.h"

class RangeDownloader;

class DownloaderManager{
    public:
        static const char *downloaderPartialFolder;
        DownloaderManager(sp<HttpRequest> &req,const char *filePath,long contentLength);
        class CompleteObserver{
            public:
                CompleteObserver(DownloaderManager *mg){
                    mMng = mg;
                }
                inline void onComplete(Range &rg){
                    AutoMutex _l(mMng->mMutex);
                    //scan ranges and update status

                }
                inline void onFailed(Range &rg){
                    AutoMutex _l(mMng->mMutex);

                }
                inline void onProgress(Range &rg){

                }
            private:
                DownloaderManager *mMng;
        };
    private:
        std::string getFilePathByRange(Range &rg);
        std::string mfilePath;
        void divdeContentLength(long content);
        Range *mRanges;
        RangeDownloader *mDownloadThreads;
        sp<HttpRequest> mMainRequest;
        int mCompleteCount;
        Mutex mMutex;
};

#endif
