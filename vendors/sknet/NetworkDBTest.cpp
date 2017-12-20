#include "NetworkDatabase.h"
#include "Log.h"
#include <netinet/in.h>
#include <arpa/inet.h>
#include "BufferUtils.h"
#include "Vector.h"
#include "SocksConnect.h"
#include "DNSCache.h"
/*
 *localhost:10443
 */

int main(){
    ALOGD("Hello world");
    sp<NetworkDatabase>& db = NetworkDatabase::getInstance();
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
    TaskInfo task1;
    task1.mUrl = "www.jd.com";
    task1.mTaskId = "helloworld1";
    task1.mModuleName = "network";
    const char *data="send data is OK";
    task1.mSendData->write(data,strlen(data));
    task1.mSendData->offset(0,SEEK_SET);
    int count = task1.mSendData->read(ipAddr,sizeof(ipAddr));
    ALOGD("count = %d ipAddr = %s ",count,ipAddr);
    db->xTaskInsert(task1);

    Vector<SocketAddress> helo ;
    int ss = db->getAddrByHost("www.baidu.com",helo);
    ALOGD("ss = %d size helo = %zd ",ss,helo.size());

    Vector<TaskInfo> tasks;
    int cx = db->xTaskGetTasks(tasks);
    ALOGD("cx = %d size helo = %zd ",cx,tasks.size());

    count = tasks[0].mSendData->read(ipAddr,sizeof(ipAddr));
    ALOGD("task 0 count = %d ipAddr = %s ",count,ipAddr);
    SocketAddress sockAddrx("115.239.210.27");
    sockAddrx.setPort(80);
    SocksConnect connect(sockAddrx);
    int ret = connect.connect(5000);
    if(ret != OK){
        ALOGD("connect fail ");
    }
    DnsCache Cache;
    const Vector<SocketAddress> &jdAddrs = Cache.getAddrs("www.jd.com","http");
    if(jdAddrs.size() != 0){
        SocksConnect jdConnect(jdAddrs);
        ret = jdConnect.connect(5000);
        if(ret != OK){
            ALOGD("jdconnect fail ");
        }
    }else{
        ALOGD("failed to get address");
    }

    return 0;
}
