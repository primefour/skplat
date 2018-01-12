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
#include <errno.h>
#include "FileUtils.h"
#include <sys/time.h>
#include "Log.h"

static const char *dir_sep = "/";
static const char *cur_dir= ".";
/*
 * return path/prefix_offset.txt
 */
std::string FileUtils::makeFileNameByOffset(const char *dir,const char *prefix,const char *ext,long offset){
    ASSERT(dir != NULL && prefix != NULL && ext != NULL,"Invalidate parameters");
    char filePath[PATH_MAX +1]={0};
    snprintf(filePath,sizeof(filePath) -1,"%s%s%s_%ld.%s",dir,dir_sep,prefix,offset,ext);
    return filePath;
}
//create a file name with path .prefix ,ext
//prefix_time.ext
std::string FileUtils::makeFileNameByTime(const char *dir,const char *prefix,const char *ext){
    ASSERT(dir != NULL && prefix != NULL && ext != NULL,"Invalidate parameters");
    struct timeval t;
    t.tv_sec = t.tv_usec = 0;
    gettimeofday(&t, NULL);
    time_t sec = t.tv_sec;
    tm tcur = *localtime((const time_t*)&sec);
    char filePath[PATH_MAX +1]={0};
    snprintf(filePath,sizeof(filePath) -1,"%s%s%s_%d%02d%02d.%s",
            dir,dir_sep,prefix,1900 + tcur.tm_year, 1 + tcur.tm_mon, tcur.tm_mday,ext);
    return filePath;
}
//check file whether or not is dir
int FileUtils::isDir(const char *path){
    struct stat dir_stat ={0};
    int ret = stat(path,&dir_stat);
    if (ret < 0){
        ALOGE("file or path is not exist or need permission %s",path);
        return false;
    }else{
        if(S_ISDIR(dir_stat.st_mode)){
            return true;
        }else{
            return false;
        }
    }
}
//check file whether or not is regular file
int FileUtils::isRegularFile(const char *file){
    struct stat file_stat ={0};
    int ret = lstat(file,&file_stat);
    if (ret < 0){
        ALOGE("file or path is not exist or need permission %s",file);
        return false;
    }else{
        if(S_ISREG(file_stat.st_mode)){
            return true;
        }else{
            return false;
        }
    }
}
//delete files in the path or single file if exist
void FileUtils::deleteFiles(const char *filePath){
    int ret = 0;
    char fileBuff[PATH_MAX +1] ={0};
    if(isDir(filePath)){
        DIR *dp;  
        struct dirent *entry;  
        struct stat statbuf;  
        if ((dp = opendir(filePath)) == NULL) {  
            return ;  
        }
        std::string file_path; 
        while ((entry = readdir(dp)) != NULL) {  
            if (strcmp(entry->d_name, ".") == 0 ||   
                    strcmp(entry->d_name, "..") == 0 ){
                continue;
            }
            memset(fileBuff,0,sizeof(fileBuff));
            snprintf(fileBuff,sizeof(fileBuff) -1,"%s%s%s",filePath,dir_sep,entry->d_name);
            lstat(fileBuff, &statbuf);  
            if(S_ISREG(statbuf.st_mode)){
                ret = remove(fileBuff);
                if(ret < 0){
                    ALOGE("delete file %s failed ",fileBuff);
                }
            }
        }
        closedir(dp); 
    }else if (isRegularFile(filePath)){
        ret = remove(filePath);
        if(ret < 0){
            ALOGE("delete file %s failed ",filePath);
        }
    }else{
        ALOGW("file or path is not exist or need permission %s ",filePath);
    }
}

//append the second file after file1
void FileUtils::appendFiles(const char *file1,const char *file2){
    ASSERT(file1 != NULL && file2 != NULL,"invalidate parameters");
    //check file2
    struct stat file_stat ={0};
    int ret = lstat(file2,&file_stat);
    if (ret < 0){
        ALOGE("can't get property of file2 %s ",file2);
        return; 
    }
    if(file_stat.st_size == 0){
        ALOGW("file2 %s size is 0",file2);
        return ;
    }

    ret = lstat(file1,&file_stat);
    if (ret < 0){
        ALOGE("can't get property of file1 %s ",file1);
        return; 
    }

    FILE* src_file = fopen(file2, "rb");

    if (NULL == src_file) {
        ALOGE("can't open file2 %s ",file2);
        return ;
    }
    FILE* dest_file = fopen(file1,"ab");
    if (NULL == dest_file) {
        ALOGE("can't open file1 %s ",file1);
        fclose(src_file);
        return;
    }

    fseek(src_file, 0, SEEK_END);
    long src_file_len = ftell(src_file);
    long dst_file_len = ftell(dest_file);
    fseek(src_file, 0, SEEK_SET);
    char buffer[4096] = {0};
    while (true) {
        if (feof(src_file)) {
            break;
        }

        size_t read_ret = fread(buffer, 1, sizeof(buffer), src_file);

        if (read_ret == 0){
            break;
        }

        if (ferror(src_file)) {
            break;
        }

        fwrite(buffer, 1, read_ret, dest_file);

        if (ferror(dest_file)) {
            break;
        }
    }

    if (dst_file_len + src_file_len > ftell(dest_file)) {
        ftruncate(fileno(dest_file), dst_file_len);
        fclose(src_file);
        fclose(dest_file);
        return ;
    }
    fclose(src_file);
    fclose(dest_file);
    return ;

}

