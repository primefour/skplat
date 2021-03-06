#ifndef __HTTP_TRANSFER_H__
#define __HTTP_TRANSFER_H__

/*
	StatusContinue:           "Continue",
	StatusSwitchingProtocols: "Switching Protocols",
	StatusProcessing:         "Processing",

	StatusOK:                   "OK",
	StatusCreated:              "Created",
	StatusAccepted:             "Accepted",
	StatusNonAuthoritativeInfo: "Non-Authoritative Information",
	StatusNoContent:            "No Content",
	StatusResetContent:         "Reset Content",
	StatusPartialContent:       "Partial Content",
	StatusMultiStatus:          "Multi-Status",
	StatusAlreadyReported:      "Already Reported",
	StatusIMUsed:               "IM Used",

	StatusMultipleChoices:   "Multiple Choices",
	StatusMovedPermanently:  "Moved Permanently",
	StatusFound:             "Found",
	StatusSeeOther:          "See Other",
	StatusNotModified:       "Not Modified",
	StatusUseProxy:          "Use Proxy",
	StatusTemporaryRedirect: "Temporary Redirect",
	StatusPermanentRedirect: "Permanent Redirect",

	StatusBadRequest:                   "Bad Request",
	StatusUnauthorized:                 "Unauthorized",
	StatusPaymentRequired:              "Payment Required",
	StatusForbidden:                    "Forbidden",
	StatusNotFound:                     "Not Found",
	StatusMethodNotAllowed:             "Method Not Allowed",
	StatusNotAcceptable:                "Not Acceptable",
	StatusProxyAuthRequired:            "Proxy Authentication Required",
	StatusRequestTimeout:               "Request Timeout",
	StatusConflict:                     "Conflict",
	StatusGone:                         "Gone",
	StatusLengthRequired:               "Length Required",
	StatusPreconditionFailed:           "Precondition Failed",
	StatusRequestEntityTooLarge:        "Request Entity Too Large",
	StatusRequestURITooLong:            "Request URI Too Long",
	StatusUnsupportedMediaType:         "Unsupported Media Type",
	StatusRequestedRangeNotSatisfiable: "Requested Range Not Satisfiable",
	StatusExpectationFailed:            "Expectation Failed",
	StatusTeapot:                       "I'm a teapot",
	StatusUnprocessableEntity:          "Unprocessable Entity",
	StatusLocked:                       "Locked",
	StatusFailedDependency:             "Failed Dependency",
	StatusUpgradeRequired:              "Upgrade Required",
	StatusPreconditionRequired:         "Precondition Required",
	StatusTooManyRequests:              "Too Many Requests",
	StatusRequestHeaderFieldsTooLarge:  "Request Header Fields Too Large",
	StatusUnavailableForLegalReasons:   "Unavailable For Legal Reasons",

	StatusInternalServerError:           "Internal Server Error",
	StatusNotImplemented:                "Not Implemented",
	StatusBadGateway:                    "Bad Gateway",
	StatusServiceUnavailable:            "Service Unavailable",
	StatusGatewayTimeout:                "Gateway Timeout",
	StatusHTTPVersionNotSupported:       "HTTP Version Not Supported",
	StatusVariantAlsoNegotiates:         "Variant Also Negotiates",
	StatusInsufficientStorage:           "Insufficient Storage",
	StatusLoopDetected:                  "Loop Detected",
	StatusNotExtended:                   "Not Extended",
	StatusNetworkAuthenticationRequired: "Network Authentication Required",
    */
#include<string>
#include"Timers.h"
#include"Mutex.h"
#include"HttpRequest.h"
#include"Url.h"
#include"HttpHeader.h"
#include<unistd.h>
#include<fcntl.h>
#include"RefBase.h"
#include"RawFile.h"
#include"HttpsTransfer.h"
#include"HttpChunkFilter.h"
#include"GzipDecodeFilter.h"
#include"BufferFilter.h"

struct TaskInfo;

struct Range{
    long begin;
    long end;
    long total;
    long state;
    long idx;
    long size;
    enum {
        RANGE_INIT = -3,
        RANGE_FAILED = -2,
        RANGE_DONE = -1,
    };

    Range(long bg,long ed){
        begin = bg;
        end = ed;
        state = RANGE_INIT;
        total = begin - end > 0 ?begin - end + 1:0;
        size = 0;
    }
    Range(){
        begin = 0;
        end=-1;
        total = 0;
        state = RANGE_INIT;
        size = 0;
    }
    inline void setFailed(){
        state = RANGE_FAILED ;
    }

    inline void setDone(){
        state = RANGE_DONE;
    }

    inline void set(long st){
        this->state  = st;
    }

    inline void reset(){
        state = RANGE_INIT;
    }

    inline int get(){
        return state;
    }
    inline bool done(){
        return state == RANGE_DONE;
    }
    inline bool failed(){
        return state == RANGE_FAILED;
    }
};

