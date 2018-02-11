#include <sys/time.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <err.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>
#include"Vector.h"
#include"SocketAddress.h"
#include"Mutex.h"
#include"Timers.h"
#include"Log.h"
#include"SocksConnect.h"

/* create mutil-address connect*/
SocksConnect::SocksConnect(const Vector<SocketAddress>& Addrs){
    mAddrs = Addrs;
    //init sockets
    initSocket();
}
/*create one address connect*/
SocksConnect::SocksConnect(SocketAddress&Addrs){
    mAddrs.add(Addrs); 
    //init sockets
    initSocket();
}

SocksConnect::~SocksConnect(){
    ALOGD("%s %d ",__func__,__LINE__);
    dispose();
}

//interrupt select wait
int SocksConnect::interrupt(){
    if(mMutex.tryLock() != 0){
        ALOGD(" wait for read socks connect");
        return write(mPipe[1],"a",1);
    }else{
        mMutex.unlock();
    }
    return 0;
}

void SocksConnect::initSocket(){
    pipe2(mPipe,0);
    //create socket for each address
    int size = mAddrs.size();
    mFds = new int[size *2];
    int i = 0;
    ALOGD("init socket size = %d ",size);
    for (i = 0;i< size;i++){
        if(mAddrs[i].getType() == SocketAddress::SOCKADDR_TYPE_V4){
            mFds[i] = socket(AF_INET,SOCK_STREAM, 0);
        }else if(mAddrs[i].getType() == SocketAddress::SOCKADDR_TYPE_V6){
            mFds[i] = socket(AF_INET6,SOCK_STREAM,0);
        }
        ASSERT(mFds[i] >= 0,"create fd fail ");
        //set flags as non-block
        int flags = fcntl(mFds[i],F_GETFL,0);
        int ret = fcntl(mFds[i],F_SETFL, flags|O_NONBLOCK); 
        ASSERT(ret == 0,"set non-block flags fail %d fd = %d ",i,mFds[i]);
    }
    //init state as connect init
    mStates = mFds + size;
    memset(mStates,0,sizeof(int)*size);
    mError = 0;
    mConnFdIdx = -1;
}

//dispose 
void SocksConnect::dispose(){
    //wait for connect function getting out
    mMutex.lock();
    if(mPipe[0] > 0){
        close(mPipe[0]);
        close(mPipe[1]);
    }
    int size = mAddrs.size();
    int i = 0;
    for (i = 0 ;i < size;i++){
        close(mFds[i]);
    }
    delete[] mFds;
    mFds = NULL;
}

//reset
void SocksConnect::reset(){
    //release all resource
    dispose();
    //initailize again
    initSocket();
}

/*
 * will block and wait for connect sucessfully or timeout
 */
