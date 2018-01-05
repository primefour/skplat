#ifndef __RANGE_DOWNLOADER_H__
#define __RANGE_DOWNLOADER_H__
#include<string>
#include"HttpRequest.h"
#include"HttpTransfer.h"
#include"DownloaderManager.h"
#include"Threads.h"

class RangeDownloader:public Thread{
    public:
        RangeDownloader(sp<HttpRequest> &req,
                const char *filePath,Range &range,DownloaderManager::CompleteObserver &observer);
        virtual bool threadLoop();
        inline bool isFailed(){
            return mRg.failed();
        }
        inline std::string& filePath(){
            return mfilePath;
        }
    private:
        sp<HttpTransfer> mTransfer;
        sp<HttpRequest> mReq;
        std::string mfilePath;
        Range &mRg;
        DownloaderManager::CompleteObserver &mObserver;
};
#endif 
