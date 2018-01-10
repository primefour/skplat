#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <string>
#include "DNSCache.h"
#include "NetworkDatabase.h"
#include "Log.h"
#include <netinet/in.h>
#include <arpa/inet.h>
#include "BufferUtils.h"
#include "Vector.h"
#include "SocksConnect.h"
#include "RefBase.h"
#include<strings.h>

static Mutex gDnsCacheMutex;

template<> HostAddress LruCache<std::string, HostAddress>::mNullItem = HostAddress();
sp<DnsCache> DnsCache::mDnsCacheInst = NULL;
//mutex for single instance

sp<DnsCache>& DnsCache::getInstance(){
    AutoMutex _l(gDnsCacheMutex);
    if(mDnsCacheInst == NULL){
        mDnsCacheInst = new DnsCache(20);
    }
    return mDnsCacheInst;
}

DnsCache::DnsCache(int count):mHostCache(count,getStringHash){

}

/*
 * get host address by host name and service type
 * eg:www.baidu.com,http/8080
 */
int DnsCache::getAddrInfo(const char *host,const char *service){
    HostAddress hostAddrs;
    struct addrinfo hints;
    struct addrinfo *result, *rp;
    int ret;

    /* Obtain address(es) matching host/port */
    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_UNSPEC;    /* Allow IPv4 or IPv6 */
    hints.ai_socktype = 0; /* Any socket connect type*/
    hints.ai_flags = 0;
    hints.ai_protocol = 0;          /* Any protocol */
    ret = getaddrinfo(host,service, &hints, &result);
    if (ret != 0) {
        ALOGE("getaddrinfo: %s\n", gai_strerror(ret));
        return UNKNOWN_ERROR;
    }
    sp<NetworkDatabase>& db = NetworkDatabase::getInstance();
    /* 
     * getaddrinfo() returns a list of address structures.
     */
    for (rp = result; rp != NULL; rp = rp->ai_next) {
        char xbuff[256]={0};
        ALOGD("family %d socket type %d \n",rp->ai_family,rp->ai_socktype);
        if(rp->ai_family ==AF_INET){
            struct sockaddr_in *xaddr = (struct sockaddr_in *) rp->ai_addr;
            inet_ntop(rp->ai_family,&(xaddr->sin_addr),xbuff,sizeof(xbuff));
            ALOGD("service = %s address %s  port %d \n",service,xbuff,ntohs(xaddr->sin_port));
            SocketAddress sa(host,xbuff,xaddr);
            //add to cache
            hostAddrs.mAddrs.push(sa);
            //insert to database
            db->xDnsInsert(sa);
        }else if(rp->ai_family == AF_INET6){
            struct sockaddr_in6 *xaddr6 = (struct sockaddr_in6 *) rp->ai_addr;
            inet_ntop(rp->ai_family,&(xaddr6->sin6_addr),xbuff,sizeof(xbuff));
            ALOGD("service = %s address %s  port %d \n",service,xbuff,ntohs(xaddr6->sin6_port));
            SocketAddress sa6(host,xbuff,xaddr6);
            //add to cache
            hostAddrs.mAddrs.push(sa6);
            //insert to database
            db->xDnsInsert(sa6);
        }
    }
    /* No longer needed */
    freeaddrinfo(result); 

    //add key for cache hash
    hostAddrs.mKey= host;

    if(service != NULL){
        hostAddrs.mKey += service;
    }
    //update cache item
    mHostCache.add(hostAddrs.mKey,hostAddrs);
}

void DnsCache::getHostByDB(const char *host,Vector<SocketAddress> &addrs){
    sp<NetworkDatabase>& db = NetworkDatabase::getInstance();
    int ss = db->getAddrByHost(host,addrs);
    ALOGD("ss = %d size addrs = %zd ",ss,addrs.size());
}

void DnsCache::addToCache(const char *key,Vector<SocketAddress> &addrs){
    HostAddress hostAddrs;
    hostAddrs.mKey = key;
    hostAddrs.mAddrs = addrs;
    mHostCache.add(hostAddrs.mKey,hostAddrs);
}

const Vector<SocketAddress>& DnsCache::getHostByCache(const char *key){
    std::string xkey = key;
    const HostAddress& tmpHost = mHostCache.get(xkey);
    return tmpHost.mAddrs;
}

const Vector<SocketAddress>& DnsCache::getAddrs(const char *host,const char *service){
    std::string tmpKey = host ;
    if(service != NULL){
        tmpKey += service;
    }

    const Vector<SocketAddress>& addrs = getHostByCache(tmpKey.c_str());
    ALOGD("get address info from cache items %zd ",addrs.size());
    if(addrs.size() != 0){
        //return cache address
        return addrs;
    }else {
        Vector<SocketAddress> dbAddrs; 
        getHostByDB(host,dbAddrs);
        ALOGD("get address info from dns database items %zd service:%s",dbAddrs.size(),service);
        int size = dbAddrs.size();
        if(size == 0){
            getAddrInfo(host,service);
        }else{
            //set port
            int port = 80;
            if(service){
                 port = atoi(service);
                 ALOGD("%d update port as %d ",__LINE__,port);
                 if(port == 0){

                 ALOGD("%d update port as %d ",__LINE__,port);
                     if(strncasecmp(service,"https",5) == 0){
                 ALOGD("%d update port as %d ",__LINE__,port);
                         port = 443;
                     }else if(strncasecmp(service,"http",4) == 0){

                 ALOGD("%d update port as %d ",__LINE__,port);
                         port = 80;
                     }else{

                 ALOGD("%d update port as %d ",__LINE__,port);
                         port = 80;
                     }
                 }
            }
            ALOGD("update port as %d ",port);
            int i = 0;
            for(i = 0;i < size ;i++){
                dbAddrs.editItemAt(i).setPort(port);
            }
        }
        addToCache(tmpKey.c_str(),dbAddrs);
    }

    //return cache address
    const Vector<SocketAddress>& lastaddrs =  getHostByCache(tmpKey.c_str());
    return lastaddrs;
}

void DnsCache::onConnectFailed(std::string &host){
    HostAddress& hostAddrs = mHostCache.editGet(host);
    hostAddrs.mFailedTimes ++;
}
