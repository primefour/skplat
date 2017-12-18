#ifndef __NETWORK_DATABASE_H__
#define __NETWORK_DATABASE_H__
#include"RefBase.h"
#include"SocketAddress.h"
#include"SqliteWrapper.h"
#include"Vector.h"
#include"KeyedHash.h"
#include"TaskInfo.h"
#include<string>


class NetworkDatabase:public RefBase{
    public:
        virtual ~NetworkDatabase();
        static sp<NetworkDatabase>& getInstance();
        static sp<SqliteWrapper>& getDBWrapper();
        static int xDnsVCallback(KeyedHash<std::string,ColumnEntry> *colEntries,void *pArgs);
        static int xTaskInfoVCallback(KeyedHash<std::string,ColumnEntry> *colEntries,void *pArgs);
        int xDnsInsert(SocketAddress &sa);
        //check host and ip whether is exist or not
        int xDnsExist(SocketAddress &sa);
        /*
         *(host text,ip text,ip_type int default 0,"
         *"fetch_type int default 0,conn_profile int64 default -1);";
         */
        int getAddrByHost(const char *host,Vector<SocketAddress> &ips);
        int xDnsUpdateProf(const char *host,const char *ip,int64_t conn_profile);
        int xDnsUpdateHostIp(const char *host,const char *ip,const char *new_ip);
        int xDnsDelete(const char *host,const char *ip);
        int xDnsDelete(SocketAddress &sa);
        int xTaskGetTasks(Vector<TaskInfo> &tasks,int taskState = TASK_STATE_IDLE);
        int xTaskInsert(TaskInfo& task,int taskState = TASK_STATE_IDLE);
        int xTaskCount(int taskState);
        int xTaskDelete(std::string taskId);
        int xTaskUpdateState(std::string taskId,int state);
        int xTaskUpdateTrytimes(std::string &taskId,int times);
        int xTaskUpdateStartTime(std::string &taskId,long startTime);
        int xTaskUpdateConnTime(std::string &taskId,long connTime);
        sp<SqliteWrapper>& getDB(){ return mDBWrapper; }
    private:
        NetworkDatabase();
        NetworkDatabase(const NetworkDatabase &);
        void createTables();
        static sp<NetworkDatabase> mNetworkDatabase;
        sp<SqliteWrapper> mDBWrapper;
};
#endif
