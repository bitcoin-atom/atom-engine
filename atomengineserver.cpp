// Copyright 2018 AtomicSwap Solutions Ltd. All rights reserved.
// Use of this source code is governed by Microsoft Reference Source
// License (MS-RSL) that can be found in the LICENSE file.

#include "atomengineserver.h"
#include <QDebug>
#include <QJsonArray>
#include <QTextStream>
#include <QStringList>
#include "info.h"
#include <QByteArrayList>
#include "logger.h"
#include <QDateTime>
#include "dbmanager.h"

namespace {
    const QString backupFileName = "info.dat";
    const QString curVersion = "0.3";
    const QString settingsFileName = "Settings.conf";
}

AtomEngineServer::AtomEngineServer() :
    curOrderId_(0),
    curTradeId_(0),
    backupFile_(backupFileName),
    settings_(nullptr),
    maxRequestSize_(0),
    requestCheckingInterval_(0),
    requestsCount_(0)
{
    settings_ = new QSettings("settings.conf", QSettings::IniFormat);

    server_ = new QTcpServer();
    connect(server_, SIGNAL(newConnection()), this, SLOT(onNewConnection()));
}

AtomEngineServer::~AtomEngineServer()
{
    for (auto it = connections_.begin(); it != connections_.end(); ++it) {
        it->second->close();
    }
    delete server_;
    Logger::info() << "Atom engine was closed";
}

bool AtomEngineServer::run()
{
    Logger::info() << "Atom engine start";
    if (!settings_) {
        Logger::info() << "Start failed: settings are not initialized";
        return false;
    }
    int port = settings_->value("server/port", -1).toInt();
    if (port < 0) {
        Logger::info() << "Start failed: need set a port";
        return false;
    }
    load();
    QString dbName = settings_->value("database/name", "engine.db").toString();
    if (!DBManager::instance().init(dbName)) {
        return false;
    }
    if (server_->listen(QHostAddress::Any, port)) {
        maxRequestSize_ = settings_->value("security/request_max_size_bytes", 0).toLongLong();
        requestsCount_ = settings_->value("security/requests_count", 0).toLongLong();
        Logger::info() << "Atom engine was started success, port = " + QString::number(port) + " version = " + curVersion;
        Logger::info() << "Max request size in bytes = " + QString::number(maxRequestSize_);
        Logger::info() << "Requests checking interval in ms = " + QString::number(requestCheckingInterval_);
        Logger::info() << "Request count from client at checking interval = " + QString::number(requestsCount_);
        return true;
    } else {
        Logger::info() << "Atom engine starting failed";
        return false;
    }
}

void AtomEngineServer::onNewConnection()
{
    QTcpSocket* clientSocket = server_->nextPendingConnection();
    QString clientIp = clientSocket->peerAddress().toString();

    if (blackList_.find(clientIp) != blackList_.end()) {
        Logger::info() << "Attempt of connection from IP in black list. IP = " + clientIp;
        clientSocket->close();
        return;
    }

    connect(clientSocket, SIGNAL(disconnected()), this, SLOT(onClientDisconnected()));
    int socketId = clientSocket->socketDescriptor();
    connections_[socketId] = clientSocket;
    connect(clientSocket, SIGNAL(readyRead()), this, SLOT(onReadyRead()));
    Logger::info() << "New connection id = " + QString::number(socketId) + ", active connections = " + QString::number(connections_.size());
}

