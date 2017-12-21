
struct HttpResponse{
    public:
        // Location returns the URL of the response's "Location" header,
        // if present. Relative redirects are resolved relative to
        // the Response's Request. ErrNoLocation is returned if no
        // Location header is present.
        int reLocation(Url & url){
            std::string reloc = mHeader.Get("Location");
            if (reloc.isempty() ){
                return BAD_VALUE; 
            }
            if(mRequest != NULL){
                return mRequest.parseReloc(reloc);
            }

            return Url.Parse(reloc);
        }

    private:
        HttpRequest *mRequest;
        HttpHeader mHeader;

        std::string mStatus   // e.g. "200 OK"
        int mStatusCode       // e.g. 200
        std::string mProto    // e.g. "HTTP/1.0"
        int ProtoMajor        // e.g. 1
        int ProtoMinor        // e.g. 0

        // ContentLength records the length of the associated content. The
        // value -1 indicates that the length is unknown. Unless Request.Method
        // is "HEAD", values >= 0 indicate that the given number of bytes may
        // be read from Body.
        long mContentLength ; 

        // Contains transfer encodings from outer-most to inner-most. Value is
        // nil, means that "identity" encoding is used.
        std::string mTransferEncoding;

        // Close records whether the header directed that the connection be
        // closed after reading Body. The value is advice for clients: neither
        // ReadResponse nor Response.Write ever closes a connection.
        bool mClose; 

        // Uncompressed reports whether the response was sent compressed but
        // was decompressed by the http package. When true, reading from
        // Body yields the uncompressed content instead of the compressed
        // content actually set from the server, ContentLength is set to -1,
        // and the "Content-Length" and "Content-Encoding" fields are deleted
        // from the responseHeader. To get the original response from
        // the server, set Transport.DisableCompression to true.
        bool mUncompressed;

};
