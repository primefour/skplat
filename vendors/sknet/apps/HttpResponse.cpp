#include"HttpResponse.h"
#include"HttpRequest.h"

int HttpResponse::relocation(const char *reloc){

    if (reloc == NULL){
        return BAD_VALUE; 
    }
    //check is there any schema
    const char * schema = strstr(reloc,":");
    if(schema == NULL){
        mRequest->mUrl.parseReloc(reloc.c_str());
        return OK:
    }else{
        if(Url::parseUrl(reloc.c_str(),&mRequest->mUrl) == NULL){
            return NUKNOWN_ERROR;
        }else{
            return OK;
        }
    }

}
