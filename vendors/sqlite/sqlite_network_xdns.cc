#include<stdio.h>
#include<stdlib.h>
#include<assert.h>
#include<string>
#include<vector>
#include"sqlite3.h"
#include"sqlite_wrapper.h"
#include"sqlite_network_xdns.h"

int insert_host_ip(SQLite *sqlite,network_dns *entry){
    char sql_buff[4096]={0};

    if(entry == NULL|| entry->ip.empty()){
        return -1;
    }

    if(entry->host.empty()){
        entry->host = entry->ip;
    }

    snprintf(sql_buff,sizeof(sql_buff),"insert into xdns(host,ip,dns_server,ip_type,dns_type,fail_times,conn_profile) values (\'%s\',\'%s\',\'%s\',%d,%d,%d,%ld);",
            entry->host.c_str(), entry->ip.c_str(),
            (!entry->dns_server.empty()?entry->dns_server.c_str():"NULL"),
            entry->ip_type >=0 ?entry->ip_type:IPTYPE_V4,
            entry->dns_type >=0 ?entry->dns_type:DNSTYPE_RAW,
            entry->fail_times >=0 ?entry->fail_times:0,
            entry->conn_profile >=0 ?entry->conn_profile:MAX_CONN_PROFILE);

    char *zErr = NULL;
    int rc = sqlite3_exec(sqlite->db,sql_buff,NULL,NULL, &zErr);
    if(rc != SQLITE_OK){
        skerror("sqlite sql %s %s",sql_buff,zErr);
        sqlite3_free(zErr);
        zErr = NULL;
        return -1;
    }
    return 0;
}

//(host text,ip text,ip_type int,dns_type int,dns_server text,count_down int);";
int dns_host_exist(SQLite *sqlite,const char *host,const char *ip){
    char sql_buff[4096]={0};
    sqlite3_stmt *pStmt = NULL;
    if(host == NULL || ip == NULL){
        skwarn("input value is invalidate");
        return -1;
    }

    snprintf(sql_buff,sizeof(sql_buff),"select count(*) from xdns where host = '%s' AND ip = '%s' ;",host,ip);
    int rc = sqlite3_prepare_v2(sqlite->db,sql_buff, -1, &pStmt,NULL);

    if(rc!=SQLITE_OK || pStmt == NULL){
        skerror("prepare sql %s  fail ",sql_buff);
        return -1;
    }
    rc = sqlite3_step(pStmt);
    int count = 0;

    if(rc != SQLITE_ROW){
        skerror("execute sql  %s fail ",sql_buff);
    }else{
        count = sqlite3_column_int(pStmt,0);
    }
    sqlite3_finalize(pStmt);
    return count;
}


static int dns_callback(void*p,sqlite3_stmt *pStmt){
    if(p == NULL){
        return -1;
    }

    std::vector<network_dns> *ptr_dns = (std::vector<network_dns>*)p;
    network_dns entry; 
    int i = 0;
    //host text
    entry.host = std::string((const char *)sqlite3_column_text(pStmt,i++));
    //ip text
    entry.ip = std::string((const char *)sqlite3_column_text(pStmt,i++));
    //dns_server text
    entry.dns_server = std::string((const char *)sqlite3_column_text(pStmt,i++));
    //ip_type int
    entry.ip_type = sqlite3_column_int(pStmt,i++);
    //dns_type int
    entry.dns_type = sqlite3_column_int(pStmt,i++);
    //fail times int
    entry.fail_times = sqlite3_column_int(pStmt,i++);
    //conn profile int64_t
    entry.conn_profile = sqlite3_column_int64(pStmt,i++);
    //add to vector
    ptr_dns->push_back(entry);
    return 1;
}


int get_hosts_by_ip_type(SQLite *sqlite,const char *ip,std::vector<network_dns> &ips){
    char sql_buff[4096]={0};
    snprintf(sql_buff,sizeof(sql_buff),"select host,ip,dns_server,ip_type,dns_type,"
                                "fail_times,conn_profile from xdns where ip = '%s' order by conn_profile;",ip);

    int rc = sqlitew_exec_sql(sqlite,sql_buff,&ips,dns_callback);
    if(rc != SQLITE_OK){
        skerror("query fail %s ",sql_buff);
        return -1;
    }else{
        return ips.size();
    }
}

int get_ips_by_host(SQLite *sqlite,const char *host,std::vector<network_dns> &ips){

    char sql_buff[4096]={0};
    snprintf(sql_buff,sizeof(sql_buff),"select host,ip,dns_server,ip_type,dns_type,"
                                    "fail_times,conn_profile from xdns where host = '%s' order by conn_profile;",host);

    int rc = sqlitew_exec_sql(sqlite,sql_buff,&ips,dns_callback);
    if(rc != SQLITE_OK){
        skerror("query fail %s ",sql_buff);
        return -1;
    }else{
        return ips.size();
    }

}

int get_all_hosts(SQLite *sqlite,std::vector<network_dns> &ips){

    char sql_buff[4096]={0};
    snprintf(sql_buff,sizeof(sql_buff),"select host,ip,dns_server,ip_type,dns_type,"
                                    "fail_times,conn_profile from xdns order by conn_profile;"); 

    int rc = sqlitew_exec_sql(sqlite,sql_buff,&ips,dns_callback);
    if(rc != SQLITE_OK){
        skerror("query fail %s ",sql_buff);
        return -1;
    }else{
        return ips.size();
    }

}

