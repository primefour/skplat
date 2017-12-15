#include"NetworkDatabase.h"
#include"SocketAddress.h"
#include"SqliteWrapper.h"
#include"Vector.h"
#include"KeyedHash.h"
#include"RefBase.h"
#include"Mutex.h"
#include<stdlib.h>
#include<stdio.h>
#include<fcntl.h>
#include<string.h>
#include<string>
#include"Log.h"


//mutex for single instance
static Mutex gDatabaseMutex;
//database store path
static const char *gNetDBPath = "./NetworkDB.db";
//init static member of network database
//sp<NetworkDatabase> NetworkDatabase::mNetworkDatabase = NULL;


sp<NetworkDatabase>& NetworkDatabase::getInstance(){
    AutoMutex _l(gDatabaseMutex);

        ALOGE("%s  %d ",__func__,__LINE__);
        new NetworkDatabase();
        ALOGE("%s  %d ",__func__,__LINE__);
    /*
    if(mNetworkDatabase == NULL){
        ALOGE("%s  %d ",__func__,__LINE__);
        //NetworkDatabase *tmp = ;
        new NetworkDatabase();
        ALOGE("%s  %d ",__func__,__LINE__);
        //tmp->incStrong(NULL);
        ALOGE("%s  %d ",__func__,__LINE__);
        //mNetworkDatabase  = tmp;
    }
    */
    //return mNetworkDatabase;
    //return NULL;
}

sp<SqliteWrapper>&NetworkDatabase:: getDBWrapper(){
    //return NULL;
    /*
    if(mNetworkDatabase == NULL){
        getInstance();
    }
    return getInstance()->getDB();
    */
}


void NetworkDatabase::createTables(){
    //create network database tables
    //don't change items order of each create sql language
    const char* xDnsSQL= "create table if not exists xdns (host text,ip text,ip_type int default 0,"
                                "fetch_type int default 0,conn_profile int64 default -1);";
    const char *xTaskSQL= "create table if not exists xtask (task_id text,module_name text,url text,method int,send_data blob,task_type int,"
                                "send_only int,recv_file text,send_file text,retry_times int,conn_timeout int64,task_timeout int64,"
                                "task_state int default 0,start_time int64 default 0,conn_time int64 default 0,try_times int default 0);";
    mDBWrapper->createTable(xDnsSQL);
    mDBWrapper->createTable(xTaskSQL);
}

NetworkDatabase::NetworkDatabase(){
    ALOGE("xxxx mDBWrapper  %s %d ",__func__,__LINE__);
    mDBWrapper = new SqliteWrapper(gNetDBPath);
    ALOGE("eeee mDBWrapper  %s %d ",__func__,__LINE__);
    //open database
    mDBWrapper->open();
    //create table
    createTables();
}


NetworkDatabase::~NetworkDatabase(){

}

//insert socket address to database
int NetworkDatabase::xDnsInsert(SocketAddress &sa){
    char sql_buff[1024]={0};
    snprintf(sql_buff,sizeof(sql_buff),
            "insert into xdns(host,ip,ip_type,fetch_type,conn_profile) values (\'%s\',\'%s\',%d,%d,%ld);",
            sa.getHostName().c_str(),sa.getIp().c_str(),sa.getType(),sa.getFetchType(),sa.getConnProf());
    int ret = mDBWrapper->insert(sql_buff);
    return ret;
}

//check host and ip whether is exist or not
int NetworkDatabase::xDnsExist(SocketAddress &sa){
    char sql_buff[1024]={0};
    snprintf(sql_buff,sizeof(sql_buff),
            "select count(*) from xdns where host = '%s' AND ip = '%s' ;",
            sa.getHostName().c_str(),sa.getIp().c_str());
    int count = mDBWrapper->count(sql_buff);
    return count > 0?count:0;
}

/*
 *(host text,ip text,ip_type int default 0,"
 *"fetch_type int default 0,conn_profile int64 default -1);";
 */
int NetworkDatabase::xDnsVCallback(KeyedHash<std::string,ColumnEntry> *colEntries,void *pArgs){
    static std::string hostKey("host");
    static std::string ipKey("ip");
    static std::string ipTypeKey("ip_type");
    static std::string fetchTypeKey("fetch_type");
    static std::string profKey("conn_profile");
    if(colEntries  == NULL || pArgs == NULL){
        ALOGW("%s arguments is NULL",__func__);
        return -1;
    }
    //get array of socket address
    Vector<SocketAddress> *saArray  = (Vector<SocketAddress> *)pArgs ;
    //create socket address entry
    SocketAddress sa(colEntries->get(hostKey).getString(),colEntries->get(ipKey).getString());
    sa.setFetchType(colEntries->get(fetchTypeKey).getLong());
    sa.setConnProf(colEntries->get(profKey).getLong());
    //insert to vector
    saArray->push(sa);
    return 1;
}