void AtomEngineServer::onClientDisconnected()
{
    QTcpSocket* clientSocket = (QTcpSocket*)sender();
    QString clientIp = clientSocket->peerAddress().toString();

    Addrs disconnectedAddrs;

    auto itConnection = connections_.begin();
    while (itConnection != connections_.end()) {
        if (itConnection->second == clientSocket) {
            auto itAddr = addrs_.begin();
            while (itAddr != addrs_.end()) {
                if (itAddr->second == itConnection->first) {
                    disconnectedAddrs.insert(itAddr->first);
                    itAddr = addrs_.erase(itAddr);
                } else {
                    ++itAddr;
                }
            }
            auto itBuf = buffers_.find(itConnection->first);
            if (itBuf != buffers_.end()) {
                buffers_.erase(itBuf);
            }
            itConnection = connections_.erase(itConnection);
        } else {
            ++itConnection;
        }
    }

    sendDisconnectedAddrs(disconnectedAddrs);

    auto it1 = checkingTime_.find(clientIp);
    if (it1 != checkingTime_.end()) {
        checkingTime_.erase(it1);
    }

    auto it2 = checkingCount_.find(clientIp);
    if (it2 != checkingCount_.end()) {
        checkingCount_.erase(it2);
    }

    Logger::info() << "Client disconnected, active connections = " + QString::number(connections_.size());
}

void AtomEngineServer::sendDisconnectedAddrs(const Addrs& addrs)
{
    QString rep = "{\"reply\":\"user_disconnected\", \"addrs\": [";
    for (auto it = addrs.begin(); it != addrs.end(); ++it) {
        if (it != addrs.begin()) {
            rep +=  ", ";
        }
        rep += "\"" + *it + "\"";
    }
    rep += "]}\n";
    for (auto it = connections_.begin(); it != connections_.end(); ++it) {
        it->second->write(rep.toStdString().c_str());
    }
}

void AtomEngineServer::sendConnectedAddrs(qintptr curDescr)
{
    QString rep = "{\"reply\":\"user_connected\", \"addrs\": [";
    for (auto it = addrs_.begin(); it != addrs_.end(); ++it) {
        if (it != addrs_.begin()) {
            rep +=  ", ";
        }
        rep += "\"" + it->first + "\"";
    }
    rep += "]}\n";
    for (auto it = connections_.begin(); it != connections_.end(); ++it) {
        if (it->first != curDescr) {
            it->second->write(rep.toStdString().c_str());
        }
    }
}

