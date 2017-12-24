#ifndef __HTTP_REQUEST_H__
#define __HTTP_REQUEST_H__
#include"Url.h"
#include"HttpHeader.h"
#include"BufferUtils.h"
#include<string>
#include"RefBase.h"
#include"HttpResponse.h"

struct HttpResponse;

struct HttpRequest:public RefBase{
    HttpRequest(){
        mProtoMajor = 1;
        mProtoMinor = 1;
        mProto = "HTTP/1.1"; 
        mClose = true;
        mUseProxy = false;
        mContentLength = 0;
    }
	// Method specifies the HTTP method (GET, POST, PUT, etc.).
	// For client requests an empty string means GET.
    std::string mMethod; 
	// For client requests, the URL's Host specifies the server to
	// connect to, while the Request's Host field optionally
	// specifies the Host header value to send in the HTTP
	// request.
    Url mUrl;
	// The protocol version for incoming server requests.
	//
	// For client requests these fields are ignored. The HTTP
	// client code always uses either HTTP/1.1 or HTTP/2.
	// See the docs on Transport for details.
    std::string mProto; // "HTTP/1.0"
	int mProtoMajor;    // 1
	int mProtoMinor;    // 0
	// For client requests, certain headers such as Content-Length
	// and Connection are automatically written when needed and
	// values in Header may be ignored. See the documentation
	// for the Request.Write method.
    HttpHeader mHeader;

	// ContentLength records the length of the associated content.
	// The value -1 indicates that the length is unknown.
	// Values >= 0 indicate that the given number of bytes may
	// be read from Body.
	// For client requests, a value of 0 with a non-nil Body is
	// also treated as unknown.
	long mContentLength; 

	// TransferEncoding lists the transfer encodings from outermost to
	// innermost. An empty list denotes the "identity" encoding.
	// TransferEncoding can usually be ignored; chunked encoding is
	// automatically added and removed as necessary when sending and
	// receiving requests.
    std::string mTransferEncoding;

	// Body is the request's body.
	// For client requests a nil body means the request has no
	// body, such as a GET request. The HTTP Client's Transport
	// is responsible for calling the Close method.
	BufferUtils mBody;
	// For client requests, setting this field prevents re-use of
	// TCP connections between requests to the same hosts, as if
	// Transport.DisableKeepAlives were set.
	bool mClose; 
    // response from server
    sp<HttpResponse> mResp;

    //Proxy url
    Url mProxyUrl;
    bool mUseProxy;
};

#endif //__HTTP_REQUEST_H__
