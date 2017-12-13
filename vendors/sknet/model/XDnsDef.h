#ifndef __X_DNS_DEF_H__
#define __X_DNS_DEF_H__
//ip type of address
enum ADDRESS_TYPE{
    SOCKADDR_TYPE_IV = 0,//invalidate
    SOCKADDR_TYPE_V4,//ipv4
    SOCKADDR_TYPE_V6,//ipv6
};

enum FETCH_TYPE{
    FETCH_BY_DNS,
    FETCH_BY_HTTP,
    FETCH_BY_HTTPS,
};




#endif