void AtomEngineServer::onReadyRead()
{
    QTcpSocket* clientSocket = (QTcpSocket*)sender();
    QString clientIp = clientSocket->peerAddress().toString();
    if (blackList_.find(clientIp) != blackList_.end()) {
        return;
    }

    QByteArray& buffer = buffers_[clientSocket->socketDescriptor()];

    QByteArray answer = clientSocket->readAll();
    buffer.append(answer);

    if (buffer.size() > maxRequestSize_) {
        Logger::info() << "Request too match size from ip = " + clientIp;
        auto it = buffers_.find(clientSocket->socketDescriptor());
        if (it != buffers_.end()) {
            buffers_.erase(it);
        }
        blackList_.insert(clientIp);
        DBManager::instance().addToBlackList(clientIp);
        clientSocket->close();
        return;
    }

    int pos = buffer.lastIndexOf("\n");
    if (pos < 0) {
        return;
    }

    auto it1 = checkingTime_.find(clientIp);
    if (it1 != checkingTime_.end()) {
        long long curRequestsCount = checkingCount_[clientIp];
        ++curRequestsCount;
        if (QDateTime::currentDateTime().toTime_t() - it1->second < requestCheckingInterval_) {
            if (curRequestsCount > requestsCount_) {
                auto it2 = checkingCount_.find(clientIp);
                if (it2 != checkingCount_.end()) {
                    checkingCount_.erase(it2);
                }
                checkingTime_.erase(it1);
                Logger::info() << "Requests too match count from ip = " + clientIp;
                auto it = buffers_.find(clientSocket->socketDescriptor());
                if (it != buffers_.end()) {
                    buffers_.erase(it);
                }
                blackList_.insert(clientIp);
                DBManager::instance().addToBlackList(clientIp);
                clientSocket->close();
                return;
            } else {
                checkingCount_[clientIp] = curRequestsCount;
            }
        } else {
            auto it2 = checkingCount_.find(clientIp);
            if (it2 != checkingCount_.end()) {
                checkingCount_.erase(it2);
            }
            checkingTime_.erase(it1);
        }
    } else {
        checkingTime_[clientIp] = QDateTime::currentDateTime().toTime_t();
        checkingCount_[clientIp] = 1;
    }

    QByteArrayList commands = buffer.left(pos).split('\n');
    int rightCount = buffer.length() - pos - 1;
    if (rightCount > 0) {
        buffer = buffer.right(rightCount);
    } else {
        buffer.clear();
    }

    for (int i = 0; i < commands.size(); ++i) {
        QByteArray& commandStr = commands[i];
        if (commandStr.length() == 0) {
            continue;
        }
        QJsonDocument doc = QJsonDocument::fromJson(commandStr);
        commandStr.append("\n");
        Logger::info() << QString("client descr = ") + QString::number(clientSocket->socketDescriptor()) + QString(" ") + commandStr;
        if (doc.isObject()) {
            QJsonObject req = doc.object();
            QString command = req["command"].toString();
            if (command == "init") {
                Addrs activeAddrs;
                QJsonArray curs = req["curs"].toArray();
                for (int i = 0; i < curs.size(); ++i) {
                    QJsonObject curInfo = curs[i].toObject();
                    QJsonArray addrs = curInfo["addrs"].toArray();
                    for (int i = 0; i < addrs.size(); ++i) {
                        QString addr = addrs[i].toString();
                        addrs_[addr] = clientSocket->socketDescriptor();
                        activeAddrs.insert(addr);
                    }
                }
                QString rep = "{\"reply\": \"init_success\", \"isActual\": true, \"orders\": [";
                for (auto it = orders_.begin(); it != orders_.end(); ++it) {
                    if (it != orders_.begin()) {
                        rep += ", ";
                    }
                    rep += it->second->getJson();
                }
                rep += "], \"trades\": [";
                bool firstTrade = true;
                for (auto it = trades_.begin(); it != trades_.end(); ++it) {
                    const QString& addr1 = it->second->order_->getAddress_;
                    const QString& addr2 = it->second->initiatorAddress_;
                    if (activeAddrs.find(addr1) != activeAddrs.end() || activeAddrs.find(addr2) != activeAddrs.end()) {
                        if (!firstTrade) {
                            rep +=  ", ";
                        }
                        rep += it->second->getJson();
                        firstTrade = false;
                    }
                }
                rep += "], \"commissions\": [], \"active_addrs\": [";
                for (auto it = addrs_.begin(); it != addrs_.end(); ++it) {
                    if (it != addrs_.begin()) {
                        rep +=  ", ";
                    }
                    rep += "\"" + it->first + "\"";
                }
                rep += "]}\n";
                clientSocket->write(rep.toStdString().c_str());

                sendConnectedAddrs(clientSocket->socketDescriptor());
            }
            if (command == "request_swap_commission") {
                QJsonArray curs = req["curs"].toArray();
                for (int i = 0; i < curs.size(); ++i) {
                    QJsonObject curInfo = curs[i].toObject();
                    QJsonArray addrs = curInfo["addrs"].toArray();
                    for (int i = 0; i < addrs.size(); ++i) {
                        QString addr = addrs[i].toString();
                        addrs_[addr] = clientSocket->socketDescriptor();
                    }
                }
                QString rep = "{\"reply\": \"request_swap_commission_success\", \"commissions\": []}\n";
                clientSocket->write(rep.toStdString().c_str());
            }
            if (command == "create_order") {
                QJsonObject orderJson = req["order"].toObject();
                QString key = "";
                if (req.contains("key")) {
                    key = req["key"].toString();
                }
                OrderInfoPtr newOrder = createOrder(key, orderJson);
                if (newOrder) {
                    DBManager::instance().addToOrders(newOrder);
                    QString rep1 = "{\"reply\": \"create_order_success\", \"order\": " + newOrder->getJson() + "}\n";
                    QString rep2 = "{\"reply\": \"create_order\", \"order\": " + newOrder->getJson() + "}\n";
                    clientSocket->write(rep1.toStdString().c_str());
                    for (auto it = connections_.begin(); it != connections_.end(); ++it) {
                        if (it->first != clientSocket->socketDescriptor()) {
                            it->second->write(rep2.toStdString().c_str());
                        }
                    }
                    bool newAddrForOrder = false;
                    if (addrs_.find(newOrder->getAddress_) == addrs_.end()) {
                        newAddrForOrder = true;
                    }
                    addrs_[newOrder->getAddress_] = clientSocket->socketDescriptor();
                    if (newAddrForOrder) {
                        sendConnectedAddrs(clientSocket->socketDescriptor());
                    }
                }
            }
            if (command == "delete_order") {
                long long id = req["id"].toVariant().toLongLong();
                QString key = "";
                if (req.contains("key")) {
                    key = req["key"].toString();
                }
                bool deleted = deleteOrder(key, id);
                QString rep1 = "{\"reply\": \"delete_order_success\", \"id\": " + QString::number(id) + "}\n";
                clientSocket->write(rep1.toStdString().c_str());
                if (deleted) {
                    DBManager::instance().deleteFromOrders(id);
                    QString rep2 = "{\"reply\": \"delete_order\", \"id\": " + QString::number(id) + "}\n";
                    for (auto it = connections_.begin(); it != connections_.end(); ++it) {
                        if (it->first != clientSocket->socketDescriptor()) {
                            it->second->write(rep2.toStdString().c_str());
                        }
                    }
                }
            }
            if (command == "create_trade") {
                long long orderId = req["orderId"].toVariant().toLongLong();
                QString initiatorAddr = req["address"].toString();
                addrs_[initiatorAddr] = clientSocket->socketDescriptor();
                QString key = "";
                if (req.contains("key")) {
                    key = req["key"].toString();
                }
                TradeInfoPtr trade = createTrade(key, orderId, initiatorAddr);
                if (trade) {
                    DBManager::instance().addToTrades(trade);

                    QString rep1 = "{\"reply\": \"delete_order\", \"id\": " + QString::number(orderId) + "}\n";
                    QString rep2 = "{\"reply\": \"create_trade\", \"trade\": " + trade->getJson() + "}\n";
                    QString rep3 = "{\"reply\": \"create_trade_success\", \"trade\": " + trade->getJson() + "}\n";

                    clientSocket->write(rep3.toStdString().c_str());

                    int firstSocketDescr = clientSocket->socketDescriptor();
                    int secondSocketDescr = -1;
                    auto it = addrs_.find(trade->order_->getAddress_);
                    if (it != addrs_.end()) {
                        auto itCon = connections_.find(it->second);
                        if (itCon != connections_.end()) {
                            secondSocketDescr = itCon->first;
                            if (secondSocketDescr != firstSocketDescr) {
                                itCon->second->write(rep2.toStdString().c_str());
                            }
                        }
                    }

                    for (auto it = connections_.begin(); it != connections_.end(); ++it) {
                        if (it->first != firstSocketDescr && it->first != secondSocketDescr) {
                            it->second->write(rep1.toStdString().c_str());
                        }
                    }
                } else {
                    QString rep = "{\"reply\": \"create_trade_failed\", \"reasone\": \"order out of date\"}\n";
                    clientSocket->write(rep.toStdString().c_str());
                }
            }
            if (command == "update_trade") {
                QJsonObject tradeJson = req["trade"].toObject();
                QString key = "";
                if (req.contains("key")) {
                    key = req["key"].toString();
                }
                TradeInfoPtr trade = updateTrade(key, tradeJson);
                QString rep1 = "{\"reply\": \"update_trade_success\"}\n";
                clientSocket->write(rep1.toStdString().c_str());
                if (trade) {
                    DBManager::instance().updateTrade(trade);
                    QString rep2 = "{\"reply\": \"update_trade\", \"trade\": " + trade->getJson() + "}\n";
                    const QString& firstAddr = trade->order_->getAddress_;
                    const QString& secondAddr = trade->initiatorAddress_;
                    auto itFirstDescr = addrs_.find(firstAddr);
                    auto itSecondDescr = addrs_.find(secondAddr);
                    int anotherConnectionDescr = -1;
                    if (itFirstDescr != addrs_.end() && itSecondDescr != addrs_.end() && itFirstDescr->second == clientSocket->socketDescriptor()) {
                        anotherConnectionDescr = itSecondDescr->second;
                    } else if (itFirstDescr != addrs_.end() && itSecondDescr != addrs_.end()) {
                        anotherConnectionDescr = itFirstDescr->second;
                    }
                    if (anotherConnectionDescr != -1) {
                        auto it = connections_.find(anotherConnectionDescr);
                        if (it != connections_.end()) {
                            it->second->write(rep2.toStdString().c_str());
                        }
                    }
                }
            }
        }
    }
}

