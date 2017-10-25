#ifndef __SQLITE_NETWORK_XDNS_H__
#define __SQLITE_NETWORK_XDNS_H__
#include<string>
#include"sqlite_wrapper.h"

#define MAX_CONN_PROFILE 0x0fffffff

enum IPTYPE{
    IPTYPE_V4,
    IPTYPE_V6, 
};

enum DNSTYPE{
    DNSTYPE_RAW,
    DNSTYPE_HTTP,
    DNSTYPE_HTTPS 
};

struct network_dns{
    std::string host;
    std::string ip;
    std::string dns_server;
    int ip_type;
    int dns_type;
    int fail_times;
    int64_t conn_profile;
    network_dns(){
    }
    network_dns(const char *xhost,
            const char *xip,
            const char *xdns_server):
        host(xhost),ip(xip),
        dns_server(xdns_server){
        ip_type = IPTYPE_V4;
        dns_type = DNSTYPE_RAW;
    }

    network_dns(const char *xhost,
            const char *xip,
            const char *xdns_server,
            int xdns_type):
        host(xhost),ip(xip),
        dns_server(xdns_server),
        dns_type(xdns_type){
        ip_type = IPTYPE_V4;
    }

    network_dns(const char *xhost,
            const char *xip,
            int xip_type,
            const char *xdns_server,
            int xdns_type):
        host(xhost),ip(xip),ip_type(xip_type),
        dns_server(xdns_server),
        dns_type(xdns_type){
    }
};
#endif
