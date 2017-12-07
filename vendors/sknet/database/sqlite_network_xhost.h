#ifndef __SQLITE_NETWORK_XHOST_H__
#define __SQLITE_NETWORK_XHOST_H__
#include<stdio.h>
#include<vector>
#include<string>
#include"sqlite_wrapper.h"


struct NetworkXHost{
    std::string host;
    int port;
    int type;
};

int get_xhost_all_hosts(SQLite *sqlite,std::vector<NetworkXHost> &ips);
int get_xhost_by_type(SQLite *sqlite,std::vector<NetworkXHost> &ips,int type );
int insert_xhost_host(SQLite *sqlite,const char *host,int port,int type);
int delete_xhost_by_port(SQLite *sqlite,const char *host,int port);
int delete_xhost_by_host(SQLite *sqlite,const char *host);
#endif //__SQLITE_NETWORK_XHOST_H__
