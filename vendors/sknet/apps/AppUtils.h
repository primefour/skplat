#ifndef __APP_UTILS_H__
#define __APP_UTILS_H__
#include<stdio.h>
#include<string.h>
#include<string>
std::string canonicalMIMEHeaderKey(const char *headerEntry,int size = 0);
// trim returns s with leading and trailing spaces and tabs removed.
// It does not assume Unicode or UTF-8.
std::string trim(const char *s,int end);
// trim returns s with leading and trailing spaces and tabs removed.
// It does not assume Unicode or UTF-8.
std::string trim(const char *s,int begin,int end);
// trim returns s with leading and trailing spaces and tabs removed.
// It does not assume Unicode or UTF-8.
std::string trim(const char *s);
//parse hex digit to digit
long parseHex(const char *str,long &data);
#endif 
