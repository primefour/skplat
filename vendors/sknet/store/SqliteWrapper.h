#ifndef __SQLITE_WRAPPER_H__
#define __SQLITE_WRAPPER_H__
#include<string>
#include"Mutex.h"
#include"RefBase.h"
#include"KeyedHash.h"
extern "C" {
#include"sqlite3.h"
}

struct ColumnEntry{
    std::string Name;
    union{
        const char *charValue;
        double floatValue;
        long longValue;
    }Value;
    ColumnEntry(){
        memset(&Value,0,sizeof(Value));
        Type = 0;
        Length = 0;
    }
    int Type;
    int Length;
    inline const std::string& getKey() const{
        return Name;
    }
    inline long getLong() const{
        return Value.longValue;
    }
    inline double getFloat() const{
        return Value.floatValue;
    }
    inline const char *getString() const{
        if(Value.charValue == NULL){
            ALOGW("key %s value is %s",Name.c_str(),Value.charValue);
            return NULL;
        }else{
            if(strcmp(Value.charValue,"NULL")== 0){
                ALOGW("key %s value is %s",Name.c_str(),Value.charValue);
                return NULL;
            }else{
                return Value.charValue;
            }
        }
    }
    inline int size()const{
        return Length;
    }
    bool operator==(const ColumnEntry& ce) const{
        if(this->Type != ce.Type){
            return false;
        }
        return memcmp(&Value,&ce.Value,sizeof(Value));
    }
};

typedef int (*xCallback)(sqlite3_stmt *tStmt,void *pArg);

typedef int (*vCallback)(KeyedHash<std::string,ColumnEntry> *colEntries,void *pArgs);


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
        int execSql(const char *sql,vCallback cb,void *arg); 
        //run stmt and call xcallback with the result 
        //if xcallback return < 0 this function will abort or 
        //run until get SQLITE_DONE
        int execStmt(void *arg,sqlite3_stmt *stmt,xCallback cb);
        int execStmt(void *arg,sqlite3_stmt *stmt,vCallback cb);
        //compile sql and generate sqlite_stmt
        sqlite3_stmt* compileSQL(const char *sql);
        inline sqlite3_stmt* bindValue(sqlite3_stmt* pStmt,int idx,const void*data,size_t size){
            sqlite3_bind_blob(pStmt,idx,data,size,SQLITE_TRANSIENT);
            return pStmt;
        }
        
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
        //count xcallback 
        static int countCallback(sqlite3_stmt *tStmt,void *pArg);
        //cancel current sql query
        inline int cancel(){ mCanceled = 1; }
        static bool mVerboseLog;
        inline void errorInc(){ mDatabaseError ++; }

        int createTable(const char *sql);
        int insert(const char *sql);
        int update(const char *sql);
        int xdelete(const char *sql);
        int query(const char *sql,vCallback cb,void *pArgs);
        int query(const char *sql,xCallback cb,void *pArgs);
        int count(const char *sql);
        void ErrorMsg();
        int finalize(sqlite3_stmt* pStmt);
    private:
        //parse data from sqlite query 
        void getRowData(sqlite3_stmt *pStmt,int nCol,KeyedHash<std::string,ColumnEntry> *colEntries);
        //void getRowData(sqlite3_stmt *pStmt,int nCol,ColumnEntry *colEntries);
        SqliteWrapper(const SqliteWrapper &sw);
        sqlite3 *mDatabase; //instance of database
        std::string mDatabasePath; //database path
        int mOpenFlags; //flag using when open database 
        int mDatabaseError; //Is there any error
        volatile int mCanceled; //cancel query
        Mutex mMutex; //mutex for execute sql
};

#endif
