#include"HttpTransfer.h"
#include"SocksConnect.h"
#include"Vector.h"
#include"DNSCache.h"
#include"SocketAddress.h"
#include"BufferUtils.h"
#include"Url.h"
#include"HttpHeader.h"
#include<unistd.h>
#include<fcntl.h>
#include<string>
#include"Log.h"
#include"TaskInfo.h"
#include<stdlib.h>
#include"AppUtils.h"
#include<stdarg.h>
#include"RawFile.h"
#include"FileUtils.h"
#include"DownloaderManager.h"
#include"HttpChunkFilter.h"
#include"GzipDecodeFilter.h"

#define TRANSFER_CONNECT_TIMEOUT 15000 //ms
#define TRANSFER_SELECT_TIMEOUT 60000 //ms

int HttpTransfer::mRelocationLimited = 3;
const char *HttpTransfer::HttpGetHints = "GET";
const char *HttpTransfer::HttpPostHints = "POST";
const char *HttpTransfer::HttpChunkedEOFHints = "\r\n0\r\n";
const char *HttpTransfer::downloadDefaultPath= "./";
const char *HttpTransfer::serverRangeUnits = "bytes";

void HttpTransfer::setHeaderEntry(const char *entryName,const char *format,...){
    if(mRequest != NULL){
        va_list params;
        va_start(params, format);
        mRequest->mHeader.setEntry(entryName,format,params);
        va_end(params);
    }
}


HttpRequest *HttpTransfer::createRequest(const char *url){
    //create a get request obj
    HttpRequest *req= new HttpRequest();
    if(Url::parseUrl(url,&(req->mUrl)) == NULL){
        delete(req);
        return NULL;
    }
    req->mProto="HTTP/1.1";
    req->mProtoMajor = 1;
    req->mProtoMinor = 1;
    req->mHeader.setEntry(HttpHeader::acceptHints,"text/html,application/xhtml+xml,application/xml;q=0.9,image/webp,image/apng,*/* q=0.8");
    req->mHeader.setEntry(HttpHeader::userAgentHints,"Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/63.0.3239.84 Safari/537.36");
    req->mHeader.setEntry(HttpHeader::acceptLanguageHints,"zh-CN,zh;q=0.9");
    //req->mHeader.setEntry(HttpHeader::acceptEncodingHints,"gzip,deflate");
    req->mHeader.setEntry(HttpHeader::hostHints,req->mUrl.mHost.c_str());
    if(mIsDownload == HTTP_CHILD_DOWNLOAD){
        //Range:bytes=554554- 
        //Range:bytes=0-100 
        req->mHeader.setEntry(HttpHeader::clientRangeHints,"bytes=%d-%d",mPartialData.begin,mPartialData.end);
    }
    return req;
}

int HttpTransfer::doRangeDownload(sp<HttpRequest> &req,const char *filePath,Range &rg){
    mIsDownload = HTTP_CHILD_DOWNLOAD;
    mfilePath = filePath;
    mPartialData = rg;
    //get offset and resume from the end of file
    return doGet(req->mUrl.mHref.c_str());
}

int HttpTransfer::doDownload(const char *url,const char *filePath){
    mIsDownload = HTTP_PARENT_DOWNLOAD;
    mfilePath = filePath;
    return doGet(url);
}

int HttpTransfer::doGet(const char *url){
    HttpRequest * req = createRequest(url);
    if(req == NULL){
        return BAD_VALUE;
    }
    req->mMethod = "GET";
    //bind request and response
    mRequest = req;
    HttpResponse *response = new HttpResponse();
    mRequest->mResp = response;
    mResponse = response;
    mResponse->mRequest = req;
    //do request
    return httpGet(req);
}

int HttpTransfer::doPost(const char *url,sp<BufferUtils> &buffer){
    HttpRequest * req = createRequest(url);
    //create a post request obj
    req->mMethod = "POST";
    ASSERT(buffer->size() > 0,"Invalidate post body");
    if(req == NULL ||buffer->size() <= 0){
        return BAD_VALUE;
    }
    //bind request and response
    mRequest = req;
    HttpResponse *response = new HttpResponse();
    mRequest->mResp = response;
    mResponse = response;
    mResponse->mRequest = req;
    mRequest->mBody = buffer;
    //post action
    return httpPost(req);
}



int HttpTransfer::doPost(const char *url,BufferUtils &buff){
    HttpRequest * req = createRequest(url);
    //create a post request obj
    req->mMethod = "POST";
    ASSERT(buff.size() > 0,"Invalidate post body");
    if(req  == NULL ||buff.size() <= 0){
        return BAD_VALUE;
    }
    //bind request and response
    mRequest = req;
    HttpResponse *response = new HttpResponse();
    mRequest->mResp = response;
    mResponse = response;
    mResponse->mRequest = req;
    mRequest->mBody = new BufferUtils();
    *(mRequest->mBody) = buff;
    //post action
    return httpPost(req);
}

int HttpTransfer::httpGet(HttpRequest *req){
    return httpDoTransfer(req);
}

int HttpTransfer::httpPost(HttpRequest *req){
    return httpDoTransfer(req);
}

