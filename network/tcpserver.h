#ifndef TcpServer_H_
#define TcpServer_H_

#include "unix_socket.h"
#include "socketselect.h"
#include "mutex.h"
#include "condition.h"
#include "thread.h"

class XLogger;
class SocketSelect;
class TcpServer;

class MTcpServer {
  public:
    virtual ~MTcpServer() {}
    virtual void OnCreate(TcpServer* _server) = 0;
    virtual void OnAccept(TcpServer* _server, SOCKET _sock, const sockaddr_in& _addr) = 0;
    virtual void OnError(TcpServer* _server, int _error) = 0;
};

class TcpServer {
  public:
    TcpServer(const char* _ip, uint16_t _port, MTcpServer& _observer, int _backlog = 256);
    TcpServer(uint16_t _port, MTcpServer& _observer, int _backlog = 256);
    TcpServer(const sockaddr_in& _bindaddr, MTcpServer& _observer, int _backlog = 256);
    ~TcpServer();

    SOCKET Socket() const;
    const sockaddr_in& Address() const;

    bool StartAndWait(bool* _newone = NULL);
    void StopAndWait();
    void operator()();

  private:
    TcpServer(const TcpServer&);
    TcpServer& operator=(const TcpServer&);

  private:
    void __ListenThread();

  protected:
    MTcpServer&         observer_;
    Thread              thread_;
    Mutex                  mutex_;
    Condition            cond_;

    SOCKET                 listen_sock_;
    sockaddr_in         bind_addr_;
    const int             backlog_;
    SocketSelectBreaker breaker_;
};

#endif /* TcpServer_H_ */
