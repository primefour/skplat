#include<string>
#include"HttpRequest.h"
#include"HttpTransfer.h"
#include"DownloaderManager.h"
#include"RangeDownloader.h"

RangeDownloader::RangeDownloader(sp<HttpRequest> &req,
        const char *filePath,Range &range,
        DownloaderManager::CompleteObserver &observer):mObserver(observer),mRg(range){
    mTransfer = new HttpTransfer();
    mReq = req;
    mfilePath = filePath;
}



bool RangeDownloader::threadLoop(){
    int ret = mTransfer->doRangeDownload(mReq,mfilePath.c_str(),mRg);
    if(ret < 0){
        mRg.setFailed();
        mObserver.onFailed(mRg);
    }else{
        mRg.setDone();
        mObserver.onComplete(mRg);
    }
}
