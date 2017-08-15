#ifndef _SOCKSTSELECT_
#define _SOCKSTSELECT_

#include <sys/select.h>
#include <poll.h>
#include <vector>

#if __APPLE__
#import <TargetConditionals.h>
#if TARGET_OS_MAC
#include <sys/event.h>
#endif
#endif

#include "lock.h"
#include "unix_socket.h"

class SocketSelectBreaker {
    public:
        SocketSelectBreaker();
        ~SocketSelectBreaker();

        bool IsCreateSuc() const;
        bool ReCreate();
        void Close();

        bool Break();
        bool Clear();

        bool IsBreak() const;
        int BreakerFD() const;

    private:
        SocketSelectBreaker(const SocketSelectBreaker&);
        SocketSelectBreaker& operator=(const SocketSelectBreaker&);

    private:
        int pipes_[2];
        bool create_success_;
        bool broken_;
        Mutex mutex_;
};

class SocketSelect {
    public:
        SocketSelect(SocketSelectBreaker& _breaker, bool _autoclear = false);
        ~SocketSelect();

        void PreSelect();
        void Consign(SocketSelect& _consignor);
        void Read_FD_SET(int _socket);
        void Write_FD_SET(int _socket);
        void Exception_FD_SET(int _socket);

        virtual int Select();
        virtual int Select(int _msec);

        int  Errno() const;

        bool Report(SocketSelect& _consignor, int64_t _timeout);
        int  Read_FD_ISSET(int _socket) const;
        int  Write_FD_ISSET(int _socket) const;
        int  Exception_FD_ISSET(int _socket) const;

        bool IsBreak() const;
        bool IsException() const;

        SocketSelectBreaker& Breaker();

    private:
        SocketSelect(const SocketSelect&);
        SocketSelect& operator=(const SocketSelect&);

    protected:
        SocketSelectBreaker&       breaker_;
        std::vector<struct pollfd> vfds_;

        int         ret_;
        int         errno_;
        const bool  autoclear_;
};

#endif
