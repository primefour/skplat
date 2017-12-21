#include"HttpHeader.h"
#include"SimpleHash.h"

static const char lineHints[] = "\r\n";

struct HttpHeader{
        //get entry value 
        const std::string& getValues(const std::string key);
        //get entry value 
        const std::string& getValues(const char *key);

        //create request header
        void toString(BufferUtils &buffer);

        //add entry
        void setEntry(const std::string key,const std::value);

        void setEntry(const char *value,const char *format,...);

        //entries key=>value
        SimpleHash<std::string,HttpHeaderEntry> mEntries;
};

//static function to parser header only
//Buffer is data of header
static HttpHeader* HttpHeader::parser(BufferUtils &buffer,HttpHeader *ptrHeader){
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
        std::size_t pos = str.find(":");

        newLine +=sizeof(lineHints);
        lineStart = newLine;
        newLine = strstr(lineStart,lineHints);
        if(pos < 0){
            ALOGW("get a imform header %s ",entryString.c_str()); 
            continue ;
        }
        std::string key = ::trim(entryString.c_str(),pos);
        std::string value = ::trim(entryString.c_str() + pos +1,entryString.size() - pos - 1);
        ptrHeader->setEntry(key,value);
    }
    return ptrHeader;
}

//add entry
void HttpHeader::setEntry(const std::string key,const std::value){
    HttpHeaderEntry tmpEntry;
    tmpEntry->mKey = key;
    tmpEntry->mValue = value;
    mEntries.add(key,tmpEntry);
}


#define MAX_STRING       (1024) 

void HttpHeader::setEntry(const char *key,const char *format,...){
	char s[MAX_STRING] ={0};
	va_list params;
	va_start(params, format);
	vsnprintf(s, sizeof(s), format, params);
	va_end(params);
    HttpHeaderEntry tmpEntry;
    tmpEntry->mKey = key;
    tmpEntry->mValue = s;
    mEntries.add(key,tmpEntry);
}


void HttpHeader::toString(BufferUtils &buffer){
    //scan header and create buffer
    int idx = -1;
    while(1){
        const HttpHeaderEntry &tmpEntry = mEnties.next(&idx);
        if(tmpEntry == mEnties.getNull()){
            break;
        }
        buffer.append(tmpEntry.mkey.c_str(),tmpEntry.mkey.size());
        buffer.append(":",1);
        buffer.append(tmpEntry.mValue.c_str(),tmpEntry.mValue.size());
        buffer.append("\r\n",2);
    }
}

//get entry value 
const std::string& getValues(const std::string key){
    int idx = -1;
    return mEntries.get(key,&idx);
}

//get entry value 
const std::string& getValues(const char *key){
    std::string xkey = key;
    return getValues(xkey);
}
