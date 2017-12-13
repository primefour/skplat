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

/*
 *In memory, the struct sockaddr_in and struct sockaddr_in6 share the same beginning structure as struct sockaddr, 
 *and you can freely cast the pointer of one type to the other without any harm, except the possible end of the universe.
 * Just kidding on that end-of-the-universe thing...if the universe does end when you cast a struct sockaddr_in* to a struct sockaddr*, 
 * I promise you it's pure coincidence and you shouldn't even worry about it.
 * So, with that in mind, remember that whenever a function says it takes a struct sockaddr* you can cast your struct sockaddr_in*, struct sockaddr_in6*, 
 * or struct sockadd_storage* to that type with ease and safety.
 *struct sockaddr_storage is a struct you can pass to accept() or recvfrom() when you're trying to write IP version-agnostic code and 
 *you don't know if the new address is going to be IPv4 or IPv6. 
 *The struct sockaddr_storage structure is large enough to hold both types, unlike the original small struct sockaddr.
 *
 *struct sockaddr {
 *    uint8_t sa_family;        //address family, AF_xxx
 *    char    sa_data[14];      //14 bytes of protocol address
 *};
 *struct sockaddr_in {
 *  sa_family_t               sin_family; //Address family 
 *  unsigned short int        sin_port;    //Port number 
 *  struct in_addr        sin_addr;        //Internet address
 *  char      __ss_pad1[_SS_PAD1SIZE];
 *};
 * 
 *struct in6_addr {
 *    union {
 *        char   __u6_addr8[16];
 *    } __u6_addr;           //128-bit IP6 address 
 *};
 * 
 *struct sockaddr_in6 {
 *    unsigned short int  sin6_family;    //AF_INET6 
 *    uint16_t    sin6_port;      //Transport layer port #
 *    uint32_t    sin6_flowinfo;  //IPv6 flow information 
 *    struct in6_addr sin6_addr;  //IPv6 address 
 *    uint32_t    sin6_scope_id;  //scope id (new in RFC2553) 
 *};
 *General socket address holding structure, big enough to hold either struct sockaddr_in or struct sockaddr_in6 data:
 *struct sockaddr_storage {
 *    sa_family_t  ss_family;     // address family
 *    // all this is padding, implementation specific, ignore it:
 *    char      __ss_pad1[_SS_PAD1SIZE];
 *    int64_t   __ss_align;
 *    char      __ss_pad2[_SS_PAD2SIZE];
 *};
 *
*/

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
    struct sockaddr_storage mAddr;
    std::string mCharIpv4;
    std::string mCharIpv6;
    std::string mHost;
    uint16_t  mPort;
    uint16_t  mType;
};
#endif //__SOCKET_ADDRESS_H__
