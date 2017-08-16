/*******************************************************************************
 **      Filename: platform.h
 **        Author: crazyhorse                  
 **   Description: ---
 **        Create: 2017-08-14 16:19:48
 ** Last Modified: 2017-08-14 16:19:48
 ******************************************************************************/
#ifndef __SKPLAT_PLATFORM_H__
#define __SKPLAT_PLATFORM_H__
intmax_t GetPid();
intmax_t GetTid();
intmax_t GetMainTid();
void ConsolePrint(int level,const char *_log);
//wrap of console print
void ConsolePrintf(const char* _tips_format, ...);
int getNetInfo();

#endif

