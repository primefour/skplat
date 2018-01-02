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

int HttpTransfer::mRelocationLimited = 11;
const char *HttpTransfer::HttpGetHints = "GET";
const char *HttpTransfer::HttpPostHints = "POST";
const char *HttpTransfer::HttpChunkedEOFHints = "\r\n0\r\n\r\n";

int HttpTransfer::doGet(const char *url){
    //create a get request obj
    HttpRequest *req= new HttpRequest();
    req->mMethod = "GET";
    if(Url::parseUrl(url,&(req->mUrl)) == NULL){
        return BAD_VALUE;
    }
    req->mProto="HTTP/1.1";
    req->mProtoMajor = 1;
    req->mProtoMinor = 1;
    req->mHeader.setEntry("Accept","text/html,application/xhtml+xml,application/xml;q=0.9,image/webp,image/apng,*/* q=0.8");
    req->mHeader.setEntry("User-Agent","Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/63.0.3239.84 Safari/537.36");
    req->mHeader.setEntry("Accept-Language","zh-CN,zh;q=0.9");
    //req->mHeader.setEntry("Connection","close");
    req->mHeader.setEntry("Connection","keep-alive");
    //req->mHeader.setEntry("Accept-Encoding","gzip, deflate");
    //req->mHeader.setEntry("Content-Length","%d",0);
    req->mHeader.setEntry("Referer","http://www.163.com/");
    //set host entry
    req->mHeader.setEntry("Host",req->mUrl.mHost.c_str());
    req->mHeader.setEntry("Upgrade-Insecure-Requests","%d",1);
    //req->mHeader.setEntry("Cookie","vjuids=-3dd6df54d.15e9ad44c20.0.5ce710462eea; _ntes_nnid=5fa8714d6ebdf9ab84afc38eb5322a23,1505836157990; usertrack=c+xxClnBPH6NJ66MBFFFAg==; _ntes_nuid=5fa8714d6ebdf9ab84afc38eb5322a23; UM_distinctid=15e9ad4cb11106-08a044a68e29-1c29160a-1fa400-15e9ad4cb12462; __gads=ID=b69a461dd3308690:T=1505836191:S=ALNI_MbOv77TDsxHiHEimeD1bCNsaE7O3A; mail_psc_fingerprint=d5d80616f93a7a977100394b9c21a1df; P_INFO=primefour@163.com|1511711367|0|163|00&21|shh&1511410577&163#shh&null#10#0#0|158993&0|163&mail163|primefour@163.com; CNZZDATA1271207190=485797003-1512128773-http%253A%252F%252Fnews.163.com%252F%7C1512128773; NNSSPID=0fea259fd0b646e5a83725c72757f3f9; Province=021; City=021; NTES_hp_textlink1=old; vjlast=1505836158.1514474647.11; CNZZDATA1256734798=1152423267-1511578113-http%253A%252F%252Fnews.163.com%252F%7C1514473122; CNZZDATA1256336326=1972669584-1505835976-http%253A%252F%252Fnews.163.com%252F%7C1514474288; ne_analysis_trace_id=1514474658373; s_n_f_l_n3=7d24fc61281cd9631514474658382; vinfo_n_f_l_n3=7d24fc61281cd963.1.13.1505836157998.1514117875751.1514474821367");

    //set requst and response
    mRequest = req;
    HttpResponse *response = new HttpResponse();
    mRequest->mResp = response;
    mResponse = response;
    mResponse->mRequest = req;
    httpGet(req);
}