bool AtomEngineServer::load()
{
    DBManager::instance().loadOrders(orders_);
    DBManager::instance().loadTrades(trades_);
    DBManager::instance().loadBlackList(blackList_);
    return true;
}

OrderInfoPtr AtomEngineServer::createOrder(const QString& key, const QJsonObject& orderJson)
{
    ++curOrderId_;
    OrderInfoPtr order = std::make_shared<OrderInfo>(curOrderId_, orderJson);
    order->sign(key);
    orders_[curOrderId_] = order;
    return order;
}

bool AtomEngineServer::deleteOrder(const QString& key, long long id)
{
    auto it = orders_.find(id);
    if (it != orders_.end() && it->second->checkKey(key)) {
        orders_.erase(it);
        return true;
    } else {
        return false;
    }
}

TradeInfoPtr AtomEngineServer::createTrade(const QString& key, long long orderId, const QString& initiatorAddress)
{
    auto it = orders_.find(orderId);
    if (it != orders_.end()) {
        OrderInfoPtr order = it->second;
        orders_.erase(it);
        ++curTradeId_;
        TradeInfoPtr trade = std::make_shared<TradeInfo>(curTradeId_, order, initiatorAddress);
        trade->sign(key);
        trades_[curTradeId_] = trade;
        return trade;
    } else {
        return TradeInfoPtr();
    }
}

