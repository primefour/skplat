#ifndef __SIMPLE_HASHE_H__
#define __SIMPLE_HASHE_H__
#include"BasicHashtable.h"
#include"RefBase.h"
#include"Log.h"
/*
 * this is simple basic hash wrapper 
 * Key must support the following contract:
 *     bool operator==(const TKey& other) const;  // return true if equal
 *     bool operator!=(const TKey& other) const;  // return true if unequal
 *
 * Value must support the following contract:
 *     const Key& getKey() const;  // get the key from the entry
 */
template<typename KEY,typename VALUE>
class SimpleHash:RefBase{
    typedef hash_t (*GetHashCodeFpn)(const KEY& key);
    public :
        SimpleHash(int capacity,GetHashCodeFpn fpn);
        virtual ~SimpleHash();
        //get value by key
        //idx if is NULL,will return the first entry of this key,
        //or return the latest entry after idx;
        //note:idx if is not NULL,idx will return the idx of entry
        const VALUE& get(KEY& key,int *idx);

        //remove item by key 
        void remove(KEY& key);

        //for scan entries
        //first index should set -1 or NULL to get fisrt entry
        //note:idx if is not NULL,idx will return the idx of entry
        const VALUE& next(int *index) const {
            int i = 0;
            if(index == NULL){
                 i = BasicHashtableImpl::next(-1);
            }else{
                i = BasicHashtableImpl::next(*index);
                if(index != NULL){
                    *index = i;
                }
                if(i == -1){
                    return mNullItem;
                }
            }
            return mContain.entryAt(idx);
        }

        //add entry and return idx of entry
        int add(KEY& key,VALUE& Value);
        //invalidate value for get
        static VALUE mNullItem;
        static VALUE& getNull(){
            return mNullItem;
        }
    private:
        SimpleHash(const SimpleHash&);
        BasicHashtable<KEY,VALUE> mContain;
        int mSize;
        GetHashCodeFpn mFpnhashCode;
};

template<typename KEY,typename VALUE>
SimpleHash<KEY,VALUE>::~SimpleHash(){

}

template<typename KEY,typename VALUE>
 SimpleHash<KEY,VALUE>::SimpleHash(int capacity,GetHashCodeFpn fpn):
                                mContain(capacity,0.75),mFpnhashCode(fpn){
    ALOGD("create SimpleHash mcapacity = %d ",capacity);
    mSize = 0;
}

template<typename KEY,typename VALUE>
const VALUE& SimpleHash<KEY,VALUE>::get(KEY& key,int *idx){
    int i = -1;
    if(idx == NULL){
        i = mContain.find(-1,mFpnhashCode(key),key);
    }else{
        i = mContain.find(*idx,mFpnhashCode(key),key);
    }
    if(i == -1 ){
        return mNullItem;
    }
    *idx = i;
    return mContain.entryAt(idx);
}


template<typename KEY,typename VALUE>
void SimpleHash<KEY,VALUE>::remove(KEY& key,int *idx){
    int i = 0;
    if(idx == NULL){
        i = mContain.find(-1,mFpnhashCode(key),key);
    }else{
        i = mContain.find(*idx,mFpnhashCode(key),key);
    }

    if(i == -1 ){
        return ;
    }
    //remove it
    mContain.removeAt(i);
    mSize --;
}


template<typename KEY,typename VALUE>
int SimpleHash<KEY,VALUE>::add(KEY& key,VALUE& Value){
    int idx = mContain.find(-1,mFpnhashCode(key),key);
    if(idx != -1 ){
        ALOGD("item exist and will update by default");
        //remove it
        mContain.removeAt(idx);
        mSize --;
    }
    idx = mContain.add(mFpnhashCode(key),Value);
    //ALOGD("hash add  idx = %d ",idx);
    mSize ++;
    return idx; 
}
#endif