int HttpTransfer::doPost(const char *url,BufferUtils &buff){
    //create a post request obj
    HttpRequest *req= new HttpRequest();
    req->mMethod = "POST";
    if(Url::parseUrl(url,&(req->mUrl)) == NULL){
        return BAD_VALUE;
    }
    req->mProto="HTTP/1.1";
    req->mProtoMajor = 1;
    req->mProtoMinor = 1;
    req->mHeader.setEntry("Accept","text/html,application/xhtml+xml,application/xml;q=0.9,image/webp,image/apng,*/* q=0.8");
    req->mHeader.setEntry("User-Agent","Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/63.0.3239.84 Safari/537.36");
    req->mHeader.setEntry("Accept-Language","zh-CN,zh;q=0.9");
    //req->mHeader.setEntry("Connection","close");
    req->mHeader.setEntry("Connection","keep-alive");
    //req->mHeader.setEntry("Accept-Encoding","gzip, deflate");
    //req->mHeader.setEntry("Content-Length","%d",0);
    req->mHeader.setEntry("Referer","http://www.163.com/");
    //set host entry
    req->mHeader.setEntry("Host",req->mUrl.mHost.c_str());
    req->mHeader.setEntry("Upgrade-Insecure-Requests","%d",1);
    //req->mHeader.setEntry("Cookie","vjuids=-3dd6df54d.15e9ad44c20.0.5ce710462eea; _ntes_nnid=5fa8714d6ebdf9ab84afc38eb5322a23,1505836157990; usertrack=c+xxClnBPH6NJ66MBFFFAg==; _ntes_nuid=5fa8714d6ebdf9ab84afc38eb5322a23; UM_distinctid=15e9ad4cb11106-08a044a68e29-1c29160a-1fa400-15e9ad4cb12462; __gads=ID=b69a461dd3308690:T=1505836191:S=ALNI_MbOv77TDsxHiHEimeD1bCNsaE7O3A; mail_psc_fingerprint=d5d80616f93a7a977100394b9c21a1df; P_INFO=primefour@163.com|1511711367|0|163|00&21|shh&1511410577&163#shh&null#10#0#0|158993&0|163&mail163|primefour@163.com; CNZZDATA1271207190=485797003-1512128773-http%253A%252F%252Fnews.163.com%252F%7C1512128773; NNSSPID=0fea259fd0b646e5a83725c72757f3f9; Province=021; City=021; NTES_hp_textlink1=old; vjlast=1505836158.1514474647.11; CNZZDATA1256734798=1152423267-1511578113-http%253A%252F%252Fnews.163.com%252F%7C1514473122; CNZZDATA1256336326=1972669584-1505835976-http%253A%252F%252Fnews.163.com%252F%7C1514474288; ne_analysis_trace_id=1514474658373; s_n_f_l_n3=7d24fc61281cd9631514474658382; vinfo_n_f_l_n3=7d24fc61281cd963.1.13.1505836157998.1514117875751.1514474821367");

    //set requst and response
    mRequest = req;
    HttpResponse *response = new HttpResponse();
    mRequest->mResp = response;
    mResponse = response;
    mResponse->mRequest = req;
    httpPost(req);
}

int HttpTransfer::httpGet(HttpRequest *req){
    httpDoTransfer(req);
}


