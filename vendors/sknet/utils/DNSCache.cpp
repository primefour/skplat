/*******************************************************************************
 **      Filename: utils/DNSCache.cpp
 **        Author: crazyhorse                  
 **   Description: ---
 **        Create: 2017-12-19 19:12:15
 ** Last Modified: 2017-12-19 19:12:15
 ******************************************************************************/

DnsCache::DnsCache():{
    mHostCache = new LruCache<std::string, SocketAddress>(10,getStringHash);
}
        Vector<SocketAddress> getHostByDB(const char *host);
        const SocketAddress& getHostByCache(const char *host);
        Vector<SocketAddress> getHostByName(const char *host);
    private:
        void addHost(const char *host);
        sp<LruCache<std::string, SocketAddress> > mHostCache; 
