#include"TaskInfo.h"
#include "BufferUtils.h"

TaskInfo::TaskInfo(){
    //set default value
    mSendOnly = false;
    mPersist = false;
    mMethod = TASK_METHOD_HTTP_GET;
    mRetryTimes = RETRY_DEFAULT_TIMES;
    mTaskType = TASK_TYPE_HTTP;
    mConnTimeout = CONNECT_DEFAULT_TIMEOUT;//15s
    mTaskTimeout = TASK_DEFAULT_TIMEOUT;//1min
    mTaskState = TASK_STATE_IDLE;
    mTryTimes = 0;
    mStartTime = 0;
    mStartConnTime = 0;
    mRecvData = new BufferUtils();
    mSendData = new BufferUtils();
    mHttpTransfer = NULL;
    mWorkUnit = NULL;
    mCanceled = false;
}
