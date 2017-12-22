#include"HttpHeader.h"
#include"SimpleHash.h"
#include<string.h>
#include<string>
#include"AppUtils.h"
#include<stdarg.h>

static const char lineHints[] = "\r\n";
template<> HttpHeaderEntry SimpleHash<std::string, HttpHeaderEntry>::mNullItem = HttpHeaderEntry();
#define MAX_STRING       (1024) 
//static function to parser header only
//Buffer is data of header
HttpHeader* HttpHeader::parser(BufferUtils &buffer,HttpHeader *ptrHeader){
    //get size of buffer data 
    int size = buffer.offset(0,SEEK_END);
    //seek to 0
    buffer.offset(0,SEEK_SET);

    //get buffer data
    const char *data = (const char *)buffer.data();

    if(size == 0){
        return NULL;
    }

    //get lines and parse
    const char *newLine = strstr(data,lineHints);
    const char *lineStart = data;

    //parse header entry
    while(newLine != NULL && (newLine - data) < size){
        std::string entryString =  ::trim(lineStart,newLine-lineStart);
        //split key and value
        std::size_t pos = entryString.find(":");

        newLine +=sizeof(lineHints);
        lineStart = newLine;
        newLine = strstr(lineStart,lineHints);
        if(pos < 0){
            ALOGW("get a malform header %s ",entryString.c_str()); 
            continue ;
        }
        std::string key = ::trim(entryString.c_str(),pos);
        std::string value = ::trim(entryString.c_str() + pos +1,entryString.size() - pos - 1);
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
