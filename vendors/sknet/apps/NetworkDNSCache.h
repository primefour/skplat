#ifndef __NETWORK_DNS_CACHE_H__
#define __NETWORK_DNS_CACHE_H__
#include "SocketAddress.h"
#include<string>

class NetworkDnsCache{
    public:
        NetwokDnsCache(int capacity);
        int add(const SocketAddress &sa);
        int add(const SocketAddress *saPtr);
        void remove(const SocketAddress &sa);
        void remove(const SocketAddress *saPtr);
        int cacheHost(const char *host);
        SocketAddress& getAddrByName(const char *host);
        SocketAddress& getAddrByName(std::string host);
    private:
        const SocketAddress& fetchHostAddr(const char *host);
        const SocketAddress& fetchHostAddrByHttp(const char *host,const char *serverIp);
        const SocketAddress& fetchHostAddrByHttp(const char *host,const SocketAddress &sa);
        const SocketAddress& fetchHostAddrByHttps(const char *host,const char *serverIp);
        const SocketAddress& fetchHostAddrByHttps(const char *host,const SocketAddress &sa);
}

#endif //__NETWORK_DNS_CACHE_H__
