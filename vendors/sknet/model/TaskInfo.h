#ifndef __TASK_INFO_H__
#define __TASK_INFO_H__
#include<stdio.h>
#include<string>
#include<string.h>
#include<stdlib.h>

#define RETRY_DEFAULT_TIMES 5
#define CONNECT_DEFAULT_TIMEOUT 15000 //ms
#define TASK_DEFAULT_TIMEOUT 60000 //ms

enum TASK_INFO_TYPE{
    TASK_TYPE_HTTP,
    TASK_TYPE_HTTPS,
    TASK_TYPE_HTTP_DOWNLOAD,
    TASK_TYPE_HTTPS_DOWNLOAD,
    TASK_TYPE_TLS,
    TASK_TYPE_TCP,
    TASK_TYPE_MAX,
};

struct BlobData{
    BlobData(){
        xdata = NULL;
        xlength = 0;
    }

    BlobData(const char *data,int length){
        xdata = new char[length];
        assert(xdata != NULL);
        memcpy(xdata,data,length);
        xlength = length;
    }

    void update(const char *data,int length){
        if(xdata != NULL){
            delete[] xdata;
            xlength = length;
        }
        xdata = new char[length];
        assert(xdata != NULL);
        memcpy(xdata,data,length);
        xlength = length;

    }

    BlobData(const BlobData &d){
        xdata = new char[d.xlength];
        assert(xdata != NULL);
        memcpy(xdata,d.xdata,d.xlength);
        xlength = d.xlength;
    }

    char *xdata;
    int xlength;
    ~BlobData(){
        if(xdata != NULL){
            delete[] xdata;
        }
    }
};

class TaskInfo{

    private:
        std::string mTaskId; //task name or id
        std::string mModuleName; //for notify callback
        std::string mUrl;
        //only for download task 
        std::string mSaveFile; //the path download data where to save
        //send file 
        std::string mSendFile; //send a file to server
        //send data
        blob_data mSendData; //data will send to server
        //write buffer


        bool send_only; 
        int retry_times; //retry times
        int task_type;
        //profile and limit for network 
        long int conn_timeout; //ms
        long int task_timeout; //ms
};
#endif//
