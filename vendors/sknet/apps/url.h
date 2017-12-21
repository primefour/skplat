/* -*- encoding: utf-8; -*- */
/* -*- c-mode -*- */
/* File-name:    <url.h> */
/* Author:       <Xsoda> */
/* Create:       <Friday December 20 12:47:08 2013> */
/* Time-stamp:   <Wednesday December 25, 10:9:18 2013> */
/* Mail:         <Xsoda@Live.com> */

#ifndef __URL_H__
#define __URL_H__

/*
 * parse url like this
 *
 * schema://username:password@host:port/path?key=value#fragment
 * \____/   \______/ \______/ \__/ \__/ \__/ \_______/ \______/
 *   |         |        |       |    |    |      |         |
 * schema      |     password   |   port  |    query    fragment
 *          username          host      path
 *
 * note:
 *   - username, password, port, path, query, fragment is optional.
 *   - scheme, host must be setting.
 *   - username and password must be paired.
 *
 */
struct Query{
    std::string mKey;
    const char *getKey(){
        return mKey;
    }
    std::string mValue;
    std::string toString(){
        return mKey+?+mValue; 
    }
};

struct Url{
    std::string mHref;
    std::string mSchema;
    std::string mUsername;
    std::string mPassword;
    std::string mHost;
    std::string mPort;
    std::string mPath;
    std::string mFragment;
    KeyedHash<std::string,Query> mQueries;
    void parseLocation(const char *url);
    static int parse(const char *url,Url &url);
};
#endif /* !__URI_H__ */

