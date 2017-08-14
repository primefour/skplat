/*******************************************************************************
 **      Filename: log/fileutils.c
 **        Author: crazyhorse                  
 **   Description: ---
 **        Create: 2017-03-30 19:26:04
 ** Last Modified: 2017-03-30 19:26:04
 ******************************************************************************/
#include <unistd.h>
#include <sys/types.h>
#include <fcntl.h>
#include <stdio.h>
#include <inttypes.h>
#include <sys/mount.h>
#include <ctype.h>
#include <assert.h>
#include <unistd.h>
#include <zlib.h>
#include <string>
#include <algorithm>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>  
#include <string.h>  
#include <sys/stat.h>  
#include <stdlib.h>  
#include "strutils.h"
#include "fileutils.h"
#include "platform.h"

void make_logfilename(const timeval& _tv, const std::string& _logdir, 
        const char* _prefix, const std::string& _fileext, 
        char* _filepath, unsigned int _len) {
    time_t sec = _tv.tv_sec;
    tm tcur = *localtime((const time_t*)&sec);

    std::string logfilepath = _logdir;
    logfilepath += "/";
    logfilepath += _prefix;
    char temp [64] = {0};
    snprintf(temp, 64, "_%d%02d%02d", 1900 + tcur.tm_year, 1 + tcur.tm_mon, tcur.tm_mday);
    logfilepath += temp;
    logfilepath += ".";
    logfilepath += _fileext;
    strncpy(_filepath, logfilepath.c_str(), _len - 1);
    _filepath[_len - 1] = '\0';
}


int check_dir(const char *dir){
    struct stat dir_stat ={0};
    int ret = stat(dir,&dir_stat);
    if (ret < 0){
        ConsolePrintf("check log dir %s fail",dir);
        return -1;
    }else{
        if(S_ISDIR(dir_stat.st_mode)){
            //ConsolePrintf("check log dir %s sucessfully",dir);
            return 0;
        }else{
            ConsolePrintf("check log dir %s fail",dir);
            return -1;
        }
    }
}

int check_regular(const char *file){
    struct stat file_stat ={0};
    int ret = lstat(file,&file_stat);
    if (ret < 0){
        return -1;
    }else{
        if(S_ISREG(file_stat.st_mode)){
            return 0;
        }else{
            return -1;
        }
    }
}
 
void del_files(const std::string& _forder_path) {
    if(_forder_path.empty() || !check_dir(_forder_path.c_str())){
        return ;
    }
    DIR *dp;  
    struct dirent *entry;  
    struct stat statbuf;  
    if ((dp = opendir(_forder_path.c_str())) == NULL) {  
        return ;  
    }
    std::string file_path; 

    while ((entry = readdir(dp)) != NULL) {  
        if (strcmp(entry->d_name, ".") == 0 ||   
                strcmp(entry->d_name, "..") == 0 ){
            continue;
        }
        std::string file_path = _forder_path + "/" + entry->d_name;
        lstat(file_path.c_str(), &statbuf);  
        if(S_ISREG(statbuf.st_mode)){
            remove(file_path.c_str());
        }
    }
    closedir(dp); 
}

//3days
#define kMaxLogAliveTime (60 * 60 * 24 *3)

void del_timeout_file(const std::string& _log_path) {
    time_t now_time = time(NULL);
    if(_log_path.empty() || !check_dir(_log_path.c_str())){
        return ;
    }
    DIR *dp;
    struct dirent *entry; 
    struct stat statbuf; 
    if ((dp = opendir(_log_path.c_str())) == NULL) {  
        return ;  
    }
    std::string file_path; 

    while ((entry = readdir(dp)) != NULL) {  
        if (strcmp(entry->d_name, ".") == 0 ||   
                strcmp(entry->d_name, "..") == 0 ){
            continue;
        }
        std::string file_path = _log_path + entry->d_name;
        lstat(file_path.c_str(), &statbuf);  
        time_t fileModifyTime = statbuf.st_mtime;
        if(S_ISREG(statbuf.st_mode)){
            if (now_time > fileModifyTime && now_time - fileModifyTime > kMaxLogAliveTime) {
                remove(file_path.c_str());
            }
        }else if (S_ISDIR(statbuf.st_mode)) {  
            del_files(file_path);
        }

    }
    closedir(dp);     
}

