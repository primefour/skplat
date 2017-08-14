#ifndef _SK_STRUTIL_H_
#define _SK_STRUTIL_H_

#include <string>
#include <vector>

// declaration
namespace strutil {
/////////////////////// string /////////////////////////////
std::string& URLEncode(const std::string& url, std::string& encodeUrl);

std::string& TrimLeft(std::string& str);
std::string& TrimRight(std::string& str);
std::string& Trim(std::string& str);

std::string& ToLower(std::string& str);
std::string& ToUpper(std::string& str);

bool StartsWith(const std::string& str, const std::string& substr);
bool EndsWith(const std::string& str, const std::string& substr);
bool EqualsIgnoreCase(const std::string& str1, const std::string& str2);

std::vector<std::string>& SplitToken(const std::string& str,
                                     const std::string& delimiters, std::vector<std::string>& ss);

// T1 is iterator, T2 is string or wstring
template<typename T1, typename T2> bool MergeToken(const T1& begin, const T1& end, const T2& delimiter, T2& result);
/////////////////////// wstring /////////////////////////////
std::wstring& TrimLeft(std::wstring& str);
std::wstring& TrimRight(std::wstring& str);
std::wstring& Trim(std::wstring& str);

bool StartsWith(const std::wstring& str, const std::wstring& substr);
bool EndsWith(const std::wstring& str, const std::wstring& substr);
bool EqualsIgnoreCase(const std::wstring& str1, const std::wstring& str2);

std::wstring& ToLower(std::wstring& str);
std::wstring& ToUpper(std::wstring& str);

std::vector<std::wstring>& SplitToken(const std::wstring& str,
                                      const std::wstring& delimiters, std::vector<std::wstring>& ss);

// Tokenizer class
inline std::string default_delimiters(std::string) {return " \t\n\r";}
inline std::wstring default_delimiters(std::wstring) {return L" \t\n\r";}

template<class T>
class Tokenizer {
  public:
    Tokenizer(const T& str, const T& delimiters = default_delimiters(T()))
        : offset_(0), string_(str),  delimiters_(delimiters) {}

    void Reset() {offset_ = 0;}
    const T GetToken() const {return token_;}
    bool NextToken() {return NextToken(delimiters_);}
    bool NextToken(const T& delimiters) {
        // find the start charater of the next token.
        typename T::size_type i = string_.find_first_not_of(delimiters, offset_);

        if (i == T::npos) {
            offset_ = string_.length();
            return false;
        }

        // find the end of the token.
        typename T::size_type j = string_.find_first_of(delimiters, i);

        if (j == T::npos) {
            token_ = string_.substr(i, string_.length() - i);
            offset_ = string_.length();
            return true;
        }

        // to intercept the token and save current position
        token_ = string_.substr(i, j - i);
        offset_ = j;
        return true;
    }

  private:
    Tokenizer(const Tokenizer&);
    Tokenizer& operator=(const Tokenizer&);

  protected:
    typename T::size_type offset_;

    const T string_;
    T token_;
    T delimiters_;
};

template<typename T1, typename T2> bool MergeToken(const T1& begin, const T1& end, const T2& delimiter, T2& result) {
    if (begin == end) {
        return false;
    }

    if (delimiter.empty()) {
        return false;
    }

    result.clear();

    for (T1 iter = begin; iter != end; ++iter) {
        result += *iter;

        if (iter + 1 != end) {
            result += delimiter;
        }
    }

    return true;
}

std::string Hex2Str(const char* _str, unsigned int _len);
std::string Str2Hex(const char* _str, unsigned int _len);

unsigned int Str2UInt(const std::string& _str);
std::string ReplaceChar(const char* const input_str, char be_replaced='@', char replace_with='.');
}
#endif	//_SK_STRUTIL_H_
