#ifndef __KEYED_HASHE_H__
#define __KEYED_HASHE_H__
#include"BasicHashtable.h"
#include"RefBase.h"
#include"Log.h"
/*
 * this is simple keyed hash implement,no duplicate
 * Key must support the following contract:
 *     bool operator==(const TKey& other) const;  // return true if equal
 *     bool operator!=(const TKey& other) const;  // return true if unequal
 *
 * Value must support the following contract:
 *     const Key& getKey() const;  // get the key from the entry
 */
template<typename KEY,typename VALUE>
class KeyedHash:RefBase{
    typedef hash_t (*GetHashCodeFpn)(const KEY& key);
    public :
        KeyedHash(int capacity,GetHashCodeFpn fpn);
        virtual ~KeyedHash();
        //get value by key
        const VALUE& get(KEY& key);
        //remove item by key 
        void remove(KEY& key);
        //add item
        int add(KEY& key,VALUE& Value);
        //invalidate value for get
        static VALUE mInvalidate;
    private:
        KeyedHash(const KeyedHash &);
        BasicHashtable<KEY,VALUE> mContain;
        int mSize;
        GetHashCodeFpn mFpnhashCode;
};

template<typename KEY,typename VALUE>
KeyedHash<KEY,VALUE>::~KeyedHash(){

}

template<typename KEY,typename VALUE>
KeyedHash<KEY,VALUE>::KeyedHash(int capacity,GetHashCodeFpn fpn):mContain(capacity,0.75),mFpnhashCode(fpn){
    ALOGD("create KeyedHash mcapacity = %d ",capacity);
    mSize = 0;
}

template<typename KEY,typename VALUE>
const VALUE& KeyedHash<KEY,VALUE>::get(KEY& key){
    int idx = mContain.find(-1,mFpnhashCode(key),key);
    if(idx == -1 ){
        return mInvalidate;
    }
    return mContain.entryAt(idx);
}


template<typename KEY,typename VALUE>
void KeyedHash<KEY,VALUE>::remove(KEY& key){
    int idx = mContain.find(-1,mFpnhashCode(key),key);
    if(idx == -1 ){
        return ;
    }
    //remove it
    mContain.removeAt(idx);
    mSize --;
}


template<typename KEY,typename VALUE>
int KeyedHash<KEY,VALUE>::add(KEY& key,VALUE& Value){
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
