/*******************************************************************************
 **      Filename: log/fileutils.h
 **        Author: crazyhorse                  
 **   Description: ---
 **        Create: 2017-03-30 19:28:35
 ** Last Modified: 2017-03-30 19:28:35
 ******************************************************************************/
#ifndef __FILE_UTILS_H__
#define __FILE_UTILS_H__
#include <string>

void make_logfilename(const timeval& _tv, const std::string& _logdir, 
        const char* _prefix, const std::string& _fileext, 
        char* _filepath, unsigned int _len);
int check_dir(const char *dir);
int check_regular(const char *file);
void del_files(const std::string& _forder_path);
void del_timeout_file(const std::string& _log_path); 
bool append_file(const std::string& _src_file, const std::string& _dst_file); 
void move_old_files(const std::string& _src_path, const std::string& _dest_path,const std::string& _nameprefix);
bool create_dir(const std::string&dir);
void extract_function_name(const char* _func, char* _func_ret, int _len);
const char* extract_file_name(const char* _path);
#define LOG_EXT "log"
#define LOG_PREFIX "sklog"
#endif