TradeInfoPtr AtomEngineServer::updateTrade(const QString& key, const QJsonObject& tradeJson)
{
    long long id = tradeJson["id"].toVariant().toLongLong();
    auto it = trades_.find(id);
    if (it != trades_.end() && (it->second->checkKey(key) || it->second->checkOrderKey(key))) {
        TradeInfoPtr trade = it->second;
        trade->secretHash_ = tradeJson["secretHash"].toString();
        trade->contractInitiator_ = tradeJson["contractInitiator"].toString();
        trade->contractParticipant_ = tradeJson["contractParticipant"].toString();
        trade->initiatorContractTransaction_ = tradeJson["initiatorContractTransaction"].toString();
        trade->participantContractTransaction_ = tradeJson["participantContractTransaction"].toString();
        trade->initiatorRedemptionTransaction_ = tradeJson["initiatorRedemptionTransaction"].toString();
        trade->participantRedemptionTransaction_ = tradeJson["participantRedemptionTransaction"].toString();
        if (!trade->initiatorCommissionPaid_) {
            trade->initiatorCommissionPaid_ = tradeJson["commissionInitiatorPaid"].toBool();
        }
        if (!trade->participantCommissionPaid_) {
            trade->participantCommissionPaid_ = tradeJson["commissionParticipantPaid"].toBool();
        }

        if (tradeJson.contains("refundedInit")) {
            trade->refundedInit_ = tradeJson["refundedInit"].toBool();
        }
        if (tradeJson.contains("refundedPart")) {
            trade->refundedPart_ = tradeJson["refundedPart"].toBool();
        }

        if (tradeJson.contains("refundTimeInit")) {
            trade->refundTimeInit_ = tradeJson["refundTimeInit"].toVariant().toLongLong();
        }

        if (tradeJson.contains("refundTimePart")) {
            trade->refundTimePart_ = tradeJson["refundTimePart"].toVariant().toLongLong();
        }

        return trade;
    } else {
        return TradeInfoPtr();
    }
}