long HttpTransfer::parseHex(const char *str,long &data){
    int i = 0;
    if(str == NULL){
        data = 0;
        return 0;
    }
    int size = strlen(str);
    for(i = 0 ;i < size ;i++){
        int tmp = 0;
        if(str[i] >= '0' && str[i] <='9'){
            tmp = str[i] - '0' ;
        }else if(str[i] >= 'a' && str[i] <= 'f'){
            tmp = str[i] -'a' + 10;
        }else if(str[i] >= 'A' && str[i] <= 'F'){
            tmp = str[i] -'A' + 10;
        }else{
            break;
        }
        data <<= 4;
        data |= tmp;
    }
	return data;
}

int HttpTransfer::chunkedEOF(void *obj,const char *txd,int size,sp<BufferUtils> &buffer){
    HttpTransfer *hobj = (HttpTransfer*)obj;
    const char *data = (const char *)txd;

    if(size >= 0){
        hobj->mBufferFilter->write(txd,size);
        while(hobj->mBufferFilter->read(buffer));
    }

    if(hobj != NULL && hobj->mObserver != NULL){
        hobj->mObserver->onProgress(hobj->mBufferFilter->size(),-1);
    }
    if(hobj->mBufferFilter->endOfFile()){
        return true;
    }else{
        return false;
    }

}


int HttpTransfer::socketReader(sp<BufferUtils> &recvBuffer,struct timeval &tv,BreakFpn breakFpn ){
    //ALOGD("socketReader recvBuffer %s  size :%zd ",recvBuffer->data(),recvBuffer->size());
    //need more data
    int n = 0;
    int ret = 0;
    fd_set rdSet,wrSet;
    char tmpBuff[1024] ={0};
    while(1){
        FD_ZERO(&rdSet);
        FD_SET(mPipe[1],&rdSet);
        FD_SET(mFd,&rdSet);
        int maxFd = mPipe[1] > mFd ?mPipe[1]:mFd;
        maxFd ++;
        //ALOGD("http get read wait select begin tv timeout value %ld ",tv.tv_sec *1000 + tv.tv_usec/1000);
        ret = select(maxFd,&rdSet,NULL,NULL,&tv);
        //ALOGD("http get read wait select end");
        if(ret > 0){
            if(FD_ISSET(mPipe[1],&rdSet)){
                ALOGW("http get write abort by user");
                return ABORT_ERROR;
            }else if(FD_ISSET(mFd,&rdSet)){
                if(!mIsSeucre){
                    n = read(mFd,tmpBuff,sizeof(tmpBuff));
                }else{
                    n = mHttpsSupport->read(tmpBuff,sizeof(tmpBuff));
                    if(n == HTTPS_WOULD_BLOCK){
                        continue;
                    }
                }
                if(n > 0){
                    if(breakFpn(this,tmpBuff,n,recvBuffer)){
                        return recvBuffer->size();
                    }
                    //ALOGD("tmpBuff = %s ",tmpBuff);
                    //recvBuffer->append(tmpBuff,n);
                    //check whether is complete
                    //ALOGD("n = %d ==>  %s ==> %zd ",n,recvBuffer->data(),recvBuffer->size());
                    //if(breakFpn(this,recvBuffer->data(),recvBuffer->size())){
                    //    return OK;
                    //}
                }else if(n < 0){
                    ALOGD("recv data fail %p size: %zd error:%s",
                            recvBuffer->data(),recvBuffer->size(),strerror(errno));
                    mError ++;
                    return UNKNOWN_ERROR;
                }else{
                    ALOGD("recv data complete %p size: %zd  error:%s",
                            recvBuffer->data(),recvBuffer->size(),strerror(errno));
                    break;
                }
            }else{
                ALOGE("http recv data fail!");
                return UNKNOWN_ERROR;
            }
        }else if(ret == 0){
            ALOGE("http recv data timeout !");
            return TIMEOUT_ERROR;
        }else{
            ALOGE("http recv data  fail! ret = %d %s ",ret,strerror(errno));
            return UNKNOWN_ERROR;
        }
    }
    return OK;
}


int HttpTransfer::chunkedReader(sp<BufferUtils> &recvBuffer,struct timeval &tv){
    return socketReader(recvBuffer,tv,chunkedEOF);
}

