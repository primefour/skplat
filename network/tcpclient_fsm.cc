#include "tcpclient_fsm.h"
#include <stdlib.h>
#include <limits.h>
#include "sklog.h"
#include "socketselect.h"
#include "unix_socket.h"
#include "time_utils.h"
#include "platform_network.h"

TcpClientFSM::TcpClientFSM(const sockaddr& _addr):addr_(&_addr) {
    status_ = EStart;
    last_status_ = EStart;
    error_ = 0;
    remote_close_ = false;
    request_send_ = false;

    sock_ = INVALID_SOCKET;
    start_connecttime_ = 0;
    end_connecttime_ = 0;
}

TcpClientFSM::~TcpClientFSM() {
    Close(false);
    skassert2(INVALID_SOCKET == sock_, "%d", sock_);
}

void TcpClientFSM::RequestSend() {
    request_send_ = true;
}

TcpClientFSM::TSocketStatus TcpClientFSM::Status() const {
    return status_;
}

void TcpClientFSM::Status(TSocketStatus _status) {
    last_status_ = _status;
    status_ = _status;
}

TcpClientFSM::TSocketStatus TcpClientFSM::LastStatus() const {
    return last_status_;
}

int TcpClientFSM::Error() const {
    return error_;
}

bool TcpClientFSM::IsEndStatus() const {
    return EEnd == status_;
}

SOCKET TcpClientFSM::Socket() const {
    return sock_;
}

void TcpClientFSM::Socket(SOCKET _sock) {
    sock_ = _sock;
}

const sockaddr& TcpClientFSM::Address() const {
    return addr_.address();
}

const char* TcpClientFSM::IP() const {
    return addr_.ip();
}

uint16_t TcpClientFSM::Port() const {
    return addr_.port();
}

void TcpClientFSM::Close(bool _notify) {
    if (INVALID_SOCKET == sock_) return;

    if (remote_close_ || 0 != error_) {
        socket_close(sock_);
        sock_ = INVALID_SOCKET;
        return;
    }

    skinfo("sock:%d, (%s:%" PRIu16 "), close local socket close, notify:%d", sock_, addr_.ip(), addr_.port(), _notify);

    socket_close(sock_);
    sock_ = INVALID_SOCKET;

    last_status_ = status_;
    status_ = EEnd;
    error_ = 0;

    if (_notify){
        _OnClose(last_status_, -1, remote_close_);
    }
}

bool TcpClientFSM::RemoteClose() const {
    return remote_close_;
}

void TcpClientFSM::PreSelect(SocketSelect& _sel) {
    
    switch(status_) {
        case EStart: {
            PreConnectSelect(_sel);
            break;
        }
        case EConnecting: {
            _sel.Write_FD_SET(sock_);
            _sel.Exception_FD_SET(sock_);
            break;
        }
        case EReadWrite: {
            PreReadWriteSelect(_sel);
            break;
        }
        default:
            skassert2(false, "preselect status error");
    }
}

void TcpClientFSM::AfterSelect(SocketSelect& _sel) {
    if (EConnecting == status_) {
        AfterConnectSelect(_sel);
    } else if (EReadWrite == status_) {
        AfterReadWriteSelect(_sel);
    }

    if (EEnd == status_ && INVALID_SOCKET != sock_) {
        _OnClose(last_status_, error_, false);
    }
}

int TcpClientFSM::Timeout() const {

    if (EConnecting == status_) {
        return ConnectTimeout();
    }

    if (EReadWrite == status_){
        return ReadWriteTimeout();
    }

    if (EEnd == status_) {
        return 0;
    }

    return INT_MAX;
}

