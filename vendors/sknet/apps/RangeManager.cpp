#include<string>
#include"HttpRequest.h"
#include"TaskInfo.h"
#include"HttpTransfer.h"
#include"RangeManager.h"
#include<unistd.h>
#include<fcntl.h>
#include<stdio.h>
#include<sys/types.h>
#include <dirent.h>
#include <string.h>
#include <sys/stat.h>
#include "RangeDownloader.h"
#include "FileUtils.h"

#define DOWNLOAD_MAX_THREADS 10
const char *RangeManager::downloaderPartialFolder = "./.sknetDownload";
const char *RangeManager::downloaderFolder = "./Downloader";

RangeManager::RangeManager(sp<HttpRequest> &req,
        const char *filePath,long contentLength){
    mMainRequest = req;
    mfilePath = filePath;
    mFailedCount =0 ;
    mCompleteCount = 0;
    mDownloadThreads = NULL;
    mDownloadCount = 0;
    mRanges = NULL;
    //init mRanges
    divdeContentLength(contentLength);
}

void RangeManager::getFilePathByRange(Range &rg,char *buff,int count){
    memset(buff,0,count);
    snprintf(buff,count,"%s.part_%ld_%ld", mfilePath.c_str(),rg.begin,rg.end);
    return ;
}

void RangeManager::divdeContentLength(long content){
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
    mDownloadThreads = new sp<RangeDownloader>[mDownloadCount];
    long avg = content/mDownloadCount ;
    long model = content%mDownloadCount;
    int i = 0;
    int start = 0;
    //create range
    for(i = 0 ;i < mDownloadCount;i++){
        mRanges[i].begin = start ;
        mRanges[i].end = start + avg -1;
        mRanges[i].total = avg;
        mRanges[i].idx = i;
        start = start + avg;
    }
    mRanges[i-1].end += model;
    mRanges[i-1].total += model;

    char xfilePath[1024];
    //create thread for downloading
    for(i = 0 ;i < mDownloadCount;i++){
        getFilePathByRange(mRanges[i],xfilePath,sizeof(xfilePath) -1);
        mDownloadThreads[i] = new RangeDownloader(mMainRequest,
                xfilePath,mRanges[i],new RangeDLObserver(this) );
    }
}

void RangeManager::start(){
    int i = 0;
    //create thread for downloading
    for(i = 0 ;i < mDownloadCount;i++){
        mDownloadThreads[i]->run();
    }
}

void RangeManager::cancel(){
    int i = 0;
    //create thread for downloading
    for(i = 0 ;i < mDownloadCount;i++){
        mDownloadThreads[i]->cancel();
    }
}

int RangeManager::wait4Complete(){
    AutoMutex _l(mMutex);
    ALOGD("enter mFailedCount = %d  mCompleteCount = %d  mDownloadCount = %d ",mFailedCount,mCompleteCount,mDownloadCount);
    while(mFailedCount + mCompleteCount < mDownloadCount){
        mCond.wait(mMutex);
    }
    ALOGD("exit mFailedCount = %d  mCompleteCount = %d  mDownloadCount = %d ",mFailedCount,mCompleteCount,mDownloadCount);
    RawFile mergeFile(mfilePath.c_str());
    int ret = mergeFile.open(O_RDWR|O_CREAT);

    char tmpBuff[1024]={0};
    if(ret == OK && mCompleteCount >= mDownloadCount){
        int i = 0;
        for (i = 0 ;i < mDownloadCount ;i++){
            const char *tfile = mDownloadThreads[i]->filePath();
            const Range rg = mDownloadThreads[i]->range();
            ALOGD("tfile  %s %ld %ld ",tfile,mRanges[rg.idx].begin,mRanges[rg.idx].end);
            {
                RawFile rdFile(tfile);
                rdFile.open();
                int n = 0;
                mergeFile.lseek(mRanges[rg.idx].begin,SEEK_SET);
                while(1){
                    n = rdFile.read(tmpBuff,sizeof(tmpBuff));
                    if(n == 0 || n < 0){
                        break;
                    }else{
                        mergeFile.write(tmpBuff,n);
                    }
                }
            }
            FileUtils::deleteFiles(tfile);
        }
        return OK;
    }else{
        return UNKNOWN_ERROR;
    }
}

void RangeManager::recoverFailedTask(){
    int i = 0;
    //create thread for downloading
    for(i = 0 ;i < mDownloadCount;i++){
        //try again
        if(mDownloadThreads[i]->isFailed()){
            mDownloadThreads[i]->reset();
            mDownloadThreads[i]->run();
        }
    }
}

RangeManager::~RangeManager(){
    int i = 0;
    for(i = 0 ;i < mDownloadCount;i++){
        mDownloadThreads[i]->join();
    }
    delete[] mRanges;
    delete[] mDownloadThreads;
}