int HttpTransfer::chunkedParser(const char *srcData,int srcSize ,sp<BufferUtils> &recvBuffer,int &moreData,int &leftSz){
    const char *data = srcData;
    const char *seekData = data;
    const char *hints = NULL;
    long count = 0;
    int complete = 0;
    //do with more date
    if(moreData != 0){
        if(srcSize < moreData){
            recvBuffer->append(srcData,srcSize);
            moreData -= srcSize;
            leftSz = 0;
            ALOGD("moreData = %d  leftSz = %d ",moreData,leftSz);
            return NEED_MORE;
        }else{
            recvBuffer->append(srcData,srcSize);
            moreData = 0;
            data = data + moreData;
            seekData = data;
            srcSize -= moreData;
        }
    }

    //remove /r/n
    if(srcSize > 2 && *seekData == '\r' && *(seekData+1) == '\n'){
        data += 2;
        seekData = seekData;
        srcSize -= 2;
    }

    while(1){
        ALOGD("seekData = %s ",seekData);
        hints = strstr(seekData,HttpHeader::lineHints);
        if(hints == NULL){
            ALOGD("hints not found ");
            break;
        }
        //trim \t \n for same site
        while(seekData < hints  && (*seekData == ' ' || *seekData == '\t')){
            seekData ++;
        }

        count = parseHex(seekData,count);
        if(count == 0){
            ALOGD("count is 0 for string:%s ",seekData);
            complete = 1;
            break;
        }else{
            ALOGD("count is %ld ",count);
        }
        seekData = hints + 2;

        int dataSize = srcSize - (long)seekData + (long)data;
        ALOGD("data size is %d ",dataSize);
        if(dataSize < count){
            //empty
            recvBuffer->append(seekData,dataSize);
            count -= dataSize;
            seekData += dataSize;
            ALOGD("data size is empty");
            break;
        }else{
            recvBuffer->append(seekData,count);
            seekData += count;
            count = 0;
        }

        dataSize = srcSize - (long)seekData + (long)data;

        if(dataSize > 2){
            seekData +=2;
        }else{
            //empty
            ALOGD("xkjdata size is empty %d ",dataSize);
            break;
        }
    }
    leftSz = srcSize - (long)seekData + (long)data;
    moreData = count;
    ALOGD("complete = %d moreData = %d  leftSz = %d ",complete,moreData,leftSz);
    if(complete == 1){
        return OK;
    }else{
        return NEED_MORE;
    }
}


int HttpTransfer::identifyBreak(void *obj,const char *data,int length,sp<BufferUtils> &buffer){
    HttpTransfer *hobj = (HttpTransfer*)obj;
    ALOGD("xxx length  = %ld hobj->mResponse->mContentLength %ld ",buffer->size(),hobj->mResponse->mContentLength);
    if(length < 0){
        return true;
    }
    buffer->append(data,length);
    if(hobj != NULL && hobj->mObserver != NULL){
        hobj->mObserver->onProgress(length,hobj->mResponse->mContentLength);
    }

    if(buffer->size() >= hobj->mResponse->mContentLength){
        return true; 
    }else{
        return false;
    }

}

int HttpTransfer::identifyReader(sp<BufferUtils> &recvBuffer,struct timeval &tv){
    ASSERT(mResponse->mContentLength >= 0,"mResponse->mContentLength is %ld ",mResponse->mContentLength);
    return socketReader(recvBuffer,tv,identifyBreak);
}

int HttpTransfer::doRelocation(){
    if(mRelocationCount > mRelocationLimited ){
        ALOGE("relocation count is beyond the limit:%d :count %d",mRelocationLimited, mRelocationCount);
        return INVALID_OPERATION ;
    }
    std::string locHost = mResponse->mHeader.getValues(HttpHeader::locationHints);
    if(locHost.empty()){
        ALOGE("NO LOCATION ENTRY IN HEADER");
        return BAD_VALUE;
    }

    mRelocationCount ++;
    ALOGD("relocation is %s ",locHost.c_str());
    mResponse->relocation(locHost.c_str());
    //delete response and rebind the new response
    mResponse->mRequest->mResp = new HttpResponse();
    mRequest->mResp->mRequest = mRequest;
    mResponse = mRequest->mResp;
    //do get requset
    return httpGet(mRequest.get());
}


int HttpTransfer::parseStatus(const char *buff){
    int i = 0 ;
    int begin = 0,end = 0;
    std::string sArray[4];
    int j = 0;
    while( buff[i] != '\r' && buff[i+1] != '\n'){
        if(buff[i] == ' '){
            sArray[j++] = ::trim(buff,begin,i);
            begin = i;
        }
        i++;
        if(buff[i] == '\r' && buff[i +1] == '\n'){
            sArray[j++] = ::trim(buff,begin,i);
        }
    }
    //log
    for(i = 0 ;i < 4 ;i ++){
        if(!sArray[i].empty()){
            ALOGD("%s ",sArray[i].c_str());
        }
    }
    //check status description 
    if(sArray[2].empty()){
        ALOGE("malformed HTTP response %s",buff);
        return BAD_VALUE;
    }

    ALOGD("HTTP response %s",sArray[2].c_str());
    //check status code
	if (sArray[1].size() != 3 ){
        ALOGE("malformed HTTP status code %s",sArray[1].c_str());
		return BAD_VALUE;
	}

    mResponse->mProto = sArray[0];// e.g. "HTTP/1.0"
    mResponse->mStatusCode = ::atoi(sArray[1].c_str());
    mResponse->mStatus = sArray[1] + " " + sArray[2];   // e.g. "200 OK"
    if(mResponse->mStatusCode < 0){
        ALOGE("malformed HTTP status code %s ",sArray[1].c_str());
        return BAD_VALUE;
    }

    if(parseHttpVersion(sArray[0].c_str(),mResponse->mProtoMajor,mResponse->mProtoMinor) == BAD_VALUE){
		ALOGE("malformed HTTP version %s ",sArray[0].c_str());
        return BAD_VALUE;
    } 
    return OK;
}

