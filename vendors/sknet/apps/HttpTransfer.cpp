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

int HttpTransfer::doGet(const char *url){
    //create a get request obj
    HttpRequest *req= new HttpRequest();
    req->mMethod = "GET";
    if(Url::parseUrl(url,&(req->mUrl)) == NULL){
        return BAD_VALUE;
    }
    req->mHeader.setEntry("Accept","*/*");
    req->mHeader.setEntry("User-Agent","sknet");
    req->mHeader.setEntry("Accept-Language","zh-cn,zh;q=0.5");
    req->mHeader.setEntry("Accept-Charset","GBK,utf-8;q=0.7,*;q=0.7");
    req->mHeader.setEntry("Connection","keep-alive");
    req->mHeader.setEntry("Content-Length","%d",0);
    HttpGet(req);
    delete(req);
}

int HttpTransfer::doPost(const char *url,BufferUtils &buff){
    //create a post request obj
}

int HttpTransfer::HttpGet(HttpRequest *req){
    //get Address
    sp<DnsCache>& Cache = DnsCache::getInstance() ;
    Vector<SocketAddress> addrs = Cache->getAddrs(req->mUrl.mHost.c_str(),req->mUrl.mPort.empty()?"http":req->mUrl.mPort.c_str());
    //connect to server
    SocksConnect connect(addrs);
    int ret = connect.connect(5000);
    if(ret != OK){
        ALOGD("connect fail ");
    }
    int fd = connect.getSocket();
    ALOGD("fd = %d ",fd);
    //reset for use again
    //connect.reset();

    BufferUtils sendBuffer;
    char tmpBuff[1024]={0};
    snprintf(tmpBuff,sizeof(tmpBuff),"%s %s %s \r\n",req->mMethod.c_str(),req->mUrl.mHref.c_str(),req->mProto.c_str());
    sendBuffer.append(tmpBuff,strlen(tmpBuff));

    req->mHeader.toString(sendBuffer);

    ALOGD("%s ",sendBuffer.data());

    int n = 0;
    int nsended = 0;
    while(nsended < sendBuffer.size()){
        n = write(fd,sendBuffer.data(),sendBuffer.size() - n);
        if(n > 0){
            sendBuffer.offset(n,SEEK_CUR);
            nsended += n;
        }else if(n < 0){
            ALOGD("send data fail %p size: %zd  ret = %d err :%s ",sendBuffer.data(),sendBuffer.size(),n,strerror(errno));
            mError ++;
            break;
        }
    }
    if(mError){
        return UNKNOWN_ERROR;
    }
    n = 0;
    BufferUtils recvBuffer;
    int nrecved = 0;
    while(1){
        n = read(fd,tmpBuff,sizeof(tmpBuff));
        if(n > 0){
            recvBuffer.append(tmpBuff,n);
            nrecved += n;
        }else if(n < 0){
            ALOGD("recv data fail %p size: %zd ",recvBuffer.data(),recvBuffer.size());
            mError ++;
            break;
        }else{
            ALOGD("recv data complete %p size: %zd ",recvBuffer.data(),recvBuffer.size());
            break;
        }

    }
    ALOGD("%s ",recvBuffer.data());
    close(fd);
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
