#include"SocketAddress.h"
#include"XDnsDef.h"
#include <string>


SocketAddress::SocketAddress(){
    mPort = -1; 
    mType = SOCKADDR_TYPE_IV;
}

SocketAddress::SocketAddress(const char *host,const char *ip,sockaddr *addr):mHost(host),mCharIpv4(ip){
    memcpy(&mAddr,addr,sizeof(sockaddr));
    mType = SOCKADDR_TYPE_V4;
}

SocketAddress::SocketAddress(const char *host,const char *ip,sockaddr_in *addr):mHost(host),mCharIpv4(ip){
    memcpy(&mAddr,addr,sizeof(sockaddr_in));
    mType = SOCKADDR_TYPE_V4;
}

SocketAddress::SocketAddress(const char *host,const char *ip,sockaddr_in6 *addr):mHost(host),mCharIpv6(ip){
    memcpy(&mAddr,addr,sizeof(sockaddr_in6));
    mType = SOCKADDR_TYPE_V6;
}

SocketAddress::SocketAddress(const char *host,const char *ip):mHost(host){
    //check ip type
    char*tmp =  strstr(ip,":");
    if(tmp != NULL){
        //ipv6
        sockaddr_in6 *ipv6 =(sockaddr_in6 *)&mAddr;
        ipv6->sin6_family = AF_INET6;
        mCharIpv6 = ip;
        mType = SOCKADDR_TYPE_V4; 
        inet_pton(AF_INET6,&(ipv6->sin6_addr),sizeof(struct in6_addr));
    }else{
        //ipv4
        sockaddr_in *ipv4 = (sockaddr_in *)&mAddr;
        mCharIpv4 = ip;
        mType = SOCKADDR_TYPE_V6; 
        ipv4->sin_family = AF_INET; 
        inet_pton(AF_INET,ip,&(ipv4->sin_addr),sizeof(struct in_addr));
    }
}

SocketAddrees::SocketAddrees(const SocketAddress &sa){
    memcpy(&mAddr,&(sa.mAddr),sizeof(struct sockaddr_storage));
    mCharIpv4 = sa.mCharIpv4;
    mCharIpv6 = sa.mCharIpv6 ;
    mHost = sa.mHost ;
    mPort = sa.mPort;
    mType = sa.mType;
    mFetchType = sa.mFetchType;
    mConnProf = sa.mConnProf;
}

//get host
const std::string& SocketAddress::getHostName(){
    return mHost;
}

const std::string& SocketAddress::getIp(){
    if(mType == SOCKADDR_TYPE_V4){
        return mCharIpv4;
    }else{
        return mCharIpv6;
    }
}

//set port
void SocketAddress::setPort(uint16_t port){ 
    ASSERT(mType == SOCKADDR_TYPE_V4||mType == SOCKADDR_TYPE_V6);
    mPort = port; 
    if(mType = SOCKADDR_TYPE_V6){
        sockaddr_in6 *ipv6 =(sockaddr_in6 *)&mAddr;
        ipv6->sin6_port = port;
    }else{
        sockaddr_in *ipv4 = (sockaddr_in *)&mAddr;
        ipv4->sin_port = port; 
    }
}


