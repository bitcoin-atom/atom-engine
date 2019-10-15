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
#include <QSettings>

struct OrderInfo;
using OrderInfoPtr = std::shared_ptr<OrderInfo>;
using Orders = std::map<long long, OrderInfoPtr>;

struct TradeInfo;
using TradeInfoPtr = std::shared_ptr<TradeInfo>;
using Trades = std::map<long long, TradeInfoPtr>;

using Connections = std::map<qintptr, QTcpSocket*>;
using Buffers = std::map<qintptr, QByteArray>;

using ActiveAddrs = std::map<QString, qintptr>;
using Addrs = std::set<QString>;

using BlackList = std::set<QString>;
using RequestCheckingTime = std::map<QString, long long>;
using RequestCheckingCount = std::map<QString, long long>;

class AtomEngineServer : public QObject
{
    Q_OBJECT
public:
    AtomEngineServer();
    ~AtomEngineServer();

    bool run();
private:
    bool load();
    OrderInfoPtr createOrder(const QString& key, const QJsonObject& orderJson);
    bool deleteOrder(const QString& key, long long id);
    TradeInfoPtr createTrade(const QString& key, long long orderId, const QString& initiatorAddress);
    TradeInfoPtr updateTrade(const QString& key, const QJsonObject& tradeJson);
    void sendDisconnectedAddrs(const Addrs& addrs);
    void sendConnectedAddrs(qintptr curDescr);
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
    QSettings* settings_;
    BlackList blackList_;
    long long maxRequestSize_;
    long long requestCheckingInterval_;
    long long requestsCount_;
    RequestCheckingTime checkingTime_;
    RequestCheckingCount checkingCount_;
};

#endif // ATOMENGINESERVER_H