// "HTTP/1.0" 
int HttpTransfer::parseHttpVersion(const char *version,int &major,int &minor){
    static const char *httpVersionHints11 = "HTTP/1.1";
    static const char *httpVersionHints10 = "HTTP/1.0";
    if(strstr(version,httpVersionHints11) != NULL){
        major = 1;
        minor = 1;
        return OK;
    }

    if(strstr(version,httpVersionHints10) != NULL){
        major = 1;
        minor = 0;
        return OK;
    }
    const char *dot = strstr(version,".");
    if(dot != NULL){
        major = ::atoi(dot -1);
        minor = ::atoi(dot +1);
    }else{
        major = 0;
        minor = 0;
        return BAD_VALUE;
    }
}

int HttpTransfer::httpDoTransfer(HttpRequest *req){
    const char *host = NULL;
    const char *service = NULL;
    bool isSecure = false;
    if(mObserver != NULL){
        mObserver->onStartConnect();
    }

    if(req->mUseProxy){
        host = req->mProxyUrl.mHost.c_str();
        if(req->mProxyUrl.mPort.empty()){
            service = req->mProxyUrl.mSchema.c_str();
        }else{
            service = req->mProxyUrl.mPort.c_str();
        }
        isSecure  = req->mProxyUrl.isSecure();
    }else{
        host = req->mUrl.mHost.c_str();
        if(req->mUrl.mPort.empty()){
            service = req->mUrl.mSchema.c_str();
        }else{
            service = req->mUrl.mPort.c_str();
        }
        isSecure  = req->mUrl.isSecure();
    }
    mIsSeucre = isSecure;
    //get address using dns cache
    sp<DnsCache>& Cache = DnsCache::getInstance() ;
    //get Address
    Vector<SocketAddress> addrs = Cache->getAddrs(host,service);
    int ret = 0;
    TaskInfo *task = (TaskInfo*)mTask;
    {
        //connect to server
        SocksConnect connect(addrs);
        if(task != NULL){
            ret = connect.connect(task->mConnTimeout);
        }else{
            ret = connect.connect(TRANSFER_CONNECT_TIMEOUT);
        }

        if(ret != OK){
            ALOGD("http get connect fail ");
            if(mObserver != NULL){
                mObserver->onConnected(false);
            }
            return UNKNOWN_ERROR;   
        }else{
            if(mObserver != NULL){
                ret = mObserver->onConnected(true);
                if(!ret){
                    ALOGE("abort by user");
                    return ABORT_ERROR;
                }
            }
        }
        //get validate socket fd
        mFd = connect.getSocket();
    }
    ALOGD("socket fd = %d ",mFd);

    //check is secure connect
    if(mIsSeucre){
        mHttpsSupport = new HttpsTransfer(mFd,host);
        //do https shake 
        //note:for lack of root cert,we will ignore the verify result of web cert now
        //fix me
        mHttpsSupport->sslShake();
    }

    BufferUtils sendBuffer;
    char tmpBuff[1024]={0};
    //create http header
    if(req->mUseProxy){
        /*
         * GET http://www.example.com/ HTTP/1.1
         * Host: www.example.com
         * Proxy-Connection: keep-alive
         */
        snprintf(tmpBuff,sizeof(tmpBuff),"%s %s %s \r\n",req->mMethod.c_str(),
                req->mUrl.mHref.c_str(),req->mProto.c_str());
        req->mHeader.setEntry("Proxy-Connection","keep-alive");
    }else{
        std::string xpath = "/";
        if(!req->mUrl.mPath.empty()){
            xpath += req->mUrl.mPath;
        }
        snprintf(tmpBuff,sizeof(tmpBuff),"%s %s %s\r\n",req->mMethod.c_str(), 
                xpath.c_str(),req->mProto.c_str());
        req->mHeader.setEntry("Connection","keep-alive");
    }

    sendBuffer.append(tmpBuff,strlen(tmpBuff));

    if(req->mMethod == HttpPostHints){
        if(req->mBody->size() != 0){
            req->mHeader.setEntry(HttpHeader::contentLengthHints,"%d",req->mBody->size());
        }
    }

    //add http header entry
    req->mHeader.toString(sendBuffer);
    ALOGD("%s ",sendBuffer.data());
    if(req->mMethod == HttpPostHints){
        ASSERT(req->mBody->size() > 0,"invalidate post request ");
        //add post data
        sendBuffer.append(req->mBody->data(),req->mBody->size());
    }

    fd_set rdSet,wrSet;
    int n = 0;
    int nsended = 0;
    struct timeval tv;

    if(task != NULL){
        tv.tv_sec = task->mTaskTimeout /1000;
        tv.tv_usec = task->mTaskTimeout %1000 * 1000;
    }else{
        tv.tv_sec = TRANSFER_SELECT_TIMEOUT;
        tv.tv_usec = 0;
    }

    while(nsended < sendBuffer.size()){
        FD_ZERO(&wrSet); 
        FD_ZERO(&rdSet);
        FD_SET(mPipe[1],&rdSet);
        FD_SET(mFd,&wrSet);
        int maxFd = mPipe[1] > mFd ?mPipe[1]:mFd;
        maxFd ++;
        ALOGD("http get write wait select begin tv timeout value %ld ",tv.tv_sec *1000 + tv.tv_usec/1000);
        ret = select(maxFd,&rdSet,&wrSet,NULL,&tv);
        ALOGD("http get write wait select end");
        if(ret > 0){
            if(FD_ISSET(mPipe[1],&rdSet)){
                ALOGD("transfer http get is aborted by user");
                mError ++;
                return ABORT_ERROR;
            }else if (FD_ISSET(mFd,&wrSet)){
                if(!mIsSeucre){
                    n = write(mFd,sendBuffer.dataWithOffset(),sendBuffer.size() - sendBuffer.offset());
                }else{
                    n = mHttpsSupport->write(sendBuffer.dataWithOffset(),sendBuffer.size() - sendBuffer.offset());
                    if(n == HTTPS_WOULD_BLOCK){
                        continue;
                    }
                }
                if(n > 0){
                    sendBuffer.offset(n,SEEK_CUR);
                    nsended += n;
                    if(mObserver != NULL){
                        mObserver->onSending(nsended,sendBuffer.size());
                    }
                }else if(n < 0){
                    ALOGD("send data fail %p size: %zd  ret = %d err :%s ",sendBuffer.data(),sendBuffer.size(),n,strerror(errno));
                    mError ++;
                    return UNKNOWN_ERROR;
                }
            }else{
                //unknown error
                ALOGE("SOCKET ERROR %s ",strerror(errno));
                mError ++;
                return UNKNOWN_ERROR;
            }
        }else if(ret == 0){
            //timeout
            ALOGW("ret %d SOCKET SEND TIMEOUT %s ",ret,strerror(errno));
            mError ++;
            return TIMEOUT_ERROR;
        }else {
            //socket error
            ALOGE("ret %d SOCKET ERROR %s ",ret,strerror(errno));
            return UNKNOWN_ERROR;
        }
    }

    if(mError){
        ALOGE("UNKNOWN_ERROR HTTP DO TRANSFER");
        return UNKNOWN_ERROR;
    }

    if(mObserver != NULL){
        ret = mObserver->onSended();
        if(!ret){
            ALOGW("send complete and abort by user");
            return ABORT_ERROR;
        }
    }

    sp<BufferUtils> tmpBuffer = new BufferUtils();
    n = 0;
    int nrecved = 0;
    int headerFind = 0;
    while(1){
        FD_ZERO(&rdSet);
        FD_SET(mPipe[1],&rdSet);
        FD_SET(mFd,&rdSet);
        int maxFd = mPipe[1] > mFd ?mPipe[1]:mFd;
        maxFd ++;
        ALOGD("http get read wait select begin tv timeout value %ld ",tv.tv_sec *1000 + tv.tv_usec/1000);
        ret = select(maxFd,&rdSet,NULL,NULL,&tv);
        ALOGD("http get read wait select end");
        if(ret > 0){
            if(FD_ISSET(mPipe[1],&rdSet)){
                ALOGW("http get write abort by user");
                return ABORT_ERROR;
            }else if(FD_ISSET(mFd,&rdSet)){
                memset(tmpBuff,0,sizeof(tmpBuff));
                if(!mIsSeucre){
                    n = read(mFd,tmpBuff,sizeof(tmpBuff) -1);
                }else{
                    n = mHttpsSupport->read(tmpBuff,sizeof(tmpBuff) -1);
                    if(n == HTTPS_WOULD_BLOCK){
                        continue;
                    }
                }

                if(n > 0){
                    tmpBuffer->append(tmpBuff,n);
                    nrecved += n;
                    int noffset = -1;
                    //check header whether is complete
                    if(!headerFind && (noffset = HttpHeader::checkHeader(tmpBuffer)) != -1){
                        headerFind  = 1;
                        //parse header
                        if(OK != parseStatus((const char *)tmpBuffer->data())){
                            ALOGE("recv data http header %s failed",(const char *)tmpBuffer->data());
                            return UNKNOWN_ERROR;
                        }else{
                            ALOGD("version %d.%d status code %d status description: %s",
                                    mResponse->mProtoMajor,mResponse->mProtoMinor,
                                    mResponse->mStatusCode,mResponse->mStatus.c_str());
                        }
                        //ALOGD("recv data http header %s ",(const char *)tmpBuffer->data());
                        if(mResponse->mHeader.parser(tmpBuffer,&mResponse->mHeader) == NULL){
                            ALOGE("recv data parse http header fail ");
                            return UNKNOWN_ERROR;
                        }else{
                            sp<BufferUtils> debugBuffer = new BufferUtils();
                            mResponse->mHeader.toString(*debugBuffer);
                            ALOGD("recv data parse http header offset : %d :%s ",noffset,(const char *)debugBuffer->data());
                            //seek to data
                            tmpBuffer->offset(noffset,SEEK_SET);
                            break;
                        }
                    }
                    //ALOGD("recv data %p size: %zd n :%d ", tmpBuffer->data(),tmpBuffer->size(),n);
                }else if(n < 0){
                    ALOGD("recv data fail %p size: %zd error:%s",
                            tmpBuffer->data(),tmpBuffer->size(),strerror(errno));
                    mError ++;
                    return UNKNOWN_ERROR;
                }else{
                    ALOGD("recv data complete %p size: %zd  error:%s",
                            tmpBuffer->data(),tmpBuffer->size(),strerror(errno));
                    break;
                }

            }else{
                ALOGE("http get recv data  fail!");
                return UNKNOWN_ERROR;
            }
        }else if(ret == 0){
            ALOGE("http get recv data timeout !");
            return TIMEOUT_ERROR;
        }else{
            ALOGE("http get recv data  fail! ret = %d %s ",ret,strerror(errno));
            return UNKNOWN_ERROR;
        }
    }

    //    301 (Moved Permanently)
    //    302 (Found)
    //    303 (See Other)
    //    307 (Temporary Redirect)
    //    308 (Permanent Redirect)
    if(mResponse->mStatusCode == HttpHeader::StatusFound ||
            mResponse->mStatusCode == HttpHeader::StatusSeeOther||
            mResponse->mStatusCode == HttpHeader::StatusMovedPermanently||
            mResponse->mStatusCode == HttpHeader::StatusTemporaryRedirect ||
            mResponse->mStatusCode == HttpHeader::StatusPermanentRedirect 
      ){
        //close fd 
        close(mFd);
        mFd = -1;
        //do redirect
        if(doRelocation() != OK){
            return UNKNOWN_ERROR;
        }
    }
    
    if(mResponse->mStatusCode >= HttpHeader::StatusBadRequest &&
            mResponse->mStatusCode <= HttpHeader::StatusUnavailableForLegalReasons){
        ALOGE("request format error code is %d ",mResponse->mStatusCode);
        return BAD_VALUE;
    }

    if(mResponse->mStatusCode >= HttpHeader::StatusInternalServerError &&
            mResponse->mStatusCode <= HttpHeader::StatusNetworkAuthenticationRequired){
        ALOGE("server error code is %d ",mResponse->mStatusCode);
        return UNKNOWN_ERROR;
    }


    if(mResponse->mStatusCode == HttpHeader::StatusSwitchingProtocols ){
        ALOGE("don't support switch protocol code is %d ",mResponse->mStatusCode);
        return BAD_VALUE;
    }

    std::string len = mResponse->mHeader.getValues(HttpHeader::contentLengthHints);
    if(!len.empty()){
        ALOGD(">>>len = %s",len.c_str());
        mResponse->mContentLength = ::atoi(len.c_str()); 
        if(mResponse->mContentLength < 0){
            ALOGE("content lenght is malform  %ld < 0",mResponse->mContentLength);
            return UNKNOWN_ERROR;
        }
    }else{
        mResponse->mContentLength = -1;
    }

    //parse http head 
    std::string contentEncoding = mResponse->mHeader.getValues(HttpHeader::contentEncodingHints);
    if(!contentEncoding.empty()){
        if(strcasestr(contentEncoding.c_str(),"gzip")){
            mResponse->mUncompressed =false;
        }else if(strcasestr(contentEncoding.c_str(),"deflate")){
            mResponse->mUncompressed =false;
        }else{
            ALOGE("content encode is %s not support",contentEncoding.c_str());
            return UNKNOWN_ERROR;
        }
        mGzipFilter = new GzipDecodeFilter();
    }

    mResponse->mTransferEncoding = mResponse->mHeader.getValues(HttpHeader::transferEncodingHints); 
    if(mResponse->mTransferEncoding == HttpHeader::encodingIdentifyHints){
        mResponse->mUncompressed =true;
    }else if(mResponse->mTransferEncoding == "gzip"){
        mResponse->mUncompressed =false;
        mGzipFilter = new GzipDecodeFilter();
    }if(mResponse->mTransferEncoding == "deflate"){
        mResponse->mUncompressed =false;
        mGzipFilter = new GzipDecodeFilter();
    }

    //check connect header entry
    std::string conn = mResponse->mHeader.getValues(HttpHeader::connectionHints);
    if(conn.empty()){
        mResponse->mClose = true;
    }else if(conn == "Close" || conn == "close"){
        mResponse->mClose = true;
    }else{
        mResponse->mClose = false;
    }

    sp<BufferUtils> recvBuffer = NULL;
    if(task != NULL){
        recvBuffer = task->mRecvData;
    }else{
        ALOGW("HTTP TRANSFER NO TASK BIND");
        mResponse->mBody = new BufferUtils();
        recvBuffer = mResponse->mBody;
    }

    //ALOGD("tmpBuffer->dataWithOffset() = %s tmpBuffer->size():%zd  tmpBuffer->offset() = %ld ",
    //        tmpBuffer->dataWithOffset(),tmpBuffer->size(),tmpBuffer->offset());
    if(mIsDownload == HTTP_NONE_DOWNLOAD){
        ret = OK;
        if(mResponse->mTransferEncoding == HttpHeader::encodingChunkedHints || mResponse->mContentLength == -1){
            mChunkFilter = new HttpChunkFilter();
            installFilters();
            if(tmpBuffer->size() > tmpBuffer->offset()){
                mBufferFilter->write(tmpBuffer->dataWithOffset(),tmpBuffer->size() - tmpBuffer->offset());
                while(mBufferFilter->read(recvBuffer));
            }

            if(!mBufferFilter->endOfFile()){
                ret = chunkedReader(recvBuffer,tv);
            }else{
                if(mObserver != NULL){
                    mObserver->onCompleted();
                }
            }
            if(ret >= 0){
                ret = recvBuffer->size();
            }
        }else{
            if(tmpBuffer->size() > tmpBuffer->offset()){
                recvBuffer->append(tmpBuffer->dataWithOffset(),tmpBuffer->size() - tmpBuffer->offset());
            }

            if(recvBuffer->size() < mResponse->mContentLength){
                ret = identifyReader(recvBuffer,tv);
            }else{
                if(mObserver != NULL){
                    mObserver->onCompleted();
                }
            }
            if(ret >= 0){
                ret = recvBuffer->size();
            }

        }
    }else if(mIsDownload == HTTP_CHILD_DOWNLOAD){
        RawFile rawFile(mfilePath.c_str());
        int ret = rawFile.open(O_RDWR|O_CREAT);
        if(ret < 0){
            ALOGE("create file %s failed msg :%s ",mfilePath.c_str(),strerror(errno));
            return BAD_VALUE;
        }
        //get content size 
        //Content-Range :bytes 580-
        //Content-Range: 1-200/300\r\n"
        //
        std::string serverRange = mResponse->mHeader.getValues(HttpHeader::serverRangeHints);
        Range rg;
        parseServerRange(serverRange.c_str(),rg);
        int count = rg.end - rg.begin +1;
        if(tmpBuffer->size() > tmpBuffer->offset()){
            int downloadSize =  tmpBuffer->size() - tmpBuffer->offset();
            count -= downloadSize;
            rawFile.append(tmpBuffer->dataWithOffset(),tmpBuffer->size() - tmpBuffer->offset());
            if(count <= 0){
                return downloadSize;;
            }
        }
        ret = commonReader(rawFile,count,tv);
        if(ret < 0){
            ALOGE("download partial file %s failed",mfilePath.c_str());
            return UNKNOWN_ERROR;
        }
        return rg.end - rg.begin +1;
    }else {
        //create RangeDownloader to  do mutil-download
        ASSERT(mResponse->mContentLength >= 0,"mResponse->mContentLength is %ld ",mResponse->mContentLength);
        //check support range download 
        std::string rangeSupport = mResponse->mHeader.getValues(HttpHeader::acceptRangeHints);
        ALOGD("range Support is %s ",rangeSupport.c_str());
        FileUtils::makeDir(DownloaderManager::downloaderFolder);
        if(mfilePath.empty()){
            std::string fileName = getDownloadFilePath();
            mfilePath = DownloaderManager::downloaderFolder;
            mfilePath += "/";
            mfilePath += fileName;
        }
        ALOGD("download file path is %s ",mfilePath.c_str());
        if(!rangeSupport.empty() && rangeSupport == serverRangeUnits){
            //create DownloaderManager
            DownloaderManager  *dm = new DownloaderManager(mRequest,mfilePath.c_str(),mResponse->mContentLength);
            dm->start();
            dm->wait4Complete();
            delete dm;
            return OK;
        }else{
            //remove current file and create a new one;
            FileUtils::deleteFiles(mfilePath.c_str());
            ASSERT(mResponse->mContentLength >= 0,"mResponse->mContentLength is %ld ",mResponse->mContentLength);
            //download by this transfer
            RawFile rawFile(mfilePath.c_str());
            int ret = rawFile.open(O_RDWR|O_CREAT);
            if(ret < 0){
                ALOGE("create file %s failed msg :%s ",mfilePath.c_str(),strerror(errno));
                return BAD_VALUE;
            }
            int count = mResponse->mContentLength;
            if(tmpBuffer->size() > tmpBuffer->offset()){
                int downloadSize =  tmpBuffer->size() - tmpBuffer->offset();
                count -= downloadSize;
                rawFile.append(tmpBuffer->dataWithOffset(),tmpBuffer->size() - tmpBuffer->offset());
                if(count <= 0){
                    return downloadSize;;
                }
            }
            ret = commonReader(rawFile,count,tv);
            if(ret < 0){
                ALOGE("download partial file %s failed",mfilePath.c_str());
                return UNKNOWN_ERROR;
            }
            return mResponse->mContentLength;
        }
    }
    return ret;
}

