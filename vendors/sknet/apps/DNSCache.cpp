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

template<> HostAddress LruCache<std::string, HostAddress>::mNullItem = HostAddress();
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
            ALOGD("address %s  \n",xbuff);
            SocketAddress sa(host,xbuff,xaddr);
            //add to cache
            hostAddrs.mAddrs.push(sa);
            //insert to database
            db->xDnsInsert(sa);
        }else if(rp->ai_family == AF_INET6){
            struct sockaddr_in6 *xaddr6 = (struct sockaddr_in6 *) rp->ai_addr;
            inet_ntop(rp->ai_family,&(xaddr6->sin6_addr),xbuff,sizeof(xbuff));
            ALOGD("address %s  \n",xbuff);
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
    hostAddrs.mHost = host;
    //update cache item
    mHostCache.add(hostAddrs.mHost,hostAddrs);
}

void DnsCache::getHostByDB(const char *host,Vector<SocketAddress> &addrs){
    sp<NetworkDatabase>& db = NetworkDatabase::getInstance();
    int ss = db->getAddrByHost(host,addrs);
    ALOGD("ss = %d size addrs = %zd ",ss,addrs.size());
}

void DnsCache::addToCache(const char *host,Vector<SocketAddress> &addrs){
    HostAddress hostAddrs;
    hostAddrs.mHost = host;
    hostAddrs.mAddrs = addrs;
    mHostCache.add(hostAddrs.mHost,hostAddrs);
}

const Vector<SocketAddress>& DnsCache::getHostByCache(const char *host){
    std::string xhost = host;
    const HostAddress& tmpHost = mHostCache.get(xhost);
    return tmpHost.mAddrs;
}

const Vector<SocketAddress>& DnsCache::getAddrs(const char *host,const char *service){
    const Vector<SocketAddress>& addrs = getHostByCache(host);
    if(addrs.size() != 0){
        //return cache address
        return addrs;
    }else {
        Vector<SocketAddress> dbAddrs; 
        if(dbAddrs.size() == 0){
            getAddrInfo(host,service);
        }else{
            getHostByDB(host,dbAddrs);
            addToCache(host,dbAddrs);
        }
    }
    //return cache address
    const Vector<SocketAddress>& lastaddrs =  getHostByCache(host);
    return lastaddrs;
}

void DnsCache::onConnectFailed(std::string &host){
    HostAddress& hostAddrs = mHostCache.editGet(host);
    hostAddrs.mFailedTimes ++;
}
