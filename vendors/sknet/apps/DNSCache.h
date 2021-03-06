#ifndef __DNS_CACHE_H__
#define __DNS_CACHE_H__
#include<string>
#include"SocketAddress.h"
#include"Vector.h"
#include"LruCache.h"
#include"RefBase.h"

//dns cache provides dns query and cache function
struct HostAddress{
    const std::string& getKey() const{
        return mKey;
    }
    HostAddress(){
        mFailedTimes = 0;
    }
    std::string mKey;
    Vector<SocketAddress> mAddrs;
    int mFailedTimes;
};

class DnsCache:public RefBase{
    public:
        static sp<DnsCache>& getInstance();
        /*
         * get host address by host name and service type
         * eg:www.baidu.com,http/8080
         */
        const Vector<SocketAddress>& getAddrs(const char *host,const char *service);
        void onConnectFailed(std::string &host);
    private:
        DnsCache(int count=256);
        int  getAddrInfo(const char *host,const char *service);
        void getHostByDB(const char *host,Vector<SocketAddress> &addrs);
        void addToCache(const char *host,Vector<SocketAddress> &addrs);
        const Vector<SocketAddress>& getHostByCache(const char *host);
        LruCache<std::string, HostAddress> mHostCache; 
        static sp<DnsCache> mDnsCacheInst;
};

#endif//__DNS_CACHE_H__
