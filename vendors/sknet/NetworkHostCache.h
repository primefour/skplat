#ifndef __NETWORK_HOST_CACHE_H__
#define __NETWORK_HOST_CACHE_H__

enum XHOST_TYPE{
    HTTP_SERVER,
    HTTP_DNS_SERVER,
    HTTPS_SERVER,
    DNS_SERVER,
    TCP_SERVER,
    TLS_SERVER,
    HOST_TYPE_MAX,
};

struct NetworkHost{
    std::string host;
    int port;
    int type;
};

int get_xhost_all_hosts(SQLite *sqlite,std::vector<NetworkXHost> &ips);
int get_xhost_by_type(SQLite *sqlite,std::vector<NetworkXHost> &ips,int type );
int insert_xhost_host(SQLite *sqlite,const char *host,int port,int type);
int delete_xhost_by_port(SQLite *sqlite,const char *host,int port);
int delete_xhost_by_host(SQLite *sqlite,const char *host);
#endif
