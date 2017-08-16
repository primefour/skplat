#include "udpserver.h"
#include "sklog.h"
#include "socket_address.h"
#include "udpclient.h"

#define DELETE_AND_NULL(a) {if (a) delete a; a = NULL;}
#define MAX_DATAGRAM 65536

struct UdpServerSendData {
    explicit UdpServerSendData(struct sockaddr_in* _addr) {
        memcpy(&addr, _addr, sizeof(sockaddr_in));
    }
    UdpServerSendData(const UdpServerSendData& rhs): addr(rhs.addr) {
    }
    AutoBuffer data;
    struct sockaddr_in addr;
};

UdpServer::UdpServer(int _port, IAsyncUdpServerEvent* _event)
    : fd_socket_(INVALID_SOCKET)
    , event_(_event)
    , selector_(breaker_, true) {
    thread_ = new Thread(this);

    __InitSocket(_port);
    thread_->start();
}

void UdpServer::operator()(){
    __RunLoop();
}

UdpServer::~UdpServer() {
    if (thread_->isruning()) {
        event_ = NULL;
        breaker_.Break();
        thread_->join();
    }

    breaker_.Break();
    DELETE_AND_NULL(thread_);

    list_buffer_.clear();

    if (fd_socket_ != INVALID_SOCKET)
        socket_close(fd_socket_);
}

void UdpServer::SendBroadcast(int _port, void* _buf, size_t _len) {
    sockaddr_in dstAddr;
    bzero(&dstAddr, sizeof(dstAddr));
    dstAddr = *(struct sockaddr_in*)(&socket_address(IPV4_BROADCAST_IP, _port).address());

    if (!__SetBroadcastOpt())
        return;

    SendAsync(&dstAddr, _buf, _len);
}

void UdpServer::SendAsync(const std::string& _ip, int _port, void* _buf, size_t _len) {
    sockaddr_in dstAddr;
    bzero(&dstAddr, sizeof(dstAddr));
    dstAddr = *(struct sockaddr_in*)(&socket_address(_ip.c_str(), _port).address());

    if (IPV4_BROADCAST_IP == _ip && !__SetBroadcastOpt())
        return;

    SendAsync(&dstAddr, _buf, _len);
}

void UdpServer::SendAsync(struct sockaddr_in* _addr, void* _buf, size_t _len) {
    skassert2((fd_socket_ != INVALID_SOCKET && event_ != NULL), "socket invalid");

    if (fd_socket_ == INVALID_SOCKET || event_ == NULL)
        return;

    ScopedLock lock(mutex_);
    list_buffer_.push_back(UdpServerSendData(_addr));
    list_buffer_.back().data.Write(_buf, _len);

    if (!thread_->isruning())
        thread_->start();

    breaker_.Break();
}

void UdpServer::__InitSocket(int _port) {
    int errCode = 0;
    struct sockaddr_in servAddr;

    bzero(&servAddr, sizeof(servAddr));
    servAddr.sin_family = AF_INET;
    servAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servAddr.sin_port = (uint16_t)htons(_port);

    fd_socket_ = socket(AF_INET, SOCK_DGRAM, 0);

    if (fd_socket_ == INVALID_SOCKET) {
        errCode = socket_errno;
        skerror("udp socket create error, error: %s", socket_strerror(errCode));
        return;
    }

    if (bind(fd_socket_, (struct sockaddr*)&servAddr, sizeof(servAddr)) != 0) {
        errCode = socket_errno;
        skerror("udp bind error, error: %s", socket_strerror(errCode));
    }
}

void UdpServer::__RunLoop() {
    skassert2(fd_socket_ != INVALID_SOCKET, "socket invalid");

    if (fd_socket_ == INVALID_SOCKET)
        return;

    char* readBuffer = new char[MAX_DATAGRAM];
    void* buf = NULL;
    size_t len = 0;
    int ret = 0;
    struct sockaddr_in addr;

    while (true) {
        mutex_.lock();
        bool bWriteSet = list_buffer_.size() > 0;

        if (bWriteSet) {
            buf = list_buffer_.front().data.Ptr();
            len = list_buffer_.front().data.Length();
            memcpy(&addr, &list_buffer_.front().addr, sizeof(sockaddr_in));
        } else {
            bzero(readBuffer, MAX_DATAGRAM);
            buf = readBuffer;
            len = MAX_DATAGRAM - 1;
            bzero(&addr, sizeof(sockaddr_in));
        }

        mutex_.unlock();

        int err = 0;
        ret = __DoSelect(bWriteSet ? false : true, bWriteSet, buf, len, &addr, err);    // only read or write can be true

        if (ret == -1) {
            skerror("select error");

            if (event_)
                event_->OnError(this, err);

            break;
        }

        if (ret == -2 && event_ == NULL) {
            skinfo("normal break");
            break;
        }

        if (ret == -2)
            continue;

        if (bWriteSet) {
            ScopedLock lock(mutex_);
            list_buffer_.pop_front();
            continue;
        }
    }

    delete[] readBuffer;
}

bool UdpServer::__SetBroadcastOpt() {
    int on = 1;

    if (setsockopt(fd_socket_, SOL_SOCKET, SO_BROADCAST, (const char*)&on, sizeof(on)) != 0) {
        int errCode = socket_errno;
        skerror("udp set broadcast error: %s", socket_strerror(errCode));
        return false;
    }

    return true;
}

/*
 * return -2 break, -1 error, 0 timeout, else handle size
 */
int UdpServer::__DoSelect(bool _bReadSet, bool _bWriteSet, void* _buf, size_t _len, struct sockaddr_in* _addr, int& _errno) {
    skassert2((!(_bReadSet && _bWriteSet) && (_bReadSet || _bWriteSet)), "only read or write can be true, not both");

    selector_.PreSelect();

    if (_bWriteSet)
        selector_.Write_FD_SET(fd_socket_);

    if (_bReadSet)
        selector_.Read_FD_SET(fd_socket_);

    selector_.Exception_FD_SET(fd_socket_);

    int ret = selector_.Select();

    if (ret < 0) {
        skerror("udp select error: %s", socket_strerror(selector_.Errno()));
        _errno = selector_.Errno();
        return -1;
    }

    //    if (ret == 0)
    //    {
    //        skinfo("udp select timeout:%d ms", _timeoutMs);
    //        return 0;
    //    }

    // user break
    if (selector_.IsException()) {
        _errno = selector_.Errno();
        skerror("sel exception");
        return -1;
    }

    if (selector_.IsBreak()) {
        skinfo("sel breaker");
        return -2;
    }

    if (selector_.Exception_FD_ISSET(fd_socket_)) {
        _errno = socket_errno;
        skerror("socket exception error");
        return -1;
    }

    if (selector_.Write_FD_ISSET(fd_socket_)) {
        int ret = (int)sendto(fd_socket_, (const char*)_buf, _len, 0, (sockaddr*)_addr, sizeof(sockaddr_in));

        if (ret == -1) {
            _errno = socket_errno;
            skerror("sendto error: %s", socket_strerror(_errno));
            return -1;
        }

        return ret;
    }

    if (selector_.Read_FD_ISSET(fd_socket_)) {
        socklen_t len = sizeof(sockaddr_in);
        int ret = (int)recvfrom(fd_socket_, (char*)_buf, _len, 0, (sockaddr*)_addr, &len);

        if (ret == -1) {
            _errno = socket_errno;
            skerror("recvfrom error: %s", socket_strerror(_errno));
            return -1;
        }

        if (event_)
            event_->OnDataGramRead(this, _addr, _buf, ret);

        return ret;
    }

    return -1;
}



