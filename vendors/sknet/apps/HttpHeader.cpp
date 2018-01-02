#include"HttpHeader.h"
#include"SimpleHash.h"
#include<string.h>
#include<string>
#include"AppUtils.h"
#include<stdarg.h>

const char * HttpHeader::connectionHints = "Connection";
const char * HttpHeader::hostHints = "Host";
const char * HttpHeader::userAgentHints = "User-Agent";
const char * HttpHeader::contentLengthHints = "Content-Length";
const char * HttpHeader::transferEncodingHints = "Transfer-Encoding";
const char * HttpHeader::encodingChunkedHints = "chunked";
const char * HttpHeader::trailerHints = "Trailer";
const char * HttpHeader::encodingCompressHints = "compress";
const char * HttpHeader::encodingDeflateHints = "deflate";
const char * HttpHeader::encodingGzipHints = "gzip";
const char * HttpHeader::encodingIdentifyHints = "identity";
const char * HttpHeader::locationHints ="Location" ;
const char * HttpHeader::lineHints= "\r\n";
const char * HttpHeader::headerHints = "\r\n\r\n";

template<> HttpHeaderEntry SimpleHash<std::string, HttpHeaderEntry>::mNullItem = HttpHeaderEntry();
#define MAX_STRING       (1024) 

int HttpHeader::checkHeader(sp<BufferUtils> &buffer){
    int size = buffer->size();
    if(size == 0){
        return -1;
    }
    //get buffer data
    const char *data = (const char *)buffer->data();
    const char *tmp = NULL;
    if((tmp = strstr(data,headerHints)) != NULL){
        return tmp - data + strlen(headerHints);
    }else{
        return -1;
    }
}
//static function to parser header only
//Buffer is data of header
HttpHeader* HttpHeader::parser(sp<BufferUtils> &buffer,HttpHeader *ptrHeader){
    //get size of buffer data 
    int size = buffer->size();
    //get buffer data
    const char *data = (const char *)buffer->data();
    if(size == 0){
        return NULL;
    }

    //get lines and parse
    const char *newLine = strstr(data,lineHints);
    newLine += 2;//seek to http header entries
    const char *lineStart = newLine ;
    const char *headerEnd = strstr(data,headerHints);

    //seek to the first header entries
    newLine = strstr(lineStart,lineHints);
    //parse header entry
    while(newLine != NULL && newLine < headerEnd){
        std::string entryString =  ::trim(lineStart,newLine-lineStart);
        //ALOGD("%s ",entryString.c_str());
        //split key and value
        std::size_t pos = entryString.find(":");

        newLine +=strlen(lineHints);
        lineStart = newLine;
        newLine = strstr(lineStart,lineHints);
        if(pos < 0){
            ALOGW("get a malform header %s ",entryString.c_str()); 
            continue ;
        }
        std::string key = ::trim(entryString.c_str(),pos);
        std::string value = ::trim(entryString.c_str() + pos +1,entryString.size() - pos - 1);
        ALOGD("key : %s value:%s ",key.c_str(),value.c_str());
        ptrHeader->setEntry(key,value);
    }
    return ptrHeader;
}

//add entry
void HttpHeader::setEntry(const std::string key,const std::string value){
    HttpHeaderEntry tmpEntry;
    tmpEntry.mKey = key;
    tmpEntry.mValue = value;
    mEntries.add(tmpEntry.mKey,tmpEntry);
}

void HttpHeader::setEntry(const char *key,const char *format,...){
	char s[MAX_STRING] ={0};
	va_list params;
	va_start(params, format);
	vsnprintf(s, sizeof(s), format, params);
	va_end(params);
    HttpHeaderEntry tmpEntry;
    tmpEntry.mKey = key;
    tmpEntry.mValue = s;
    mEntries.add(tmpEntry.mKey,tmpEntry);
}


void HttpHeader::toString(BufferUtils &buffer){
    //scan header and create buffer
    int idx = -1;
    while(1){
        const HttpHeaderEntry &tmpEntry = mEntries.next(&idx);
        if(tmpEntry == mEntries.getNull()){
            break;
        }
        buffer.append(tmpEntry.mKey.c_str(),tmpEntry.mKey.size());
        buffer.append(":",1);
        buffer.append(tmpEntry.mValue.c_str(),tmpEntry.mValue.size());
        buffer.append("\r\n",2);
    }
    buffer.append("\r\n\r\n",4);
}

//get entry value 
const std::string& HttpHeader::getValues(std::string key){
    int idx = -1;
    const HttpHeaderEntry &tmp = mEntries.get(key,&idx);
    return tmp.mValue;
}

//get entry value 
const std::string& HttpHeader::getValues(const char *key){
    std::string xkey = key;
    return getValues(xkey);
}