int HttpTransfer::commonReader(sp<BufferUtils> &recvBuffer,int count,struct timeval &tv){
    int n = 0;
    int ret = 0;
    fd_set rdSet,wrSet;
    char tmpBuff[10] ={0};
    int nrecved = 0;
    while(nrecved < count){
        FD_ZERO(&rdSet);
        FD_SET(mPipe[1],&rdSet);
        FD_SET(mFd,&rdSet);
        int maxFd = mPipe[1] > mFd ?mPipe[1]:mFd;
        maxFd ++;
        ALOGD("http get read wait select begin tv timeout value %ld ",tv.tv_sec *1000 + tv.tv_usec/1000);
        ret = select(maxFd,&rdSet,NULL,NULL,mTask != NULL ?&tv:NULL);
        ALOGD("http get read wait select end");
        if(ret > 0){
            if(FD_ISSET(mPipe[1],&rdSet)){
                ALOGW("http get write abort by user");
                return ABORT_ERROR;
            }else if(FD_ISSET(mFd,&rdSet)){
                int rsize = count - nrecved;
                if(rsize > sizeof(tmpBuff) -1){
                    rsize = sizeof(tmpBuff) -1;
                }
                n = read(mFd,tmpBuff,rsize);
                if(n > 0){
                    recvBuffer->append(tmpBuff,n);
                    nrecved += n;
                    //check header whether is complete
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



int HttpTransfer::chunkedEOF(void *obj,const void *txd,int size){
    const char *data = (const char *)txd;
    if(size < 7){
        return false;
    }
    //ALOGD("size is %d  ==> %s ",size,(data + size - 7));
    const char *tmpData = data + size - 7;
    if(strstr(tmpData,HttpChunkedEOFHints) != NULL){
        return true;
    }else{
        return false;
    }
}


int HttpTransfer::socketReader(sp<BufferUtils> &recvBuffer,struct timeval &tv,BreakFpn breakFpn ){
    //ALOGD("socketReader recvBuffer %s  size :%zd ",recvBuffer->data(),recvBuffer->size());
    if(breakFpn(this,recvBuffer->data(),recvBuffer->size())){
        return OK;
    }
    //need more data
    int n = 0;
    int ret = 0;
    fd_set rdSet,wrSet;
    char tmpBuff[10] ={0};
    while(1){
        FD_ZERO(&rdSet);
        FD_SET(mPipe[1],&rdSet);
        FD_SET(mFd,&rdSet);
        int maxFd = mPipe[1] > mFd ?mPipe[1]:mFd;
        maxFd ++;
        //ALOGD("http get read wait select begin tv timeout value %ld ",tv.tv_sec *1000 + tv.tv_usec/1000);
        ret = select(maxFd,&rdSet,NULL,NULL,mTask != NULL ?&tv:NULL);
        //ALOGD("http get read wait select end");
        if(ret > 0){
            if(FD_ISSET(mPipe[1],&rdSet)){
                ALOGW("http get write abort by user");
                return ABORT_ERROR;
            }else if(FD_ISSET(mFd,&rdSet)){
                n = read(mFd,tmpBuff,sizeof(tmpBuff) -1);
                if(n > 0){
                    //ALOGD("tmpBuff = %s ",tmpBuff);
                    recvBuffer->append(tmpBuff,n);
                    //check whether is complete
                    //ALOGD("n = %d ==>  %s ==> %zd ",n,recvBuffer->data(),recvBuffer->size());
                    if(breakFpn(this,recvBuffer->data(),recvBuffer->size())){
                        return OK;
                    }
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
    if(chunkedEOF(this,recvBuffer->data(),recvBuffer->size())){
        return OK;
    }
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


int HttpTransfer::identifyBreak(void *obj,const void *data,int length){
    HttpTransfer *hobj = (HttpTransfer*)obj;
    ALOGD("xxx length  = %d hobj->mResponse->mContentLength %ld ",length,hobj->mResponse->mContentLength);
    if(length >= hobj->mResponse->mContentLength){
        return true; 
    }else{
        return false;
    }

}



int HttpTransfer::identifyReader(sp<BufferUtils> &recvBuffer,struct timeval &tv){
    ASSERT(mResponse->mContentLength >= 0,"mResponse->mContentLength is %ld ",mResponse->mContentLength);
    if(recvBuffer->size() >= mResponse->mContentLength){
        return OK;
    }
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
    //delete response
    mResponse->mRequest->mResp = new HttpResponse();
    mRequest->mResp->mRequest = mRequest;
    mResponse = mRequest->mResp;
    //do get requset
    httpGet(mRequest.get());
    return OK;
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

int HttpTransfer::httpPost(HttpRequest *req){

}

int HttpTransfer::httpDoTransfer(HttpRequest *req){
    const char *host = NULL;
    const char *service = NULL;
    if(req->mUseProxy){
        host = req->mProxyUrl.mHost.c_str();
        if(req->mProxyUrl.mPort.empty()){
            service = req->mProxyUrl.mSchema.c_str();
        }else{
            service = req->mProxyUrl.mPort.c_str();
        }
    }else{
        host = req->mUrl.mHost.c_str();
        if(req->mUrl.mPort.empty()){
            service = req->mUrl.mSchema.c_str();
        }else{
            service = req->mUrl.mPort.c_str();
        }
    }

    //get address using dns cache
    sp<DnsCache>& Cache = DnsCache::getInstance() ;
    //get Address
    Vector<SocketAddress> addrs = Cache->getAddrs(host,service);

    //connect to server
    SocksConnect connect(addrs);
    int ret = 0;
    TaskInfo *task = (TaskInfo*)mTask;
    if(task != NULL){
        ret = connect.connect(task->mConnTimeout);
    }else{
        ret = connect.connect();
    }

    if(ret != OK){
        ALOGD("http get connect fail ");
        return UNKNOWN_ERROR;   
    }

    //get validate socket fd
    mFd = connect.getSocket();
    ALOGD("socket fd = %d ",mFd);

    BufferUtils sendBuffer;
    char tmpBuff[1024]={0};
    //create http header
    if(req->mUseProxy){
        snprintf(tmpBuff,sizeof(tmpBuff),"%s %s %s \r\n",req->mMethod.c_str(),
                req->mUrl.mHref.c_str(),req->mProto.c_str());
    }else{
        std::string xpath = "/";
        if(!req->mUrl.mPath.empty()){
            xpath += req->mUrl.mPath;
        }
        snprintf(tmpBuff,sizeof(tmpBuff),"%s %s %s\r\n",req->mMethod.c_str(), 
                xpath.c_str(),req->mProto.c_str());
    }
    sendBuffer.append(tmpBuff,strlen(tmpBuff));
    //add http header entry
    req->mHeader.toString(sendBuffer);
    ALOGD("%s ",sendBuffer.data());

    if(req->mMethod == HttpPostHints ){
        ASSERT(req->mBody->size() <= 0,"invalidate post request ");
        //add post data
        sendBuffer.append(req->mBody->data(),req->mBody->size());
    }

    fd_set rdSet,wrSet;
    int n = 0;
    int nsended = 0;
    struct timeval tv;

    if(task != NULL && task->mTaskTimeout != 0){
        tv.tv_sec = task->mTaskTimeout /1000;
        tv.tv_usec = task->mTaskTimeout %1000 * 1000;
    }

    while(nsended < sendBuffer.size()){
        FD_ZERO(&wrSet); 
        FD_ZERO(&rdSet);
        FD_SET(mPipe[1],&rdSet);
        FD_SET(mFd,&wrSet);
        int maxFd = mPipe[1] > mFd ?mPipe[1]:mFd;
        maxFd ++;
        ALOGD("http get write wait select begin tv timeout value %ld ",tv.tv_sec *1000 + tv.tv_usec/1000);
        ret = select(maxFd,&rdSet,&wrSet,NULL,mTask != NULL ?&tv:NULL);
        ALOGD("http get write wait select end");
        if(ret > 0){
            if(FD_ISSET(mPipe[1],&rdSet)){
                ALOGD("transfer http get is aborted by user");
                mError ++;
                return ABORT_ERROR;
            }else if (FD_ISSET(mFd,&wrSet)){
                n = write(mFd,sendBuffer.dataWithOffset(),sendBuffer.size() - sendBuffer.offset());
                if(n > 0){
                    sendBuffer.offset(n,SEEK_CUR);
                    nsended += n;
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
        ret = select(maxFd,&rdSet,NULL,NULL,mTask != NULL ?&tv:NULL);
        ALOGD("http get read wait select end");
        if(ret > 0){
            if(FD_ISSET(mPipe[1],&rdSet)){
                ALOGW("http get write abort by user");
                return ABORT_ERROR;
            }else if(FD_ISSET(mFd,&rdSet)){
                memset(tmpBuff,0,sizeof(tmpBuff));
                n = read(mFd,tmpBuff,sizeof(tmpBuff) -1);
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

    //parse http head 
    mResponse->mTransferEncoding = mResponse->mHeader.getValues(HttpHeader::transferEncodingHints); 
    //check whether is chunked
    if(mResponse->mTransferEncoding.empty()){
        mResponse->mTransferEncoding = HttpHeader::encodingIdentifyHints; 
    }

    if(mResponse->mTransferEncoding != HttpHeader::encodingChunkedHints){
        std::string len = mResponse->mHeader.getValues(HttpHeader::contentLengthHints);
        ALOGD(">>>len = %s",len.c_str());
        mResponse->mContentLength = ::atoi(len.c_str()); 
        if(mResponse->mContentLength < 0){
            ALOGE("content lenght is malform  %ld < 0",mResponse->mContentLength);
            return UNKNOWN_ERROR;
        }
    }if(mResponse->mTransferEncoding == HttpHeader::encodingIdentifyHints){
        mResponse->mUncompressed =true;
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
/*
    if(task == NULL){
        ALOGE("BAD_VALUE HTTP GET TRANSFER");
        return BAD_VALUE;
    }
    sp<BufferUtils> recvBuffer = task->mRecvData;
    */

    sp<BufferUtils> recvBuffer = new BufferUtils();
    //ALOGD("tmpBuffer->dataWithOffset() = %s tmpBuffer->size():%zd  tmpBuffer->offset() = %ld ",
    //        tmpBuffer->dataWithOffset(),tmpBuffer->size(),tmpBuffer->offset());
    if(tmpBuffer->size() > tmpBuffer->offset()){
        recvBuffer->append(tmpBuffer->dataWithOffset(),tmpBuffer->size() - tmpBuffer->offset());
    }

    ret = OK;
    if(mResponse->mTransferEncoding == HttpHeader::encodingChunkedHints){
        ret = chunkedReader(recvBuffer,tv);
    }else{
        ret = identifyReader(recvBuffer,tv);
    }
    //ALOGD("recvBuffer %s ",recvBuffer->data());
    return ret;
}


    

/*
    void
http_get(http_t *conn, char *lurl)
{
	*conn->request = 0;
	if (conn->proxy) {
		const char *proto = scheme_from_proto(conn->proto);
		http_addheader(conn, "GET %s%s%s HTTP/1.0", proto, conn->host, lurl);
	} else {
		http_addheader(conn, "GET %s HTTP/1.0", lurl);
		if ((conn->proto == PROTO_HTTP &&
		     conn->port != PROTO_HTTP_PORT) ||
		    (conn->proto == PROTO_HTTPS &&
		     conn->port != PROTO_HTTPS_PORT))
			http_addheader(conn, "Host: %s:%i", conn->host,
				       conn->port);
		else
			http_addheader(conn, "Host: %s", conn->host);
	}
	if (*conn->auth)
		http_addheader(conn, "Authorization: Basic %s", conn->auth);
	if (*conn->proxy_auth)
		http_addheader(conn, "Proxy-Authorization: Basic %s",
			       conn->proxy_auth);
	http_addheader(conn, "Accept: *//*");
	if (conn->firstbyte) {
		if (conn->lastbyte)
			http_addheader(conn, "Range: bytes=%lld-%lld",
				       conn->firstbyte, conn->lastbyte);
		else
			http_addheader(conn, "Range: bytes=%lld-",
				       conn->firstbyte);
	}
}

var (
	tr = &http.Transport{
		TLSClientConfig: &tls.Config{InsecureSkipVerify: true},
	}
	client = &http.Client{Transport: tr}
)

var (
	acceptRangeHeader   = "Accept-Ranges"
	contentLengthHeader = "Content-Length"
)

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
