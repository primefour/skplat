#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include "logbase.h"
#include "compile.h"
#include <assert.h>
#include "__assert.h"
#include "fileutils.h"
#include "fileprint.h"
#include "bufferio.h"
#include "platform.h"


static LogBaseIntf *Printer = NULL;
static bool LogBaseEnable= true;
static bool LogBaseConsole= true;
static LogLevel LogBaseLevel=kLevelVerbose ; 
static std::string LogBaseDir="/sdcard/sklog/";
static std::string LogBasePrefix ="sklog";
BOOT_RUN_EXIT(log_file_close);


void log_formater(const LogInfo* _info, const char* _logbody, BufferIO& _log) {
    static const char* levelStrings[] = {
        "V",  //vorbase
        "D",  // debug
        "I",  // info
        "W",  // warn
        "E",  // error
        "F"   // fatal
    };

    assert((unsigned int)_log.CurrPos() == _log.Length());
    static int error_count = 0;
    static int error_size = 0;
    if (_log.Cap() <= _log.Length() + MAX_LINE_LEN) {
        ++error_count;
        error_size = (int)strnlen(_logbody,MAX_LINE_LEN);
        if (_log.Cap() >= _log.Length() + 128) {
            char err_buff[128]={0};
            snprintf(err_buff,sizeof(err_buff), "[F]format log buff size <= 2048, err(%d, %d)\n", error_count, error_size);  // **CPPLINT SKIP**
            _log.Write(err_buff,strlen(err_buff));
            error_count = 0;
            error_size = 0;
        }
        assert(false);
        return;
    }

    if (NULL != _info) {
        const char* filename = extract_file_name(_info->filename);
        char strFuncName [128] = {0};
        extract_function_name(_info->func_name, strFuncName, sizeof(strFuncName));
        char temp_time[64] = {0};
        if (0 != _info->timeval.tv_sec) {
            time_t sec = _info->timeval.tv_sec;
            tm tm = *localtime((const time_t*)&sec);
            snprintf(temp_time, sizeof(temp_time), "%d-%02d-%02d %+.1f %02d:%02d:%02d.%.3ld", 1900 + tm.tm_year, 1 + tm.tm_mon, tm.tm_mday,
                     tm.tm_gmtoff / 3600.0, tm.tm_hour, tm.tm_min, tm.tm_sec, _info->timeval.tv_usec / 1000);
        }

        int ret = snprintf((char*)_log.EmptyBuffer(), 1024, "[%s][%s][%d %d][%s][%s,%s,%d] ",
                           _logbody ? levelStrings[_info->level] : levelStrings[kLevelFatal], temp_time,
                           _info->pid, _info->tid,_info->tag? _info->tag:"sklog",
                           filename, strFuncName,_info->line);

        assert(0 <= ret);
        _log.UpdateCursor(_log.CurrPos() + ret, _log.Length() + ret);
        assert((unsigned int)_log.CurrPos() == _log.Length());
    }

    if (NULL != _logbody) {
        int bodylen = strnlen(_logbody,MAX_LINE_LEN);
        _log.Write(_logbody, bodylen);
    } else {
        _log.Write("error!! NULL==_logbody");
    }
    char nextline = '\n';
    if (*((char*)_log.EmptyBuffer() - 1) != nextline){
        _log.Write(&nextline, 1);
    }
}


LogLevel log_get_level() {
    return LogBaseLevel;
}

int log_for(LogLevel _level){
    if(LogBaseEnable){
        return LogBaseLevel <= _level;
    }else{
        return 0;
    }
}

void log_set_level(LogLevel _level){
    LogBaseLevel = _level;
}

void log_set_enable(bool enable){
    LogBaseEnable = enable;
}

void log_set_console_enable(bool enable){
    LogBaseConsole = enable;
}

void log_file_init(const char* _dir, const char* _nameprefix){
    if(_dir != NULL){
        LogBaseDir = std::string(_dir);
    }
    if(_nameprefix != NULL){
        LogBasePrefix = std::string(_nameprefix);
    }
    Printer->SetParameters(LogBaseDir.c_str(),LogBasePrefix.c_str());
}

void log_flush(){
    if(Printer == NULL){
        return;
    }
    Printer->FileFlush();
}

void log_file_close(){
    if(Printer != NULL){
        Printer->FileFlush();
        Printer->Close();
    }
}

void log_write(const LogInfo * _info, const char* _log) {
    char temp[4*1024] = {0}; 
    if (NULL == _log) {
        if (_info) {
            LogInfo* info = (LogInfo*)_info;
            info->level = kLevelFatal;
        }
    }

    if(_info != NULL){
        if (-1==_info->pid && -1==_info->tid && -1==_info->maintid){
            LogInfo* info = (LogInfo*)_info;
            info->pid = GetPid();
            info->tid = GetTid();
            info->maintid = GetMainTid();
        }
        BufferIO log_buff(temp, 0, sizeof(temp));
        log_formater(_info, _log, log_buff);
    }

    if(LogBaseConsole){
        if(_info != NULL){
            ConsolePrint(_info->level,temp);
        }else{
            ConsolePrintf(_log);
        }
    }

    if (NULL == _log) {
        Printer->FilePrint("NULL == _log");
    } else {
        if(_info != NULL){
            Printer->FilePrint(temp);
        }else{
            Printer->FilePrint(_log);
        }
    }
}

void log_vprint(const LogInfo* _info, const char* _format, va_list _list){
    if (NULL == _format) {
        LogInfo* info = (LogInfo*)_info;
        info->level = kLevelFatal;
        log_write(_info, "NULL == _format");
    } else {
        char temp[4096] = {'\0'};
        vsnprintf(temp, 4096, _format, _list);
        log_write(_info, temp);
    }
}


void log_print(const LogInfo* _info, const char* _format, ...) {
	va_list valist;
	va_start(valist, _format);
    log_vprint(_info, _format, valist);
	va_end(valist);
}

void log_assertp(const LogInfo* _info, const char* _expression, const char* _format, ...){
	va_list valist;
	va_start(valist, _format);
    __ASSERTV2(_info->filename, _info->line, _info->func_name, _expression, _format,valist);
	va_end(valist);
}

void log_assert(const LogInfo* _info, const char* _expression, const char* _log){
    __ASSERT2(_info->filename, _info->line, _info->func_name, _expression,"%s",_log);
}
