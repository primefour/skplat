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


class HttpTransfer :public RefBase{
    public:
        typedef int (*BreakFpn)(void *obj,const void *data,int size);
        static const char *HttpGetHints;
        static const char *HttpPostHints;
        static const char *HttpChunkedEOFHints;

        HttpTransfer(){
            init();
        }
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
        void init(){
            mFd = -1;
            mState = HTTP_INIT;
            mRequest = NULL;
            pipe2(mPipe,O_NONBLOCK);
            mError = 0;
            mTask = NULL;
            mRelocationCount = 0;
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

        int httpDoTransfer(HttpRequest *req);
        int doGet(const char *url);
        int doPost(const char *url,BufferUtils &buff);
        int httpPost(HttpRequest *req);
        int httpGet(HttpRequest *req);
        int parseHttpVersion(const char *version,int &major,int &minor);
        int parseStatus(const char *buff);
        int doRelocation();
        int identifyReader(sp<BufferUtils> &recvBuffer,struct timeval &tv);
        int commonReader(sp<BufferUtils> &recvBuffer,int count,struct timeval &tv);
        int chunkedReader(sp<BufferUtils> &recvBuffer,struct timeval &tv);
        long parseHex(const char *str,long &data);
        int chunkedParser(const char *srcData,int srcSize ,sp<BufferUtils> &recvBuffer,int &moreData,int &leftSz);
        int socketReader(sp<BufferUtils> &recvBuffer,struct timeval &tv,BreakFpn breakFpn);
    private:
        static int chunkedEOF(void *obj,const void *data,int size);
        static int identifyBreak(void *obj,const void *data,int length);

        int mFd;
        int mState;
        int mPipe[2];
        int mError;
        DurationTimer mDuration;
        sp<HttpRequest> mRequest;
        sp<HttpResponse> mResponse;
        Mutex mMutex;
        void *mTask;
        static int mRelocationLimited;
        int mRelocationCount;
};

#endif //__HTTP_H__