/*
 *Content-Range: 1-200/300\r\n"
 */
void HttpTransfer::parseServerRange(const char *range,Range &rg){
    const char *bytes = strstr(range,serverRangeUnits);
    if(bytes == NULL){
        bytes = range;
    }else{
        bytes += strlen(serverRangeUnits);
    }
    while(*(bytes++) != ' ');
    int begin = atoi(bytes);
    int end = -1;
    const char *split = strchr(bytes,'-');
    const char *dash = strchr(range,'/');
    rg.begin  = begin;
    if(split == NULL){
        //use input end value
        rg.end = mPartialData.end;
        return ;
    }
    end = atoi(split +1);
    rg.end = end;
    int total = 0;
    if(dash != NULL){
        total = atoi(dash+1);
    }
    ALOGD("begin %d - %d  total:%d ",begin,end,total);
}


int HttpTransfer::commonReader(RawFile &wfile,int count,struct timeval &tv){
    int n = 0;
    int ret = 0;
    fd_set rdSet,wrSet;
    char tmpBuff[1024] ={0};
    int nrecved = 0;
    ALOGD("COUNT %d ",count);
    while(nrecved < count){
        FD_ZERO(&rdSet);
        FD_SET(mPipe[1],&rdSet);
        FD_SET(mFd,&rdSet);
        int maxFd = mPipe[1] > mFd ?mPipe[1]:mFd;
        maxFd ++;
        //ALOGD("http get read wait select begin tv timeout value %ld ",tv.tv_sec *1000 + tv.tv_usec/1000);
        ret = select(maxFd,&rdSet,NULL,NULL,&tv);
        //ALOGD("http get read wait select end");
        if(ret > 0){
            if(FD_ISSET(mPipe[1],&rdSet)){
                ALOGW("http get write abort by user");
                return ABORT_ERROR;
            }else if(FD_ISSET(mFd,&rdSet)){
                int rsize = count - nrecved;
                if(rsize > sizeof(tmpBuff)){
                    rsize = sizeof(tmpBuff);
                }

                if(!mIsSeucre){
                    n = read(mFd,tmpBuff,rsize);
                }else{
                    n = mHttpsSupport->read(tmpBuff,rsize);
                    if(n == HTTPS_WOULD_BLOCK){
                        continue;
                    }
                }
                if(n > 0){
                    wfile.append(tmpBuff,n);
                    nrecved += n;
                    //check header whether is complete
                }else if(n < 0){
                    mError ++;
                    return UNKNOWN_ERROR;
                }else{
                    break;
                }
            }else{
                ALOGE("http recv data fail!");
                return UNKNOWN_ERROR;
            }
        }else if(ret == 0){
            ALOGE("http recv data timeout !");
            return TIMEOUT_ERROR;
        }else{
            ALOGE("http recv data  fail! ret = %d %s ",ret,strerror(errno));
            return UNKNOWN_ERROR;
        }
    }
    return OK;
}

