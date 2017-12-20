#ifndef __DNS_CACHE_H__
#define __DNS_CACHE_H__
#include<string>
#include"SocketAddress.h"
#include"Vector.h"
#include"LruCache.h"

//dns cache provides dns query and cache function
struct HostAddress{
    const std::string& getKey() const{
        return mHost;
    }
    HostAddress(){
        mFailedTimes = 0;
    }
    std::string mHost;
    Vector<SocketAddress> mAddrs;
    int mFailedTimes;
};

class DnsCache{
    public:
        DnsCache(int count=256);
        /*
         * get host address by host name and service type
         * eg:www.baidu.com,http/8080
         */
        const Vector<SocketAddress>& getAddrs(const char *host,const char *service);
        void onConnectFailed(std::string &host);
    private:
        int  getAddrInfo(const char *host,const char *service);
        void getHostByDB(const char *host,Vector<SocketAddress> &addrs);
        void addToCache(const char *host,Vector<SocketAddress> &addrs);
        const Vector<SocketAddress>& getHostByCache(const char *host);
        LruCache<std::string, HostAddress> mHostCache; 
};

#endif//__DNS_CACHE_H__
