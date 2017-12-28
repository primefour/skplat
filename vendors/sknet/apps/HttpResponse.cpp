#include"HttpResponse.h"
#include"HttpRequest.h"
#include"HttpHeader.h"
#include"Log.h"
#include"Url.h"
#include<string>



HttpResponse::HttpResponse(){
    mContentLength = -1; 
    mClose = true; 
    mUncompressed = true;
}

int HttpResponse::relocation(const char *reloc){
    if (reloc == NULL){
        return BAD_VALUE; 
    }
    //check is there any schema
    const char * schema = strstr(reloc,":");
    if(schema == NULL){
        mRequest->mUrl.parseReloc(reloc);
        return OK;
    }else{
        if(Url::parseUrl(reloc,&mRequest->mUrl) == NULL){
            return UNKNOWN_ERROR;
        }else{
            return OK;
        }
    }
}
