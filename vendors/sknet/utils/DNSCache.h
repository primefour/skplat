#ifndef __DNS_CACHE_H__
#define __DNS_CACHE_H__
//dns cache provides dns query and cache function

        //sp<LruCache<std::string, SocketAddress> tmpCache(10,getStringHash);
struct HostAddress{
    const std::string& getKey() const{
        return mHost;
    }
    std::string mHost;
    int mFailedTimes;
    Vector<SocketAddress> mAddrs;
};

class DnsCache{
    public:
        DnsCache();
        Vector<SocketAddress> getHostByName(const char *host);
        Vector<SocketAddress> getHostByName(std::string host);
        //find cache and then find db and last query dns server by network
        //if connect fail times beyond the limit,should remove cache and database record and then update host ip address
        void onConnectFailed(const std::string &host);
    private:
        void addHost(const char *host);
        sp<LruCache<std::string, HostAddress> > mHostCache; 
};
#endif//__DNS_CACHE_H__
