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
        inline std::string& fileSPath(){
            return mfilePath;
        }

        inline const char* filePath(){
            ALOGD("mfilePath %s ",mfilePath.c_str());
            return mfilePath.c_str();
        }

        inline const Range& range(){
            return mRg;
        }

        void cancel();
    private:
        std::string mfilePath;
        Range &mRg;
        sp<HttpTransfer> mTransfer;
        sp<HttpRequest> mReq;
        DownloaderManager::CompleteObserver &mObserver;
};
#endif 
