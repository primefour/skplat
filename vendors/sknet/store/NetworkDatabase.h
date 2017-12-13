#ifndef __NETWORK_DATABASE_H__
#define __NETWORK_DATABASE_H__
class NetworkDatabase:RefBase{
    public:
        static sp<NetworkDatabase>& getInstance();
        static sp<SqliteWrapper>& getDBWrapper();
        virtual ~NetworkDatabase();
    private:
        NetworkDatabase();
        NetworkDatabase(const NetworkDatabase &);
        void createTables();
        static sp<NetworkDatabase> mNetworkDatabase;
        sp<SqliteWrapper> mDBWrapper;
};
#endif
