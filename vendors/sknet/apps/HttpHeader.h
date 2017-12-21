/*******************************************************************************
 **      Filename: apps/HttpHeader.h
 **        Author: crazyhorse                  
 **   Description: ---
 **        Create: 2017-12-20 18:09:04
 ** Last Modified: 2017-12-20 18:09:04
 ******************************************************************************/
// ReadMIMEHeader reads a MIME-style header from r.
// The header is a sequence of possibly continued Key: Value lines
// ending in a blank line.
// response header
// The returned map m maps CanonicalMIMEHeaderKey(key) to a
//　　HTTP/1.0 200 OK 
//　　Date:Mon,31Dec200104:25:57GMT 
//　　Server:Apache/1.3.14(Unix) 
//　　Content-type:text/html 
//　　Last-modified:Tue,17Apr200106:46:28GMT 
//　　Etag:"a030f020ac7c01:1e9f" 
//　　Content-length:39725426 
//　　Content-range:bytes554554-40279979/40279980 /
//
///requese header
//　　GET http://download.microtool.de:80/somedata.exe 
//　　Host: download.microtool.de 
//　　Accept:*/* 
//　　Pragma: no-cache 
//　　Cache-Control: no-cache 
//　　Referer: http://download.microtool.de/ 
//　　User-Agent:Mozilla/4.04[en](Win95;I;Nav) 
//　　Range:bytes=554554- 
// sequence of values in the same order encountered in the input.
// For example, consider this input:
//	My-Key: Value 1
//	Long-Key: Even
//	       Longer Value
//	My-Key: Value 2
//
//setEntry("User-Agent","SOHUWapRebot");
//setEntry("Accept-Language","zh-cn,zh;q=0.5");
//setEntry("Accept-Charset","GBK,utf-8;q=0.7,*;q=0.7");
//setEntr("Connection","keep-alive");
//
//    const char *HttpHeaderEntries[] ={
//        "Accept",
//        "Accept-Charset",
//        "Accept-Encoding",
//        "Accept-Language",
//        "Accept-Ranges",
//        "Cache-Control",
//        "Cc",
//        "Connection",
//        "Content-Id",
//        "Content-Language",
//        "Content-Length",
//        "Content-Transfer-Encoding",
//        "Content-Type",
//        "Cookie",
//        "Date",
//        "Dkim-Signature",
//        "Etag",
//        "Expires",
//        "From",
//        "Host",
//        "If-Modified-Since",
//        "If-None-Match",
//        "In-Reply-To",
//        "Last-Modified",
//        "Location",
//        "Message-Id",
//        "Mime-Version",
//        "Pragma",
//        "Received",
//        "Return-Path",
//        "Server",
//        "Set-Cookie",
//        "Subject",
//        "To",
//        "User-Agent",
//        "Via",
//        "X-Forwarded-For",
//        "X-Imforwards",
//        "X-Powered-By",
//    }; 
//
//http header entry
struct HttpHeaderEntry{
    std::string mKey;
    const std::string getKey() const{
        return mKey;
    }
    std::string mValue;
};

struct HttpHeader{
        //response
        static HttpHeader& parser(BufferUtils &buffer,HttpHeader& header);
        //get entry value 
        const std::string& getValues(const std::string key);
        //get entry value 
        const std::string& getValues(const char *key);

        //create request header
        const char *toString();
        //create request header
        char *toString(char *buff);

        //add entry
        void setEntry(const std::string key,const std::value);
        void setEntry(const char *value,const char *format,...);

        //entries key=>value
        BasicHashtable<std::string,HttpHeaderEntry> mEntries;
};

