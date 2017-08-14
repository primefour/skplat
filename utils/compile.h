/*******************************************************************************
 **      Filename: compile.h
 **        Author: crazyhorse                  
 **   Description: ---
 **        Create: 2017-08-08 19:14:45
 ** Last Modified: 2017-08-08 19:14:45
 ******************************************************************************/
#ifndef _UTILS_COMPILE_H_
#define _UTILS_COMPILE_H_
#include <stdlib.h>

#if defined(__GNUC__)
#define WEAK_FUNC     __attribute__((weak))
#elif defined(_MSC_VER) && !defined(_LIB)
#define WEAK_FUNC __declspec(selectany)
#else
#define WEAK_FUNC
#endif

#if defined(__GNUC__)
#define EXPORT_FUNC __attribute__ ((visibility ("default")))
#elif defined(_MSC_VER)
#define EXPORT_FUNC __declspec(dllexport)
#else
#error "export"
#endif

#ifndef VARIABLE_IS_NOT_USED
#ifdef __GNUC__
#define VARIABLE_IS_NOT_USED __attribute__ ((unused))
#else
#define VARIABLE_IS_NOT_USED
#endif
#endif

#ifdef __cplusplus
extern "C" {
#endif

__inline int boot_run_atstartup(void (*func)(void)) { func(); return 0;}
__inline int boot_run_atexit(void (*func)(void)) { return atexit(func);}

#ifdef __cplusplus
}
#endif

#define BOOT_RUN_STARTUP(func) VARIABLE_IS_NOT_USED static int __anonymous_run_variable_startup_##func = boot_run_atstartup(func)
#define BOOT_RUN_EXIT(func) VARIABLE_IS_NOT_USED static int __anonymous_run_variable_exit_##func = boot_run_atexit(func)

#include <vector>
template <typename T>
std::vector<T>& BootRegister_Container() {
    static std::vector<T> s_register;
    return s_register;
}

template <typename T>
bool BootRegister_Add(const T& _data) {
    BootRegister_Container<T>().push_back(_data);
    return true;
}
#define BOOT_REGISTER(data) BOOT_REGISTER_IMPL_I(data, __LINE__)
#define BOOT_REGISTER_IMPL_I(data, line) BOOT_REGISTER_IMPL_II(data, line)
#define BOOT_REGISTER_IMPL_II(data, line) VARIABLE_IS_NOT_USED static bool __int_anonymous_##line = BootRegister_Add(data)
#define BOOT_REGISTER_CHECK(name, data) VARIABLE_IS_NOT_USED bool __test_##name##_check = BootRegister_Add(data)
#endif //_UTILS_COMPILE_H_
