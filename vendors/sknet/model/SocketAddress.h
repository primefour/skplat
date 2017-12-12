#ifndef __SOCKET_ADDRESS_H__
#define __SOCKET_ADDRESS_H__
#include<string.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <sys/uio.h>
#include <netinet/tcp.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <netdb.h>
#include <net/if.h>
#include <unistd.h>
#include <string>

class SocketAddress{
  public:
    //ip type of address
    enum {
        SOCKADDR_TYPE_IV = 0,//invalidate
        SOCKADDR_TYPE_V4,//ipv4
        SOCKADDR_TYPE_V6,//ipv6
    };

    //construct function
    SocketAddress();
    SocketAddress(const char *host,const char *ip,sockaddr *addr);
    SocketAddress(const char *host,const char *ip,sockaddr_in *addr);
    SocketAddress(const char *host,const char *ip,sockaddr_in6 *addr);
    //get host
    const std::string& getHostName();
    //get Ip string
    const std::string& getIp();
    //get server host port
    inline uint16_t getPort(){ return mPort;}
    //set port
    inline void setPort(uint16_t port){ mPort = port; }
  private:
    union{
        struct sockaddr     mSaAddr;
        struct sockaddr_in  mInAddr;
        struct sockaddr_in6 mIn6Addr;
    }mAddr;
    std::string mCharIpv4;
    std::string mCharIpv6;
    std::string mHost;
    uint16_t  mPort;
    uint16_t  mType;
};
#endif //__SOCKET_ADDRESS_H__