class HttpTransfer :public RefBase{
    public:
        typedef int (*BreakFpn)(void *obj,const char*data,int size,sp<BufferUtils> &buff);
        class TransferObserver:public RefBase{
            public :
                TransferObserver(sp<TaskInfo> task = NULL);
                virtual void onStartConnect();
                //return false will stop this transfer or continue
                virtual void onConnected();
                virtual void onSending(long bytes,long total);
                virtual void onSended();
                virtual void onProgress(long bytes,long total);
                virtual void onCompleted();
                virtual void onFailed(int error);
                sp<TaskInfo> mTask;
        };

        static const char *HttpGetHints;
        static const char *HttpPostHints;
        static const char *HttpChunkedEOFHints;
        static const char *downloadDefaultPath;
        static const char *serverRangeUnits;
        enum {
            HTTP_NONE_DOWNLOAD,
            HTTP_PARENT_DOWNLOAD,
            HTTP_CHILD_DOWNLOAD,
        };
        enum {
            HTTP_INIT,
            HTTP_CONNECTING,
            HTTP_CONNECTED,
            HTTP_REDIRECTING,
            HTTP_WRITING,
            HTTP_READING,
            HTTP_DONE,
            HTTP_FAIL,
        };

        HttpTransfer(sp<TaskInfo> task){
            init();
            mTask = task;
            //use default observer
            mObserver = new TransferObserver(mTask);
        }

        ~HttpTransfer(){
            ALOGD("%s %d ",__func__,__LINE__);
        }

        void setObserver(sp<TransferObserver> &obs){
            mObserver = obs;
        }

        void init(){
            mFd = -1;
            mState = HTTP_INIT;
            mRequest = NULL;
            mCanceled = false;
            pipe2(mPipe,O_NONBLOCK);
            mError = 0;
            mTask = NULL;
            mRelocationCount = 0;
            mIsDownload = HTTP_NONE_DOWNLOAD;
            mfilePath = "";
            mChunkFilter = NULL;
            mGzipFilter = NULL;
            mBufferFilter = NULL;
            mIsSeucre = false;
        }

        void cancel(){
            interrupt();
            mCanceled = true;
            return;
        }

        void interrupt(){//may be block
            ALOGD(" http transfer wait for write");
            write(mPipe[1],"a",1);
            return ;
        }

        void dispose(){
            close(mPipe[0]);
            close(mPipe[1]);
            if(mFd != -1){
                close(mFd);
                mFd = -1;
            }
            mState = HTTP_INIT;
        }

        void reset(){
            dispose();
            init();
        }

        inline sp<HttpResponse> getResponse(){
            return mResponse;
        }

        inline sp<HttpRequest> getRequest(){
            return mRequest;
        }

        void setHeaderEntry(const char *entryName,const char *format,...);
        int httpDoTransfer(HttpRequest *req);
        int doGet(const char *url);
        int doPost(const char *url,BufferUtils &buff);
        int doPost(const char *url,sp<BufferUtils> &buffer);
        int doDownload(const char *url,const char *filePath);
        int doRangeDownload(sp<HttpRequest> &req,const char *filePath,Range &rg);
        int httpPost(HttpRequest *req);
        int httpGet(HttpRequest *req);
        int parseHttpVersion(const char *version,int &major,int &minor);
        int parseStatus(const char *buff);
        int doRelocation();
        int identifyReader(sp<BufferUtils> &recvBuffer,struct timeval &tv);
        int commonReader(RawFile &wfile,int count,struct timeval &tv);
        int chunkedReader(sp<BufferUtils> &recvBuffer,struct timeval &tv);
        long parseHex(const char *str,long &data);
        int chunkedParser(const char *srcData,int srcSize ,sp<BufferUtils> &recvBuffer,int &moreData,int &leftSz);
        int socketReader(sp<BufferUtils> &recvBuffer,struct timeval &tv,BreakFpn breakFpn);
        void parseServerRange(const char *rangeStr,Range &range);
    private:
        void installFilters();
        std::string getDownloadFilePath();
        HttpRequest *createRequest(const char *url);
        static int chunkedEOF(void *obj,const char *data,int size,sp<BufferUtils> &buffer);
        static int identifyBreak(void *obj,const char *data,int length,sp<BufferUtils> &buffer);

        int mFd;
        int mState;
        int mPipe[2];
        int mCanceled;
        int mError;
        DurationTimer mDuration;
        sp<HttpRequest> mRequest;
        sp<HttpResponse> mResponse;
        sp<TaskInfo> mTask;
        Mutex mMutex;
        static int mRelocationLimited;
        int mRelocationCount;
        int mIsDownload;
        std::string mfilePath;
        Range mPartialData;
        sp<TransferObserver> mObserver;
        sp<HttpsTransfer> mHttpsSupport;
        sp<HttpChunkFilter> mChunkFilter;
        sp<GzipDecodeFilter> mGzipFilter;
        sp<BufferFilter> mBufferFilter;
        bool mIsSeucre;
};

#endif //__HTTP_TRANSFER_H__
