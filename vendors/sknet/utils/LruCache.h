#ifndef __LRU_CACHE_H__
#define __LRU_CACHE_H__
#include"BasicHashtable.h"
#include"RefBase.h"
#include"Timers.h"
#include"Log.h"
/*
 * this is simple lru cache implement,
 * Key must support the following contract:
 *     bool operator==(const TKey& other) const;  // return true if equal
 *     bool operator!=(const TKey& other) const;  // return true if unequal
 *
 * Value must support the following contract:
 *     const Key& getKey() const;  // get the key from the entry
 */
template<typename KEY,typename VALUE>
class LruCache :RefBase{
    typedef hash_t (*GetHashCodeFpn)(const KEY& key);
    public :
        LruCache(int capacity,GetHashCodeFpn fpn);
        virtual ~LruCache();
        //get value by key
        const VALUE& get(KEY& key);
        //get for edit
        VALUE& editGet(KEY& key);
        //remove item by key 
        void remove(KEY& key);
        //add item
        int add(KEY& key,VALUE& Value);

        //invalidate value for get
        static VALUE mNullItem;
    private:
        //when contain is full,shrink count size for new items
        void shrinkCache();
        long nowTime(); ///time of system milliseconds 
        LruCache(const LruCache &);
        BasicHashtable<KEY,VALUE> mContain;
        int mCapacity;
        int mElapseCap;
        int mSize;
        long *mElapses;
        GetHashCodeFpn mFpnhashCode;
};

template<typename KEY,typename VALUE>
LruCache<KEY,VALUE>::~LruCache(){
    if(mElapses != NULL){
        delete[] mElapses;
        mElapses = NULL;
    }
}

template<typename KEY,typename VALUE>
LruCache<KEY,VALUE>::LruCache(int capacity,GetHashCodeFpn fpn):mContain(capacity,0.75),mFpnhashCode(fpn){
    ALOGD("create LruCache mcapacity = %d ",capacity);
    mCapacity = capacity;
    mElapseCap = mContain.bucketCount();
    mSize = 0;
    mElapses = new long[mElapseCap];
    memset(mElapses,0,sizeof(long) *mElapseCap);
    ASSERT(mElapses != NULL,"no memory to create lrucache");
}


template<typename KEY,typename VALUE>
long LruCache<KEY,VALUE>::nowTime(){ //milis second
    nsecs_t nsec = systemTime();
    return ns2ms(nsec);
}

template<typename KEY,typename VALUE>
void LruCache<KEY,VALUE>::shrinkCache(){
    int min;
    int hashIdx = -1;
    int i = 0;
    if(mSize < mCapacity){
        return ;
    }

    for(i = 0 ;i < mElapseCap; i ++){
        if(mElapses[i] != 0){
            min = mElapses[i];
            break;
        }
    }

    for(;i < mElapseCap;i++){
        if(mElapses[i] < min){
            hashIdx = i; 
        }
    }

    mElapses[hashIdx] = 0;
    mContain.removeAt(hashIdx);
    mSize --;
}



template<typename KEY,typename VALUE>
const VALUE& LruCache<KEY,VALUE>::get(KEY& key){
    int idx = mContain.find(-1,mFpnhashCode(key),key);
    if(idx == -1 ){
        return mNullItem;
    }
    //update time
    mElapses[idx]= nowTime();
    return mContain.entryAt(idx);
}


/*
 * don't change the key using this function
 */
template<typename KEY,typename VALUE>
VALUE& LruCache<KEY,VALUE>::editGet(KEY& key){
    int idx = mContain.find(-1,mFpnhashCode(key),key);
    if(idx == -1 ){
        return mNullItem;
    }
    //update time
    mElapses[idx]= nowTime();
    return mContain.editEntryAt(idx);
}


template<typename KEY,typename VALUE>
void LruCache<KEY,VALUE>::remove(KEY& key){
    int idx = mContain.find(-1,mFpnhashCode(key),key);
    if(idx == -1 ){
        return ;
    }
    //update time
    mElapses[idx]= 0;
    //remove it
    mContain.removeAt(idx);
    mSize --;
}


template<typename KEY,typename VALUE>
int LruCache<KEY,VALUE>::add(KEY& key,VALUE& Value){
    int idx = mContain.find(-1,mFpnhashCode(key),key);
    if(idx != -1 ){
        ALOGD("item exist and will update by default");
        //update time
        mElapses[idx]= 0;
        //remove it
        mContain.removeAt(idx);
        mSize --;
    }

    while(mSize >= mCapacity){
        shrinkCache();
    }

    idx = mContain.add(mFpnhashCode(key),Value);
    ALOGD("lrc add  idx = %d ",idx);
    //make sure there is no item
    ASSERT(idx > 0 && idx < mElapseCap && mElapses[idx] == 0,"lru add fail idx = %d",idx);
    mElapses[idx] = nowTime();
    mSize ++;
    return idx; 
}

#endif
