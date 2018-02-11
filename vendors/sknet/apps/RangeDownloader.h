#ifndef __RANGE_DOWNLOADER_H__
#define __RANGE_DOWNLOADER_H__
#include<string>
#include"HttpRequest.h"
#include"HttpTransfer.h"
#include"RangeManager.h"
#include"Threads.h"
#include"RefBase.h"

class RangeDownloader:public Thread{
    public:
        class TransferListener:public  HttpTransfer::TransferObserver{
            public:
                TransferListener(RangeDownloader *rder){
                    mRgDownloader = rder;
                }

                virtual void onStartConnect(){
                    ALOGD("RangeDownloader start connecting...");
                }

                //return false will stop this transfer or continue
                virtual void onConnected(bool success){
                    ALOGD("RangeDownloader connect completely...");
                }

                virtual void onSending(long bytes,long total){
                    ALOGD("RangeDownloader send data %ld:%ld",total,bytes);
                    return ;
                }

                virtual void onSended(){
                    ALOGD("RangeDownloader send completely...");
                }

                virtual void onProgress(long bytes,long total){
                    mRgDownloader->mObserver->onProgress(mRgDownloader->mRg.idx,bytes);
                    return;
                }

                virtual void onCompleted(){
                    ALOGD("RangeDownloader recv data completely...");
                    return;
                }

                virtual void onFailed(int error){
                    ALOGD("RangeDownloader http transfer failed error :%d ",error);
                    return;
                }
            private:
                sp<RangeDownloader> mRgDownloader;
        };

        RangeDownloader(sp<HttpRequest> &req,
                const char *filePath,Range &range,sp<RangeObserver> observer);
        virtual ~RangeDownloader();

        virtual bool threadLoop();

        inline bool isFailed(){
            return mRg.failed();
        }

        inline void reset(){
            mRg.reset();
            join();
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
        Range mRg;
        sp<HttpTransfer> mTransfer;
        sp<HttpRequest> mReq;
        sp<RangeObserver> mObserver;
        sp<HttpTransfer::TransferObserver> mTransferObserver;
};
#endif 
