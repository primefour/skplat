#include<string>
#include"HttpRequest.h"
#include"HttpTransfer.h"
#include"DownloaderManager.h"

const char *DownloaderManager::downloaderPartialFolder = "./.sknetDownload";

DownloaderManager::DownloaderManager(sp<HttpRequest> &req,const char *filePath,long contentLength){
    mMainRequest = req;
    mfilePath = filePath;
    //init mRanges
    divdeContentLength(contentLength);
    mCompleteCount = 0;
}

std::string DownloaderManager::getFilePathByRange(Range &rg){
    return "";
}

void DownloaderManager::divdeContentLength(long content){

}
