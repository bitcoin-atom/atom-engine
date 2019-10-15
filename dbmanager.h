#ifndef DBMANAGER_H
#define DBMANAGER_H

#include <QString>
#include "QtSql/QSqlDatabase"
#include <map>
#include <set>
#include "QSqlQuery"

struct OrderInfo;
using OrderInfoPtr = std::shared_ptr<OrderInfo>;
using Orders = std::map<long long, OrderInfoPtr>;

struct TradeInfo;
using TradeInfoPtr = std::shared_ptr<TradeInfo>;
using Trades = std::map<long long, TradeInfoPtr>;

using BlackList = std::set<QString>;

class DBManager {
public:
    static DBManager& instance();
    bool init(const QString& dbName);
    void addToOrders(OrderInfoPtr order);
    void addToTrades(TradeInfoPtr trade);
    void addToBlackList(const QString& blackListIP);
    void deleteFromOrders(long long orderId);
    void deleteFromTrades(long long tradeId);
    void updateTrade(TradeInfoPtr trade);
    void loadOrders(Orders& orders);
    void loadTrades(Trades& trades);
    void loadBlackList(BlackList& blackList);
private:
    DBManager();
    ~DBManager();
    DBManager(const DBManager&);
    DBManager& operator = (const DBManager&);
private:
    QSqlDatabase db;
    QSqlQuery queryAddToOrders;
    QSqlQuery queryAddToTrades;
    QSqlQuery queryAddToBL;
    QSqlQuery queryDeleteFromOrders;
    QSqlQuery queryDeleteFromTrades;
    QSqlQuery queryUpdateTrade;
    QSqlQuery queryLoadOrders;
    QSqlQuery queryLoadTrades;
    QSqlQuery queryLoadBL;
};

#endif // DBMANAGER_H
