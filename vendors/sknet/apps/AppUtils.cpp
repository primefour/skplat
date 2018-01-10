#include<stdio.h>
#include<string.h>
#include<string>
static std::string NullString = std::string();
static const int toLower = 'a' - 'A';
// For invalid inputs (if a contains spaces or non-token bytes), a
// is unchanged and a string copy is returned.
std::string canonicalMIMEHeaderKey(const char *headerEntry,int size = 0){
	// See if a looks like a header key. If not, return it unchanged.
    if(headerEntry == NULL){
        return NullString;
    }
    if(size == 0){
        size = strlen(headerEntry);
    }
    int i = 0;
    char *tmpStr = new char[size +1];
    memset(tmpStr,0,size+1);
	bool upper = true;
    for(i = 0 ;i < size ;i ++){
		// Canonicalize: first letter upper case
		// and upper case after each dash.
		// (Host, User-Agent, If-Modified-Since).
		// MIME headers are ASCII only, so no Unicode issues.
        tmpStr[i] = *(headerEntry+i);
		if (upper && 'a' <= *(headerEntry+i) && *(headerEntry+i) <= 'z' ){
			tmpStr[i] -= toLower;
		} else if (!upper && 'A' <= *(headerEntry+i) && *(headerEntry+i) <= 'Z' ){
		    tmpStr[i] += toLower;
		}
		upper = *(headerEntry+i) == '-';// for next time
	}
    printf("tmpStr = %s ",tmpStr);
    std::string ret(tmpStr);
    delete[] tmpStr;
    return ret;
}

// trim returns s with leading and trailing spaces and tabs removed.
// It does not assume Unicode or UTF-8.
std::string trim(const char *s,int end){
    if(s == NULL){
        return NullString;
    }
	int i = 0;
    for(;i < end && (s[i] == ' ' || s[i] == '\t');) {
        i++;
    }
    int n = end;
	for(;n > i && (s[n-1] == ' ' || s[n-1] == '\t');) {
		n--;
	}
    std::string ret(s+i,n-i);
	return ret; 
}

// trim returns s with leading and trailing spaces and tabs removed.
// It does not assume Unicode or UTF-8.
std::string trim(const char *s,int begin,int end){
    if(s == NULL){
        return NullString;
    }
	int i = begin;
    for(;i < end && (s[i] == ' ' || s[i] == '\t');) {
        i++;
    }
    int n = end;
	for(;n > i && (s[n-1] == ' ' || s[n-1] == '\t');) {
		n--;
	}
    std::string ret(s+i,n-i);
	return ret; 
}

// trim returns s with leading and trailing spaces and tabs removed.
// It does not assume Unicode or UTF-8.
std::string trim(const char *s){
    if(s == NULL){
        return NullString;
    }
	int i = 0;
    int size = strlen(s);
    for(;i < size && (s[i] == ' ' || s[i] == '\t');) {
        i++;
    }
    int n = size;
	for(;size > i && (s[n-1] == ' ' || s[n-1] == '\t');) {
		n--;
	}
    std::string ret(s+i,n-i);
	return ret; 
}

long parseHex(const char *str,long &data){
    int i = 0;
    if(str == NULL){
        data = 0;
        return 0;
    }
    int size = strlen(str);
    for(i = 0 ;i < size ;i++){
        int tmp = 0;
        if(str[i] >= '0' && str[i] <='9'){
            tmp = str[i] - '0' ;
        }else if(str[i] >= 'a' && str[i] <= 'f'){
            tmp = str[i] -'a' + 10;
        }else if(str[i] >= 'A' && str[i] <= 'F'){
            tmp = str[i] -'A' + 10;
        }else{
            break;
        }
        data <<= 4;
        data |= tmp;
    }
	return data;
}

