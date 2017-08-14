#ifndef __ASSERT_H__
#define __ASSERT_H__

#include <sys/cdefs.h>
#include <stdarg.h>

# if (!__ISO_C_VISIBLE >= 1999)
#error "C Version < C99"
# endif

//__BEGIN_DECLS
void ENABLE_ASSERT();
void DISABLE_ASSERT();
int IS_ASSERT_ENABLE();

#ifdef __GNUC__
__attribute__((__nonnull__(1, 3, 4)))
#endif
void __ASSERT(const char*, int, const char*, const char*);

#ifdef __GNUC__
__attribute__((__nonnull__(1, 3, 4, 5))) __attribute__((__format__(printf, 5, 6)))
#endif
void __ASSERT2(const char*, int, const char*, const char*, const char*, ...);

#ifdef __GNUC__
__attribute__((__nonnull__(1, 3, 4, 5))) __attribute__((__format__(printf, 5, 0)))
#endif
void __ASSERTV2(const char*, int, const char*, const char*, const char*, va_list);

//__END_DECLS

#define    ASSERT(e)                 ((e) ? (void)0 : __ASSERT(__FILE__, __LINE__, __func__, #e))
#define    ASSERT2(e, fmt, ...)     ((e) ? (void)0 : __ASSERT2(__FILE__, __LINE__, __func__, #e, fmt, ##__VA_ARGS__))
#define    ASSERTV2(e, fmt, valist) ((e) ? (void)0 : __ASSERTV2(__FILE__, __LINE__, __func__, #e, fmt, valist))

#endif
