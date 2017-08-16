#include <stdlib.h>
#include "tcpserver.h"
#include "lock.h"
#include "sklog.h"
#include "socket_address.h"

TcpServer::TcpServer(const char* _ip, uint16_t _port, MTcpServer& _observer, int _backlog)
    : observer_(_observer)
    , thread_(this)
    , listen_sock_(INVALID_SOCKET), backlog_(_backlog) {
    memset(&bind_addr_, 0, sizeof(bind_addr_));
    bind_addr_ = *(struct sockaddr_in*)(&socket_address(_ip, _port).address());
}

TcpServer::TcpServer(uint16_t _port, MTcpServer& _observer, int _backlog)
    : observer_(_observer)
    , thread_(this)
    , listen_sock_(INVALID_SOCKET), backlog_(_backlog) {
    memset(&bind_addr_, 0, sizeof(bind_addr_));
    bind_addr_.sin_family = AF_INET;
    bind_addr_.sin_addr.s_addr = htonl(INADDR_ANY);
    bind_addr_.sin_port = htons(_port);
}

TcpServer::TcpServer(const sockaddr_in& _bindaddr, MTcpServer& _observer, int _backlog)
    : observer_(_observer)
    , thread_(this)
    , listen_sock_(INVALID_SOCKET), bind_addr_(_bindaddr), backlog_(_backlog)
{}

TcpServer::~TcpServer() {
    StopAndWait();
}

SOCKET TcpServer::Socket() const {
    return listen_sock_;
}

const sockaddr_in& TcpServer::Address() const {
    return bind_addr_;
}

bool TcpServer::StartAndWait(bool* _newone) {
    ScopedLock lock(mutex_);
    bool newone = false;
    thread_.start(&newone);

    if (_newone) *_newone = newone;

    if (newone) {
        breaker_.Clear();
        cond_.wait(lock);
    }

    return INVALID_SOCKET != Socket();
}

void TcpServer::StopAndWait() {
    ScopedLock lock(mutex_);

    if (!breaker_.Break()) {
        skassert(false);
        breaker_.Close();
        breaker_.ReCreate();
    }

    lock.unlock();

    if (thread_.isruning())
        thread_.join();
}

void TcpServer::operator()(){
    __ListenThread();
}

void TcpServer::__ListenThread() {
    char ip[16] = {0};
    inet_ntop(AF_INET, &(bind_addr_.sin_addr),  ip, sizeof(ip));

    do {
        ScopedLock lock(mutex_);

        skassert2(INVALID_SOCKET == listen_sock_, "m_listen_sock:%d", listen_sock_);

        SOCKET listen_sock = socket(AF_INET, SOCK_STREAM, 0);

        if (INVALID_SOCKET == listen_sock) {
            skerror("socket create err:(%d, %s)", socket_errno, socket_strerror(socket_errno)) ;
            cond_.notifyAll(lock);
            break;
        }

        if (0 > socket_reuseaddr(listen_sock, 1)) { // make sure before than bind
        
            skerror("socket reuseaddr err:(%d, %s)", socket_errno, socket_strerror(socket_errno));
            socket_close(listen_sock);
            cond_.notifyAll(lock);
            break;
        }

        if (0 > bind(listen_sock, (struct sockaddr*) &bind_addr_, sizeof(bind_addr_))) {
            skerror("socket bind err:(%d, %s)", socket_errno, socket_strerror(socket_errno));
            socket_close(listen_sock);
            cond_.notifyAll(lock);
            break;
        }

        if (0 > listen(listen_sock, backlog_)) {
            skerror("socket listen err:(%d, %s)", socket_errno, socket_strerror(socket_errno));
            socket_close(listen_sock);
            cond_.notifyAll(lock);
            break;
        }

        listen_sock_ = listen_sock;
        cond_.notifyAll(lock);
        lock.unlock();

        skinfo("listen start sock:(%d, %s:%d)", listen_sock_, ip, ntohs(bind_addr_.sin_port));
        observer_.OnCreate(this);

        while (true) {
            SocketSelect sel(breaker_);
            sel.PreSelect();
            sel.Exception_FD_SET(listen_sock_);
            sel.Read_FD_SET(listen_sock_);

            int selret = sel.Select();

            if (0 > selret) {
                skerror("select ret:%d, err:(%d, %s)", selret, sel.Errno(), socket_strerror(sel.Errno()));
                break;
            }

            if (sel.IsException()) {
                skerror("breaker exception");
                break;
            }

            if (sel.IsBreak()) {
                skinfo("breaker by user");
                break;
            }

            if (sel.Exception_FD_ISSET(listen_sock_)) {
                skerror("socket exception err:(%d, %s)", socket_error(listen_sock_), socket_strerror(socket_error(listen_sock_)));
                break;
            }

            if (!sel.Read_FD_ISSET(listen_sock_)) {
                skerror("socket unreadable but break by unknown");
                break;
            }

            struct sockaddr_in client_addr = {0};

            socklen_t client_addr_len = sizeof(client_addr);

            SOCKET client = accept(listen_sock_, (struct sockaddr*) &client_addr, &client_addr_len);

            if (INVALID_SOCKET == client) {
                skerror("accept return client invalid:%d, err:(%d, %s)", client, socket_errno, socket_strerror(socket_errno));
                break;
            }

            char cli_ip[16] = {0};
            inet_ntop(AF_INET, &(client_addr.sin_addr),  cli_ip, sizeof(cli_ip));
            skinfo("listen accept sock:(%d, %s:%d) cli:(%d, %s:%d)", listen_sock_, ip, ntohs(bind_addr_.sin_port), client, cli_ip, ntohs(client_addr.sin_port));

            observer_.OnAccept(this, client, client_addr);
        }
    } while (false);

    skinfo("listen end sock:(%d, %s:%d), ", listen_sock_, ip, ntohs(bind_addr_.sin_port));

    if (INVALID_SOCKET != listen_sock_) {
        socket_close(listen_sock_);
        listen_sock_ = INVALID_SOCKET;
    }

    observer_.OnError(this, socket_errno);
}
