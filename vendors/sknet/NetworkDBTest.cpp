#include "NetworkDatabase.h"
#include "Log.h"
#include <netinet/in.h>
#include <arpa/inet.h>
#include "BufferUtils.h"
#include "Vector.h"
#include "SocksConnect.h"
#include "DNSCache.h"
#include "Url.h"
#include "HttpHeader.h"
#include "HttpTransfer.h"
#include "HttpsTransfer.h"

/*
GET 17/1228/16/D6OPQNV6000189FH.html HTTP/1.1
Accept-Language:zh-CN,zh;q=0.9
Host:news.163.com
User-Agent:Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/63.0.3239.84 Safari/537.36
Content-Length:0
Accept:text/html,application/xhtml+xml,application/xml;q=0.9,image/webp,image/apng,*
/* q=0.8


 

GET /17/1228/16/D6OPQNV6000189FH.html HTTP/1.1
Host: news.163.com
Connection: keep-alive
Cache-Control: max-age=0
Upgrade-Insecure-Requests: 1
User-Agent: Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/63.0.3239.84 Safari/537.36
Accept: text/html,application/xhtml+xml,application/xml;q=0.9,image/webp,image/apng,*
/*;q=0.8
Referer: http://www.163.com/
Accept-Encoding: gzip, deflate
Accept-Language: zh-CN,zh;q=0.9
Cookie: vjuids=-3dd6df54d.15e9ad44c20.0.5ce710462eea; _ntes_nnid=5fa8714d6ebdf9ab84afc38eb5322a23,1505836157990; usertrack=c+xxClnBPH6NJ66MBFFFAg==; _ntes_nuid=5fa8714d6ebdf9ab84afc38eb5322a23; UM_distinctid=15e9ad4cb11106-08a044a68e29-1c29160a-1fa400-15e9ad4cb12462; __gads=ID=b69a461dd3308690:T=1505836191:S=ALNI_MbOv77TDsxHiHEimeD1bCNsaE7O3A; mail_psc_fingerprint=d5d80616f93a7a977100394b9c21a1df; P_INFO=primefour@163.com|1511711367|0|163|00&21|shh&1511410577&163#shh&null#10#0#0|158993&0|163&mail163|primefour@163.com; CNZZDATA1271207190=485797003-1512128773-http%253A%252F%252Fnews.163.com%252F%7C1512128773; NNSSPID=0fea259fd0b646e5a83725c72757f3f9; Province=021; City=021; NTES_hp_textlink1=old; vjlast=1505836158.1514474647.11; CNZZDATA1256734798=1152423267-1511578113-http%253A%252F%252Fnews.163.com%252F%7C1514473122; CNZZDATA1256336326=1972669584-1505835976-http%253A%252F%252Fnews.163.com%252F%7C1514474288; ne_analysis_trace_id=1514474658373; s_n_f_l_n3=7d24fc61281cd9631514474658382; vinfo_n_f_l_n3=7d24fc61281cd963.1.13.1505836157998.1514117875751.1514474821367
If-Modified-Since: Thu, 28 Dec 2017 15:25:53 GMT

/*
 *localhost:10443
 */