void TcpClientFSM::PreConnectSelect(SocketSelect& _sel) {
    skassert2(EStart == status_, "%d", status_);
    _OnCreate();
    skinfo("addr:(%s:%d), ", addr_.ip(), addr_.port());
    sock_ = socket(addr_.address().sa_family, SOCK_STREAM, IPPROTO_TCP);

    if (sock_ == INVALID_SOCKET) {
        error_ = socket_errno;
        last_status_ = status_;
        status_ = EEnd;
        _OnClose(last_status_, error_, false);
        skerror("close socket err:(%d, %s)", error_, socket_strerror(error_));
        return;
    }

    if (::getNetInfo() == kWifi && socket_fix_tcp_mss(sock_) < 0) {
#ifdef ANDROID
        skinfo("wifi set tcp mss error:%s", strerror(socket_errno));
#endif
    }

    if (0 != socket_ipv6only(sock_, 0)){
        skwarn("set ipv6only failed. error %s",strerror(socket_errno));
    }
    
    if (0 != socket_set_nobio(sock_)) {
        error_ = socket_errno;
        skerror("close socket_set_nobio:(%d, %s)", error_, socket_strerror(error_));
    } else {
        skinfo("socket:%d, ", sock_);
    }

    if (0 != error_) {
        last_status_ = status_;
        status_ = EEnd;
        _OnClose(last_status_, error_, false);
        return;
    }

    start_connecttime_ = gettickcount();

    int ret = connect(sock_, &(addr_.address()), addr_.address_length());

    if (0 != ret && !IS_NOBLOCK_CONNECT_ERRNO(socket_errno)) {
        end_connecttime_ = ::gettickcount();
        error_ = socket_errno;
        skwarn("close connect err:(%d, %s), localip:%s", error_, socket_strerror(error_), socket_address::getsockname(sock_).ip());
    } else {
        skinfo("connect");
        _sel.Write_FD_SET(sock_);
        _sel.Exception_FD_SET(sock_);
    }

    last_status_ = status_;

    if (0 != error_){
        status_ = EEnd;
    } else {
        status_ = EConnecting;
    }

    if (0 == error_) {
        _OnConnect();
    }
}

void TcpClientFSM::AfterConnectSelect(const SocketSelect& _sel) {
    skassert2(EConnecting == status_, "%d", status_);

    int timeout = ConnectTimeout();
    skinfo("sock:%d, (%s:%" PRIu16 "), ", sock_, addr_.ip(), addr_.port());

    if (_sel.Exception_FD_ISSET(sock_)) {
        socklen_t len = sizeof(error_);

        if (0 != getsockopt(sock_, SOL_SOCKET, SO_ERROR, &error_, &len)) { error_ = socket_errno; }

        skwarn("sock %d close connect exception: (%d, %s)", sock_, error_, socket_strerror(error_));

        end_connecttime_ = gettickcount();
        last_status_ = status_;
        status_ = EEnd;
        return;
    }

    error_ = socket_error(sock_);
    
    if (0 != error_) {
        skwarn("close connect error:(%d, %s), ", error_, socket_strerror(error_));
        end_connecttime_ = gettickcount();
        last_status_ = status_;
        status_ = EEnd;
        return;
    }
    
    if (0 == error_ && _sel.Write_FD_ISSET(sock_)){
        end_connecttime_ = gettickcount();
        last_status_ = status_;
        status_ = EReadWrite;
        skinfo("connected Rtt:%d, ", Rtt());
        _OnConnected(Rtt());
        return;
    }

    if (0 >= timeout) {
        end_connecttime_ = gettickcount();
        skwarn("close connect timeout:(%d, %d), (%d, %s)", ConnectAbsTimeout(), -timeout, SOCKET_ERRNO(ETIMEDOUT), socket_strerror(SOCKET_ERRNO(ETIMEDOUT)));

        error_ = SOCKET_ERRNO(ETIMEDOUT);
        last_status_ = status_;
        status_ = EEnd;
        return;
    }
}

void TcpClientFSM::PreReadWriteSelect(SocketSelect& _sel) {
    skassert2(EReadWrite == status_, "%d", status_);

    _sel.Read_FD_SET(sock_);
    _sel.Exception_FD_SET(sock_);

    if (0 < send_buf_.Length() || request_send_) {
        _sel.Write_FD_SET(sock_);
    }
}