std::string HttpTransfer::getDownloadFilePath(){
    //Content-Disposition: attachment; filename=“filename.xls”
    std::string nameString = mResponse->mHeader.getValues(HttpHeader::dispositionHints);
    if(nameString.empty()){
        //get url path last segment as file name
        return FileUtils::extractFileName(mRequest->mUrl.mPath.c_str());
    }else{
        const char *attachment = nameString.c_str();
        const char *nbegin = strchr(attachment,'"');
        const char *nend = strrchr(attachment,'"');
        if(nend-nbegin > 0){
            return std::string(nbegin,nend -nbegin);
        }else{
            ALOGE("no file name for this download");
            return "defaultFile";
        }
    }
}


void HttpTransfer::installFilters(){
    if(mGzipFilter != NULL){
        mBufferFilter = new BufferFilter(); 
        mBufferFilter->setFilterHeader(mGzipFilter.get());
        if(mChunkFilter != NULL){
            mGzipFilter->setChild(mChunkFilter.get());
        }
        return;
    }else if(mChunkFilter != NULL){
        mBufferFilter = new BufferFilter(); 
        mBufferFilter->setFilterHeader(mChunkFilter.get());
    }
}


 /*   
	acceptRangeHeader   = "Accept-Ranges"
	contentLengthHeader = "Content-Length"

type HttpDownloader struct {
	url       string
	file      string
	par       int64
	len       int64
	ips       []string
	skipTls   bool
	parts     []Part
	resumable bool
}

func NewHttpDownloader(url string, par int, skipTls bool) *HttpDownloader {
	var resumable = true

	parsed, err := stdurl.Parse(url)
	FatalCheck(err)

	ips, err := net.LookupIP(parsed.Host)
	FatalCheck(err)

	ipstr := FilterIPV4(ips)
	Printf("Resolve ip: %s\n", strings.Join(ipstr, " | "))

	req, err := http.NewRequest("GET", url, nil)
	FatalCheck(err)

	resp, err := client.Do(req)
	FatalCheck(err)

	if resp.Header.Get(acceptRangeHeader) == "" {
		Printf("Target url is not supported range download, fallback to parallel 1\n")
		//fallback to par = 1
		par = 1
	}

	//get download range
	clen := resp.Header.Get(contentLengthHeader)
	if clen == "" {
		Printf("Target url not contain Content-Length header, fallback to parallel 1\n")
		clen = "1" //set 1 because of progress bar not accept 0 length
		par = 1
		resumable = false
	}

	Printf("Start download with %d connections \n", par)

	len, err := strconv.ParseInt(clen, 10, 64)
	FatalCheck(err)

	sizeInMb := float64(len) / (1024 * 1024)

	if clen == "1" {
		Printf("Download size: not specified\n")
	} else if sizeInMb < 1024 {
		Printf("Download target size: %.1f MB\n", sizeInMb)
	} else {
		Printf("Download target size: %.1f GB\n", sizeInMb/1024)
	}

	file := filepath.Base(url)
	ret := new(HttpDownloader)
	ret.url = url
	ret.file = file
	ret.par = int64(par)
	ret.len = len
	ret.ips = ipstr
	ret.skipTls = skipTls
	ret.parts = partCalculate(int64(par), len, url)
	ret.resumable = resumable
MultipartEntity mutiEntity = newMultipartEntity();
File file = new File("d:/photo.jpg");
mutiEntity.addPart("desc",new StringBody("美丽的西双版纳", Charset.forName("utf-8")));
mutiEntity.addPart("pic", newFileBody(file));
 
 
httpPost.setEntity(mutiEntity);
HttpResponse  httpResponse = httpClient.execute(httpPost);
HttpEntity httpEntity =  httpResponse.getEntity();
String content = EntityUtils.toString(httpEntity);
*/
