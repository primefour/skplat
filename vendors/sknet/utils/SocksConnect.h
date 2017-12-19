#ifndef __SOCK_CONNECT_H__
#define __SOCK_CONNECT_H__
#include"Vector.h"
#include"SocketAddress.h"
#include"Mutex.h"
#include"Timers.h"
class SocksConnect{
    enum {
        CONNECT_INIT,
        CONNECT_CONNECTING,
        CONNECT_CONNECTED,
        CONNECT_CLOSE,
        CONNECT_FAILED,
    };
    public:
        /* create mutil-address connect*/
        SocksConnect(Vector<SocketAddress>& Addrs);
        /*create one address connect*/
        SocksConnect(SocketAddress &Addrs);
        virtual ~SocksConnect();
        int interrupt();//may be block
        void initSocket();
        void dispose();
        void reset();
        /*
         * will block and wait for connect sucessfully or timeout
         */
        int connect(long timeout = -1);
        /*
         *return connect socket fd,if no connect fd,return -1
         *return fd should close by user
         */
        int getSocket();
    private:
        SocksConnect(const SocksConnect &connect);
        int *mFds;
        int *mStates;
        Vector<SocketAddress> mAddrs;
        int mPipe[2];
        int mError;
        int mConnFdIdx;
        DurationTimer mDuration;
        Mutex mMutex;
};
#endif
