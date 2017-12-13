#include<string.h>
#include"SqliteTable.h"
#include"SqliteWrapper.h"

//refer to db and the sql to create table
SqliteTable::SqliteTable(sp<SqliteWrapper> db,const char *sql)
                                    :mTableSql(sql),mDB(db){
}

SqliteTable::~SqliteTable(){

}

int SqliteTable::createTable(){
    int rc = mDB->execSql(mTableSql.c_str(),(vCallback)NULL,NULL);
    if(rc != OK){
        ALOGW("create table fail %s ",mTableSql.c_str());
    }
    return rc;
}

int SqliteTable::insertOp(const char *sql){
    int rc = mDB->execSql(sql,(vCallback)NULL,NULL);
    if(rc != OK){
        ALOGW("insert value fail %s ",sql);
    }
    return rc;
}

int SqliteTable::updateOp(const char *sql){
    int rc = mDB->execSql(sql,(vCallback)NULL,NULL);
    if(rc != OK){
        ALOGW("update value fail %s ",sql);
    }
    return rc;
}

int SqliteTable::deleteOp(const char *sql){
    int rc = mDB->execSql(sql,(vCallback)NULL,NULL);
    if(rc != OK){
        ALOGW("delete value fail %s ",sql);
    }
    return rc;
}

int SqliteTable::queryOp(const char *sql,vCallback cb,void *pArgs){
    int rc = mDB->execSql(sql,cb,pArgs);
    if(rc != OK){
        ALOGW("delete value fail %s ",sql);
    }
    return rc;
}

int SqliteTable::queryDirectOp(const char *sql,xCallback cb,void *pArgs){
    int rc = mDB->execSql(sql,cb,pArgs);
    if(rc != OK){
        ALOGW("delete value fail %s ",sql);
    }
    return rc;
}