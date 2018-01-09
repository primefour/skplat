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
    mTransferObserver = new TransferListener(this);
}

void RangeDownloader::cancel(){
    mTransfer->interrupt();
}

RangeDownloader::~RangeDownloader(){
    ALOGD("%s %d ",__func__,__LINE__);
}

bool RangeDownloader::threadLoop(){
    //download by this transfer
    long size= 0;
    int ret = 0;
    {
        RawFile rawFile(mfilePath.c_str());
        ret = rawFile.open(O_RDWR|O_CREAT);
        if(ret < 0){
            ALOGE("%s file can't be access",mfilePath.c_str());
            return false;
        }else{
            size = rawFile.size();
        }
        ALOGD("%s file size is %ld ",mfilePath.c_str(),size);
        if(size >= mRg.end - mRg.begin + 1){
            mRg.setDone();
            mObserver.onComplete(mRg);
            return false;
        }
        mRg.begin += size;
    }

    mTransfer->setObserver(mTransferObserver);
    ret = mTransfer->doRangeDownload(mReq,mfilePath.c_str(),mRg);
    if(ret < 0){
        mRg.setFailed();
        mObserver.onFailed(mRg);
    }else{
        mRg.setDone();
        mObserver.onComplete(mRg);
    }
    return false;
}
