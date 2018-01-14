#include"HttpsTransfer.h"
#include"Log.h"
#include<string>
#include<unistd.h>
#include<stdlib.h>

#define GET_REQUEST "GET / HTTP/1.0\r\n\r\n"
#define DEBUG_LEVEL 1

const char* HttpsTransfer::entropyCustomSeed = "CustomSeed";
//constrution
HttpsTransfer::HttpsTransfer(int fd,std::string &host):mHost(host){
    mbedtls_ssl_init(&mSslCtx);
    mbedtls_ssl_config_init(&mSslConfig);
    mbedtls_x509_crt_init(&mRootCert);
    mbedtls_ctr_drbg_init(&mCtrDrbg);
    mbedtls_entropy_init(&mEntropy);
    mServerFd.fd = fd;
    mError = OK;
}

HttpsTransfer::HttpsTransfer(int fd,const char *host):mHost(host){
    mbedtls_ssl_init(&mSslCtx);
    mbedtls_ssl_config_init(&mSslConfig);
    mbedtls_x509_crt_init(&mRootCert);
    mbedtls_ctr_drbg_init(&mCtrDrbg);
    mbedtls_entropy_init(&mEntropy);
    mServerFd.fd = fd;
    mError = OK;
}

//mbedtls errstring implements
void HttpsTransfer::errString(int errNum){
    char errBuff[100];
    mbedtls_strerror(errNum,errBuff,sizeof(errBuff));
    errBuff[sizeof(errBuff) -1] = '\0';
    ALOGE("Last error was: %d - %s",errNum,errBuff);
}

HttpsTransfer::~HttpsTransfer(){
    mbedtls_x509_crt_free(&mRootCert);
    mbedtls_ssl_free(&mSslCtx);
    mbedtls_ssl_config_free(&mSslConfig);
    mbedtls_ctr_drbg_free(&mCtrDrbg);
    mbedtls_entropy_free(&mEntropy);
}

void HttpsTransfer::sslShake(){
    int ret = 0;
    //initialize random seed
    ALOGD("initialize random seed ...");
    if( ( ret = mbedtls_ctr_drbg_seed(&mCtrDrbg, 
                    mbedtls_entropy_func,&mEntropy, 
                    (const unsigned char *)entropyCustomSeed, strlen(entropyCustomSeed))) != 0 ){
        ALOGE("create random number failed ");
        mError = UNKNOWN_ERROR;
        return ;
    }

    //Loading the CA root certificate ... 
    //fix me,there is no any root certificate
    ALOGD("Loading the CA root certificate ...");
    ret = mbedtls_x509_crt_parse(&mRootCert,(const unsigned char *)mbedtls_test_cas_pem,mbedtls_test_cas_pem_len);
    if( ret < 0 ){
        ALOGE("failed mbedtls_x509_crt_parse returned -0x%x", -ret );
        mError = ret;
        return ;
    }
    //Setting up the SSL/TLS structure...
    ALOGD("Setting up the SSL/TLS structure...");
    if( ( ret = mbedtls_ssl_config_defaults( &mSslConfig,
                    MBEDTLS_SSL_IS_CLIENT,
                    MBEDTLS_SSL_TRANSPORT_STREAM,
                    MBEDTLS_SSL_PRESET_DEFAULT )) != 0 ){
        ALOGE( " failed  mbedtls_ssl_config_defaults returned %d", ret );
        mError = UNKNOWN_ERROR;
        return;
    }
    /* OPTIONAL is not optimal for security,
     * but makes interop easier in this simplified example */
    mbedtls_ssl_conf_authmode( &mSslConfig, MBEDTLS_SSL_VERIFY_OPTIONAL );
    mbedtls_ssl_conf_ca_chain( &mSslConfig, &mRootCert, NULL );
    mbedtls_ssl_conf_rng(&mSslConfig, mbedtls_ctr_drbg_random, &mCtrDrbg);

    if( ( ret = mbedtls_ssl_setup( &mSslCtx, &mSslConfig) ) != 0 ) {
        ALOGE( " failed mbedtls_ssl_setup returned %d", ret );
        mError = UNKNOWN_ERROR;
        return;
    }

    if((ret = mbedtls_ssl_set_hostname(&mSslCtx,mHost.c_str())) != 0 ){
        ALOGE(" failed mbedtls_ssl_set_hostname returned %d", ret );
        mError = UNKNOWN_ERROR;
        return;
    }

    //set io interface
    mbedtls_ssl_set_bio(&mSslCtx, &mServerFd, mbedtls_net_send, mbedtls_net_recv, NULL );
    /*
     * 4. Handshake
     */
    //Performing the SSL/TLS handshake...
    while( ( ret = mbedtls_ssl_handshake( &mSslCtx) ) != 0 ) {
        if( ret != MBEDTLS_ERR_SSL_WANT_READ && ret != MBEDTLS_ERR_SSL_WANT_WRITE ) {
            ALOGE( " failed mbedtls_ssl_handshake returned -0x%x", -ret );
            mError = UNKNOWN_ERROR;
            return;
        }
    }
    int flags;
    /*
     * 5. Verify the server certificate Verifying peer X.509 certificate...
     */
    /* In real life, we probably want to bail out when ret != 0 */
    if( ( flags = mbedtls_ssl_get_verify_result( &mSslCtx) ) != 0 ){
        char verifyBuff[512];
        ALOGE("verify failed");
        mbedtls_x509_crt_verify_info(verifyBuff, sizeof( verifyBuff ), "  ! ", flags );
        ALOGE( "%s", verifyBuff );
        //mError = UNKNOWN_ERROR;
    } else {
        //shake completely and return
        mError = OK;
        return ;
    }
}