int update_dns_usetimes(SQLite *sqlite,const char *host,const char *ip){
    char sql_buff[4096] ={0};
    if(ip == NULL){
        return -1;
    }

    if(host == NULL){
        host = ip;
    }
    snprintf(sql_buff,sizeof(sql_buff),"update xdns set use_times = use_times +1 where host = '%s' and ip = '%s' ;",host,ip);
    char *zErr = NULL;
    int rc = sqlite3_exec(sqlite->db,sql_buff,NULL,NULL, &zErr);
    if(rc != SQLITE_OK){
        skerror("update %s fail %s ",sql_buff,zErr);
        sqlite3_free(zErr);
        zErr = NULL;
        return -1;
    }
    return 0;
}


int update_dns_connprofile(SQLite *sqlite,const char *host,const char *ip,int64_t conn_profile){
    char sql_buff[4096] ={0};

    if(ip == NULL){
        return -1;
    }

    if(host == NULL){
        host = ip;
    }

    snprintf(sql_buff,sizeof(sql_buff),"update xdns set conn_profile = %ld where host = '%s' and ip = '%s' ;",conn_profile,host,ip);
    char *zErr = NULL;
    int rc = sqlite3_exec(sqlite->db,sql_buff,NULL,NULL, &zErr);
    if(rc != SQLITE_OK){
        skerror("update %s fail %s ",sql_buff,zErr);
        sqlite3_free(zErr);
        zErr = NULL;
        return -1;
    }
    return 0;
}


int update_dns_failtimes(SQLite *sqlite,const char *host,const char *ip,int fail_times){
    char sql_buff[4096] ={0};
    if(ip == NULL){
        return -1;
    }

    if(host == NULL){
        host = ip;
    }

    snprintf(sql_buff,sizeof(sql_buff),"update xdns set fail_times = %d where host = '%s' and ip = '%s' ;",fail_times,host,ip);
    char *zErr = NULL;
    int rc = sqlite3_exec(sqlite->db,sql_buff,NULL,NULL, &zErr);
    if(rc != SQLITE_OK){
        skerror("update %s fail %s ",sql_buff,zErr);
        sqlite3_free(zErr);
        zErr = NULL;
        return -1;
    }
    return 0;
}

int update_dns_ip(SQLite *sqlite,const char *host,const char *ip,const char *new_ip){
    char sql_buff[4096] ={0};
    if(host == NULL || ip == NULL || new_ip == NULL){
        return -1;
    }
    snprintf(sql_buff,sizeof(sql_buff),"update xdns set ip = '%s'where host = '%s' and ip = '%s' ;",new_ip,host,ip);
    char* zErr = NULL;
    int rc = sqlite3_exec(sqlite->db,sql_buff,NULL,NULL, &zErr);
    if(rc != SQLITE_OK){
        skerror("update %s fail %s ",sql_buff,zErr);
        sqlite3_free(zErr);
        zErr = NULL;
    }
    return 0;

}

int delete_dns_entries(SQLite *sqlite, network_dns *entries,const char *host,const char *ip){
    char sql_buff[4096] ={0};

    if(ip == NULL){
        return -1;
    }

    if(host == NULL){
        host = ip;
    }

    snprintf(sql_buff,sizeof(sql_buff),"delete from xdns where host = %s and ip = %s ;", host, ip);
    char* zErr = NULL;
    int rc = sqlite3_exec(sqlite->db,sql_buff,NULL,NULL, &zErr);
    if(rc != SQLITE_OK){
        skerror("sqlite sql %s %s",sql_buff,zErr);
        sqlite3_free(zErr);
        zErr = NULL;
        return -1;
    }
    return 0;
}

int delete_dns_by_success(SQLite *sqlite,float percent){
    char sql_buff[4096] ={0};

    if(percent < 0 ){
        return -1;
    }

    snprintf(sql_buff,sizeof(sql_buff),"delete from xdns where fail_times/use_time < %f ;",percent);
    char* zErr = NULL;
    int rc = sqlite3_exec(sqlite->db,sql_buff,NULL,NULL, &zErr);
    if(rc != SQLITE_OK){
        skerror("sqlite sql %s %s",sql_buff,zErr);
        sqlite3_free(zErr);
        zErr = NULL;
        return -1;
    }
    return 0;
}

int delete_dns_by_connprofile(SQLite *sqlite, int64_t limit){
    char sql_buff[4096] ={0};

    if(limit < 0 ){
        return -1;
    }

    snprintf(sql_buff,sizeof(sql_buff),"delete from xdns where conn_profile > %ld;",limit);

    char* zErr = NULL;
    int rc = sqlite3_exec(sqlite->db,sql_buff,NULL,NULL, &zErr);
    if(rc != SQLITE_OK){
        skerror("sqlite sql %s %s",sql_buff,zErr);
        sqlite3_free(zErr);
        zErr = NULL;
        return -1;
    }
    return 0;
}

