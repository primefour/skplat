#ifndef __SQLITE_WRAPPER_H__
#define __SQLITE_WRAPPER_H__
#include"sqlite3.h"
#define skinfo printf
#define skerror printf
#define skwarn printf

typedef struct SQLite{
    sqlite3* db;
    char *path;
    int openFlags;
    int dbError;
    volatile int canceled;
    pthread_mutex_t mutex;
}SQLite;

void sqlitew_init();
SQLite* sqlitew_open(const char *dbname,int flags);
void sqlitew_close(SQLite **sqlitedb);
//sql string should be encoded with utf8
sqlite3_stmt* sqlitew_prepare_sql(SQLite *sqlitedb,const char *sql);
/*
** Run a prepared statement
*/
void sqlitew_exec_stmt(void *pArg, sqlite3_stmt *pStmt, /* Statment to run */ 
                    int (*xCallback)(void *pArg,sqlite3_stmt *tStmt));
/*
**Run a sql statement
*/
void sqlitew_exec_sql(SQLite *sqlitedb,const char *sql, void *pArg,
                    int (xCallback)(void *pArg,sqlite3_stmt *tStmt));

#endif //__SQLITE_WRAPPER_H__
