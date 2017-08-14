#include"logintf.h"
#include"logbase.h"
#include<string>

void sklog_open(const char* _dir, const char* _nameprefix){
    if(_dir != NULL && _nameprefix != NULL){
        log_file_init(_dir,_nameprefix);
    }
}

void sklog_open_with_cache(TSklogMode _mode, const std::string& _cachedir, const std::string& _logdir, const char* _nameprefix){
    if(_logdir.empty() || _nameprefix == NULL) {
        return ; 
    }
    log_file_init(_logdir.c_str(),_nameprefix);
}

void sklog_flush(){
    log_flush();
}

void sklog_set_level(int level){
    log_set_level((LogLevel)level);
}

void sklog_flush_sync(){
    log_flush();
}

void sklog_close(){
    log_flush();
    log_file_close();
}

void sklog_setmode(TSklogMode _mode){

}

void sklog_set_console(bool _is_open){
    log_set_console_enable(_is_open);
}
