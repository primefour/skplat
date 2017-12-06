#ifndef __SQLITE_WRAPPER_H__
#define __SQLITE_WRAPPER_H__
#include<string>
#include"Mutex.h"
#include"RefBase.h"
extern "C" {
#include"sqlite3.h"
}

typedef int (*xCallback)(sqlite3_stmt *tStmt,void *pArg);

typedef int (*dCallback)(void *columnName,void *columnValues,void *pArgs);

struct ColumnEntry{
    const char *Name;
    union{
        const char *charValue;
        double floatValue;
        long longValue;
    }Value;
    int Type;
    int Length;
}

class SqliteWrapper:public RefBase{
    public:
        //construct
        SqliteWrapper(std::string path);
        //desconstruction
        virtual ~SqliteWrapper();
        //open and initialize database
        int open(int flags = -1);
        //release
        void dispose();
        //run sql and call xcallback with the result 
        //if xcallback return < 0 this function will abort or 
        //run until get SQLITE_DONE
        int execSql(const char *sql,xCallback cb,void *arg); 
        int execSql(const char *sql,dCallback cb,void *arg); 

        //called each time a statement begin to execution.when traceing is enable 
        static void traceCallback(void *data,const char *sql);
        //called each time a statement finishes execution.when profiling is enable
        static void profileCallback(void *data,const char *sql,sqlite3_uint64 tm);
        // Called each time a message is logged.
        static void logCallback(void* data, int err_code, const char* msg); 
        //sqlite error string msg
        static void errString(int errCode, const char*sqliteMsg,std::string &errMsg);
        //collection compare function
        static int collLocalized( void *not_used, int nKey1, const void *pKey1, int nKey2, const void *pKey2);
        //cancel current sql query
        inline int cancel(){ mCanceled = 1; }
        static bool mVerboseLog;
        inline void errorInc(){ mDatabaseError ++; }
    private:
        //run stmt and call xcallback with the result 
        //if xcallback return < 0 this function will abort or 
        //run until get SQLITE_DONE
        int execStmt(void *arg,sqlite3_stmt *stmt,xCallback cb);
        int execStmt(void *arg,sqlite3_stmt *stmt,dCallback cb);
        //compile sql and generate sqlite_stmt
        sqlite3_stmt* compileSQL(const char *sql);

        SqliteWrapper(const SqliteWrapper &sw);
        sqlite3 *mDatabase; //instance of database
        std::string mDatabasePath; //database path
        int mOpenFlags; //flag using when open database 
        int mDatabaseError; //Is there any error
        volatile int mCanceled; //cancel query
        Mutex mMutex; //mutex for execute sql
};

#endif
