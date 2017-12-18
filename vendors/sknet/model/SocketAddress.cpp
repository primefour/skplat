#include"SocketAddress.h"
#include"Log.h"
#include <string>


SocketAddress::SocketAddress(){
    mPort = -1; 
    mType = SOCKADDR_TYPE_IV;
    mFetchType = 0;
    mConnProf = 0;
    memset(&mAddr,0,sizeof(struct sockaddr_storage));
}

SocketAddress::SocketAddress(const char *host,const char *ip,sockaddr *addr):mHost(host),mCharIpv4(ip){
    memcpy(&mAddr,addr,sizeof(sockaddr));
    sockaddr_in* tmp = (sockaddr_in*)addr;
    mPort = ntohs(tmp->sin_port); 
    mType = SOCKADDR_TYPE_V4;
    mFetchType = 0;
    mConnProf = 0;
}

SocketAddress::SocketAddress(const char *host,const char *ip,sockaddr_in *addr):mHost(host),mCharIpv4(ip){
    memcpy(&mAddr,addr,sizeof(sockaddr_in));
    mPort = ntohs(addr->sin_port); 
    mType = SOCKADDR_TYPE_V4;
    mFetchType = 0;
    mConnProf = 0;
}

SocketAddress::SocketAddress(const char *host,const char *ip,sockaddr_in6 *addr):mHost(host),mCharIpv6(ip){
    memcpy(&mAddr,addr,sizeof(sockaddr_in6));
    mPort = ntohs(addr->sin6_port);
    mType = SOCKADDR_TYPE_V6;
    mFetchType = 0;
    mConnProf = 0;
}

SocketAddress::SocketAddress(const char *ip){
    ASSERT(ip != NULL,"Ip is NULL");
    //check ip type
    const char*tmp =  strstr(ip,":");
    mHost = ip;
    if(tmp != NULL){
        //ipv6
        sockaddr_in6 *ipv6 =(sockaddr_in6 *)&mAddr;
        ipv6->sin6_family = AF_INET6;
        mCharIpv6 = ip;
        mType = SOCKADDR_TYPE_V6; 
        inet_pton(AF_INET6,ip,&(ipv6->sin6_addr));
    }else{
        //ipv4
        sockaddr_in *ipv4 = (sockaddr_in *)&mAddr;
        mCharIpv4 = ip;
        mType = SOCKADDR_TYPE_V4; 
        ipv4->sin_family = AF_INET; 
        inet_pton(AF_INET,ip,&(ipv4->sin_addr));
    }
    mFetchType = 0;
    mConnProf = 0;
    mPort = -1;
}

SocketAddress::SocketAddress(const char *host,const char *ip){
    ASSERT(ip != NULL,"IP SHOULD GIVE A VALUE BEFORE CREATE SOCKETADDRESS");
    //check ip type
    const char*tmp =  strstr(ip,":");
    if(host == NULL){
        host = ip ;
    }
    mHost = host;
    if(tmp != NULL){
        //ipv6
        sockaddr_in6 *ipv6 =(sockaddr_in6 *)&mAddr;
        ipv6->sin6_family = AF_INET6;
        mCharIpv6 = ip;
        mType = SOCKADDR_TYPE_V6; 
        inet_pton(AF_INET6,ip,&(ipv6->sin6_addr));
    }else{
        //ipv4
        sockaddr_in *ipv4 = (sockaddr_in *)&mAddr;
        mCharIpv4 = ip;
        mType = SOCKADDR_TYPE_V4; 
        ipv4->sin_family = AF_INET; 
        inet_pton(AF_INET,ip,&(ipv4->sin_addr));
    }
    mFetchType = 0;
    mConnProf = 0;
    mPort = -1;
}

SocketAddress::SocketAddress(const SocketAddress &sa){
    memcpy(&mAddr,&(sa.mAddr),sizeof(struct sockaddr_storage));
    mCharIpv4 = sa.mCharIpv4;
    mCharIpv6 = sa.mCharIpv6 ;
    mHost = sa.mHost ;
    mPort = sa.mPort;
    mType = sa.mType;
    mFetchType = sa.mFetchType;
    mConnProf = sa.mConnProf;
}

const char* SocketAddress::getHostString(){
    return mHost.c_str();
}

const char* SocketAddress::getIpString(){
    if(mType == SOCKADDR_TYPE_V4){
        return mCharIpv4.c_str();
    }else{
        return mCharIpv6.c_str();
    }
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
    ASSERT(mType == SOCKADDR_TYPE_V4||mType == SOCKADDR_TYPE_V6,"please set ip address first");
    mPort = port; 
    if(mType == SOCKADDR_TYPE_V6){
        sockaddr_in6 *ipv6 =(sockaddr_in6 *)&mAddr;
        ipv6->sin6_port = htons(port);
    }else{
        sockaddr_in *ipv4 = (sockaddr_in *)&mAddr;
        ipv4->sin_port = htons(port); 
    }
}

void* SocketAddress::getSockAddr(){
    //check ip type
    if(mPort == -1){
        ALOGW("port is invalidate please set it first");
        return NULL;
    }
    return (void *)&mAddr;
}


