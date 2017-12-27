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
/*
GET / HTTP/1.1
Host: www.163.com
Connection: keep-alive
Cache-Control: max-age=0
Upgrade-Insecure-Requests: 1
User-Agent: Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/63.0.3239.84 Safari/537.36
Accept: text/html,application/xhtml+xml,application/xml;q=0.9,image/webp,image/apng,*
/*;q=0.8
Accept-Encoding: gzip, deflate
Accept-Language: zh-CN,zh;q=0.9
Cookie: vjuids=-3dd6df54d.15e9ad44c20.0.5ce710462eea; _ntes_nnid=5fa8714d6ebdf9ab84afc38eb5322a23,1505836157990; usertrack=c+xxClnBPH6NJ66MBFFFAg==; _ntes_nuid=5fa8714d6ebdf9ab84afc38eb5322a23; UM_distinctid=15e9ad4cb11106-08a044a68e29-1c29160a-1fa400-15e9ad4cb12462; __gads=ID=b69a461dd3308690:T=1505836191:S=ALNI_MbOv77TDsxHiHEimeD1bCNsaE7O3A; mail_psc_fingerprint=d5d80616f93a7a977100394b9c21a1df; P_INFO=primefour@163.com|1511711367|0|163|00&21|shh&1511410577&163#shh&null#10#0#0|158993&0|163&mail163|primefour@163.com; Province=021; City=021; vjlast=1505836158.1514081471.11; NNSSPID=a4ba10487e644f73a8a90f05932ad86c; vinfo_n_f_l_n3=7d24fc61281cd963.1.10.1505836157998.1514081879084.1514083480106; NTES_hp_textlink1=old


GET / HTTP/1.1
Host:www.163.com
User-Agent:Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/63.0.3239.84 Safari/537.36
Accept:text/html,application/xhtml+xml,application/xml;q=0.9,image/webp,image/apng,*
Accept-Language:zh-CN,zh;q=0.9
Connection:keep-alive
Content-Length:0
/* q=0.8
*/

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
    //req->mHeader.setEntry("Connection","keep-alive");
    //req->mHeader.setEntry("Accept-Encoding","gzip, deflate");
    req->mHeader.setEntry("Content-Length","%d",0);
    //set host entry
    req->mHeader.setEntry("Host",req->mUrl.mHost.c_str());

    //set requst and response
    mRequest = req;
    HttpResponse *response = new HttpResponse();
    mRequest->mResp = response;
    mResponse = response;
    mResponse->mRequest = req;
    HttpGet(req);
}

int HttpTransfer::doPost(const char *url,BufferUtils &buff){
    //create a post request obj
}

int HttpTransfer::HttpGet(HttpRequest *req){
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
        snprintf(tmpBuff,sizeof(tmpBuff),"%s %s %s\r\n",req->mMethod.c_str(),
                req->mUrl.mPath.empty()?"/":req->mUrl.mPath.c_str(),req->mProto.c_str());
    }
    sendBuffer.append(tmpBuff,strlen(tmpBuff));
    //add http header entry
    req->mHeader.toString(sendBuffer);
    ALOGD("%s ",sendBuffer.data());

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
                n = write(mFd,sendBuffer.data(),sendBuffer.size() - n);
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
        ALOGE("UNKNOWN_ERROR HTTP GET TRANSFER");
        return UNKNOWN_ERROR;
    }

    //for test 
#if 1
    sp<BufferUtils> recvBuffer = new BufferUtils();
#else
    if(task == NULL){
        ALOGE("BAD_VALUE HTTP GET TRANSFER");
        return BAD_VALUE;
    }
    sp<BufferUtils> recvBuffer = task->mRecvData;
#endif

    n = 0;
    int nrecved = 0;
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
                    recvBuffer->append(tmpBuff,n);
                    nrecved += n;
                    //check header whether is complete
                    if(HttpHeader::checkHeader(recvBuffer)){
                        //parse header
                        if(mRequest->mHeader.parser(recvBuffer,&mRequest->mHeader) == NULL){
                            ALOGE("recv data %s parse http header fail ",(const char *)recvBuffer->data());
                            return UNKNOWN_ERROR;
                        }else{
                            sp<BufferUtils> debugBuffer = new BufferUtils();
                            mRequest->mHeader.toString(*debugBuffer);
                            ALOGD("recv data parse http header %s ",(const char *)debugBuffer->data());
                        }
                    }
                    //ALOGD("recv data %p size: %zd n :%d ", recvBuffer->data(),recvBuffer->size(),n);
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
