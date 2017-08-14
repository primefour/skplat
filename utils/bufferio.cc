#include "bufferio.h"
#include <string.h>
#include "__assert.h"

const BufferIO KNullBufferIO(0, 0, 0);

#ifndef max
#define max(a, b) (((a) > (b)) ? (a) : (b))
#endif
#ifndef min
#define min(a, b) (((a) < (b)) ? (a) : (b))
#endif

BufferIO::BufferIO(void* _ptr,size_t _cap)
    : parray_((unsigned char*)_ptr)
    , pos_(0)
    , length_(0)
    , cap_(_cap) {
    ASSERT(length_ <= cap_);
}

BufferIO::BufferIO(void* _ptr, size_t _len, size_t _cap)
    : parray_((unsigned char*)_ptr)
    , pos_(0)
    , length_(_len)
    , cap_ (_cap) {
    ASSERT(length_ <= cap_);
}


BufferIO::BufferIO() {
    Reset();
}

BufferIO::~BufferIO() {

}

void BufferIO::Write(const void* _pBuffer, size_t _nLen) {
    Write(_pBuffer, _nLen, CurrPos());
    Seek(_nLen, kSeekCur);
}

void BufferIO::Write(const void* _pBuffer, size_t _nLen, off_t _nPos) {
    ASSERT(NULL != _pBuffer);
    ASSERT(0 <= _nPos);
    ASSERT((unsigned int)_nPos <= Length());
    size_t copylen = min(_nLen, cap_ - _nPos);
    length_ = max(length_, copylen + _nPos);
    memcpy((unsigned char*)Buffer() + _nPos, _pBuffer, copylen);
}

size_t BufferIO::Read(void* _pBuffer, size_t _nLen) {
    size_t nRead = Read(_pBuffer, _nLen, CurrPos());
    Seek(nRead, kSeekCur);
    return nRead;
}

size_t BufferIO::Read(void* _pBuffer, size_t _nLen, off_t _nPos) const {
    ASSERT(NULL != _pBuffer);
    ASSERT(0 <= _nPos);
    ASSERT((unsigned int)_nPos < Length());

    size_t nRead = Length() - _nPos;
    nRead = min(nRead, _nLen);
    memcpy(_pBuffer, EmptyBuffer(), nRead);
    return nRead;
}


void BufferIO::UpdateCursor(off_t _position,size_t _length){
    ASSERT(0 <= _position);
    ASSERT((size_t)_position <= _length);
    ASSERT(_length <= Cap());

    length_ = cap_ < _length ? cap_ : _length;
    Seek(_position, kSeekStart);
}

void  BufferIO::Seek(off_t _nOffset,  TSeek _eOrigin) {
    switch (_eOrigin) {
    case kSeekStart:
        pos_ = _nOffset;
        break;

    case kSeekCur:
        pos_ += _nOffset;
        break;

    case kSeekEnd:
        pos_ = length_ + _nOffset;
        break;

    default:
        ASSERT(false);
        break;
    }

    if (pos_ < 0)
        pos_ = 0;

    if ((unsigned int)pos_ > length_)
        pos_ = length_;
}

void*  BufferIO::Buffer() {
    return parray_;
}

const void*  BufferIO::Buffer() const {
    return parray_;
}

void* BufferIO::EmptyBuffer() {
    return ((unsigned char*)Buffer()) + CurrPos();
}

const void* BufferIO::EmptyBuffer() const {
    return ((unsigned char*)Buffer()) + CurrPos();
}

off_t BufferIO::CurrPos() const {
    return pos_;
}

size_t BufferIO::Length() const {
    return length_;
}

size_t BufferIO::Cap() const {
    return cap_;
}

void BufferIO::Attach(void* _pBuffer, size_t _nLen, size_t _maxlen) {
    Reset();
    parray_ = (unsigned char*)_pBuffer;
    length_ = _nLen;
    cap_ = _maxlen;
}

void BufferIO::Attach(void* _pBuffer, size_t _nLen) {
    Attach(_pBuffer, _nLen, _nLen);
}

void BufferIO::Reset() {
    parray_ = NULL;
    pos_ = 0;
    length_ = 0;
    cap_ = 0;
}

