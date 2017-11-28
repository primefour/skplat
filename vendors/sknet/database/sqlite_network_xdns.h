#ifndef __SQLITE_NETWORK_XDNS_H__
#define __SQLITE_NETWORK_XDNS_H__
#include<string>
#include<vector>
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
        ip_type = IPTYPE_V4;
        dns_type = DNSTYPE_RAW;
        fail_times = 0;
        conn_profile= 0;
    }

    network_dns(const char *xhost,
            const char *xip,
            const char *xdns_server):
        host(xhost),ip(xip),
        dns_server(xdns_server){
        ip_type = IPTYPE_V4;
        dns_type = DNSTYPE_RAW;
        fail_times = 0;
        conn_profile= 0;
    }

    network_dns(const char *xhost,
            const char *xip,
            const char *xdns_server,
            int xdns_type):
        host(xhost),ip(xip),
        dns_server(xdns_server),
        dns_type(xdns_type){
        ip_type = IPTYPE_V4;
        fail_times = 0;
        conn_profile= 0;
    }

    network_dns(const char *xhost,
            const char *xip,
            int xip_type,
            const char *xdns_server,
            int xdns_type):
        host(xhost),ip(xip),ip_type(xip_type),
        dns_server(xdns_server),
        dns_type(xdns_type){
        fail_times = 0;
        conn_profile= 0;
    }
};

int insert_host_ip(SQLite *sqlite,network_dns *entry);
int dns_host_exist(SQLite *sqlite,const char *host,const char *ip);
int get_hosts_by_ip_type(SQLite *sqlite,const char *ip,std::vector<network_dns> &ips);
int get_ips_by_host(SQLite *sqlite,const char *host,std::vector<network_dns> &ips);
int get_all_hosts(SQLite *sqlite,std::vector<network_dns> &ips);
int update_dns_usetimes(SQLite *sqlite,const char *host,const char *ip);
int update_dns_connprofile(SQLite *sqlite,const char *host,const char *ip,int64_t conn_profile);
int update_dns_failtimes(SQLite *sqlite,const char *host,const char *ip,int fail_times);
int update_dns_ip(SQLite *sqlite,const char *host,const char *ip,const char *new_ip);
int delete_dns_entries(SQLite *sqlite, network_dns *entries,const char *host,const char *ip);
int delete_dns_by_success(SQLite *sqlite,float percent);
int delete_dns_by_connprofile(SQLite *sqlite, int64_t limit);


#endif