int HttpsTransfer::write(const char *buff,int len){
    int ret;
    if(mError != OK){
        ALOGW("write mError = %d ",mError);
    }

    ret = mbedtls_ssl_write( &mSslCtx, (const unsigned char *)buff, len ); 
    if( ret == MBEDTLS_ERR_SSL_WANT_READ && ret == MBEDTLS_ERR_SSL_WANT_WRITE ){
        ALOGD( "ssl ret %s return",ret== MBEDTLS_ERR_SSL_WANT_READ?"MBEDTLS_ERR_SSL_WANT_READ":"MBEDTLS_ERR_SSL_WANT_WRITE ");
        return HTTPS_WOULD_BLOCK;
    }
    if(ret < 0){
        ALOGE( " failed mbedtls_ssl_write returned %d",ret );
        mError = ret;
        return UNKNOWN_ERROR;
    }
    return ret;
}

int HttpsTransfer::readSelect(fd_set *rdSet){
    //ALOGD("%s in_msglen %zd ",__func__,mSslCtx.in_msglen); 
    if(mSslCtx.in_msglen  > 0 && mSslCtx.in_offt != NULL){
        //ALOGD("return true %s in_msglen %zd ",__func__,mSslCtx.in_msglen); 
        FD_ZERO(rdSet);
        FD_SET(mServerFd.fd,rdSet);
        return 1;
    }else{
        return 0;
    }
}

int HttpsTransfer::read(char *buff,int len){
    int ret = 0;
    if(mError != OK){
        ALOGW("read mError = %d ",mError);
    }
    ret = mbedtls_ssl_read( &mSslCtx, (unsigned char *)buff,len);
    if(ret == MBEDTLS_ERR_SSL_WANT_READ || ret == MBEDTLS_ERR_SSL_WANT_WRITE ){
        ALOGD( "ssl ret %s return",ret== MBEDTLS_ERR_SSL_WANT_READ?"MBEDTLS_ERR_SSL_WANT_READ":"MBEDTLS_ERR_SSL_WANT_WRITE ");
        return HTTPS_WOULD_BLOCK;
    }

    if(ret == MBEDTLS_ERR_SSL_PEER_CLOSE_NOTIFY ){
        ALOGE( "failed MBEDTLS_ERR_SSL_PEER_CLOSE_NOTIFY ");
        mError = ret;
        return UNKNOWN_ERROR;
    }
    if(ret < 0 ) {
        ALOGE( "failed mbedtls_ssl_read returned %d", ret );
        mError = ret;
        return UNKNOWN_ERROR;
    }

    if(ret == 0 ){
        ALOGD( "ssl EOF" );
        mError = OK;
        mbedtls_ssl_close_notify(&mSslCtx);
        return 0;
    }
    return ret;
}

int HttpsTransfer::getSocket() const{
    if(mError != OK){
        return UNKNOWN_ERROR;
    }else{
        return mServerFd.fd;
    }
}

