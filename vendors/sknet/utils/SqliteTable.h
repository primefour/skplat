#ifndef __SQLITE_TALBE_H__
#define __SQLITE_TALBE_H__
#include"RefBase.h"
#include<string>

typedef int(*dCallback)(void *columnName,void *columnValues,void *pArgs);
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
    protected:
        SqliteTable(const SqliteTable &st);
        std::string mTableSql;
        sp<SqliteDatabase> mDB;
};

#endif
