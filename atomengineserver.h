// Copyright 2018 AtomicSwap Solutions Ltd. All rights reserved.
// Use of this source code is governed by Microsoft Reference Source
// License (MS-RSL) that can be found in the LICENSE file.

#ifndef ATOMENGINESERVER_H
#define ATOMENGINESERVER_H

#include <QObject>
#include <QTcpServer>
#include <QTcpSocket>
#include <QJsonDocument>
#include <QJsonObject>
#include <QFile>
#include <map>
#include <memory>
#include <set>

struct OrderInfo;
typedef std::shared_ptr<OrderInfo> OrderInfoPtr;
typedef std::map<long long, OrderInfoPtr> Orders;

struct TradeInfo;
typedef std::shared_ptr<TradeInfo> TradeInfoPtr;
typedef std::map<long long, TradeInfoPtr> Trades;

typedef std::map<int, QTcpSocket*> Connections;
typedef std::map<int, QByteArray> Buffers;

typedef std::map<QString, int> ActiveAddrs;
typedef std::set<QString> Addrs;

class AtomEngineServer : public QObject
{
    Q_OBJECT
public:
    AtomEngineServer(int port);
    ~AtomEngineServer();

    bool run();
private:
    bool load();
    void saveCommand(QJsonDocument& doc);
    OrderInfoPtr createOrder(const QString& key, const QJsonObject& orderJson);
    bool deleteOrder(const QString& key, long long id);
    TradeInfoPtr createTrade(const QString& key, long long orderId, const QString& initiatorAddress);
    TradeInfoPtr updateTrade(const QString& key, const QJsonObject& tradeJson);
private slots:
    void onNewConnection();
    void onClientDisconnected();
    void onReadyRead();
private:
    QTcpServer* server_;
    Connections connections_;
    Orders orders_;
    Trades trades_;
    ActiveAddrs addrs_;
    long long curOrderId_;
    long long curTradeId_;
    QFile backupFile_;
    Buffers buffers_;
    int port_;
};

#endif // ATOMENGINESERVER_H