int NetworkDatabase::getAddrByHost(const char *host,Vector<SocketAddress> &ips){
    char sql_buff[1024]={0};
    snprintf(sql_buff,sizeof(sql_buff),"select host,ip,ip_type,fetch_type,"
                                    "conn_profile from xdns where host = '%s' order by conn_profile;",host);
    int rc = mDBWrapper->execSql(sql_buff,xDnsVCallback,&ips);
    if(rc != OK){
        ALOGE("query fail %s ",sql_buff);
        return UNKNOWN_ERROR;
    }else{
        return ips.size();
    }

}

int NetworkDatabase::xDnsUpdateProf(const char *host,const char *ip,int64_t conn_profile){
    char sql_buff[1024] ={0};
    if(ip == NULL){
        return -1;
    }

    if(host == NULL){
        host = ip;
    }

    snprintf(sql_buff,sizeof(sql_buff),
            "update xdns set conn_profile = %ld where host = '%s' and ip = '%s' ;",conn_profile,host,ip);
    int rc = mDBWrapper->execSql(sql_buff,(xCallback)NULL,NULL);
    return rc;
}

int NetworkDatabase::xDnsUpdateHostIp(const char *host,const char *ip,const char *new_ip){
    char sql_buff[1024] ={0};
    if(host == NULL || ip == NULL || new_ip == NULL){
        return UNKNOWN_ERROR;
    }
    snprintf(sql_buff,sizeof(sql_buff),
            "update xdns set ip = '%s',conn_profile = -1 where host = '%s' and ip = '%s' ;",new_ip,host,ip);
    int rc = mDBWrapper->execSql(sql_buff,(xCallback)NULL,NULL);
    return rc;

}

int NetworkDatabase::xDnsDelete(const char *host,const char *ip){
    char sql_buff[1024] ={0};
    if(ip == NULL){
        return -1;
    }
    if(host == NULL){
        host = ip;
    }
    snprintf(sql_buff,sizeof(sql_buff),"delete from xdns where host = %s and ip = %s ;", host, ip);
    int rc = mDBWrapper->execSql(sql_buff,(xCallback)NULL,NULL);
    return rc;
}

int NetworkDatabase::xTaskInfoVCallback(KeyedHash<std::string,ColumnEntry> *colEntries,void *pArgs){

    static std::string taskIdKey ="task_id";
    static std::string moduleKey = "module_name";
    static std::string urlKey = "url";
    static std::string methodKey="method";
    static std::string sendDataKey="send_data";
    static std::string taskTypeKey = "task_type";
    static std::string sendOnlyKey = "send_only";
    static std::string recvFileKey = "recv_file";
    static std::string sendFileKey = "send_file";
    static std::string retryTimesKey = "retry_times";
    static std::string connTimeoutKey = "conn_timeout";
    static std::string taskTimeoutKey = "task_timeout";
    static std::string taskStateKey = "task_state";
    static std::string startTimeKey = "start_time";
    static std::string connTimeKey = "conn_time";
    static std::string tryTimesKey = "try_times";
    if(pArgs == NULL){
        return UNKNOWN_ERROR;
    }
    ASSERT(pArgs != NULL,"Invalidate paramerter pArgs is NULL");
    Vector<TaskInfo> *pTaskArray = (Vector<TaskInfo>*)pArgs;
    TaskInfo ti;
    ti.mTaskId = colEntries->get(taskIdKey).getString();
    ti.mModuleName = colEntries->get(moduleKey).getString();
    ti.mUrl = colEntries->get(urlKey).getString();
    ti.mMethod = colEntries->get(methodKey).getLong();
    const char *data = colEntries->get(sendDataKey).getString();
    if(data != NULL){
        int size = colEntries->get(sendDataKey).size();
        //use append,will not change offset
        ti.mSendData.append(data,size);
    }
    ti.mTaskType = colEntries->get(taskTypeKey).getLong();
    ti.mSendOnly = colEntries->get(sendOnlyKey).getLong();
    ti.mRecvFile = colEntries->get(recvFileKey).getString();
    ti.mSendFile = colEntries->get(sendFileKey).getString();
    ti.mRetryTimes = colEntries->get(retryTimesKey).getLong();
    ti.mConnTimeout = colEntries->get(connTimeoutKey).getLong();
    ti.mTaskTimeout = colEntries->get(taskTimeoutKey).getLong();
    ti.mTaskState = colEntries->get(taskStateKey).getLong();
    ti.mStartTime = colEntries->get(startTimeKey).getLong();
    ti.mStartConnTime = colEntries->get(connTimeKey).getLong();
    ti.mTryTimes = colEntries->get(tryTimesKey).getLong();
    pTaskArray->push(ti);
    return 1;
}

int NetworkDatabase::xTaskGetTodoTasks(Vector<TaskInfo> &tasks){
    char sql_buff[512]={0};
    snprintf(sql_buff,sizeof(sql_buff),"select * from xtask where task_state = %d;",TASK_STATE_IDLE);
    int rc = mDBWrapper->execSql(sql_buff,xTaskInfoVCallback,&tasks);
    if(rc == OK){
        return tasks.size();
    }else{
        return rc;
    }
}

