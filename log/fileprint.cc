/*******************************************************************************
 **      Filename: fileprint.cc
 **        Author: crazyhorse                  
 **   Description: ---
 **        Create: 2017-08-11 17:47:35
 ** Last Modified: 2017-08-11 17:47:35
 ******************************************************************************/
#include <assert.h>
#include "logfilectrl.h"
#include "fileutils.h"
#include "compile.h"
#include "logbase.h"
#include "fileprint.h"

DefaultFilePrinter::DefaultFilePrinter(){
    mEnable = true;
}

void DefaultFilePrinter::Close(){
    if (!mEnable) {
        return;
    }
    mEnable = false;
    mFilePrinter.CloseLogFile();
}

void DefaultFilePrinter::FileEnable(bool enable){
    mEnable = enable;
}



void DefaultFilePrinter::SetParameters(const char *dir,const char *prefix){
	assert(dir);
	assert(prefix);
    mEnable = true;
    //delete too old log file
	del_timeout_file(dir);
    mFilePrinter.SetLogDir(dir);
}

void DefaultFilePrinter::FilePrint(const char *_log){
    if(!mEnable){
        return;
    }
    mFilePrinter.WriteLog(_log);
}

void DefaultFilePrinter::FileFlush(){
    if (!mEnable) {
        return;
    }
    mFilePrinter.Flush();
}

void DefaultFilePrinter::DumpBuffer(const void *buffer,size_t _len){

}