int main(){
    ALOGD("Hello world");
    sp<NetworkDatabase>& db = NetworkDatabase::getInstance();
    /*
    SocketAddress sockAddr1("www.baidu.com","192.168.1.23");
    sockAddr1.setPort(8080);
    SocketAddress sockAddr2("192.168.1.1");
    sockAddr2.setPort(80);
    SocketAddress sockAddr3("2000:0:0:0:1:2345:6789:abcd");
    sockAddr3.setPort(23);
    db->xDnsInsert(sockAddr1);
    db->xDnsInsert(sockAddr2);
    db->xDnsInsert(sockAddr3);
    struct sockaddr * ip = (struct sockaddr *)sockAddr1.getSockAddr(); 
    struct sockaddr_in *inetIp = (struct sockaddr_in *)ip;
    char ipAddr[1024] ={0};
    inet_ntop(AF_INET,&(inetIp->sin_addr),ipAddr,sizeof(ipAddr));
    ALOGD("ip = %s ",ipAddr);
    sp<TaskInfo> task1 = new TaskInfo();
    task1->mUrl = "www.jd.com";
    task1->mTaskId = "helloworld1";
    task1->mModuleName = "network";
    const char *data="send data is OK";
    task1->mSendData->write(data,strlen(data));
    task1->mSendData->offset(0,SEEK_SET);
    int count = task1->mSendData->read(ipAddr,sizeof(ipAddr));
    ALOGD("count = %d ipAddr = %s ",count,ipAddr);
    db->xTaskInsert(task1);

    Vector<SocketAddress> helo ;
    int ss = db->getAddrByHost("www.baidu.com",helo);
    ALOGD("ss = %d size helo = %zd ",ss,helo.size());

    Vector<sp<TaskInfo> > tasks;
    int cx = db->xTaskGetTasks(tasks);
    ALOGD("cx = %d size helo = %zd ",cx,tasks.size());

    count = tasks[0]->mSendData->read(ipAddr,sizeof(ipAddr));

    ALOGD("task 0 count = %d ipAddr = %s ",count,ipAddr);
    SocketAddress sockAddrx("115.239.210.27");
    sockAddrx.setPort(80);
    SocksConnect connect(sockAddrx);
    int ret = connect.connect(5000);
    if(ret != OK){
        ALOGD("connect fail ");
    }

    sp<DnsCache> Cache = DnsCache::getInstance();
    const Vector<SocketAddress> &jdAddrs = Cache->getAddrs("www.jd.com","http");
    if(jdAddrs.size() != 0){
        SocksConnect jdConnect(jdAddrs);
        ret = jdConnect.connect(5000);
        if(ret != OK){
            ALOGD("jdconnect fail ");
        }
    }else{
        ALOGD("failed to get address");
    }

    // schema://username:password@host:port/path?key=value#fragment
    Url tmpUrl;
    Url::parseUrl("http://www.baidu.com:1010/?hello=xjk&helk=jj",&tmpUrl);
    Url::dumpUrl(&tmpUrl);

    HttpHeader header ;
    ALOGD("exit");
    header.setEntry("Hello","World");
    header.setEntry("world","this is %s","world");
    BufferUtils xTmpbuff ;
    header.toString(xTmpbuff);
    xTmpbuff.offset(0,SEEK_SET);
    ALOGD("header to string is \n%s ",xTmpbuff.data());

    //req->mHeader.setEntry("Connection","close");
    //req->mHeader.setEntry("Accept-Encoding","gzip, deflate");
    //req->mHeader.setEntry("Content-Length","%d",0);
    //req->mHeader.setEntry("Referer","http://www.163.com/");
    //req->mHeader.setEntry("Upgrade-Insecure-Requests","%d",1);
    //set host entry
    //req->mHeader.setEntry("Cookie","vjuids=-3dd6df54d.15e9ad44c20.0.5ce710462eea; _ntes_nnid=5fa8714d6ebdf9ab84afc38eb5322a23,1505836157990; usertrack=c+xxClnBPH6NJ66MBFFFAg==; _ntes_nuid=5fa8714d6ebdf9ab84afc38eb5322a23; UM_distinctid=15e9ad4cb11106-08a044a68e29-1c29160a-1fa400-15e9ad4cb12462; __gads=ID=b69a461dd3308690:T=1505836191:S=ALNI_MbOv77TDsxHiHEimeD1bCNsaE7O3A; mail_psc_fingerprint=d5d80616f93a7a977100394b9c21a1df; P_INFO=primefour@163.com|1511711367|0|163|00&21|shh&1511410577&163#shh&null#10#0#0|158993&0|163&mail163|primefour@163.com; CNZZDATA1271207190=485797003-1512128773-http%253A%252F%252Fnews.163.com%252F%7C1512128773; NNSSPID=0fea259fd0b646e5a83725c72757f3f9; Province=021; City=021; NTES_hp_textlink1=old; vjlast=1505836158.1514474647.11; CNZZDATA1256734798=1152423267-1511578113-http%253A%252F%252Fnews.163.com%252F%7C1514473122; CNZZDATA1256336326=1972669584-1505835976-http%253A%252F%252Fnews.163.com%252F%7C1514474288; ne_analysis_trace_id=1514474658373; s_n_f_l_n3=7d24fc61281cd9631514474658382; vinfo_n_f_l_n3=7d24fc61281cd963.1.13.1505836157998.1514117875751.1514474821367");

    //HttpTransfer transfer;
    //transfer.doGet("http://www.sina.com.cn");
    //transfer.doGet("http://www.163.com");
    //transfer.doDownload("http://download.skycn.com/hao123-soft-online-bcs/soft/X/2015-12-17_XMPSetup_5.1.29.4510-video.exe","./Hello.download");
    /*
    sp<HttpResponse> resp = transfer.getResponse();
    if(resp != NULL){
        ALOGD("====> %s ",(const char *)resp->mBody->data());
    }
    */
    //transfer.doGet("http://www.baidu.com");
    //transfer.doGet("http://i.weather.com.cn/i/product/pic/m/sevp_nmc_stfc_sfer_er24_achn_l88_p9_20171228130002400.jpg");
    //transfer.doGet("http://news.163.com/17/1228/16/D6OPQNV6000189FH.html");
    /*
    const char *chunkedData = "5\r\n12345\r\n 6\r\n123456\r\n0\r\n";
    sp<BufferUtils> oldBuffer = new BufferUtils();
    oldBuffer->append(chunkedData,strlen(chunkedData));
    sp<BufferUtils> recvBuffer = new BufferUtils();
    struct timeval tv;
    transfer.chunkedReader(oldBuffer,recvBuffer,tv);
    ALOGD("====> %s ",(const char *)recvBuffer->data());
    */
    HttpTransfer transfer;
    /*
    transfer.reset();
    transfer.doDownload("http://download.skycn.com/hao123-soft-online-bcs/soft/X/2015-12-17_XMPSetup_5.1.29.4510-video.exe","");
    transfer.reset();
    transfer.doGet("https://curl.haxx.se/download/curl-7.57.0.tar.gz");
    transfer.reset();
    transfer.reset();
    //transfer.doGet("https://localhost:8081/hello");
    transfer.doGet("http://www.baidu.com");
    transfer.reset();
    transfer.doGet("http://www.baidu.com");
    transfer.reset();
    transfer.doDownload("https://curl.haxx.se/download/curl-7.57.0.tar.gz","");
    transfer.reset();
    transfer.doDownload("http://download.skycn.com/hao123-soft-online-bcs/soft/X/2015-12-17_XMPSetup_5.1.29.4510-video.exe","");
    */

    //HttpTransfer transfer;
    //transfer.doGet("http://cms-bucket.nosdn.127.net/e30ef02fbf6f4791af287c00b9fa49a720180112110836.png?imageView&thumbnail=140y88&quality=85");
    /*transfer.doGet("http://www.163.com");
    sp<HttpResponse> resp = transfer.getResponse();
    if(resp != NULL && resp->mBody != NULL){
        ALOGD("====> %zd ",resp->mBody->size());
        //ALOGD("====> %s ",resp->mBody->data());
    }
    */
    return 0;
}