std::string FileUtils::getParent(const char *path){
    ASSERT(path != NULL,"Invalidate parameters");
    int size = strlen(path);
    std::string dir = path;
    //we can't create root directory
    if(path[size -1] == dir_sep[0]){ 
        //remove last slash
        dir = dir.substr(0,size-1);
    }

    size_t found= dir.rfind(dir_sep);
    if(found != std::string::npos){
        return dir.substr(0,found);
    }
    return std::string();
}
//create directory if it is not exist and return true
//if create failed will return false
bool FileUtils::makeDir(const char *path){
    ASSERT(path != NULL,"Invalidate path");
    //check directory is all right
    //ASSERT(path[0] == dir_sep[0],"Invalidate path,should use full path");

    ALOGD("mkdir path is %s ",path);
    if(isDir(path)||strcmp(path,cur_dir) == 0){
        return true;
    }

    std::string parent = getParent(path);
    if(parent.empty()){
        return true;
    }

    if(makeDir(parent.c_str())){
        int ret = mkdir(path,0777);
        if(ret < 0 ){
            ALOGE("try to create log dir is %s %s fail",path,strerror(errno));
            return false ;
        }else{
            return true;
        }
    }
}

//move the files in the path2 or move single file to path2
void FileUtils::moveFiles(const char *path1,const char *path2){
    ASSERT(path1 != NULL && path2 != NULL,"Invalidate parameters");
    //the same path
    if(strcmp(path1,path2) == 0){
        return ;
    }

    if(isDir(path1)){
        //path2 is not a directory
        if(!isDir(path2)){
            ALOGW("path is not a directory %s ",path2);
            return;
        }
        DIR *dp;  
        struct dirent *entry;  
        if ((dp = opendir(path1)) == NULL) {  
            ALOGW("path can't open %s ",path1);
            return ;  
        }
        std::string file_path; 
        while ((entry = readdir(dp)) != NULL) {  
            if (strcmp(entry->d_name, ".") == 0 ||   
                    strcmp(entry->d_name, "..") == 0 ){
                continue;
            }

            std::string file_path = path1;
            file_path += dir_sep ;
            file_path += entry->d_name;

            std::string des_file_name = path2;
            des_file_name += dir_sep ;
            des_file_name += entry->d_name;
            int ret = rename(file_path.c_str(),des_file_name.c_str());
            if(ret < 0){
                ALOGW("z rename failed %s %s ",file_path.c_str(),des_file_name.c_str());
            }
        }
        closedir(dp); 
    }else if(isRegularFile(path1)){
        if(isDir(path2)){
            std::string fileName = extractFileName(path1);
            std::string destFile = path1;
            destFile += dir_sep ;
            destFile += fileName;
            int ret = rename(path1,destFile.c_str());
            if(ret < 0){
                ALOGW("x rename failed %s %s ",path1,destFile.c_str());
            }
        }else if(isRegularFile(path2)){
            int ret = rename(path1,path2);
            if(ret < 0){
                ALOGW("y rename failed %s %s ",path1,path2);
            }
        }
    }
}

//if path is directory will return the last fold name
//eg: /home/workplace => workplace
//else return file name with ext
//eg: /home/hello.txt => hello.txt
std::string FileUtils::extractFileName(const char *path){
    if (NULL == path) {
        return "";
    }
    const char *pos = strrchr(path,dir_sep[0]); 

    if (NULL == pos || '\0' == *(pos + 1)) {
        return path;
    } else {
        return pos + 1;
    }
}

//if path is directory will return the last fold name
//eg: /home/workplace => workplace
//else return file name with ext
//eg: /home/hello.txt => hello
std::string FileUtils::extractBaseName(const char *path){
    if (NULL == path) {
        return "";
    }
    const char *pos = strrchr(path,dir_sep[0]); 
    if (NULL == pos || '\0' == *(pos + 1)) {
        return path;
    } else {
        const char *dot= strrchr(path,'.'); 
        if(dot == NULL ||dot < pos){
            return path;
        }else{
            int size = dot - pos;
            return std::string(pos+1,size);
        }
    }
}

//if path is directory will return empty string
//else file
//eg: /home/hello.txt => txt 
std::string FileUtils::extractExt(const char *path){
    if (NULL == path) {
        return "";
    }
    const char *dot= strrchr(path,'.'); 
    if(dot == NULL){
        return "";
    }else{
        return dot +1;
    }
}
