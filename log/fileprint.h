/*******************************************************************************
 **      Filename: fileprint.h
 **        Author: crazyhorse                  
 **   Description: ---
 **        Create: 2017-08-11 17:47:37
 ** Last Modified: 2017-08-11 17:47:37
 ******************************************************************************/
#ifndef __SKLOG_FILE_PRINT_H__
#define __SKLOG_FILE_PRINT_H__
#include"logfilectrl.h"


class DefaultFilePrinter:public LogBaseIntf{
    public:
    DefaultFilePrinter();
    void FileEnable(bool enable);
    virtual void SetParameters(const char *dir,const char *prefix);
    virtual void FilePrint(const char *_log);
    virtual void FileFlush();
    virtual void Close();
    virtual void DumpBuffer(const void *buffer,size_t _len);
    private:
        LogFileCtrl mFilePrinter;
        bool mEnable;
};
#endif 

