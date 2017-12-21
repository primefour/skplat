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


//http header entry
struct HttpHeaderEntry{
    std::string mKey;
    const std::string getKey() const{
        return mKey;
    }
    std::string mValue;
};

static const char lineHints[] = "\r\n";

class HttpHeader{
    public:
        //default constructor
        HttpHeader(){
            mLength = 0;
            mVersion = "1.1";
            mStateCode = "200";
        }

        //default constructor
        HttpHeader(std::string url){
            mLength = 0;
            mVersion = "1.1";
            mStateCode = "200";
            mUrl = url;
        }

        //response
        static HttpHeader& parser(BufferUtils &buffer,HttpHeader& header);
        const std::string& getValues(const std::string key);
        std::string status();
        std::string version();
        long contentLength();
        std::string relocation();
        //request 
        const char *toString();
        char *toString(char *buff);
        void addEntry(const std::string key,const std::value);
        void addEntry(const char *value,const char *format,...);
        void addHost(const char *host);
        void addHost(const std::string host);
        void setVersion(const char *ver);
        void setContentLength(const char *ltn);

    private:
        BasicHashtable<std::string,HttpHeaderEntry> mEntries;
        std::string mVersion;
        std::string mStateCode;
        std::string mUrl;
        long mLength;
};

static HttpHeader& parser(BufferUtils &buffer,HttpHeader& header){
    int size = buffer.offset(0,SEEK_END);
    buffer.offset(0,SEEK_SET);
    const char *data = (const char *)buffer.data();
    if(size == 0){
        return header;
    }
    const char *newLine = strstr(data,lineHints);
    const char *lineStart = data;
    //parse statue line 
    std::string statusString =  ::trim(lineStart,newLine-lineStart);
    std::size_t pos = str.find("HTTP/");

    //parse header entry
    const char *lineStart = data;
    while(newLine != NULL && (newLine - data) < size){
        std::string entryString =  ::trim(lineStart,newLine-lineStart);
        std::size_t pos = str.find(":");
        if(pos < 0){
            newLine +=sizeof(lineHints);
            lineStart = newLine;
            newLine = strstr(lineStart,lineHints);
            ALOGW("get a imform header %s ",entryString.c_str()); 
            continue ;
        }
        std::string key = ::trim(entryString.c_str(),pos);
        std::string value = ::trim(entryString.c_str() + pos +1,entryString.size() - pos - 1);
        header->addEntry(key,value);
    }
    return header;
}

