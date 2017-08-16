#ifndef GETSOCKTCPINFO_H_
#define GETSOCKTCPINFO_H_

#if defined(__APPLE__) || defined(ANDROID) || defined(__linux__)
#include <netinet/in.h>
#include <netinet/tcp.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

// RETURN VALUE
//
//       On success, zero is returned.  On error, -1 is returned, and errno is set
//       appropriately.
// ERRORS         top
//
//       EBADF     The argument sockfd is not a valid descriptor.
//
//       EFAULT    The address pointed to by optval is not in a valid part of the
//                 process address space.  For getsockopt(), this error may also be
//                 returned if optlen is not in a valid part of the process address
//                 space.
//
//       EINVAL    optlen invalid in setsockopt().  In some cases this error can also
//                 occur for an invalid value in optval (e.g., for the
//                 IP_ADD_MEMBERSHIP option described in ip(7)).
//
//       ENOPROTOOPT
//                 The option is unknown at the level indicated.
//
//       ENOTSOCK  The argument sockfd is a file, not a socket.

#if defined(__APPLE__) && !defined(tcp_info)
    #define tcp_info tcp_connection_info
    #define TCP_INFO TCP_CONNECTION_INFO
#endif
    
    
    
int getsocktcpinfo(int _sockfd, struct tcp_info* _info);
char* tcpinfo2str(struct tcp_info* _info, char* _info_str_buf, size_t _buf_len);
    
#ifdef __cplusplus
}
#endif
#endif /* GETSOCKTCPINFO_H_ */
