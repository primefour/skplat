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

DnsCache::DnsCache(){
    mHostCache = new LruCache<std::string, HostAddress>(10,getStringHash);
}
/*
 * get host address by host name and service type
 * eg:www.baidu.com,http/8080
 */
DnsCache::getAddrInfo(const char *host,const char *service){
    HostAddress hostAddrs;
    struct addrinfo hints;
    struct addrinfo *result, *rp;
    int sfd, ret, j;
    size_t len;
    ssize_t nread;
    char buf[BUF_SIZE];

    /* Obtain address(es) matching host/port */
    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_UNSPEC;    /* Allow IPv4 or IPv6 */
    hints.ai_socktype = 0; /* Any socket connect type*/
    hints.ai_flags = 0;
    hints.ai_protocol = 0;          /* Any protocol */
    ret = getaddrinfo(host, argv[2], &hints, &result);
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
            hostAddrs.Addrs.push(sa);
            //insert to database
            db->xDnsInsert(sa);
        }else if(rp->ai_family == AF_INET6){
            struct sockaddr_in6 *xaddr6 = (struct sockaddr_in6 *) rp->ai_addr;
            inet_ntop(rp->ai_family,&(xaddr6->sin6_addr),xbuff,sizeof(xbuff));
            ALOGD("address %s  \n",xbuff);
            SocketAddress sa6(host,xbuff,xaddr6);
            //add to cache
            hostAddrs.Addrs.push(sa6);
            //insert to database
            db->xDnsInsert(sa6);
        }
    }
    /* No longer needed */
    freeaddrinfo(result); 
    hostAddrs->mHost = host;
    //update cache item
    mHostCache->add(hostAddrs->mHost,hostAddrs);
}

Vector<SocketAddress> getHostByDB(const char *host){
    sp<NetworkDatabase>& db = NetworkDatabase::getInstance();
    Vector<SocketAddress> addrs ;
    int ss = db->getAddrByHost(host,addrs);
    ALOGD("ss = %d size addrs = %zd ",ss,addrs.size());
    return addrs;
}

const Vector<SocketAddress>& getHostByCache(const char *host){
    std::string xhost = host;
    const HostAddress& tmpHost =  mHostCache->get(host);
    return tmpHost->Addrs;
}

const Vector<SocketAddress>& getAddrs(const char *host,const char *service){

}