/*
Parser() (MIMEHeader, error) {
	// Avoid lots of small slice allocations later by allocating one
	// large one ahead of time which we'll cut up into smaller
	// slices. If this isn't big enough later, we allocate small ones.
	var strs []string
	hint := r.upcomingHeaderNewlines()
	if hint > 0 {
		strs = make([]string, hint)
	}

	m := make(MIMEHeader, hint)
	for {
		kv, err := r.readContinuedLineSlice()
		if len(kv) == 0 {
			return m, err
		}

		// Key ends at first colon; should not have spaces but
		// they appear in the wild, violating specs, so we
		// remove them if present.
		i := bytes.IndexByte(kv, ':')
		if i < 0 {
			return m, ProtocolError("malformed MIME header line: " + string(kv))
		}
		endKey := i
		for endKey > 0 && kv[endKey-1] == ' ' {
			endKey--
		}
		key := canonicalMIMEHeaderKey(kv[:endKey])

		// As per RFC 7230 field-name is a token, tokens consist of one or more chars.
		// We could return a ProtocolError here, but better to be liberal in what we
		// accept, so if we get an empty key, skip it.
		if key == "" {
			continue
		}

		// Skip initial spaces in value.
		i++ // skip colon
		for i < len(kv) && (kv[i] == ' ' || kv[i] == '\t') {
			i++
		}
		value := string(kv[i:])

		vv := m[key]
		if vv == nil && len(strs) > 0 {
			// More than likely this will be a single-element key.
			// Most headers aren't multi-valued.
			// Set the capacity on strs[0] to 1, so any future append
			// won't extend the slice into the other strings.
			vv, strs = strs[:1:1], strs[1:]
			vv[0] = value
			m[key] = vv
		} else {
			m[key] = append(vv, value)
		}

		if err != nil {
			return m, err
		}
	}
}

// upcomingHeaderNewlines returns an approximation of the number of newlines
// that will be in this header. If it gets confused, it returns 0.
func (r *Reader) upcomingHeaderNewlines() (n int) {
	// Try to determine the 'hint' size.
	r.R.Peek(1) // force a buffer load if empty
	s := r.R.Buffered()
	if s == 0 {
		return
	}
	peek, _ := r.R.Peek(s)
	for len(peek) > 0 {
		i := bytes.IndexByte(peek, '\n')
		if i < 3 {
			// Not present (-1) or found within the next few bytes,
			// implying we're at the end ("\r\n\r\n" or "\n\n")
			return
		}
		n++
		peek = peek[i+1:]
	}
	return
}

class HttpHeader{
    enum{
        ACCEPT,
        ACCEPT_CHARSET,
        ACCEPT_ENCODING,
        ACCEPT_LANGUAGE,
        ACCEPT_RANGES,
        CACHE_CONTROL,
        CC,
        CONNECTION,
        CONTENT_ID,
        CONTENT_LANGUAGE,
        CONTENT_LENGTH,
        CONTENT_TRANSFER_ENCODING,
        CONTENT_TYPE,
        COOKIE,
        DATE,
        DKIM_SIGNATURE,
        ETAG,
        EXPIRES,
        FROM,
        HOST,
        IF_MODIFIED_SINCE,
        IF_NONE_MATCH,
        IN_REPLY_TO,
        LAST_MODIFIED,
        LOCATION,
        MESSAGE_ID,
        MIME_VERSION,
        PRAGMA,
        RECEIVED,
        RETURN_PATH,
        SERVER,
        SET_COOKIE,
        SUBJECT,
        TO,
        USER_AGENT,
        VIA,
        X_FORWARDED_FOR,
        X_IMFORWARDS,
        X_POWERED_BY,
    };
    const char *HttpHeaderEntries[] ={
        "Accept",
        "Accept-Charset",
        "Accept-Encoding",
        "Accept-Language",
        "Accept-Ranges",
        "Cache-Control",
        "Cc",
        "Connection",
        "Content-Id",
        "Content-Language",
        "Content-Length",
        "Content-Transfer-Encoding",
        "Content-Type",
        "Cookie",
        "Date",
        "Dkim-Signature",
        "Etag",
        "Expires",
        "From",
        "Host",
        "If-Modified-Since",
        "If-None-Match",
        "In-Reply-To",
        "Last-Modified",
        "Location",
        "Message-Id",
        "Mime-Version",
        "Pragma",
        "Received",
        "Return-Path",
        "Server",
        "Set-Cookie",
        "Subject",
        "To",
        "User-Agent",
        "Via",
        "X-Forwarded-For",
        "X-Imforwards",
        "X-Powered-By",
    }; 

};

httpClient4如何实现

httpClient4使用http-mime.jar包的MultipartEntity实现，代码如下（为了简洁，处理了异常处理代码）：
                           
HttpPost httpPost = newHttpPost(url);
Log.debug("post url:"+url);
httpPost.setHeader("User-Agent","SOHUWapRebot");
httpPost.setHeader("Accept-Language","zh-cn,zh;q=0.5");
httpPost.setHeader("Accept-Charset","GBK,utf-8;q=0.7,*;q=0.7");
httpPost.setHeader("Connection","keep-alive");
 
MultipartEntity mutiEntity = newMultipartEntity();
File file = new File("d:/photo.jpg");
mutiEntity.addPart("desc",new StringBody("美丽的西双版纳", Charset.forName("utf-8")));
mutiEntity.addPart("pic", newFileBody(file));
 
 
httpPost.setEntity(mutiEntity);
HttpResponse  httpResponse = httpClient.execute(httpPost);
HttpEntity httpEntity =  httpResponse.getEntity();
String content = EntityUtils.toString(httpEntity);

void http_addheader(http_t *conn, char *format, ...) {
	char s[MAX_STRING];
	va_list params;

	va_start(params, format);
	vsnprintf(s, sizeof(s) - 3, format, params);
	strcat(s, "\r\n");
	va_end(params);

	strncat(conn->request, s, MAX_QUERY - strlen(conn->request) - 1);
}

void
http_get(http_t *conn, char *lurl)
{
	*conn->request = 0;
	if (conn->proxy) {
		const char *proto = scheme_from_proto(conn->proto);
		http_addheader(conn, "GET %s%s%s HTTP/1.0", proto, conn->host,
			       lurl);
	} else {
		http_addheader(conn, "GET %s HTTP/1.0", lurl);
		if ((conn->proto == PROTO_HTTP &&
		     conn->port != PROTO_HTTP_PORT) ||
		    (conn->proto == PROTO_HTTPS &&
		     conn->port != PROTO_HTTPS_PORT))
			http_addheader(conn, "Host: %s:%i", conn->host,
				       conn->port);
		else
			http_addheader(conn, "Host: %s", conn->host);
	}
	if (*conn->auth)
		http_addheader(conn, "Authorization: Basic %s", conn->auth);
	if (*conn->proxy_auth)
		http_addheader(conn, "Proxy-Authorization: Basic %s",
			       conn->proxy_auth);
	http_addheader(conn, "Accept: */*");
	if (conn->firstbyte) {
		if (conn->lastbyte)
			http_addheader(conn, "Range: bytes=%lld-%lld",
				       conn->firstbyte, conn->lastbyte);
		else
			http_addheader(conn, "Range: bytes=%lld-",
				       conn->firstbyte);
	}
}

