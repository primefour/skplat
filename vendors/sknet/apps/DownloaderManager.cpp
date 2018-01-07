#include<string>
#include"HttpRequest.h"
#include"HttpTransfer.h"
#include"DownloaderManager.h"
#include<unistd.h>
#include<fcntl.h>
#include<stdio.h>
#include<sys/types.h>
#include <dirent.h>  
#include <string.h>  
#include <sys/stat.h>  
#include "RangeDownloader.h"

#define DOWNLOAD_MAX_THREADS 10
const char *DownloaderManager::downloaderPartialFolder = "./.sknetDownload";

DownloaderManager::DownloaderManager(sp<HttpRequest> &req,
        const char *filePath,long contentLength):mObserver(this){
    mMainRequest = req;
    mfilePath = filePath;
    mCompleteCount = 0;
    mDownloadThreads = NULL;
    mDownloadCount = 0;
    mRanges = NULL;
    //init mRanges
    divdeContentLength(contentLength);
}

std::string DownloaderManager::getFilePathByRange(Range &rg){
    char filePath[PATH_MAX +1]={0};
    snprintf(filePath,sizeof(filePath)-1,"%s.part_%ld_%ld",
            mfilePath.c_str(),rg.begin,rg.end);
    return filePath;
}

void DownloaderManager::divdeContentLength(long content){
    if(content < 2 * 1024 * 1024){
        //only two
        mDownloadCount = 2;
    }else if(content < 8 * 1024 * 1024){
        //only three 
        mDownloadCount = 3;
    }else if(content < 16 * 1024 * 1024){
        //only four
        mDownloadCount = 4;
    }else{
        //max six
        mDownloadCount = 6;
    }

    mRanges = new Range[mDownloadCount];
    mDownloadThreads = new RangeDownloader*[mDownloadCount];
    long avg = content/mDownloadCount ;
    long model = content%mDownloadCount;
    int i = 0;
    int start = 0;
    //create range
    for(i = 0 ;i < mDownloadCount;i++){
        mRanges[i].begin = start ;
        mRanges[i].end = start + avg -1;
        mRanges[i].total = avg;
        start = start + avg;
    }
    mRanges[i-1].end += model;
    mRanges[i-1].total += model;

    //create thread for downloading
    for(i = 0 ;i < mDownloadCount;i++){
        std::string tmpFile = getFilePathByRange(mRanges[i]);
        mDownloadThreads[i] = new RangeDownloader(mMainRequest,
                tmpFile.c_str(),mRanges[i],this->mObserver);
        mDownloadThreads[i]->run();
    }
}

int DownloaderManager::wait4Complete(){
    AutoMutex _l(mMutex);
    while(mFailedCount + mCompleteCount < mDownloadCount){
        mCond.wait(mMutex);
    }

    if(mCompleteCount >= mDownloadCount){
        //merge all files
        RawFile allFile(mfilePath.c_str());
        int ret = allFile.open(O_RDWR|O_CREAT);
        int i = 0 ;
        long begin = 0;
        long end = 0;
        char tmpBuff[PATH_MAX]={0};
        for (i = 0 ;i < mDownloadCount ;i++){
            const char *tfile = mDownloadThreads[i]->filePath();
            sscanf(tfile,"%s.part_%ld_%ld",tmpBuff,&begin,&end);
            RawFile rdFile(tfile);
            int n = 0;
            while(1){
                n = rdFile.read(tmpBuff,sizeof(tmpBuff));
                if(n == 0 || n < 0){
                    break;
                }else{
                    allFile.lseek(begin,SEEK_SET);
                    allFile.write(tmpBuff,n);
                }
            }
        }
        return OK;
    }else{
        return UNKNOWN_ERROR;
    }
}

void DownloaderManager::recoverFailedTask(){
    int i = 0;
    //create thread for downloading
    for(i = 0 ;i < mDownloadCount;i++){
        //try again
        if(mDownloadThreads[i]->isFailed()){
            mDownloadThreads[i]->run();
        }
    }
}

DownloaderManager::~DownloaderManager(){
    int i = 0;
    for(i = 0 ;i < mDownloadCount;i++){
        mDownloadThreads[i]->join();
        delete mDownloadThreads[i];
    }
    delete[] mRanges;
    delete[] mDownloadThreads;
}

