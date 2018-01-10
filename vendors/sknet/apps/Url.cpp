/* -*- encoding: utf-8; -*- */
/* -*- c-mode -*- */
/* File-name:    <url.c> */
/* Author:       <Xsoda> */
/* Create:       <Friday December 20 12:38:02 2013> */
/* Time-stamp:   <Wednesday December 25, 10:10:57 2013> */
/*url_field_t *url = url_parse("schema://usr:pwd@localhost:port/path?a=b&c=d&e=f#foo");*/
/* Mail:         <Xsoda@Live.com> */
#include "Url.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <string>
#include "Log.h"

static std::string NullString = std::string();

std::string Url::dupString(const char *str, int n) {
   char *dst;
   if (!str) return NullString ;
   if (n < 0) n = strlen(str);
   if (n == 0) return NullString;
   ALOGD("%s ",str);
   return std::string(str,n);
}

void Url::parseQuery(const char *query,Url *url) {
   int length;
   int offset;
   const char *chr;
   length = strlen(query);
   offset = 0;
   chr = strchr(query, '=');
   while (chr) {
       Query tmpQuery ;
       tmpQuery.mKey = dupString(query, chr - query);
       query = chr + 1;
       chr = strchr(query, '&');
       if (chr) {
           tmpQuery.mValue = dupString(query, chr - query);
           query = chr + 1;
           chr = strchr(query, '=');
           url->mQueries.add(tmpQuery.mKey,tmpQuery);
       } else {
         tmpQuery.mValue = dupString(query, -1);
         url->mQueries.add(tmpQuery.mKey,tmpQuery);
         break;
      }
   }
}

/*
 * schema://username:password@host:port/path?key=value#fragment
 * \____/   \______/ \______/ \__/ \__/ \__/ \_______/ \______/
 *   |         |        |       |    |    |      |         |
 * schema      |     password   |   port  |    query    fragment
 *          username          host      path
 */
void Url::parseReloc(const char *url){
    const char* pch = strchr(url, ':');   /* parse schema */
    if(pch == NULL){
        //relocation by path
        const char *host = strstr(mHref.c_str(),mHost.c_str());
        const char *path = strchr(host,'/');
        int size = 0;
        if(path == NULL){
            //no path partial
            size = mHref.size();
        }else{
            size = path - host + 1;
        }
        std::string href = std::string(mHref.c_str(),size);
        href += url;
        ALOGD("relocation by path: %s :href %s",url,href.c_str());
        parseUrl(href.c_str(),this);
    }else{
        //relocation by url
        ALOGD("relocation by url: %s ",url);
        parseUrl(url,this);
    }
}

Url* Url::parseUrl(const char *str,Url *url){
   ALOGD("parseUrl %s ",str);
   const char *pch;
   std::string query;
   if (str && str[0]) {
      url->mHref = dupString(str, -1);
      pch = strchr(str, ':');   /* parse schema */
      if (pch && pch[1] == '/' && pch[2] == '/'){
         url->mSchema = dupString(str, pch - str);
         str = pch + 3;
      } else{
          ALOGD("parse url fail %s ",str);
          return NULL;
      }
      pch = strchr(str, '@');   /* parse user info */
      if (pch) {
         pch = strchr(str, ':');
         if (pch) {
            url->mUsername = dupString(str, pch - str);
            str = pch + 1;
            pch = strchr(str, '@');
            if (pch) {
               url->mPassword = dupString(str, pch - str);
               str = pch + 1;
            } else {
                ALOGD("parse url fail %s ",str);
                return  NULL;
            }
         } else {
             ALOGD("parse url fail %s ",str);
             return  NULL;
         }
      }
      if (str[0] == '['){        /* parse host info */ 
         str++;
         pch = strchr(str, ']');
         if (pch) {
            url->mHost = dupString(str, pch - str);
            str = pch + 1;
            if (str[0] == ':') {
               str++;
               pch = strchr(str, '/');
               if (pch) {
                  url->mPort = dupString(str, pch - str);
                  str = pch + 1;
               } else {
                  url->mPort = dupString(str, -1);
                  str = str + strlen(str);
               }
            }
         } else {
            ALOGD("parse url fail %s ",str);
            return NULL;
         }
      } else {
         const char *pch_slash;
         pch = strchr(str, ':');
         pch_slash = strchr(str, '/');
         if (pch && (!pch_slash || (pch_slash && pch<pch_slash))) {
            url->mHost = dupString(str, pch - str);
            str = pch + 1;
            pch = strchr(str, '/');
            if (pch) {
               url->mPort = dupString(str, pch - str);
               str = pch + 1;
            } else {
               url->mPort = dupString(str, -1);
               str = str + strlen(str);
            }
         } else {
            pch = strchr(str, '/');
            if (pch) {
               url->mHost = dupString(str, pch - str);
               str = pch + 1;
            } else {
               url->mHost = dupString(str, -1);
               str = str + strlen(str);
            }
         }
      }
      if (str[0]){               /* parse path, query and fragment */
         pch = strchr(str, '?');
         if (pch) {
            url->mPath = dupString(str, pch - str);
            str = pch + 1;
            pch = strchr(str, '#');
            if (pch) {
               query = dupString(str, pch - str);
               str = pch + 1;
               url->mFragment = dupString(str, -1);
            } else {
               query = dupString(str, -1);
               str = str + strlen(str);
            }
            parseQuery(query.c_str(),url);
         } else {
            pch = strchr(str, '#');
            if (pch) {
               url->mPath = dupString(str, pch - str);
               str = pch + 1;
               url->mFragment = dupString(str, -1);
               str = str + strlen(str);
            } else {
               url->mPath = dupString(str, -1);
               str = str + strlen(str);
            }
         }
      }
   } else {
       ALOGD("parse url fail %s ",str);
       return NULL;
   }
   dumpUrl(url);
}

void Url::dumpUrl(Url*url) {
   if (!url) return;
   ALOGD("url field:");
   ALOGD("  - href:     '%s'", url->mHref.c_str());
   ALOGD("  - schema:   '%s'", url->mSchema.c_str());
   ALOGD("  - username: '%s'", url->mUsername.c_str());
   ALOGD("  - password: '%s'", url->mPassword.c_str());
   ALOGD("  - host:     '%s'", url->mHost.c_str());
   ALOGD("  - port:     '%s'", url->mPort.c_str());
   ALOGD("  - path:     '%s'", url->mPath.c_str());
   ALOGD("  - fragment: '%s'", url->mFragment.c_str());
}

