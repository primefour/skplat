#include"SocketAddress.h"

SocketAddress::SocketAddress(){
    mPort = -1; 
    mType = SOCKADDR_TYPE_IV;
}

SocketAddress::SocketAddress(const char *host,const char *ip,sockaddr *addr):
    mHost(host),mCharIp(ip){
        memcpy(&mAddr.mSaAddr,addr,sizeof(sockaddr));
        mType = SOCKADDR_TYPE_V4;
}

SocketAddress::SocketAddress(const char *host,const char *ip,sockaddr_in *addr)
    mHost(host),mCharIpv4(ip){
        memcpy(&mAddr.mInAddr,addr,sizeof(sockaddr_in));
        mType = SOCKADDR_TYPE_V4;
}

SocketAddress::SocketAddress(const char *host,const char *ip,sockaddr_in6 *addr)
    mHost(host),mCharIpv6(ip){
        memcpy(&mAddr.mIn6Addr,addr,sizeof(sockaddr_in6));
        mType = SOCKADDR_TYPE_V6;
}

//get host
const std::string& SocketAddress::getHostName(){
    return mHost;
}

const std::string& SocketAddress::getIp(){
    if(mType == SOCKADDR_TYPE_V4){
        return mCharIpv4;
    }else{
        return mCharIPv6;
    }
}