void TcpClientFSM::AfterReadWriteSelect(const SocketSelect& _sel) {
    skassert2(EReadWrite == status_, "%d", status_);

    int timeout = ReadWriteTimeout();

    skinfo("sock:%d, (%s:%d), ", sock_, IP(), Port());

    if (_sel.Exception_FD_ISSET(sock_)) {
        socklen_t len = sizeof(error_);
        if (0 != getsockopt(sock_, SOL_SOCKET, SO_ERROR, &error_, &len)) { 
            error_ = socket_errno; 
        }
        skwarn("close exception:(%d, %s), ", error_, socket_strerror(error_));
        last_status_ = status_;
        status_ = EEnd;
        return;
    }

    if (_sel.Write_FD_ISSET(sock_)) {
        if (request_send_ && 0 == send_buf_.Length()) {
            request_send_ = false;
            _OnRequestSend(send_buf_);
            skassert(0 == send_buf_.Length());
        }

        ssize_t ret = send(sock_, send_buf_.Ptr(), send_buf_.Length(), 0);

        if (0 < ret) {
            send_buf_.Move(-ret);
            skinfo_if(0 == send_buf_.Length(), "all buffer send:%zd, m_send_buf:%zd", ret, send_buf_.Length());
            _OnSend(send_buf_, ret);
        } else if (IS_NOBLOCK_SEND_ERRNO(socket_errno)) {
            skwarn("buffer full wait for next select, send err:(%zd, %d, %s)", ret, socket_errno, socket_strerror(socket_errno));
        } else {
            error_ = socket_errno;
            last_status_ = status_;
            status_ = EEnd;
            skwarn("close send err:(%zd %d, %s), localip:%s", ret, error_, socket_strerror(error_),socket_address::getsockname(sock_).ip());
            return;
        }
    }

    if (_sel.Read_FD_ISSET(sock_)) {
        if (8 * 1024 > recv_buf_.Capacity() - recv_buf_.Length()) {
            recv_buf_.AddCapacity(16 * 1024 - (recv_buf_.Capacity() - recv_buf_.Length()));
        }

        ssize_t ret = recv(sock_, ((char*) recv_buf_.Ptr() + recv_buf_.Length()), recv_buf_.Capacity() - recv_buf_.Length(), 0);

        if (0 < ret) {

            skinfo_if(0 == recv_buf_.Length(), "first buffer recv:%zd, m_recv_buf:%zd", ret, recv_buf_.Length());
            recv_buf_.Length(recv_buf_.Pos(), recv_buf_.Length() + ret);
            _OnRecv(recv_buf_, ret);
        } else if (0 == ret) {
            error_ = 0;
            last_status_ = status_;
            status_ = EEnd;
            remote_close_ = true;
            skwarn("close recv %s:(%zd, %d, %s)", "remote socket close", ret, 0, socket_strerror(0));
            return;
        } else if (IS_NOBLOCK_READ_ERRNO(socket_errno)) {
            skwarn("buffer empty wait for next select, recv err:(%zd, %d, %s)", ret, socket_errno, socket_strerror(socket_errno));
        } else {
            error_ = socket_errno;
            last_status_ = status_;
            status_ = EEnd;
            skwarn("close recv %s (%zd, %d, %s), localip:%s", "err", ret, error_, socket_strerror(error_), socket_address::getsockname(sock_).ip());
            return;
        }
    }

    if (!_sel.Write_FD_ISSET(sock_) && !_sel.Read_FD_ISSET(sock_) && 0 >= timeout) {
        skwarn("close readwrite timeout:(%d, %d), (%d, %s)", ReadWriteAbsTimeout(), -timeout, SOCKET_ERRNO(ETIMEDOUT), socket_strerror(SOCKET_ERRNO(ETIMEDOUT)));

        error_ =  SOCKET_ERRNO(ETIMEDOUT);
        last_status_ = status_;
        status_ = EEnd;
        return;
    }
}

int TcpClientFSM::ConnectTimeout() const { return INT_MAX; }
int TcpClientFSM::ReadWriteTimeout() const { return INT_MAX; }
int TcpClientFSM::ConnectAbsTimeout() const { return INT_MAX; }
int TcpClientFSM::ReadWriteAbsTimeout() const { return INT_MAX;}
int TcpClientFSM::Rtt() const { return int(end_connecttime_ - start_connecttime_);}
