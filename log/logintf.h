#ifndef __SKLOG_LOG_INTF_H_
#define __SKLOG_LOG_INTF_H_
#include <string>
#include <vector>
enum TSklogMode {
    kSklogAsync,
    kSklogSync,
};

void sklog_open(const char* _dir, const char* _nameprefix);
void sklog_open_with_cache(TSklogMode _mode, const std::string& _cachedir, const std::string& _logdir, const char* _nameprefix);
void sklog_flush();
void sklog_flush_sync();
void sklog_close();
void sklog_setmode(TSklogMode _mode);
bool sklog_get_current_log_path(char* _log_path, unsigned int _len);
bool sklog_get_current_log_cache_path(char* _logPath, unsigned int _len);
void sklog_set_console(bool _is_open);

#endif
