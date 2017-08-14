#ifndef __BUFFER_IO_H__
#define __BUFFER_IO_H__
#include <sys/types.h>
#include <string.h>

class BufferIO {
  public: enum TSeek {
        kSeekStart,
        kSeekCur,
        kSeekEnd,
    };
  public:
    BufferIO(void* _ptr, size_t _len, size_t _cap);
    BufferIO(void* _ptr, size_t _cap);
    BufferIO();
    ~BufferIO();

    template<class T> void Write(const T& _val)
    { Write(&_val, sizeof(_val)); }

    template<class T> void Write(int _nPos, const T& _val)
    { Write(&_val, sizeof(_val), _nPos);}

    void Write(const char* const _val)
    { Write(_val, (unsigned int)strlen(_val));}

    void Write(int _nPos, const char* const _val)
    { Write(_val, (unsigned int)strlen(_val), _nPos);}

    void Write(const void* _pBuffer, size_t _nLen);
    void Write(const void* _pBuffer, size_t _nLen, off_t _nPos);

    template<class T> void Read(T& _val)
    { Read(&_val, sizeof(_val)); }

    template<class T> void Read(int _nPos, const T& _val) const
    { Read(&_val, sizeof(_val), _nPos); }

    size_t Read(void* _pBuffer, size_t _nLen);
    size_t Read(void* _pBuffer, size_t _nLen, off_t _nPos) const;
    void Seek(off_t _nOffset,  TSeek _eOrigin = kSeekCur);

    void* Buffer();
    const void* Buffer() const;

    void* EmptyBuffer();
    const void* EmptyBuffer() const;

    off_t  CurrPos() const;
    size_t Length() const;
    size_t Cap() const;
    void UpdateCursor(off_t _position,size_t _length);

    void Attach(void* _pBuffer, size_t _nLen, size_t _maxlen);
    void Attach(void* _pBuffer, size_t _nLen);
    void Reset();

  private:
    BufferIO(const BufferIO& _rhs);
    BufferIO& operator = (const BufferIO& _rhs);

  private:
    unsigned char* parray_;
    off_t pos_;
    size_t length_;
    size_t cap_;
};

extern const BufferIO KNullBufferIO;
#endif