int NetworkDatabase::xTaskInsertTask(TaskInfo& task,int taskState){
    char sql_buff[4096]={0};
    if(task.mTaskId.empty() ||task.mModuleName.empty()){
        ALOGE("invalidate task,please check host module name and task id ");
        return BAD_VALUE;
    }
    sqlite3_stmt* pStmt = NULL;
    if(task.mSendData.size() > 0){
        snprintf(sql_buff,sizeof(sql_buff),"insert into xtask(task_id,module_name,url,mothed,recv_file,"
                "send_file,send_data,send_only,retry_times,task_type,conn_timeout,task_timeout)"
                "  values (\'%s\',\'%s\',\'%s\',%d,\'%s\',\'%s\',?,%d,%d,%d,%ld,%ld);",
                task.mTaskId.c_str(),task.mModuleName.c_str(),task.mUrl.c_str(),task.mMethod,
                task.mRecvFile.empty() ?"NULL":task.mRecvFile.c_str(),
                task.mSendFile.empty() ?"NULL":task.mSendFile.c_str(),
                task.mSendOnly ?1:0,
                task.mRetryTimes, 
                task.mTaskType,
                task.mConnTimeout,
                task.mTaskTimeout);
        pStmt = mDBWrapper->compileSQL(sql_buff);
        if(pStmt != NULL){
            pStmt = mDBWrapper->bindValue(pStmt,1,task.mSendData.data(),task.mSendData.size());
        }
    }else{
        snprintf(sql_buff,sizeof(sql_buff),"insert into xtask(task_id,module_name,url,mothed,recv_file,"
                "send_file,send_only,retry_times,task_type,conn_timeout,task_timeout)"
                "  values (\'%s\',\'%s\',\'%s\',%d,\'%s\',\'%s\',%d,%d,%d,%ld,%ld);",
                task.mTaskId.c_str(),task.mModuleName.c_str(),task.mUrl.c_str(),task.mMethod,
                task.mRecvFile.empty() ?"NULL":task.mRecvFile.c_str(),
                task.mSendFile.empty() ?"NULL":task.mSendFile.c_str(),
                task.mSendOnly ?1:0,
                task.mRetryTimes, 
                task.mTaskType,
                task.mConnTimeout,
                task.mTaskTimeout);
        pStmt = mDBWrapper->compileSQL(sql_buff);
    }
    int ret = mDBWrapper->execStmt(NULL,pStmt,(xCallback)NULL);
    if(ret < 0){
        return UNKNOWN_ERROR;
    }
    return ret;
}


int NetworkDatabase::xTaskCount(int taskState){
    char sql_buff[512]={0};
    int count = 0;
    snprintf(sql_buff,sizeof(sql_buff),"select count(*) from xtask where task_state = %d ;",taskState);
    count = mDBWrapper->count(sql_buff);
    return count;
}


int NetworkDatabase::xTaskDelete(std::string taskId){
    char sql_buff[1024]={0};
    if(taskId.empty()){
        return BAD_VALUE;
    }
    snprintf(sql_buff,sizeof(sql_buff),"delete from xtask where task_id = \'%s\';",taskId.c_str());
    return mDBWrapper->execSql(sql_buff,(xCallback)NULL,NULL);
}


int NetworkDatabase::xTaskUpdateState(std::string taskId,int state){
    char sql_buff[1024]={0};
    if(taskId.empty()){
        return BAD_VALUE;
    }
    snprintf(sql_buff,sizeof(sql_buff),"update xtask set task_state = %d where task_id = \'%s\';",state,taskId.c_str());
    return mDBWrapper->execSql(sql_buff,(xCallback)NULL,NULL);
}

int NetworkDatabase::xTaskUpdateTrytimes(std::string &taskId,int times){
    char sql_buff[1024]={0};
    if(taskId.empty()){
        ALOGE("invalidate task,please check task id");
        return BAD_VALUE;
    }
    snprintf(sql_buff,sizeof(sql_buff),"update xtask set try_times = %d where task_id = \'%s\';",times,taskId.c_str());
    return mDBWrapper->execSql(sql_buff,(xCallback)NULL,NULL);
}

int NetworkDatabase::xTaskUpdateStartTime(std::string &taskId,long startTime){
    char sql_buff[1024]={0};
    if(taskId.empty()){
        ALOGE("invalidate task,please check task id ");
        return BAD_VALUE;
    }
    snprintf(sql_buff,sizeof(sql_buff),"update xtask  set start_time= %ld where task_id = \'%s\';",startTime,taskId.c_str());
    return mDBWrapper->execSql(sql_buff,(xCallback)NULL,NULL);
}

int NetworkDatabase::xTaskUpdateConnTime(std::string &taskId,long connTime){
    char sql_buff[1024]={0};
    if(taskId.empty()){
        ALOGE("invalidate task,please check task id ");
        return BAD_VALUE;
    }
    snprintf(sql_buff,sizeof(sql_buff),"update xtask set conn_time = %ld where task_id = \'%s\';",connTime,taskId.c_str());
    return mDBWrapper->execSql(sql_buff,(xCallback)NULL,NULL);
}

