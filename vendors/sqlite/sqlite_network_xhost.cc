#include<stdio.h>
#include<vector>
#include<string>
#include"sqlite_network_xhost.h"
extern "C" {
#include"sqlite3.h"
}

static int xhost_callback(void*p,sqlite3_stmt *pStmt){
    std::vector<NetworkXHost> *ptr_xhost = (std::vector<NetworkXHost>*)p;
    NetworkXHost entry; 
    int i = 0;

    //host name or ip 
    const char *host_ptr = (const char *)sqlite3_column_text(pStmt,i++);
    entry.host = host_ptr;    

    //host listen port 
    entry.port = sqlite3_column_int(pStmt,i++);
    //host type 
    entry.type = sqlite3_column_int(pStmt,i++);

    //add to vector
    skinfo("host %s port %d type %d \n",entry.host.c_str(),entry.port,entry.type);
    ptr_xhost->push_back(entry);
}

int get_xhost_all_hosts(SQLite *sqlite,std::vector<NetworkXHost> &ips){
    char sql_buff[4096]={0};
    snprintf(sql_buff,sizeof(sql_buff),"select (host,port,type) from xhost ;");
    sqlitew_exec_sql(sqlite,sql_buff,&ips,xhost_callback);
    return ips.size();
}

int get_xhost_by_type(SQLite *sqlite,std::vector<NetworkXHost> &ips,int type ){
    char sql_buff[4096]={0};
    snprintf(sql_buff,sizeof(sql_buff),"select (host,port,type) from xhost where type = %d;",type);
    sqlitew_exec_sql(sqlite,sql_buff,&ips,xhost_callback);
    return ips.size();
}


int insert_xhost_host(SQLite *sqlite,const char *host,int port,int type){
    char *zErr = NULL;
    if(type >= HOST_TYPE_MAX || type < 0){
        skwarn("host type is error %d ",type);
        return -1;
    }
    if(host == NULL){
        skwarn("host is error %s ",host);
        return -1;
    }
    char sql_buff[4096] ={0};
    snprintf(sql_buff,sizeof(sql_buff),"insert into xhost(host,port,type)values('%s',%d,%d);", host,port,type);
    int rc = sqlite3_exec(sqlite->db,sql_buff,NULL,NULL, &zErr);
    if(rc != SQLITE_OK){
        skerror("sqlite sql %s %s",sql_buff,zErr);
        sqlite3_free(zErr);
        zErr = NULL;
        return -1;
    }
    return 0;
}


int delete_xhost_by_port(SQLite *sqlite,const char *host,int port){
    if(host == NULL){
        skwarn("host is error %s ",host);
        return -1;
    }
    char *zErr = NULL;
    char sql_buff[4096] ={0};
    snprintf(sql_buff,sizeof(sql_buff),"delete from xhost where host='%s' AND port = %d;", host,port);
    int rc = sqlite3_exec(sqlite->db,sql_buff,NULL,NULL, &zErr);
    if(rc != SQLITE_OK){
        skerror("sqlite sql %s %s",sql_buff,zErr);
        sqlite3_free(zErr);
        zErr = NULL;
        return -1;
    }
    return rc;
}

int delete_xhost_by_host(SQLite *sqlite,const char *host){
    if(host == NULL){
        skwarn("host is error %s ",host);
        return -1;
    }
    char sql_buff[4096] ={0};
    char *zErr = NULL;
    snprintf(sql_buff,sizeof(sql_buff),"delete from xhost where host='%s';", host);
    int rc = sqlite3_exec(sqlite->db,sql_buff,NULL,NULL, &zErr);
    if(rc != SQLITE_OK){
        skerror("sqlite sql %s %s",sql_buff,zErr);
        sqlite3_free(zErr);
        zErr = NULL;
        return -1;
    }
    return rc;
}

