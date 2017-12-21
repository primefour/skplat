/* -*- encoding: utf-8; -*- */
/* -*- c-mode -*- */
/* File-name:    <url.c> */
/* Author:       <Xsoda> */
/* Create:       <Friday December 20 12:38:02 2013> */
/* Time-stamp:   <Wednesday December 25, 10:10:57 2013> */
/*url_field_t *url = url_parse("schema://usr:pwd@localhost:port/path?a=b&c=d&e=f#foo");*/
/* Mail:         <Xsoda@Live.com> */
#include "url.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

static std::string NullString = std::string();

static std::string toString(const char *str, int n) {
   char *dst;
   if (!str) return NullString ;
   if (n < 0) n = strlen(str);
   if (n == 0) return NullString;
   return string(str,n);
}

void parseQuery(const char *query,Url *url) {
   int length;
   int offset;
   char *chr;
   length = strlen(query);
   offset = 0;
   chr = strchr(query, '=');
   while (chr) {
       Query tmpQuery ;
       tmpQuery.mKey = toString(query, chr - query);
       query = chr + 1;
       chr = strchr(query, '&');
       if (chr) {
           tmpQuery.mValue = toString(query, chr - query);
           query = chr + 1;
           chr = strchr(query, '=');
           url->mQueries->add(tmpQuery.mKey,tmpQuery);
       } else {
         tmpQuery.mValue = toString(query, -1);
         url->mQueries->add(tmpQuery.mKey,tmpQuery);
         break;
      }
   }
}

void parseUrl(const char *str,Url *url){
   const char *pch;
   char *query;
   query = NULL;
   if (str && str[0]) {
      url->mHref = toString(str, -1);
      pch = strchr(str, ':');   /* parse schema */
      if (pch && pch[1] == '/' && pch[2] == '/'){
         url->mSchema = toString(str, pch - str);
         str = pch + 3;
      } else{
          ALOGD("parse url fail %s ",str);
          return ;
      }
      pch = strchr(str, '@');   /* parse user info */
      if (pch) {
         pch = strchr(str, ':');
         if (pch) {
            url->mUsername = toString(str, pch - str);
            str = pch + 1;
            pch = strchr(str, '@');
            if (pch) {
               url->mPassword = toString(str, pch - str);
               str = pch + 1;
            } else {
                ALOGD("parse url fail %s ",str);
                return ;
            }
         } else {
             ALOGD("parse url fail %s ",str);
             return ;
         }
      }
      if (str[0] == '['){        /* parse host info */ 
         str++;
         pch = strchr(str, ']');
         if (pch) {
            url->mHost = toString(str, pch - str);
            str = pch + 1;
            if (str[0] == ':') {
               str++;
               pch = strchr(str, '/');
               if (pch) {
                  url->mPort = toString(str, pch - str);
                  str = pch + 1;
               } else {
                  url->mPort = toString(str, -1);
                  str = str + strlen(str);
               }
            }
         } else {
            ALOGD("parse url fail %s ",str);
            return;
         }
      } else {
         const char *pch_slash;
         pch = strchr(str, ':');
         pch_slash = strchr(str, '/');
         if (pch && (!pch_slash || (pch_slash && pch<pch_slash))) {
            url->mHost = toString(str, pch - str);
            str = pch + 1;
            pch = strchr(str, '/');
            if (pch) {
               url->mPort = toString(str, pch - str);
               str = pch + 1;
            } else {
               url->mPort = toString(str, -1);
               str = str + strlen(str);
            }
         } else {
            pch = strchr(str, '/');
            if (pch) {
               url->mHost = toString(str, pch - str);
               str = pch + 1;
            } else {
               url->mHost = toString(str, -1);
               str = str + strlen(str);
            }
         }
      }
      if (str[0]){               /* parse path, query and fragment */
         pch = strchr(str, '?');
         if (pch) {
            url->mPath = toString(str, pch - str);
            str = pch + 1;
            pch = strchr(str, '#');
            if (pch) {
               query = toString(str, pch - str);
               str = pch + 1;
               url->mFragment = toString(str, -1);
            } else {
               query = toString(str, -1);
               str = str + strlen(str);
            }
            parse_query(query.c_str(),url);
         } else {
            pch = strchr(str, '#');
            if (pch) {
               url->mPath = toString(str, pch - str);
               str = pch + 1;
               url->mFragment = toString(str, -1);
               str = str + strlen(str);
            } else {
               url->mPath = toString(str, -1);
               str = str + strlen(str);
            }
         }
      }
   } else {
       ALOGD("parse url fail %s ",str);
       return;
   }
}

void dumpUrl(Url*url) {
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

