#ifndef __HTTP_HEADER_H__
#define __HTTP_HEADER_H__
// ReadMIMEHeader reads a MIME-style header from r.
// The header is a sequence of possibly continued Key: Value lines
// ending in a blank line.
// response header
// The returned map m maps CanonicalMIMEHeaderKey(key) to a
//　　HTTP/1.0 200 OK 
//　　Date:Mon,31Dec200104:25:57GMT 
//　　Server:Apache/1.3.14(Unix) 
//　　Content-type:text/html 
//　　Last-modified:Tue,17Apr200106:46:28GMT 
//　　Etag:"a030f020ac7c01:1e9f" 
//　　Content-length:39725426 
//　　Content-range:bytes554554-40279979/40279980 /
//
///requese header
//　　GET http://download.microtool.de:80/somedata.exe 
//　　Host: download.microtool.de 
//　　Accept:*/* 
//　　Pragma: no-cache 
//　　Cache-Control: no-cache 
//　　Referer: http://download.microtool.de/ 
//　　User-Agent:Mozilla/4.04[en](Win95;I;Nav) 
//　　Range:bytes=554554- 
// sequence of values in the same order encountered in the input.
// For example, consider this input:
//	My-Key: Value 1
//	Long-Key: Even
//	       Longer Value
//	My-Key: Value 2
//
//setEntry("User-Agent","SOHUWapRebot");
//setEntry("Accept-Language","zh-cn,zh;q=0.5");
//setEntry("Accept-Charset","GBK,utf-8;q=0.7,*;q=0.7");
//setEntr("Connection","keep-alive");
//
//    const char *HttpHeaderEntries[] ={
//        "Accept",
//        "Accept-Charset",
//        "Accept-Encoding",
//        "Accept-Language",
//        "Accept-Ranges",
//        "Cache-Control",
//        "Cc",
//        "Connection",
//        "Content-Id",
//        "Content-Language",
//        "Content-Length",
//        "Content-Transfer-Encoding",
//        "Content-Type",
//        "Cookie",
//        "Date",
//        "Dkim-Signature",
//        "Etag",
//        "Expires",
//        "From",
//        "Host",
//        "If-Modified-Since",
//        "If-None-Match",
//        "In-Reply-To",
//        "Last-Modified",
//        "Location",
//        "Message-Id",
//        "Mime-Version",
//        "Pragma",
//        "Received",
//        "Return-Path",
//        "Server",
//        "Set-Cookie",
//        "Subject",
//        "To",
//        "User-Agent",
//        "Via",
//        "X-Forwarded-For",
//        "X-Imforwards",
//        "X-Powered-By",
//    }; 
//
//http header entry

#include<string>
#include"BufferUtils.h"
#include"SimpleHash.h"
#include"TypeHelpers.h"


struct HttpHeaderEntry{
    std::string mKey;
    const std::string getKey() const{
        return mKey;
    }
    std::string mValue;
    bool operator==(const HttpHeaderEntry &k) const{
        return mKey ==k.mKey;
    }
};

struct HttpHeader{

    enum{
            StatusContinue           = 100,// RFC 7231, 6.2.1
            StatusSwitchingProtocols = 101,// RFC 7231, 6.2.2
            StatusProcessing         = 102,// RFC 2518, 10.1

            StatusOK                   = 200,// RFC 7231, 6.3.1
            StatusCreated              = 201,// RFC 7231, 6.3.2
            StatusAccepted             = 202,// RFC 7231, 6.3.3
            StatusNonAuthoritativeInfo = 203,// RFC 7231, 6.3.4
            StatusNoContent            = 204,// RFC 7231, 6.3.5
            StatusResetContent         = 205,// RFC 7231, 6.3.6
            StatusPartialContent       = 206,// RFC 7233, 4.1
            StatusMultiStatus          = 207,// RFC 4918, 11.1
            StatusAlreadyReported      = 208,// RFC 5842, 7.1
            StatusIMUsed               = 226,// RFC 3229, 10.4.1

            StatusMultipleChoices   = 300,// RFC 7231, 6.4.1
            StatusMovedPermanently  = 301,// RFC 7231, 6.4.2
            StatusFound             = 302,// RFC 7231, 6.4.3
            StatusSeeOther          = 303,// RFC 7231, 6.4.4
            StatusNotModified       = 304,// RFC 7232, 4.1
            StatusUseProxy          = 305,// RFC 7231, 6.4.5
            _                       = 306,// RFC 7231, 6.4.6 (Unused)
            StatusTemporaryRedirect = 307,// RFC 7231, 6.4.7
            StatusPermanentRedirect = 308,// RFC 7538, 3

