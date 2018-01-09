#ifndef __HTTPS_TRANSFER_H__
#define __HTTPS_TRANSFER_H__

#if !defined(MBEDTLS_CONFIG_FILE)
#include "mbedtls/config.h"
#else
#include MBEDTLS_CONFIG_FILE
#endif

#if defined(MBEDTLS_PLATFORM_C)
#include "mbedtls/platform.h"
#else
#include <stdio.h>
#include <stdlib.h>
#define mbedtls_time       time 
#define mbedtls_time_t     time_t
#define mbedtls_fprintf    fprintf
#define mbedtls_printf     printf
#endif

#include "mbedtls/net_sockets.h"
#include "mbedtls/debug.h"
#include "mbedtls/ssl.h"
#include "mbedtls/entropy.h"
#include "mbedtls/ctr_drbg.h"
#include "mbedtls/error.h"
#include "mbedtls/certs.h"
#include <string.h>
#include "Log.h"
#include<string>

class HttpsTransfer{
    public:
        //random custom seed
        static const char* entropyCustomSeed; 
        //constrution
        HttpsTransfer(int fd,std::string &host);
        HttpsTransfer(int fd,const char *host);
        virtual ~HttpsTransfer();
        //mbedtls errstring implements
        void errString(int errNum);
        void sslShake();
        int write(const char *buff,int len);
        int read(char *buff,int len);
    private:
        mbedtls_net_context mServerFd;
        mbedtls_entropy_context mEntropy;
        mbedtls_ctr_drbg_context mCtrDrbg;
        mbedtls_ssl_context mSslCtx;
        mbedtls_ssl_config mSslConfig;
        mbedtls_x509_crt mRootCert;
        std::string mHost;
        int mError;
};

#endif 