int SocksConnect::connect(long timeout){ //millisecond
    AutoMutex _l(mMutex);
    int ret = -1;
    int errCount = 0;
    int maxFd = -1;
    mDuration.start();
    fd_set wrSet;
    FD_ZERO(&wrSet); 
    struct timeval tv;
    if(timeout != -1){
        tv.tv_sec = timeout/1000;
        tv.tv_usec = timeout %1000 * 1000;  //microsecond
    }
    int size = mAddrs.size();
    int i = 0;
    for(i = 0 ;i < size ; i ++){
        mStates[i] = SOCK_CONNECT_CONNECTING;
        ALOGD("fd %d call connect ",mFds[i]);
        char xtmpAddr[256]={0};
        struct sockaddr_in *tmp = (struct sockaddr_in *)(mAddrs[i].getSockAddr());
        inet_ntop(AF_INET,&(tmp->sin_addr),xtmpAddr,sizeof(xtmpAddr));
        ALOGD("family %d sock address port %d sock address :%s",tmp->sin_family,ntohs(tmp->sin_port),xtmpAddr);
        ret = ::connect(mFds[i],(struct sockaddr*)mAddrs[i].getSockAddr(),mAddrs[i].getAddrLen());
        ALOGD("fd %d call connect end ret = %d ",mFds[i],ret);
        if(ret == 0){
            //connect successful
            mStates[i] = SOCK_CONNECT_CONNECTED;
            mConnFdIdx = i;
            mError = 0;
            return OK;
        }else{
            if(errno == EINPROGRESS){
                ALOGD("fd %d call connect end ret = %d EINPROGRESS",mFds[i],ret);
                //non-block and add to select fdset
                FD_SET(mFds[i],&wrSet);
                if(mFds[i] > maxFd){
                    maxFd = mFds[i];
                }
            }else if(EINTR == errno){
                ALOGD("fd %d call connect end ret = %d EINTR ",mFds[i],ret);
                //reconnect it again
                ALOGD("connect call return EINTR and redo it");
                i--;
                continue;
            }else{
                //other error ignore
                errCount ++;
                ALOGD("fd %d call connect end ret = %d ERROR",mFds[i],ret);
                mStates[i] = SOCK_CONNECT_FAILED;
                continue ;
            }
        }
    }

    if(errCount == mAddrs.size()){
        //all connect fail; 
        ALOGD("connect return fail error string %s",strerror(errno));
        mError = 1;
        mDuration.stop();
        return UNKNOWN_ERROR;
    }

    fd_set rdSet;
    FD_ZERO(&rdSet); 
    FD_SET(mPipe[0],&rdSet);
    if(maxFd < mPipe[0]) {
        maxFd = mPipe[0];
    }
    //select wait for connect
    maxFd += 1;
    ALOGD("call select start maxFd =%d ",maxFd);
    ret = select(maxFd,&rdSet,&wrSet,NULL,timeout == -1 ?NULL:&tv); 
    ALOGD("call select end ");

    if (ret < 0){//select exception
        ALOGD("ret is %d error string %s ",ret,strerror(errno));
        mError = 1;
        mDuration.stop();
        return UNKNOWN_ERROR;
    } else if (0 == ret) {//timeout
        ALOGW("socket connect timeout %s ",strerror(errno));
        mDuration.stop();
        mError = 1;
        return TIMEOUT_ERROR;
    } else { //connect
        //check pipe 
        if(FD_ISSET(mPipe[0],&rdSet)){
            ALOGD("socket connect abort by user");
            return ABORT_ERROR;
        }

        size = mAddrs.size();
        i = 0 ;
        int optValue  =-1;
        socklen_t optSize = sizeof(optValue);
        ALOGD("socket connect successfuly and then filter fd");
        for (i = 0 ;i < size;i ++){
            ALOGD("socket connect successfuly and check fd %d ",mFds[i]); 
            if(FD_ISSET(mFds[i],&wrSet)) {//connect
                if (getsockopt(mFds[i],SOL_SOCKET,SO_ERROR,&optValue,&optSize) < 0) { 
                    ALOGW("getsockopt fail error string %s fd %d",strerror(errno),mFds[i]);
                    mStates[i] = SOCK_CONNECT_FAILED;
                    continue;
                } 
                if (optValue != 0) { 
                    ALOGW("connect fail error code %d fd %d",optValue,mFds[i]);
                    mStates[i] = SOCK_CONNECT_FAILED;
                    continue;
                } 
                ALOGD("connect successfully fd %d",mFds[i]);
                mError = 0;
                mStates[i] = SOCK_CONNECT_CONNECTED;
                mConnFdIdx = i;
                mDuration.stop();
                return OK;
            }
        }
    }
    ALOGD("connect exit");
}

/*
 *return connect socket fd,if no connect fd,return -1
 *return fd should close by user
 */
int SocksConnect::getSocket(){
    ALOGD("mError %d  mConnFdIdx  = %d ",mError,mConnFdIdx);
    if(!mError && mConnFdIdx != -1){
        return dup(mFds[mConnFdIdx]);
    }else{
        return -1;
    }
}

std::string SocksConnect::getHost(){
    if(!mError && mConnFdIdx != -1){
        return mAddrs[mConnFdIdx].getHostName();
    }else{
        return "";
    }
}