            StatusBadRequest                   = 400,// RFC 7231, 6.5.1
            StatusUnauthorized                 = 401,// RFC 7235, 3.1
            StatusPaymentRequired              = 402,// RFC 7231, 6.5.2
            StatusForbidden                    = 403,// RFC 7231, 6.5.3
            StatusNotFound                     = 404,// RFC 7231, 6.5.4
            StatusMethodNotAllowed             = 405,// RFC 7231, 6.5.5
            StatusNotAcceptable                = 406,// RFC 7231, 6.5.6
            StatusProxyAuthRequired            = 407,// RFC 7235, 3.2
            StatusRequestTimeout               = 408,// RFC 7231, 6.5.7
            StatusConflict                     = 409,// RFC 7231, 6.5.8
            StatusGone                         = 410,// RFC 7231, 6.5.9
            StatusLengthRequired               = 411,// RFC 7231, 6.5.10
            StatusPreconditionFailed           = 412,// RFC 7232, 4.2
            StatusRequestEntityTooLarge        = 413,// RFC 7231, 6.5.11
            StatusRequestURITooLong            = 414,// RFC 7231, 6.5.12
            StatusUnsupportedMediaType         = 415,// RFC 7231, 6.5.13
            StatusRequestedRangeNotSatisfiable = 416,// RFC 7233, 4.4
            StatusExpectationFailed            = 417,// RFC 7231, 6.5.14
            StatusTeapot                       = 418,// RFC 7168, 2.3.3
            StatusUnprocessableEntity          = 422,// RFC 4918, 11.2
            StatusLocked                       = 423,// RFC 4918, 11.3
            StatusFailedDependency             = 424,// RFC 4918, 11.4
            StatusUpgradeRequired              = 426,// RFC 7231, 6.5.15
            StatusPreconditionRequired         = 428,// RFC 6585, 3
            StatusTooManyRequests              = 429,// RFC 6585, 4
            StatusRequestHeaderFieldsTooLarge  = 431,// RFC 6585, 5
            StatusUnavailableForLegalReasons   = 451,// RFC 7725, 3

            StatusInternalServerError           = 500,// RFC 7231, 6.6.1
            StatusNotImplemented                = 501,// RFC 7231, 6.6.2
            StatusBadGateway                    = 502,// RFC 7231, 6.6.3
            StatusServiceUnavailable            = 503,// RFC 7231, 6.6.4
            StatusGatewayTimeout                = 504,// RFC 7231, 6.6.5
            StatusHTTPVersionNotSupported       = 505,// RFC 7231, 6.6.6
            StatusVariantAlsoNegotiates         = 506,// RFC 2295, 8.1
            StatusInsufficientStorage           = 507,// RFC 4918, 11.5
            StatusLoopDetected                  = 508,// RFC 5842, 7.2
            StatusNotExtended                   = 510,// RFC 2774, 7
            StatusNetworkAuthenticationRequired = 511,// RFC 6585, 6
    };

    static const char *encodingChunkedHints;
    static const char *encodingCompressHints;
    static const char *encodingDeflateHints;
    static const char *encodingGzipHints;
    static const char *encodingIdentifyHints;

    static const char *connectionHints;
    static const char *hostHints;
    static const char *userAgentHints;
    static const char *contentLengthHints;
    static const char *transferEncodingHints;
    static const char *trailerHints;
    static const char *lineHints; 
    static const char *headerHints;
    static const char *locationHints;
    static const char *clientRangeHints;
    static const char *serverRangeHints;
    static const char *acceptRangeHints;
    static const char *etagHints;
    static const char *dispositionHints;

    HttpHeader():mEntries(20,getStringHash){

    }

    static int checkHeader(sp<BufferUtils> &buffer);
    //static function to parser header only
    //Buffer is data of header
    static HttpHeader* parser(sp<BufferUtils> &buffer,HttpHeader *ptrHeader);
    //add entry
    void setEntry(const std::string key,const std::string value);
    void setEntry(const char *key,const char *format,...);
    void toString(BufferUtils &buffer);
    //get entry value 
    const std::string& getValues(const std::string key);
    //get entry value 
    const std::string& getValues(const char *key);
    //entries key=>value
    SimpleHash<std::string,HttpHeaderEntry> mEntries;
};

#endif //
