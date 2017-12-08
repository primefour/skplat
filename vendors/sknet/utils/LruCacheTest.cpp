#include"LruCache.h"
#include<string>
#include"Log.h"


class ValueType{
    public :
        ValueType(std::string s):mStr(s){
            ALOGD("ValueType parameter construct ");
        }
        ValueType(){
            ALOGD("ValueType construct ");
        }

        ValueType(const ValueType & c){
            ALOGD("ValueType copy construct ");
            mStr = c.mStr;
        }
        virtual ~ValueType(){
            ALOGD("ValueType desconstruct ");
        }

        const std::string& getKey()const{
            return mStr;
        }

        void print() const {
            ALOGD("value is %s ",mStr.c_str());
        }

    private:
        std::string mStr;
};

hash_t getStringHash(const std::string & key){
    const char *str = key.c_str();
    if(str != NULL){
        int h = 0;
        for (int i = 0;str[i] != '\0'; i++) {
            h = 31 * h + str[i];
        }
        return (hash_t )h;
    }
    return (hash_t)-1;
}

int main(){
    LruCache<std::string, ValueType> tmpCache(10,getStringHash);
    std::string hello("hello");
    ValueType helloValue(hello);
    std::string world("world");
    ValueType worldValue(world);
    tmpCache.add(hello,helloValue);
    tmpCache.add(hello,helloValue);
    const ValueType &v = tmpCache.get(hello);
    v.print();
    return 0;
}
