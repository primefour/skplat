#ifndef __SQLITE_TALBE_H__
#define __SQLITE_TALBE_H__
#include"RefBase.h"
#include<string>

class SqliteTable:RefBase{
    public:
        SqliteTable(const char *sql);
        virtual ~SqliteTable();
        int createTable();
        int insertOp(const char *sql);
        int updateOp(const char *sql);
        int deleteOp(const char *sql);
        int queryOp(const char *sql,xCallback cb,void *pArgs);
        int queryDirectOp(const char *sql,dCallback cb,void *pArgs);
        static int tCallback(sqlite3_stmt *pStmt,void *pArg);
    protected:
        SqliteTable(const SqliteTable &st);
        std::string mTableSql;
        sp<SqliteDatabase> mDB;
};

#endif
