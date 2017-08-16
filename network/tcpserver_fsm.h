#ifndef TcpServerFSM_H_
#define TcpServerFSM_H_

#include "autobuffer.h"
#include "unix_socket.h"
#include "sklog.h"
#include "mutex.h"
#include "lock.h"

class XLogger;
class SocketSelect;

class TcpServerFSM {
  public:
    enum TSocketStatus {
        kAccept,
        kReadWrite,
        kEnd,
    };

  public:
    TcpServerFSM(SOCKET _socket);
    TcpServerFSM(SOCKET _socket, const sockaddr_in& _addr);
    virtual ~TcpServerFSM();

    TSocketStatus Status() const;
    void Status(TSocketStatus _status);
    bool IsEndStatus() const;

    SOCKET Socket() const;
    const sockaddr_in& Address() const;
    const char* IP() const;
    uint16_t Port() const;
    size_t SendBufLen() {return send_buf_.Length();}
    void Close(bool _notify = true);

    bool WriteFDSet()
    {
    	ScopedLock lock (write_fd_set_mutex_);
    	return is_write_fd_set_;
    }
    void WriteFDSet(bool _is_set) {
        verbose_scope_func();
    	skdebug("_is_set:%d, is_write_fd_set_:%d", _is_set, is_write_fd_set_);
    	ScopedLock lock (write_fd_set_mutex_);
    	is_write_fd_set_  = _is_set;
    }

    virtual TSocketStatus PreSelect(SocketSelect& _sel);
    virtual TSocketStatus AfterSelect(SocketSelect& _sel);
    virtual int Timeout() const;

  private:
    TcpServerFSM(const TcpServerFSM&);
    TcpServerFSM& operator=(const TcpServerFSM&);

  protected:
    virtual TSocketStatus PreReadWriteSelect(SocketSelect& _sel);
    virtual TSocketStatus AfterReadWriteSelect(const SocketSelect& _sel);

    virtual int ReadWriteTimeout() const;
    virtual int ReadWriteAbsTimeout() const;

    virtual void _OnAccept() = 0;
    virtual void _OnRecv(AutoBuffer& _recv_buff, ssize_t _recv_len) = 0;
    virtual void _OnSend(AutoBuffer& _send_buff, ssize_t _send_len) = 0;
    virtual void _OnClose(TSocketStatus _status, int _error, bool _userclose) = 0;


  protected:
    TSocketStatus status_;
    SOCKET sock_;
    sockaddr_in addr_;
    char ip_[16];

    AutoBuffer send_buf_;
    AutoBuffer recv_buf_;

    bool is_write_fd_set_;
    Mutex write_fd_set_mutex_;
};

#endif /* TcpServerFSM_H_ */