var (
	tr = &http.Transport{
		TLSClientConfig: &tls.Config{InsecureSkipVerify: true},
	}
	client = &http.Client{Transport: tr}
)

var (
	acceptRangeHeader   = "Accept-Ranges"
	contentLengthHeader = "Content-Length"
)

type HttpDownloader struct {
	url       string
	file      string
	par       int64
	len       int64
	ips       []string
	skipTls   bool
	parts     []Part
	resumable bool
}

func NewHttpDownloader(url string, par int, skipTls bool) *HttpDownloader {
	var resumable = true

	parsed, err := stdurl.Parse(url)
	FatalCheck(err)

	ips, err := net.LookupIP(parsed.Host)
	FatalCheck(err)

	ipstr := FilterIPV4(ips)
	Printf("Resolve ip: %s\n", strings.Join(ipstr, " | "))

	req, err := http.NewRequest("GET", url, nil)
	FatalCheck(err)

	resp, err := client.Do(req)
	FatalCheck(err)

	if resp.Header.Get(acceptRangeHeader) == "" {
		Printf("Target url is not supported range download, fallback to parallel 1\n")
		//fallback to par = 1
		par = 1
	}

	//get download range
	clen := resp.Header.Get(contentLengthHeader)
	if clen == "" {
		Printf("Target url not contain Content-Length header, fallback to parallel 1\n")
		clen = "1" //set 1 because of progress bar not accept 0 length
		par = 1
		resumable = false
	}

	Printf("Start download with %d connections \n", par)

	len, err := strconv.ParseInt(clen, 10, 64)
	FatalCheck(err)

	sizeInMb := float64(len) / (1024 * 1024)

	if clen == "1" {
		Printf("Download size: not specified\n")
	} else if sizeInMb < 1024 {
		Printf("Download target size: %.1f MB\n", sizeInMb)
	} else {
		Printf("Download target size: %.1f GB\n", sizeInMb/1024)
	}

	file := filepath.Base(url)
	ret := new(HttpDownloader)
	ret.url = url
	ret.file = file
	ret.par = int64(par)
	ret.len = len
	ret.ips = ipstr
	ret.skipTls = skipTls
	ret.parts = partCalculate(int64(par), len, url)
	ret.resumable = resumable
