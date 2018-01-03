/*******************************************************************************
 **      Filename: FileUtils.h
 **        Author: crazyhorse                  
 **   Description: ---
 **        Create: 2018-01-03 10:41:22
 ** Last Modified: 2018-01-03 10:41:22
 ******************************************************************************/
#ifndef __FILE_UTILS_H__
#define __FILE_UTILS_H__
#include<string>
class FileUtils {
    public:
        static std::string makeFileNameByOffset(const char *dir,
                const char *prefix,const char *ext,long offset);
        //create a file name with path .prefix ,ext
        //prefix_time.ext
        static std::string makeFileNameByTime(const char *dir,
                const char *prefix,const char *ext);
        //check file whether or not is dir
        static int isDir(const char *path);
        //check file whether or not is regular file
        static int isRegularFile(const char *file);
        //delete files in the path or single file if exist
        static void deleteFiles(const char *path);
        //append the second file after file1
        static void appendFiles(const char *file1,const char *file2); 
        //move the files in the path2 or move single file to path2
        static void moveFiles(const char *path1,const char *path2);
        //create directory if it is not exist and return true
        //if create failed will return false
        static bool makeDir(const char *path);
        //if path is directory will return the last fold name
        //eg: /home/workplace => workplace
        //else return file name with ext
        //eg: /home/hello.txt => hello.txt
        static std::string extractFileName(const char *path);
        //if path is directory will return the last fold name
        //eg: /home/workplace => workplace
        //else return file name with ext
        //eg: /home/hello.txt => hello
        static std::string extractBaseName(const char *path);
        //if path is directory will return empty string
        //else file
        //eg: /home/hello.txt => txt 
        static std::string extractExt(const char *path);
        static std::string getParent(const char *dir);
};
#endif