bool append_file(const std::string& _src_file, const std::string& _dst_file) {
    if (_src_file == _dst_file) {
        return false;
    }

    struct stat file_stat ={0};
    int ret = lstat(_src_file.c_str(),&file_stat);
    if (ret < 0){
        return false; 
    }


    if(file_stat.st_size == 0){
        return true;
    }

    FILE* src_file = fopen(_src_file.c_str(), "rb");

    if (NULL == src_file) {
        return false;
    }

    FILE* dest_file = fopen(_dst_file.c_str(), "ab");

    if (NULL == dest_file) {
        fclose(src_file);
        return false;
    }

    fseek(src_file, 0, SEEK_END);
    long src_file_len = ftell(src_file);
    long dst_file_len = ftell(dest_file);
    fseek(src_file, 0, SEEK_SET);

    char buffer[4096] = {0};

    while (true) {
        if (feof(src_file)) break;

        size_t read_ret = fread(buffer, 1, sizeof(buffer), src_file);

        if (read_ret == 0)   break;

        if (ferror(src_file)) break;

        fwrite(buffer, 1, read_ret, dest_file);

        if (ferror(dest_file))  break;
    }

    if (dst_file_len + src_file_len > ftell(dest_file)) {
        ftruncate(fileno(dest_file), dst_file_len);
        fclose(src_file);
        fclose(dest_file);
        return false;
    }

    fclose(src_file);
    fclose(dest_file);

    return true;
}

const char dir_sep = '/';

static std::string get_parent(const std::string& dir){
    size_t found= dir.rfind(dir_sep);
    if(found != std::string::npos){
        return dir.substr(0,found);
    }
    return std::string();
}


bool create_dir(const std::string&dir){
    if(dir.empty()){
        return false;
    }

    int ret = check_dir(dir.c_str());
    //check directory is all right
    if(ret == 0){
        return true;
    }
    //no parent error
    int size = dir.size();
    if(ret < 0 || errno == ENOENT){
        std::string parent = dir;
        //we can't create root directory
        if(dir[size -1] == dir_sep){ 
            //remove last slash
            parent = dir.substr(0,size-1);
        }
        parent = get_parent(parent);
        create_dir(parent);
        ConsolePrintf("try to create log dir is %s ",dir.c_str());
        ret = mkdir(dir.c_str(),0777);
        if(ret < 0 ){
            ConsolePrintf("try to create log dir is %s %s fail",dir.c_str(),strerror(errno));
            return false ;
        }else{
            return true;
        }
    }else{
        return false;
    }
}



void move_old_files(const std::string& _src_path, const std::string& _dest_path, const std::string& _nameprefix) {
    if (_src_path == _dest_path) {
        return;
    }

    if(_src_path.empty()|| !check_dir(_src_path.c_str())){
        return ;
    }

    DIR *dp;  
    struct dirent *entry;  
    if ((dp = opendir(_src_path.c_str())) == NULL) {  
        return ;  
    }
    std::string file_path; 
    while ((entry = readdir(dp)) != NULL) {  
        if (strcmp(entry->d_name, ".") == 0 ||   
                strcmp(entry->d_name, "..") == 0 ){
            continue;
        }

        if (!strutil::StartsWith(entry->d_name, _nameprefix) || 
                !strutil::EndsWith(entry->d_name, LOG_EXT)) {
            continue;
        }

        std::string file_path = _src_path + entry->d_name;
        std::string des_file_name = _dest_path + "/" + entry->d_name;
        if (!append_file(file_path, des_file_name)) {
            break;
        }
        remove(file_path.c_str());
    }
    closedir(dp); 
}

const char* extract_file_name(const char* _path) {
    if (NULL == _path) {
        return "";
    }
    const char* pos = strrchr(_path, '\\');
    if (NULL == pos) {
        pos = strrchr(_path, '/');
    }
    if (NULL == pos || '\0' == *(pos + 1)) {
        return _path;
    } else {
        return pos + 1;
    }
}


void extract_function_name(const char* _func, char* _func_ret, int _len) {
    if (NULL == _func){
        return;
    }
    
    const char* start = _func;
    const char* end = NULL;
    const char* pos = _func;
    
    while ('\0' != *pos) {
        if (NULL == end && ' ' == *pos) {
            start = ++pos;
            continue;
        }
        
        if (':' == *pos && ':' == *(pos+1)) {
            pos += 2;
            start = pos;
            continue;
        }
        
        if ('(' == *pos) {
            end = pos;
        } else if (NULL != start && (':' == *pos || ']' == *pos)) {
            end = pos;
            break;
        }
        ++pos;
    }
    
    
    if (NULL == start || NULL == end || start + 1 >= end) {
        strncpy(_func_ret, _func, _len);
        _func_ret[_len - 1] = '\0';
        return;
    }
    
    ptrdiff_t len = end - start;
    --_len;
    len = _len < len ? _len : len;
    memcpy(_func_ret, start, len);
    _func_ret[len] = '\0';
}
